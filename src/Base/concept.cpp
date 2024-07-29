#include <iostream>

template <typename T>
concept IsPointer = std::is_pointer_v<T>;

template <typename T>
requires(!IsPointer<T>) T maxVal(T a, T b) { return a > b ? a : b; }

template <typename T>
requires IsPointer<T>
auto maxVal(T a, T b) { return maxVal(*a, *b); };

auto maxVal(IsPointer auto a, IsPointer auto b)
    // 指针所指向的值必须具有可比性
    requires std::totally_ordered_with<decltype(*a), decltype(*b)> {
  return maxVal(*a, *b);
}

int main() {
  int a = 1, b = 2;
  double c = 1.1, d = 2.2;
  int *pa = &a, *pb = &b;

  auto max1 = maxVal(a, b);
  auto max2 = maxVal(c, d);
  auto max3 = maxVal(pa, pb);

  std::cout << max1 << std::endl;
  std::cout << max2 << std::endl;
  std::cout << max3 << std::endl;
  // maxVal(&a, &b); // error
}