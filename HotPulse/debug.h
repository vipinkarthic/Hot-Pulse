#pragma once
#include <cstring>
#include <iostream>
#include <string>

namespace DBG {
template <typename T>
void print_char_array(T* iterable, int size, std::string const& statement) {
  std::cout << statement << ": [";
  if constexpr (std::is_same_v<T, char> || std::is_same_v<T, const char>) {
    std::cout << iterable;
  } else {
    for (int i = 0; i < size; i++) {
      std::cout << iterable[i] << " ";
    }
  }
  std::cout << "]" << std::endl;
}

template <typename Iterable>
void print_iterable(Iterable const& iterable, std::string const& statement) {
  std::cout << statement << ": [";
  bool first = true;
  for (auto const& item : iterable) {
    if (!first) std::cout << " ";
    std::cout << item;
    first = false;
  }
  std::cout << "]\n";
}
}  // namespace DBG
