#include "Generator.h"

#include <coroutine>
#include <iostream>

int main() {
  auto gen = Generator<int>::range(1, 50);
  if (gen.next()) {
    std::cout << gen.value() << " ";
  }
  std::cout << "\n";

  if (gen.next()) {
    std::cout << gen.value() << " ";
  }
  std::cout << "\n";

  gen.setState(5, 11);
  while (gen.next()) {
    std::cout << gen.value() << " ";
  }

  std::cout << '\n';
}