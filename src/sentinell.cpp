#include <algorithm>
#include <iostream>

struct NullTerm {
  bool operator==(auto pos) const { return *pos == '\0'; }
};

int main() {
  const char *rawString = "hello world";
  for (auto pos = rawString; pos != NullTerm{}; ++pos) {
    std::cout << *pos;
  }
  std::cout << std::endl;

  std::ranges::for_each(rawString, NullTerm{}, [](char c) { std::cout << c; });
  std::cout << std::endl;
}