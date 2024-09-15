#include <iterator>
#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include <coroutine>
#include <optional>
#include <list>
#include <vector>

template <typename Container>
class Iterator {
public:
  struct promise_type;
  using Handle = std::coroutine_handle<promise_type>;
  using Iter = typename Container::iterator;
  using T = typename Container::value_type;

  struct promise_type {
    promise_type() = default;
    explicit promise_type(Container& con) : container(&con), iter(container->begin()) {}

    Iterator get_return_object() {
      return Iterator{Handle::from_promise(*this)};
    }

    std::suspend_always initial_suspend() noexcept {
      return {};
    }

    std::suspend_always final_suspend() noexcept {
      return {};
    }

    std::suspend_always yield_value(Iter it) noexcept {
      iter = it;
      return {};
    }

    void return_void() {}

    void unhandled_exception() {
      throw;
    }

    Container* container;
    Iter iter;
  };

  explicit Iterator(Handle handle) : handle_(handle), container_(handle_.promise().container) {}
  ~Iterator() {
    if (handle_) {
      handle_.destroy();
    }
  }

  //non-copyable
  Iterator(const Iterator&) = delete;
  Iterator& operator=(const Iterator&) = delete;

  // moveable
  Iterator(Iterator&& other) : handle_(other.handle_) {
    other.handle_ = {};
  }

  Iterator& operator=(Iterator&& other) {
    if (this != &other) {
      if (handle_) {
        handle_.destroy();
        handle_ = {};
      }
      handle_ = other.handle_;
      other.handle_ = {};
    }
    return *this;
  }

  std::optional<T> next() {
    if (!handle_ || handle_.done()) {
      return std::nullopt;
    }
    handle_.resume();
    if (handle_.done()) {
      return std::nullopt;
    }
    return *handle_.promise().iter;
  }

  Iter getState() {
    if (handle_.done()) {
      return container_->end();
    }
    return handle_.promise().iter;
  }

  void setState(Iter state) {
    if (handle_) {
      handle_.destroy();
      handle_ = {};
    }
    *this = create(state, container_->end());
  }

private:
  Iterator create(Iter begin, Iter end) {
    auto it = begin;
    while (it != end) {
      co_yield it++;
    }
  }

  Handle handle_;
  Container* container_;
};

template <typename Container> 
Iterator<Container> range(Container& container) {
  auto it = container.begin();
  while (it != container.end()) {
    co_yield it++;
  }
}

template <typename Container>
class MultiIterator {
public:
  using Inner = typename Container::value_type;
  using T = typename Inner::value_type;
  Inner kEmptyInner{};
  explicit MultiIterator(Container& con) : container_(con), row_(range(container_)), rval_(row_.next()), col_(range(rval_ ? (*rval_) : kEmptyInner)) {}

  struct State {
    size_t rIndex;
    size_t cIndex;
  };

  State getState() {
    auto rIt = row_.getState();
    auto cIt = col_.getState();

    auto rIdx = std::distance(container_.begin(), rIt);
    auto cIdx = rval_ ? std::distance(rval_->begin(), cIt) : 0;
    return State {.rIndex = static_cast<size_t>(rIdx), .cIndex = static_cast<size_t>(cIdx)};
  }

  void setState(State state) {
    auto rIt = container_.begin();
    std::advance(rIt, state.rIndex);
    row_ = range(container_);
    row_.setState(rIt);

    rval_ = row_.next();
    if (rval_) {
      auto cIt = rval_->begin();
      std::advance(cIt, state.cIndex);
      col_ = range(*rval_);
      col_.setState(cIt);
    }
    else {
      col_ = range(kEmptyInner);
    }
  }

  std::optional<T> next() {
    auto cval = col_.next();
    // keep finding the next row if the row is not at end but col is at the end
    while (rval_ && !cval) {
      rval_ = row_.next();
      col_ = range(rval_ ? (*rval_) : kEmptyInner);
      cval = col_.next();
    }

    // no more new rows
    if (!rval_) {
      return std::nullopt;
    }

    return cval;
  }

private:
  Container& container_;
  Iterator<Container> row_;
  std::optional<Inner> rval_;
  Iterator<Inner> col_;
};



TEST_CASE( "Empty list", "[Iterator]" ) {
  std::list<int> list = {};
  auto iter = range(list);
  std::vector<int> res;
  while (auto val = iter.next()) {
    res.push_back(*val);
  }

  REQUIRE(res.size() == 0);
}

TEST_CASE( "Empty vector", "[Iterator]" ) {
  std::vector<int> vector = {};
  auto iter = range(vector);
  std::vector<int> res;
  while (auto val = iter.next()) {
    res.push_back(*val);
  }

  REQUIRE(res.size() == 0);
}

TEST_CASE( "Reset state in middle", "[Iterator]" ) {
  std::vector<int> vector = {1,2,3,4,5,6,7,8,9};
  auto iter = range(vector);
  std::vector<int> res;
  iter.next(); // 1
  iter.next(); // 2
  iter.next(); // 3
  iter.next(); // 4
  auto state = iter.getState();
  iter.next(); // 5
  iter.next(); // 6
  auto val = iter.next(); // 7
  REQUIRE(*val == 7);
  iter.setState(state);
  val = iter.next(); 
  REQUIRE(*val == 4);
}

TEST_CASE( "Reset state at the beginning", "[Iterator]" ) {
  std::vector<int> vector = {1,2,3,4,5,6,7,8,9};
  auto iter = range(vector);
  auto state = iter.getState();
  std::vector<int> res;
  iter.next(); // 1
  iter.next(); // 2
  iter.next(); // 3
  iter.next(); // 4
  iter.next(); // 5
  iter.next(); // 6
  auto val = iter.next(); // 7
  REQUIRE(*val == 7);
  iter.setState(state);
  val = iter.next(); 
  REQUIRE(*val == 1);
}

TEST_CASE( "Reset state at the end", "[Iterator]" ) {
  std::vector<int> vector = {1,2,3,4,5,6,7,8,9};
  auto iter = range(vector);
  while (iter.next()) {}
  auto state = iter.getState();
  iter.setState(state);
  REQUIRE(iter.next() == std::nullopt);
}

TEST_CASE( "MultiIterator", "[MultiIterator]" ) {
  std::list<std::vector<int>> container = {{}, {1,2}, {3}, {}, {}, {4,5,6}, {7}, {}, {}};
  std::vector<int> res;
  MultiIterator<std::list<std::vector<int>>> iter{container};
  while (auto val = iter.next()) {
    res.push_back(*val);
  }
  REQUIRE(res.size() == 7);
  REQUIRE(res == std::vector{1,2,3,4,5,6,7});
}

TEST_CASE( "MultiIterator Resuming", "[MultiIterator]" ) {
  std::list<std::vector<int>> container = {{}, {1,2}, {3}, {}, {}, {4,5,6}, {7}, {}, {}};
  std::vector<int> res;
  MultiIterator<std::list<std::vector<int>>> iter{container};
  iter.next(); // 1
  iter.next(); // 2
  iter.next(); // 3
  auto state = iter.getState();
  while (iter.next()) {}
  iter.setState(state);
  auto val = iter.next();
  REQUIRE(val.has_value() == true);
  REQUIRE(val.value() == 3);
}
