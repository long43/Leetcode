#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

using Value = std::variant<int, std::string>;
using Row = std::unordered_map<std::string, Value>;
using Pair = std::pair<std::string, Value>;

void print(const Value& value) {
  if (std::holds_alternative<int>(value)) {
    std::cout << std::get<int>(value) << " ";
  } else if (std::holds_alternative<std::string>(value)) {
    std::cout << std::get<std::string>(value) << " ";
  }
}

class InMemoryDB {
 public:
  InMemoryDB() = default;
  ~InMemoryDB() = default;

  // non copyable and non moveable
  InMemoryDB(const InMemoryDB&) = delete;
  InMemoryDB& operator=(const InMemoryDB&) = delete;
  InMemoryDB(InMemoryDB&&) = delete;
  InMemoryDB& operator=(InMemoryDB&&) = delete;

  void addRows() {
    // no op
  }

  template <typename... Rows>
  void addRows(Row first, Rows&&... rows) {
    db_.emplace_back(std::move(first));
    addRows(rows...);
  }

  // filter
  template <typename... Pairs>
  std::vector<Row> filter(Pairs&&... pairs) {
    return find(predict(pairs...));
  }

  // return a lambda can be used in std::sort
  template <typename... Keys>
  auto orderby(Keys&&... keys) {
    return [keys..., this](const Row& row1, const Row& row2) {
      return compare(row1, row2, keys...);
    };
  }

  // query with order by
  template <typename F, typename... Keys>
  std::vector<Row> query(F&& orderBy, Keys&&... keys) {
    auto results = filter(keys...);
    std::sort(results.begin(), results.end(), orderBy);
    return results;
  }

  template <typename... Pairs>
  void remove(Pairs... pairs) {
    std::erase_if(db_, predict(pairs...));
  }

  void dump() {
    std::cout << "dump the db: " << std::endl;
    for (const auto& row : db_) {
      for (const auto& [key, value] : row) {
        std::cout << key << ": ";
        print(value);
      }
      std::cout << std::endl;
    }
  }

 private:
  template <typename... Pairs>
  auto predict(Pairs... pairs) {
    return [pairs..., this](const auto& row) {
      return (match(row, pairs) && ...);
    };
  }

  template <typename F>
  std::vector<Row> find(F&& predict) {
    std::vector<Row> results;
    for (const auto& row : db_) {
      if (predict(row)) {
        results.push_back(row);
      }
    }

    return results;
  }

  template <typename... Keys>
  bool compare(const Row& row1, const Row& row2, Keys... keys) {
    return ((row1.at(keys) < row2.at(keys)) || ...);
  }

  // check if the row contains the key (pair.first) and value
  bool match(const Row& row, const Pair& pair) {
    if (const auto& iter = row.find(pair.first); iter != row.end()) {
      return iter->second == pair.second;
    }
    return false;
  }

  std::vector<Row> db_;
};
