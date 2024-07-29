#include <concepts>
#include <iostream>
#include <ranges>
#include <set>
#include <type_traits>
#include <utility>
#include <vector>

// concept for container with push_back()
template <typename Coll>
concept SupportPushBack = requires(Coll coll, Coll::value_type value) {
  coll.push_back(value);
};

// concept to disable narrow
template <typename From, typename To>
concept ConvertsWithoutNarrow = std::convertible_to<From, To> &&
    requires(From &&x) {
  {
    std::type_identity_t<To[]> { std::forward<From>(x) }
    } -> std::same_as<To[1]>;
};

// add() for single value
template <typename Coll, typename T>
requires ConvertsWithoutNarrow<T, typename Coll::value_type>
void add(Coll &coll, const T &value) {
  if constexpr (SupportPushBack<Coll>) {
    coll.push_back(value);
  } else {
    coll.insert(value);
  }
}

// add() for multiple values
template <typename Coll, std::ranges::input_range T>
requires ConvertsWithoutNarrow < std::ranges::range_value_t<T>,
typename Coll::value_type > void add(Coll &coll, const T &value) {
  if constexpr (SupportPushBack<Coll>) {
    coll.insert(coll.end(), std::ranges::begin(value), std::ranges::end(value));
  } else {
    coll.insert(std::ranges::begin(value), std::ranges::end(value));
  }
}

int main() {
  std::vector<int> vec;
  add(vec, 42);

  std::set<int> set;
  add(set, 42);

  short s = 42;
  add(vec, s);

  int vals[] = {1, 2, 3, 4, 5};
  add(vec, vals);

  for (const auto &elem : vec) {
    std::cout << elem << ' ';
  }
  std::cout << std::endl;
}