#include <initializer_list>
#include <iostream>
#include <ranges>

template <typename T> class ValOrColl {
  T val;

public:
  ValOrColl(const T &v) : val(v) {}
  ValOrColl(T &&v) : val(std::move(v)) {}

  void print() const { std::cout << val << std::endl; }

  void print() const requires std::ranges::range<T> {
    std::cout << "Collection: ";
    for (const auto &elem : val) {
      std::cout << elem << ' ';
    }
    std::cout << std::endl;
  }
};

int main() {
  ValOrColl v1{42};
  ValOrColl v2{std::initializer_list<int>{1, 2, 3, 4}};
  v1.print();
  v2.print();
}