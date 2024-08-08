#include <algorithm>
#include <array>
#include <cassert>
#include <iostream>
#include <vector>

void printColl(const auto &coll) {
  for (const auto &elem : coll) {
    std::cout << elem << ' ';
  }
  std::cout << std::endl;
}

bool lessNameFunc(const auto &c1, const auto &c2) { return c1 < c2; }

auto lessNameFunclambda = [](const auto &c1, const auto &c2) {
  return c1 < c2;
};

int main() {
  // printColl(64);
  printColl(std::array<int, 3>{1, 2, 3});
  printColl<std::vector<int>>(std::vector<int>{1, 2, 3, 4, 5});

  std::vector<int> vec{3, 2, 1, 4, 5};
  std::vector<int> vec2{3, 2, 1, 4, 5};
  std::sort(vec.begin(), vec.end(), lessNameFunc<int, int>);
  std::sort(vec2.begin(), vec2.end(), lessNameFunclambda);
  for (int i = 0; i < static_cast<int>(vec.size()); ++i) {
    assert(vec.at(i) == vec2.at(i));
  }
}