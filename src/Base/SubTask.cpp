#include <coroutine>
#include <iostream>
#include <utility>

class SubTask {
public:
  struct promise_type;
  using Handle = std::coroutine_handle<promise_type>;
  SubTask(Handle handle) : m_handle(handle) {}
  SubTask(SubTask &&rhs) noexcept : m_handle(std::exchange(rhs.m_handle, {})) {}
  ~SubTask() {
    if (m_handle)
      m_handle.destroy();
  }

private:
  Handle m_handle;

public:
  struct promise_type {
    auto initial_suspend() noexcept { return std::suspend_always{}; }
    auto final_suspend() noexcept { return std::suspend_always{}; }
    void unhandled_exception() { std::terminate(); }
    void return_void() {}
    SubTask get_return_object() { return {Handle::from_promise(*this)}; }
    Handle subtask_handle{nullptr};
  };

  bool resume() {
    if (!m_handle || m_handle.done()) {
      return false;
    }
    SubTask::Handle handle = m_handle;
    while (handle.promise().subtask_handle &&
           !handle.promise().subtask_handle.done()) {

      handle = handle.promise().subtask_handle;
    }

    handle.resume();
    return !m_handle.done();
  }
  /**
   * @brief awaiters interface
   */
  bool await_ready() noexcept { return false; }
  void await_suspend(Handle handle) noexcept {
    handle.promise().subtask_handle = m_handle;
  }
  void await_resume() noexcept {}
};

SubTask coro() {
  std::cout << "coro 1\n";
  co_await std::suspend_always{};
  std::cout << "coro 2\n";
}

SubTask callcoro() {
  std::cout << "callcoro begin\n";
  co_await coro();
  std::cout << "callcoro done\n";
}

int main() {
  std::cout << "main begin\n";
  auto task = callcoro();
  while (task.resume()) {
    std::cout << "main loop\n";
  }
  std::cout << "main end\n";
}