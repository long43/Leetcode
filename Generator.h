#include <coroutine>
#include <vector>
#include <string>
#include <iterator>
#include <optional>
#include <iostream>

template <typename Container>
class Generator {
public:
  struct promise_type;
  using T = typename Container::value_type;
  using Iter = typename Container::iterator;
  using Handle = std::coroutine_handle<promise_type>;

  struct promise_type {
    promise_type() = default;
    explicit promise_type(Container& ct) : iter(ct.begin()), end(ct.end()) {}

    Generator get_return_object() {
      return Generator{Handle::from_promise(*this)};
    }

    std::suspend_always initial_suspend() noexcept {
      return {};
    }

    std::suspend_always final_suspend() noexcept {
      return {};
    }

    std::suspend_always yield_value(Iter it) {
      iter = it;
      return {};
    }

    void return_void() {}
    
    void unhandled_exception() {
      throw;
    }

    Iter iter;
    Iter end;
  };
  
  explicit Generator(Handle coro) : coro_(coro) {}
  ~Generator() {
    if (coro_) {
      coro_.destroy();
    }
  }

  // non-copyable
  Generator(const Generator&) = delete;
  Generator& operator=(const Generator&) = delete;

  // moveable
  Generator(Generator&& other) : coro_(other.coro_) {}

  Generator& operator=(Generator&& other) {
    if (this != &other) {
      if (coro_) {
        coro_.destroy();
        coro_ = {};
      }
      coro_ = other.coro_;
      other.coro_ = {};
    }
    return *this;
  }

  std::optional<T> next() {
    if (coro_ && !coro_.done()) {
      coro_.resume();
    }
    if (!coro_ || coro_.done()) {
      return std::nullopt;
    }
    return *coro_.promise().iter;
  }

  Iter getState() const {
    if (coro_.done()) {
      return coro_.promise().end;
    }
    return coro_.promise().iter;
  }

  void setState(Iter state) {
    // preserve the end
    Iter end = coro_.promise().end;
    // reset coroutine
    if (coro_) {
      coro_.destroy();
      coro_ = {};
    }

    *this = createGenerator(state, end);
  }

  Generator createGenerator(Iter begin, Iter end) {
    auto it = begin;
    while (it != end) {
      co_yield it++;
    }
  }

private:
  Handle coro_;
};

template <typename Container>
Generator<Container> range(Container& container) {
  typename Container::iterator it = container.begin();
  while (it != container.end()) {
    co_yield it++;
  }
}
