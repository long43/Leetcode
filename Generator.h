#include <coroutine>
#include <iostream>
#include <iterator>

template <typename Container>
class Generator {
 public:
  struct promise_type;
  using Iter = typename Container::iterator;
  using T = typename Container::value_type;
  using Coro = std::coroutine_handle<promise_type>;

  struct promise_type {
    promise_type() {
      std::cout << "create the promise_type" << std::endl;
    }

    promise_type(Container& container)
        : iter(container.begin()), end(container.end()) {
      std::cout << "create the promise_type with container" << std::endl;
    }

    Generator get_return_object() {
      return Generator{Coro::from_promise(*this)};
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

    // store the state of the coroutine
    Iter iter;
    Iter end;
  };

  Generator(Coro coro) : coro_(std::move(coro)) {}

  ~Generator() {
    if (coro_) {
      coro_.destroy();
    }
  }

  // non copyable
  Generator(const Generator&) = delete;
  Generator& operator=(const Generator&) = delete;

  // move constructible
  Generator(Generator&& other) : coro_(std::move(other.coro_)) {}

  // move assignable
  Generator& operator=(Generator&& other) {
    if (coro_) {
      coro_.destroy();
      coro_ = {};
    }
    coro_ = std::move(other.coro_);
    other.coro_ = {};
    return *this;
  }

  Iter getState() const {
    return coro_.promise().iter;
  }

  void setState(Iter it) {
    auto end = coro_.promise().end;
    if (coro_) {
      coro_.destroy();
      coro_ = {};
    }
    std::cout << "recreate the generator " << std::endl;
    *this = createGenerator(it, end);
  }

  bool hasNext() const {
    if (!coro_ || coro_.done()) {
      return false;
    }
    coro_.resume();
    return !coro_.done();
  }

  T next() const {
    return *coro_.promise().iter;
  }

  struct Iterator {
    Iterator(Coro coro) : coro_(std::move(coro)) {}

    // *
    const T& operator*() const {
      return *coro_.promise().iter;
    }

    // ++iter
    Iterator& operator++() {
      coro_.resume();
      return coro_.promise().iter;
    }

    // iter++
    Iterator operator++(int) {
      Iterator it = *this;
      coro_.resume();
      return it;
    }

    // ==end
    bool operator==(std::default_sentinel_t) {
      return !coro_ || coro_.done();
    }

   private:
    Coro coro_;
  };

  Iterator begin() {
    return Iterator{coro_};
  }

  std::default_sentinel_t end() {
    return {};
  }

 private:
  Generator createGenerator(Iter start, Iter end) {
    Iter it = start;
    while (it != end) {
      co_yield it++;
    }
  }

  Coro coro_;
};

template <typename Container>
Generator<Container> range(Container& container) {
  auto it = container.begin();
  while (it != container.end()) {
    co_yield it++;
  }
}
