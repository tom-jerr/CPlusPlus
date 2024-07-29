#include <iostream>
#include <map>
#include <ranges>
#include <string_view>

int main() {
  namespace vws = std::views;

  std::map<std::string, int> composers{
      {"Johann Sebastian Bach", 1685}, {"Ludwig van Beethoven", 1770},
      {"Richard Wagner", 1813},        {"Claude Debussy", 1862},
      {"Arnold Schönberg", 1874},      {"Aaron Copland", 1900},
      {"Benjamin Britten", 1913}};

  for (const auto &elem : composers | vws::filter([](const auto &year) {
                            return year.second > 1800;
                          }) | vws::take(3) |
                              vws::filter([](const auto &key) {
                                return std::string_view(key.first)[0] == 'A';
                              })) {
    std::cout << elem.first << ' ' << elem.second << std::endl;
  }
}