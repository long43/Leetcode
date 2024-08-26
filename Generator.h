#include <coroutine>
#include <iostream>

template <typename T>
  requires(std::is_integral_v<T>)
struct Generator {
  struct promise_type;
  using Handle = std::coroutine_handle<promise_type>;

  struct promise_type {
    Generator get_return_object() {
      return Generator{Handle::from_promise(*this)};
    }

    std::suspend_always initial_suspend() noexcept {
      return {};
    }

    std::suspend_always final_suspend() noexcept {
      return {};
    }

    void return_void() {}

    void unhandled_exception() {
      throw;
    }

    std::suspend_always yield_value(T val) {
      value = std::move(val);
      return {};
    }

    T value;
  };

  Generator(Handle handle) : handle_(std::move(handle)) {}

  ~Generator() {
    if (handle_) {
      handle_.destroy();
    }
  }

  // not copyable
  Generator(const Generator&) = delete;
  Generator& operator=(const Generator&) = delete;

  // move constructor
  Generator(Generator&& other) : handle_(std::move(other.handle_)) {}

  // move assignment operator
  Generator& operator=(Generator&& other) {
    // destroy the handle first
    if (handle_) {
      handle_.destroy();
      handle_ = {};
    }
    handle_ = std::move(other.handle_);
    other.handle_ = {};
    return *this;
  }

  bool next() {
    if (handle_) {
      handle_.resume();
      return !handle_.done();
    }
    return false;
  }

  T value() {
    return handle_.promise().value;
  }

  void setState(T first, T last) {
    // recreate the generator
    if (handle_) {
      handle_.destroy();
      handle_ = {};
    }
    *this = range(first, last);
  }

  static Generator range(T first, T last) {
    T state = first;
    while (state < last) {
      co_yield state;
      state++;
    }
  }

 private:
  Handle handle_;
};