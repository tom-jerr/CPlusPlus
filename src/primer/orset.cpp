#include "primer/orset.h"
#include <algorithm>
#include <string>
#include <vector>
#include "common/exception.h"
#include "fmt/format.h"

namespace bustub {

template <typename T>
auto ORSet<T>::Contains(const T &elem) const -> bool {
  // throw NotImplementedException("ORSet<T>::Contains is not implemented");
  for (auto it = add_set_.begin(); it != add_set_.end(); ++it) {
    if (it->count(elem) > 0) {
      return true;
    }
  }
  return false;
}

template <typename T>
void ORSet<T>::Add(const T &elem, uid_t uid) {
  // throw NotImplementedException("ORSet<T>::Add is not implemented");
  // E U {(e, n)}
  std::map<T, uid_t> element{{elem, uid}};
  add_set_.insert(element);
  // E \ T
  for (auto it = remove_set_.begin(); it != remove_set_.end(); ++it) {
    if (add_set_.find(*it) != add_set_.end()) {
      add_set_.erase(*it);
    }
  }
}

template <typename T>
void ORSet<T>::Remove(const T &elem) {
  // throw NotImplementedException("ORSet<T>::Remove is not implemented");
  std::set<std::map<T, uid_t>> remove_element;
  for (auto it = add_set_.begin(); it != add_set_.end(); ++it) {
    if (it->count(elem) > 0) {
      remove_element.insert(*it);
    }
  }

  // E \ R
  for (auto it = remove_element.begin(); it != remove_element.end(); ++it) {
    if (add_set_.find(*it) != add_set_.end()) {
      add_set_.erase(*it);
    }
  }
  // T U R
  remove_set_.insert(remove_element.begin(), remove_element.end());
}

template <typename T>
void ORSet<T>::Merge(const ORSet<T> &other) {
  // throw NotImplementedException("ORSet<T>::Merge is not implemented");
  ORSet<T> b = other;
  // E \ B.T
  for (auto it = b.remove_set_.begin(); it != b.remove_set_.end(); ++it) {
    if (add_set_.find(*it) != add_set_.end()) {
      add_set_.erase(*it);
    }
  }
  // B.E \ T
  for (auto it = remove_set_.begin(); it != remove_set_.end(); ++it) {
    if (b.add_set_.find(*it) != b.add_set_.end()) {
      b.add_set_.erase(*it);
    }
  }
  add_set_.insert(b.add_set_.begin(), b.add_set_.end());

  // T U B.T
  remove_set_.insert(b.remove_set_.begin(), b.remove_set_.end());
}

template <typename T>
auto ORSet<T>::Elements() const -> std::vector<T> {
  // throw NotImplementedException("ORSet<T>::Elements is not implemented");
  std::set<T> elements;
  for (auto it = add_set_.begin(); it != add_set_.end(); ++it) {
    for (auto it2 = it->begin(); it2 != it->end(); ++it2) {
      elements.insert(it2->first);
    }
  }
  return std::vector<T>(elements.begin(), elements.end());
}

template <typename T>
auto ORSet<T>::ToString() const -> std::string {
  auto elements = Elements();
  std::sort(elements.begin(), elements.end());
  return fmt::format("{{{}}}", fmt::join(elements, ", "));
}

template class ORSet<int>;
template class ORSet<std::string>;

}  // namespace bustub
