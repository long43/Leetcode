#include <bits/iterator_concepts.h>
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <optional>
#include <vector>
#include <list>
#include <coroutine>
#include <iostream>

template <typename Container>
class Iterator {
public:
  using Iter = typename Container::iterator;
  using T = typename Container::value_type;

  explicit Iterator(Container& c) : container_(&c), iter_(c.begin()) {}
  virtual ~Iterator() = default;
  
  virtual std::optional<T> next() {
    if (iter_ == container_->end()) {
      return std::nullopt;
    }
    T val = *iter_;
    iter_++;
    return val;
  }

protected:
  Container* container_;
  Iter iter_;
};

template <typename Container>
class RIterator : public Iterator<Container> {
public:
  using Iter = typename Iterator<Container>::Iter;
  using T = typename Iterator<Container>::T;
  explicit RIterator(Container& c) : Iterator<Container>(c) {}

  virtual Iter getState() {
    return this->iter_;
  }

  virtual void setState(Iter it) {
    this->iter_ = it;
  }
};

template <typename Container>
class CoroIterator : public RIterator<Container> {
public:
  struct promise_type;
  using Handle = std::coroutine_handle<promise_type>;
  using Iter = typename RIterator<Container>::Iter;
  using T = typename RIterator<Container>::T;

  struct promise_type {
    Container kEmptyContainer{};
    promise_type() : container(&kEmptyContainer), iter(kEmptyContainer.begin()) {}
  
    explicit promise_type(Container& c) : container(&c), iter(c.begin()) {}

    CoroIterator get_return_object() {
      return CoroIterator{Handle::from_promise(*this)};
    }

    std::suspend_never initial_suspend() noexcept {
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
    void unhandled_exception() {}

    Container* container;
    Iter iter;
  };

  explicit CoroIterator(Handle handle) : RIterator<Container>(*handle.promise().container), handle_(handle) {}

  ~CoroIterator() {
    if (handle_) {
      handle_.destroy();
      handle_ = {};
    }
  }

  // non-copyable
  CoroIterator(const CoroIterator&) = delete;
  CoroIterator& operator=(const CoroIterator&) = delete;

  // moveable
  CoroIterator(CoroIterator&& other) : RIterator<Container>(*other.handle_.promise().container), handle_(other.handle_) {}
  CoroIterator& operator=(CoroIterator&& other) {
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

  std::optional<T> next() override {
    if (!handle_ || handle_.done()) {
      return std::nullopt;
    }
    auto val = *handle_.promise().iter;
    handle_.resume();
    return val;
  }

  Iter getState() override {
    if (!handle_ || handle_.done()) {
      return this->container_->end();
    }
    return handle_.promise().iter;
  }

  void setState(Iter state) override {
    if (handle_) {
      handle_.destroy();
      handle_ = {};
    }
    *this = create(state);
  }

private:
  CoroIterator create(Iter state) {
    auto it = state;
    while (it != this->container_->end()) {
      co_yield it++;
    }
  }

  Handle handle_;
};

template <typename Container>
CoroIterator<Container> range(Container& c) {
  auto it = c.begin();
  while (it != c.end()) {
    co_yield it++;
  }
}


template <typename Container>
class MultiIterator {
public:
  using Inner = typename Container::value_type;
  using T = typename Inner::value_type;
  using Iter = typename CoroIterator<Container>::Iter;
  Inner kEmptyInner{};

  explicit MultiIterator(Container& c) : container_(&c), row_(range(c)), rval_(row_.next()),col_(range(rval_ ? *rval_ : kEmptyInner))  {}
  ~MultiIterator() = default;

  std::optional<T> next() {
    auto cval = col_.next();
    while (rval_ && !cval) {
      rval_ = row_.next();
      col_ = range(rval_ ? *rval_ : kEmptyInner);
      cval = col_.next();
    }
    if (!rval_) {
      return std::nullopt;
    }
    return cval;
  }

  struct State {
    Iter row;
    size_t cIndex;
  };

  State getState() {
    size_t cIdx = 0;
    if (rval_) {
      cIdx = std::distance(rval_->begin(), col_.getState());
      if (cIdx >= rval_->size()) {
        cIdx = 0;
      }
    }
    auto rIndex = std::distance(container_->begin(), row_.getState());
    std::cout << "get the state at row " << rIndex << " and col " << cIdx << std::endl;
    return State{.row = row_.getState(), .cIndex = static_cast<size_t>(cIdx)};
  }

  void setState(State state) {
    row_.setState(state.row);
    rval_ = row_.next();
    if (rval_) {
      auto cIt = rval_->begin();
      std::advance(cIt, state.cIndex);
      col_ = range(*rval_);
      col_.setState(cIt);
    } else {
      col_ = range(kEmptyInner);
    }
  }

private:
  Container* container_;
  CoroIterator<Container> row_;
  std::optional<Inner> rval_;
  CoroIterator<Inner> col_;
};


TEST_CASE( "Simple Vector Iterator", "[Iterator]" ) {
  std::vector<int> vec = {1,2,3,4,5,6,7};
  auto iter = Iterator(vec);
  std::vector<int> res;
  while (auto val = iter.next()) {
    res.push_back(*val);
  }
  REQUIRE(res == vec);
}

TEST_CASE( "Simple List Iterator", "[Iterator]" ) {
  std::list<int> list = {1,2,3,4,5,6,7};
  auto iter = Iterator(list);
  std::vector<int> res;
  while (auto val = iter.next()) {
    res.push_back(*val);
  }
  REQUIRE(res == std::vector<int>{1,2,3,4,5,6,7});
}

TEST_CASE( "Resume state from mid", "[Resumeable Iterator]" ) {
  std::list<int> list = {1,2,3,4,5,6,7};
  auto iter = RIterator(list);
  iter.next(); // 1
  iter.next(); // 2
  iter.next(); // 3
  auto state = iter.getState();
  while (iter.next()) {}
  iter.setState(state);
  auto val = iter.next();
  REQUIRE(val != std::nullopt);
  REQUIRE(*val == 4);
}

TEST_CASE( "Resume state from begin", "[Resumeable Iterator]" ) {
  std::list<int> list = {1,2,3,4,5,6,7};
  auto iter = RIterator(list);
  auto state = iter.getState();
  while (iter.next()) {}
  iter.setState(state);
  auto val = iter.next();
  REQUIRE(val != std::nullopt);
  REQUIRE(*val == 1);
}

TEST_CASE( "Resume state from end", "[Resumeable Iterator]" ) {
  std::list<int> list = {1,2,3,4,5,6,7};
  auto iter = RIterator(list);
  while (iter.next()) {}

  auto state = iter.getState();
  iter.setState(state);
  auto val = iter.next();
  REQUIRE(val == std::nullopt);
}

TEST_CASE( "Simple Vector CoroIterator", "[Iterator]" ) {
  std::vector<int> vec = {1,2,3,4,5,6,7};
  auto iter = range(vec);
  std::vector<int> res;
  while (auto val = iter.next()) {
    res.push_back(*val);
  }
  REQUIRE(res == vec);
}

TEST_CASE( "Simple List CoroIterator", "[Iterator]" ) {
  std::list<int> list = {1,2,3,4,5,6,7};
  auto iter = range(list);
  std::vector<int> res;
  while (auto val = iter.next()) {
    res.push_back(*val);
  }
  REQUIRE(res == std::vector<int>{1,2,3,4,5,6,7});
}

TEST_CASE( "CoroIterator: Resume state from mid", "[Iterator]" ) {
  std::list<int> list = {1,2,3,4,5,6,7};
  auto iter = range(list);
  iter.next(); // 1
  iter.next(); // 2
  iter.next(); // 3
  auto state = iter.getState();
  while (iter.next()) {}
  iter.setState(state);
  auto val = iter.next();
  REQUIRE(val != std::nullopt);
  REQUIRE(*val == 4);
}

TEST_CASE( "CoroIterator: Resume state from begin", "[Iterator]" ) {
  std::list<int> list = {1,2,3,4,5,6,7};
  auto iter = range(list);
  auto state = iter.getState();
  while (iter.next()) {}
  iter.setState(state);
  auto val = iter.next();
  REQUIRE(val != std::nullopt);
  REQUIRE(*val == 1);
}

TEST_CASE( "CoroIterator: Resume state from end", "[Iterator]" ) {
  std::list<int> list = {1,2,3,4,5,6,7};
  auto iter = range(list);
  while (iter.next()) {}

  auto state = iter.getState();
  iter.setState(state);
  auto val = iter.next();
  REQUIRE(val == std::nullopt);
}

TEST_CASE( "MultiIterator: simple", "[Iterator]" ) {
  std::list<std::vector<int>> list = {{}, {}, {1}, {2, 3}, {4}, {}, {5,6}, {7}, {}, {}};
  auto iter = MultiIterator(list);
  std::vector<int> res;
  while (auto val = iter.next()) {
    res.push_back(*val);
  }
  REQUIRE(res == std::vector<int>{1,2,3,4,5,6,7});
}

TEST_CASE( "MultiIterator: resume state", "[Iterator]" ) {
  std::list<std::vector<int>> list = {{}, {}, {1}, {2, 3}, {4}, {}, {5,6}, {7}, {}, {}};
  auto iter = MultiIterator(list);
  iter.next(); // 1
  iter.next(); // 2
  iter.next(); // 3
  auto state = iter.getState();
  while (iter.next()) {}
  iter.setState(state);
  auto val = iter.next();
  REQUIRE(val != std::nullopt);
  REQUIRE(*val == 4);
}


