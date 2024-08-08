#include <algorithm>
#include <cstddef>
#include <iostream>
#include <ranges>
#include <span>
#include <string>
#include <vector>
/**
 * @brief template<typename ElementType, size_t Extent = dynamic_extent>  class
 * span {}；// span也是一个类模板，span具有引用语义
 *
 * @tparam T
 * @tparam Sz
 * @param sp
 */
template <typename T, std::size_t Sz> void printSpan(std::span<T, Sz> sp) {
  for (const auto &elem : sp) {
    std::cout << elem << ' ';
  }
  std::cout << '\n';
}

int main() {
  std::vector<std::string> vec{"New York", "Tokyo", "Rio", "Berlin", "Sydney"};

  std::span<const std::string> sp{vec.data(), 3};
  std::cout << "First 3 elements: ";
  printSpan(sp);

  std::ranges::sort(vec);
  std::cout << "first 3 elements after sorting: ";
  printSpan(sp);

  auto oldcap = vec.capacity();
  vec.push_back("London");
  if (oldcap != vec.capacity()) {
    sp = std::span{vec.data(), 3};
  }
  std::cout << "First 3 elements after push_back: ";
  printSpan(sp);

  sp = vec;
  std::cout << "All elements: ";
  printSpan(sp);

  sp = std::span{vec.end() - 5, vec.end()};
  std::cout << "Last 5 elements: ";
  printSpan(sp);

  sp = std::span{vec}.last(4);
  std::cout << "Last 4 elements: ";
  printSpan(sp);
}