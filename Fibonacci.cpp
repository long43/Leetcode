#include <iostream>

template <std::size_t N>
int fibonacci() {
  if constexpr (N == 0) {
    return 0;
  } else if constexpr (N == 1) {
    return 1;
  } else {
    return fibonacci<N - 1>() + fibonacci<N - 2>();
  }
}

int main() {
  std::cout << "result is " << fibonacci<19>() << std::endl;
}