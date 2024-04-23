//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// extendible_htable_bucket_page.cpp
//
// Identification: src/storage/page/extendible_htable_bucket_page.cpp
//
// Copyright (c) 2015-2023, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include <optional>
#include <utility>

#include "common/exception.h"
#include "common/macros.h"
#include "storage/page/extendible_htable_bucket_page.h"

namespace bustub {

template <typename K, typename V, typename KC>
void ExtendibleHTableBucketPage<K, V, KC>::Init(uint32_t max_size) {
  // throw NotImplementedException("ExtendibleHTableBucketPage not implemented");
  max_size_ = max_size;
  size_ = 0;
}

template <typename K, typename V, typename KC>
auto ExtendibleHTableBucketPage<K, V, KC>::KeyIndex(const K &key, const KC &cmp) const -> uint32_t {
  int size = Size();

  int left = 0;
  int right = size - 1;
  while (left <= right) {
    int mid = (left + right) / 2;
    int ret = cmp(key, array_[mid].first);

    if (ret == 1) {
      left = mid + 1;
    } else if (ret == -1) {
      right = mid - 1;
    } else {
      return mid;
    }
  }

  return left;
}

template <typename K, typename V, typename KC>
auto ExtendibleHTableBucketPage<K, V, KC>::Lookup(const K &key, V &value, const KC &cmp) const -> bool {
  int size = Size();
  if (size == 0) {
    value = {};
    return false;
  }

  uint32_t index = KeyIndex(key, cmp);
  if (index >= size) {
    value = {};
    return false;
  }

  if (cmp(key, KeyAt(index)) == 0) {
    value = ValueAt(index);
    return true;
  }
  return false;
}

template <typename K, typename V, typename KC>
auto ExtendibleHTableBucketPage<K, V, KC>::InsertAt(uint32_t idx, const K &key, const V &value) -> void {
  int size = Size();
  for (int i = size - 1; i >= static_cast<int>(idx); i--) {
    array_[i + 1] = array_[i];
  }
  array_[idx] = {key, value};
  size_++;
}

template <typename K, typename V, typename KC>
auto ExtendibleHTableBucketPage<K, V, KC>::Insert(const K &key, const V &value, const KC &cmp) -> bool {
  int size = Size();
  if (IsFull()) {
    return false;
  }

  if (size == 0) {
    InsertAt(0, key, value);
    return true;
  }

  uint32_t index = KeyIndex(key, cmp);
  // 插入相同key，直接返回false
  if (cmp(key, KeyAt(index)) == 0) {
    return false;
  }
  InsertAt(index, key, value);
  return true;
}

template <typename K, typename V, typename KC>
auto ExtendibleHTableBucketPage<K, V, KC>::Remove(const K &key, const KC &cmp) -> bool {
  int size = Size();
  if (size == 0) {
    return false;
  }
  uint32_t index = KeyIndex(key, cmp);
  if (cmp(key, KeyAt(index)) != 0) {
    return false;
  }
  RemoveAt(index);
  return true;
}

template <typename K, typename V, typename KC>
void ExtendibleHTableBucketPage<K, V, KC>::RemoveAt(uint32_t bucket_idx) {
  // throw NotImplementedException("ExtendibleHTableBucketPage not implemented");
  BUSTUB_ASSERT(bucket_idx < Size(), "bucket_idx out of bound");
  int size = Size();
  for (int i = bucket_idx; i < size - 1; i++) {
    array_[i] = array_[i + 1];
  }
  size_--;
}

template <typename K, typename V, typename KC>
auto ExtendibleHTableBucketPage<K, V, KC>::KeyAt(uint32_t bucket_idx) const -> K {
  return array_[bucket_idx].first;
}

template <typename K, typename V, typename KC>
auto ExtendibleHTableBucketPage<K, V, KC>::ValueAt(uint32_t bucket_idx) const -> V {
  return array_[bucket_idx].second;
}

template <typename K, typename V, typename KC>
auto ExtendibleHTableBucketPage<K, V, KC>::EntryAt(uint32_t bucket_idx) const -> const std::pair<K, V> & {
  return array_[bucket_idx];
}

template <typename K, typename V, typename KC>
auto ExtendibleHTableBucketPage<K, V, KC>::Size() const -> uint32_t {
  return size_;
}

template <typename K, typename V, typename KC>
auto ExtendibleHTableBucketPage<K, V, KC>::IsFull() const -> bool {
  return size_ == max_size_;
}

template <typename K, typename V, typename KC>
auto ExtendibleHTableBucketPage<K, V, KC>::IsEmpty() const -> bool {
  return size_ == 0;
}

template class ExtendibleHTableBucketPage<int, int, IntComparator>;
template class ExtendibleHTableBucketPage<GenericKey<4>, RID, GenericComparator<4>>;
template class ExtendibleHTableBucketPage<GenericKey<8>, RID, GenericComparator<8>>;
template class ExtendibleHTableBucketPage<GenericKey<16>, RID, GenericComparator<16>>;
template class ExtendibleHTableBucketPage<GenericKey<32>, RID, GenericComparator<32>>;
template class ExtendibleHTableBucketPage<GenericKey<64>, RID, GenericComparator<64>>;

}  // namespace bustub
