#include <coroutine>
#include <iostream>
#include <optional>

template <typename Container>
struct Generator {
  struct promise_type;

  using T = typename Container::value_type;

  // struct to hold the state of the coroutine
  struct promise_type {
    promise_type() = default;
    promise_type(Container& con) : end(con.end()) {}

    Generator get_return_object() {
      return Generator{Handle::from_promise(*this)};
    }

    std::suspend_always initial_suspend() noexcept {
      return {};
    }

    std::suspend_always final_suspend() noexcept {
      return {};
    }

    std::suspend_always yield_value(Container::iterator val) noexcept {
      iter = val;
      return {};
    }

    void return_void() {}

    void unhandled_exception() {
      throw;
    }

    Container::iterator iter;
    Container::iterator end;
  };

  using Handle = std::coroutine_handle<promise_type>;

  Generator(Handle coro) : coro_(std::move(coro)) {}
  ~Generator() {
    if (coro_) {
      coro_.destroy();
    }
  }

  // not copyable
  Generator(const Generator&) = delete;
  Generator& operator=(const Generator&) = delete;

  // movable
  Generator(Generator&& other) : coro_(std::move(other.coro_)) {}
  Generator& operator=(Generator&& other) {
    if (coro_) {
      coro_.destroy();
      coro_ = {};
    }

    coro_ = std::move(other.coro_);
    other.coro_ = {};
    return *this;
  }

  bool next() const {
    if (coro_) {
      coro_.resume();
      return !coro_.done();
    }

    return false;
  }

  T value() const {
    if (coro_) {
      return *coro_.promise().iter;
    }
    return {};
  }

  Container::iterator getState() const {
    return iter_;
  }

  void setState(Container::iterator state) {
    iter_ = state;
    auto end = coro_.promise().end;
    if (coro_) {
      coro_.destroy();
      coro_ = {};
    }
    *this = resumeFromState(iter_, end);
  }

  Generator resumeFromState(
      Container::iterator start,
      Container::iterator end) {
    auto& iter = start;
    while (iter != end) {
      co_yield iter++;
    }
  }

  static Generator range(Container& container) {
    auto iter = container.begin();
    while (iter != container.end()) {
      co_yield iter++;
    }
  }

 private:
  Handle coro_;
  Container::iterator iter_;
};
