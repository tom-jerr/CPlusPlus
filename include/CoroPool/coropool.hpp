#include <atomic>
#include <condition_variable>
#include <coroutine>
#include <functional>
#include <list>
#include <mutex>
#include <thread>
#include <utility>

class CoroPool;

class [[nodiscard]] CoroTask {
public:
  struct promise_type;
  using Handle = std::coroutine_handle<promise_type>;
  friend class CoroPool;
  struct promise_type {
    CoroPool *poolPtr{nullptr};
    Handle contHdl{nullptr};

    CoroTask get_return_object() {
      return CoroTask{Handle::from_promise(*this)};
    }
    auto initial_suspend() noexcept { return std::suspend_always{}; }
    void unhandled_exception() noexcept { std::terminate(); }
    void return_void() noexcept {}

    auto final_suspend() const noexcept {
      /**
       * @brief
       * 返回一个FinalAwaiter对象，该对象的await_suspend方法返回一个coroutine_handle<>对象
       *
       */
      struct FinalAwaiter {
        bool await_ready() const noexcept { return false; }
        std::coroutine_handle<> await_suspend(Handle hdl) const noexcept {
          if (hdl.promise().contHdl) {
            return hdl.promise().contHdl;
          } else {
            return std::noop_coroutine();
          }
        }
        void await_resume() const noexcept {}
      };
      return FinalAwaiter{};
    }
  };

public:
  explicit CoroTask(Handle handle) : m_handle(handle) {}
  CoroTask(CoroTask &&rhs) noexcept
      : m_handle(std::exchange(rhs.m_handle, {})) {}
  ~CoroTask() {
    if (m_handle && !m_handle.promise().poolPtr)
      m_handle.destroy();
  }

public:
  /**
   * @brief 同样的，我们为返回一个Awaiter对象的来实现co_await的操作符重载
   *
   */
  struct CoAwaiter {
    Handle newHdl;
    bool await_ready() const noexcept { return false; }
    void await_suspend(Handle hdl) const noexcept;
    void await_resume() const noexcept {}
  };

  auto operator co_await() noexcept {
    return CoAwaiter{std::exchange(m_handle, nullptr)};
  }

private:
  Handle m_handle;
};

class CoroPool {
private:
  std::list<std::jthread> m_threads;
  std::list<CoroTask::Handle> m_tasks;
  std::mutex m_mutex;
  std::condition_variable_any m_cv;
  std::atomic<int> m_numCoro{0};

public:
  explicit CoroPool(int num) {
    for (int i = 0; i < num; ++i) {
      std::jthread worker_thread{
          [this](std::stop_token st) { threadLoop(st); }};
      m_threads.push_back(std::move(worker_thread));
    }
  }

  ~CoroPool() {
    for (auto &thread : m_threads) {
      thread.request_stop();
    }
    for (auto &thread : m_threads) {
      thread.join();
    }

    for (auto &hdl : m_tasks) {
      if (hdl) {
        hdl.destroy();
      }
    }
  }

  // noncopy
  CoroPool(const CoroPool &) = delete;
  CoroPool &operator=(const CoroPool &) = delete;

public:
  /**
   * @brief RunTask方法用于将一个协程任务加入到线程池中
   *
   */
  void runTask(CoroTask &&task) noexcept {
    auto hdl = std::exchange(task.m_handle, nullptr);
    if (hdl.done()) {
      hdl.destroy();
    } else {
      // schedule coroutine in the pool
      runCoro(hdl);
    }
  }
  /**
   * @brief 将一个协程句柄加入到线程池中
   *
   * @param coro
   */
  void runCoro(CoroTask::Handle coro) noexcept {
    m_numCoro.fetch_add(1);
    coro.promise().poolPtr = this;
    {
      std::scoped_lock lock(m_mutex);
      m_tasks.push_front(coro);
      m_cv.notify_one();
    }
  }

  void threadLoop(std::stop_token st) {
    while (!st.stop_requested()) {
      // get next coro task from the pool
      CoroTask::Handle coro;
      {
        std::unique_lock lock(m_mutex);
        if (!m_cv.wait(lock, st, [&] { return !m_tasks.empty(); })) {
          return;
        }
        coro = m_tasks.back();
        m_tasks.pop_back();
      }

      // resume it
      coro.resume();

      // 递归销毁已经完成的协程句柄
      std::function<void(CoroTask::Handle)> destroyDone;
      destroyDone = [&destroyDone, this](CoroTask::Handle hdl) {
        if (hdl && hdl.done()) {
          auto nextHdl = hdl.promise().contHdl;
          hdl.destroy();
          m_numCoro.fetch_sub(1);
          destroyDone(nextHdl);
        }
      };

      destroyDone(coro);
      m_numCoro.notify_all();
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }

  void waitUntilNoCoro() {
    int num = m_numCoro.load();
    while (num > 0) {
      m_numCoro.wait(num);
      num = m_numCoro.load();
    }
  }
};
