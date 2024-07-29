#include <concepts>
#include <iostream>
template <typename T>
concept IsPointer = requires(T p) {
  *p;           // 指针解引用
  p == nullptr; // 指针比较
  { p < p } -> std::convertible_to<bool>;
};

// automaxval
auto maxval(auto a, auto b) { return a > b ? a : b; }

// automaxval to pointer
auto maxval(IsPointer auto a, IsPointer auto b) requires
    std::totally_ordered_with<decltype(*a), decltype(*b)> {
  std::cout << "compare pointer" << std::endl;
  return maxval(*a, *b);
}

int main() {
  int a = 1, b = 2;
  double c = 1.1, d = 2.2;
  int *pa = &a, *pb = &b;

  auto max1 = maxval(a, b);
  auto max2 = maxval(c, d);
  auto max3 = maxval(pa, pb);

  std::cout << max1 << std::endl;
  std::cout << max2 << std::endl;
  std::cout << max3 << std::endl;
  // maxval(&a, &b); // error
}