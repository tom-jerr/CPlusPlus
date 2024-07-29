#include <coroutine>
#include <cstdio>
#include <exception>
#include <utility>

struct Generator {
  struct promise_type;
  using Handle = std::coroutine_handle<promise_type>;
  void next() { return m_handle.resume(); }
  bool done() { return m_handle.done(); }
  int current_val() { return m_handle.promise().current_value; }

  Generator(Generator &&rhs) noexcept
      : m_handle(std::exchange(rhs.m_handle, {})) {}
  ~Generator() {
    if (m_handle)
      m_handle.destroy();
  }

  struct promise_type {
    Generator get_return_object() {
      return Generator{Handle::from_promise(*this)};
    }
    auto initial_suspend() noexcept { return std::suspend_never{}; }
    auto final_suspend() noexcept { return std::suspend_always{}; }
    void unhandled_exception() { std::terminate(); }
    void return_void() {}
    auto yield_value(int value) {
      current_value = value;
      return std::suspend_always{};
    }

    int current_value;
  };

private:
  Generator(Handle handle) : m_handle(handle) {}
  Handle m_handle;
};

Generator range(int start, int end) {
  for (int i = start; i < end; ++i)
    co_yield i;
}

int main() {
  Generator gen = range(0, 10);
  while (!gen.done()) {
    gen.next();
    printf("%d\n", gen.current_val());
  }
  return 0;
}
