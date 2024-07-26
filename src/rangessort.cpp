#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
void print(const auto &coll) {
  for (const auto &elem : coll) {
    std::cout << elem << ' ';
  }
  std::cout << std::endl;
}

int main() {
  std::vector<std::string> coll{"hello", "world", "c++", "20"};

  std::ranges::sort(coll);
  std::ranges::sort(coll[0]);
  print(coll);

  int arr[] = {1, 2, 3, 4, 5};
  std::ranges::sort(arr);
  print(arr);
}