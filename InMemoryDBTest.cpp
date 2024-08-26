#include "InMemoryDB.h"

using Pair = std::pair<std::string, std::variant<int, std::string>>;

int main() {
  InMemoryDB db;

  InMemoryDB::Row row1{{"Id", 3}, {"Name", "Michael Jackson"}, {"Age", 42}};
  InMemoryDB::Row row2{{"Id", 2}, {"Name", "Jannet Jackson"}, {"Age", 42}};
  InMemoryDB::Row row3{{"Id", 1}, {"Name", "Donald Trump"}, {"Age", 78}};
  db.addRow(row1, row2, row3);
  auto results = db.query(db.orderBy("Id"), Pair{"Age", 42});

  for (const auto& row : results) {
    for (const auto& [key, value] : row) {
      std::cout << key << ": ";
      print(value);
      std::cout << std::endl;
    }
  }

  db.remove(Pair{"Age", 42});
  db.dump();

  return 0;
}