#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

// helper type for the visitor #4
template <class... Ts>
struct overloaded : Ts... {
  using Ts::operator()...;
};
// explicit deduction guide (not needed as of C++20)
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

void print(std::variant<int, std::string> v) {
  std::visit(overloaded{[](auto arg) { std::cout << arg << ' '; }}, v);
}

class InMemoryDB {
 public:
  using Row = std::unordered_map<std::string, std::variant<int, std::string>>;
  using Pair = std::pair<std::string, std::variant<int, std::string>>;

  InMemoryDB() = default;
  ~InMemoryDB() = default;

  // not copiable and movable
  InMemoryDB(const InMemoryDB&) = delete;
  InMemoryDB& operator=(const InMemoryDB&) = delete;
  InMemoryDB(InMemoryDB&&) = delete;
  InMemoryDB& operator=(InMemoryDB&&) = delete;

  // Base case: No arguments left, no-op
  void addRow() {}

  template <typename... Rows>
  void addRow(Row first, Rows... rows) {
    db_.emplace_back(std::move(first));
    addRow(rows...);
  }

  template <typename F>
  std::vector<Row> find(F predict) {
    std::vector<Row> result;
    for (const auto& row : db_) {
      if (predict(row)) {
        result.push_back(row);
      }
    }
    return result;
  }

  bool isPairMatch(const Row& row, const Pair& pair) const {
    if (const auto& iter = row.find(pair.first); iter != row.end()) {
      if (iter->second == pair.second) {
        return true;
      }
    }
    return false;
  }

  template <typename... Pairs>
  std::vector<Row> filter(Pairs&&... pairs) {
    // Define a lambda function for each pair and use it in find
    auto predict = [this, pairs...](const Row& row) {
      return (isPairMatch(row, pairs) && ...);
    };

    return find(predict);
  }

  template <typename... Keys>
  bool compare(const Row& row1, const Row& row2, Keys&&... keys) {
    if constexpr (sizeof...(keys) == 0) {
      return false;
    }
    return ((row1.at(keys) < row2.at(keys)) && ...);
  }

  template <typename... Pairs, typename F>
  std::vector<Row> query(F orderBy, Pairs&&... pairs) {
    auto results = filter(pairs...);
    std::sort(results.begin(), results.end(), orderBy);
    return results;
  }

  /*
   * Return a lambda that takes the variadic argument list to compare rows
   */
  template <typename... Keys>
  auto orderBy(Keys... keys) {
    return [keys..., this](const Row& row1, const Row& row2) {
      return compare(row1, row2, keys...);
    };
  }

  template <typename F>
  void removeIf(F predict) {
    std::erase_if(db_, predict);
  }

  template <typename... Pairs>
  void remove(Pairs&&... pairs) {
    auto predict = [this, pairs...](const Row& row) {
      return (isPairMatch(row, pairs) && ...);
    };
    removeIf(predict);
  }

  void dump() {
    std::cout << "dump the db" << std::endl;
    for (const auto& row : db_) {
      for (const auto& [key, value] : row) {
        std::cout << key << ": ";
        print(value);
        std::cout << std::endl;
      }
    }
  }

 private:
  std::vector<Row> db_;
};
