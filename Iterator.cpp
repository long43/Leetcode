#include <iostream>
#include <vector>

#include "Generator.h"

class MultiIterator {
 public:
  std::vector<int> kEmptyVec{};
  explicit MultiIterator(std::vector<std::vector<int>>& vec)
      : row_(range(vec)) {
    rval_ = row_.next();
    col_ = rval_ ? range(*rval_) : range(kEmptyVec);
  }

  std::optional<int> next() {
    auto cval = col_.next();
    while (rval_ != std::nullopt && cval == std::nullopt) {
      rval_ = row_.next();
      col_ = rval_ ? range(*rval_) : range(kEmptyVec);
      cval = col_.next();
    }

    if (rval_ == std::nullopt) {
      return std::nullopt;
    }

    return cval;
  }

 private:
  Generator<std::vector<std::vector<int>>> row_;
  Generator<std::vector<int>> col_ = range(kEmptyVec);
  std::optional<std::vector<int>> rval_;
};

int main() {
  std::vector<std::vector<int>> vec{
      {}, {1, 2, 3}, {}, {4, 5}, {}, {6}, {7}, {}};

  MultiIterator iter(vec);
  while (auto val = iter.next()) {
    std::cout << "the item is " << *val << std::endl;
  }
}