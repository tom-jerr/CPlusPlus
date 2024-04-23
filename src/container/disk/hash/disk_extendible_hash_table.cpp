//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// disk_extendible_hash_table.cpp
//
// Identification: src/container/disk/hash/disk_extendible_hash_table.cpp
//
// Copyright (c) 2015-2023, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include <cstdint>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "common/config.h"
#include "common/exception.h"
#include "common/logger.h"
#include "common/macros.h"
#include "common/rid.h"
#include "common/util/hash_util.h"
#include "container/disk/hash/disk_extendible_hash_table.h"
#include "storage/index/hash_comparator.h"
#include "storage/page/extendible_htable_bucket_page.h"
#include "storage/page/extendible_htable_directory_page.h"
#include "storage/page/extendible_htable_header_page.h"
#include "storage/page/page_guard.h"

namespace bustub {

template <typename K, typename V, typename KC>
DiskExtendibleHashTable<K, V, KC>::DiskExtendibleHashTable(const std::string &name, BufferPoolManager *bpm,
                                                           const KC &cmp, const HashFunction<K> &hash_fn,
                                                           uint32_t header_max_depth, uint32_t directory_max_depth,
                                                           uint32_t bucket_max_size)
    : bpm_(bpm),
      cmp_(cmp),
      hash_fn_(std::move(hash_fn)),
      header_max_depth_(header_max_depth),
      directory_max_depth_(directory_max_depth),
      bucket_max_size_(bucket_max_size) {
  // throw NotImplementedException("DiskExtendibleHashTable is not implemented");
  BasicPageGuard header_guard = bpm->NewPageGuarded(&header_page_id_);
  auto header = header_guard.AsMut<ExtendibleHTableHeaderPage>();
  header->Init(header_max_depth);
}

/*****************************************************************************
 * SEARCH
 *****************************************************************************/
/**
 * @note 在header page中寻找对应的directory page，然后在directory page中寻找对应的bucket page，最后在bucket
 page中寻找对应的key
 */
template <typename K, typename V, typename KC>
auto DiskExtendibleHashTable<K, V, KC>::GetValue(const K &key, std::vector<V> *result, Transaction *transaction) const
    -> bool {
  if (header_page_id_ == INVALID_PAGE_ID) {
    return false;
  }
  ReadPageGuard header_guard = bpm_->FetchPageRead(header_page_id_);
  auto header = header_guard.As<ExtendibleHTableHeaderPage>();

  // Get the directory page id, Drop the header page and fetch the directory page
  uint32_t directory_index = header->HashToDirectoryIndex(Hash(key));
  page_id_t directory_page_id = header->GetDirectoryPageId(directory_index);
  header_guard.Drop();

  if (directory_page_id == INVALID_PAGE_ID) {
    return false;
  }
  ReadPageGuard directory_guard = bpm_->FetchPageRead(directory_page_id);
  auto directory = directory_guard.As<ExtendibleHTableDirectoryPage>();

  // Get the bucket page id, Drop the directory page and fetch the bucket page
  uint32_t bucket_index = directory->HashToBucketIndex(Hash(key));
  page_id_t bucket_page_id = directory->GetBucketPageId(bucket_index);
  directory_guard.Drop();
  if (bucket_page_id == INVALID_PAGE_ID) {
    return false;
  }
  ReadPageGuard bucket_guard = bpm_->FetchPageRead(bucket_page_id);
  auto bucket = bucket_guard.As<ExtendibleHTableBucketPage<K, V, KC>>();

  // look up the key in the bucket
  V value;
  if (bucket->Lookup(key, value, cmp_)) {
    result->push_back(value);
    return true;
  }
  return false;
}

/*****************************************************************************
 * INSERTION
 *****************************************************************************/
/*
 * 获取到DirectoryPage的锁后才将HeaderPage的锁释放
 */
template <typename K, typename V, typename KC>
auto DiskExtendibleHashTable<K, V, KC>::Insert(const K &key, const V &value, Transaction *transaction) -> bool {
  if (header_page_id_ == INVALID_PAGE_ID) {
    return false;
  }
  // Fetch the header page
  WritePageGuard header_guard = bpm_->FetchPageWrite(header_page_id_);
  auto header = header_guard.AsMut<ExtendibleHTableHeaderPage>();

  // Get the directory page id, if the directory page id is invalid, create a new directory page
  uint32_t directory_index = header->HashToDirectoryIndex(Hash(key));
  page_id_t directory_page_id = header->GetDirectoryPageId(directory_index);
  if (directory_page_id == INVALID_PAGE_ID) {
    // Create a new directory page
    BasicPageGuard new_directory_guard = bpm_->NewPageGuarded(&directory_page_id);
    auto new_directory = new_directory_guard.AsMut<ExtendibleHTableDirectoryPage>();
    new_directory->Init(directory_max_depth_);
    header->SetDirectoryPageId(directory_index, directory_page_id);
  }
  // Get the bucket page id, if the bucket page id is invalid, create a new bucket page
  WritePageGuard directory_guard = bpm_->FetchPageWrite(directory_page_id);
  auto directory = directory_guard.AsMut<ExtendibleHTableDirectoryPage>();
  header_guard.Drop();
  uint32_t bucket_index = directory->HashToBucketIndex(Hash(key));
  page_id_t bucket_page_id = directory->GetBucketPageId(bucket_index);
  if (bucket_page_id == INVALID_PAGE_ID) {
    // Create a new bucket page
    BasicPageGuard new_bucket_guard = bpm_->NewPageGuarded(&bucket_page_id);
    auto new_bucket = new_bucket_guard.AsMut<ExtendibleHTableBucketPage<K, V, KC>>();
    new_bucket->Init(bucket_max_size_);
    directory->SetBucketPageId(bucket_index, bucket_page_id);
    directory->SetLocalDepth(bucket_index, 0);
  }
  // Insert the key-value pair into the bucket
  // We maybe split the bucket, and the directory maybe changed, so we can not drop the directory page
  WritePageGuard bucket_guard = bpm_->FetchPageWrite(bucket_page_id);
  auto bucket = bucket_guard.AsMut<ExtendibleHTableBucketPage<K, V, KC>>();

  V val;
  if (bucket->Lookup(key, val, cmp_)) {
    return false;
  }
  // 如果此时bucket已经满了，需要进行split操作
  if (bucket->IsFull()) {
    // 如果全局深度小于等于Local Depth，增加全局深度
    if (directory->GetLocalDepth(bucket_index) >= directory->GetGlobalDepth()) {
      if (directory->GetGlobalDepth() >= directory->GetMaxDepth()) {
        return false;
      }
      directory->IncrGlobalDepth();
    }
    // Increase local depth for the bucket
    directory->IncrLocalDepth(bucket_index);
    if (!SplitBucket(directory, bucket, bucket_index)) {
      return false;
    }
    // 此时进行页面锁的释放

    directory_guard.Drop();
    bucket_guard.Drop();
    // 递归该插入过程
    return Insert(key, value, transaction);
  }
  // 如果Bucket未满，直接插入
  return bucket->Insert(key, value, cmp_);
}

template <typename K, typename V, typename KC>
auto DiskExtendibleHashTable<K, V, KC>::SplitBucket(ExtendibleHTableDirectoryPage *directory,
                                                    ExtendibleHTableBucketPage<K, V, KC> *bucket, uint32_t bucket_idx)
    -> bool {
  // 1. Create a new bucket page(split page)
  page_id_t split_page_id;
  BasicPageGuard split_guard = bpm_->NewPageGuarded(&split_page_id);
  if (split_page_id == INVALID_PAGE_ID) {
    return false;
  }
  auto split_bucket = split_guard.AsMut<ExtendibleHTableBucketPage<K, V, KC>>();
  split_bucket->Init();
  uint32_t split_idx = directory->GetSplitImageIndex(bucket_idx);
  uint32_t local_depth = directory->GetLocalDepth(bucket_idx);
  directory->SetBucketPageId(split_idx, split_page_id);
  directory->SetLocalDepth(split_idx, local_depth);

  // 2. Get all pointers to the split page
  uint32_t idx_diff = (1 << local_depth);
  for (int i = bucket_idx - idx_diff; i >= 0; i -= idx_diff) {
    directory->SetLocalDepth(i, local_depth);
  }
  for (int i = bucket_idx + idx_diff; i < static_cast<int>(directory->Size()); i += idx_diff) {
    directory->SetLocalDepth(i, local_depth);
  }
  for (int i = split_idx - idx_diff; i >= 0; i -= idx_diff) {
    directory->SetBucketPageId(i, split_page_id);
    directory->SetLocalDepth(i, local_depth);
  }
  for (int i = split_idx + idx_diff; i < static_cast<int>(directory->Size()); i += idx_diff) {
    directory->SetBucketPageId(i, split_page_id);
    directory->SetLocalDepth(i, local_depth);
  }

  // 3. Reallocate the entries in the bucket
  page_id_t bucket_page_id = directory->GetBucketPageId(bucket_idx);
  if (bucket_page_id == INVALID_PAGE_ID) {
    return false;
  }
  // Get all entries in the bucket
  int size = bucket->Size();
  std::list<std::pair<K, V>> entries;
  for (int i = 0; i < size; i++) {
    entries.push_back(bucket->EntryAt(i));
  }
  bucket->Clear();  // 仅仅是做个size = 0，不是真正的删除

  for (const auto &entry : entries) {
    uint32_t target_idx = directory->HashToBucketIndex(Hash(entry.first));
    page_id_t target_page_id = directory->GetBucketPageId(target_idx);
    BUSTUB_ASSERT(target_page_id == bucket_page_id || target_page_id == split_page_id, "target_page_id error");
    if (target_page_id == bucket_page_id) {
      bucket->Insert(entry.first, entry.second, cmp_);
    } else {
      split_bucket->Insert(entry.first, entry.second, cmp_);
    }
  }
  return true;
}
template <typename K, typename V, typename KC>
auto DiskExtendibleHashTable<K, V, KC>::MergeBucket(ExtendibleHTableDirectoryPage *directory,
                                                    ExtendibleHTableBucketPage<K, V, KC> *bucket, uint32_t bucket_idx)
    -> void {
  // 递归操作
  while (true) {
    if (directory->GetLocalDepth(bucket_idx) == 0) {
      return;
    }
    uint32_t split_idx = directory->GetSplitImageIndex(bucket_idx);
    page_id_t split_page_id = directory->GetBucketPageId(split_idx);
    // 1.Buckets can only be merged with their split image if their split image has the same local depth.
    if (directory->GetLocalDepth(bucket_idx) != directory->GetLocalDepth(split_idx)) {
      return;
    }
    // 2.Only empty buckets can be merged.
    WritePageGuard split_guard = bpm_->FetchPageWrite(split_page_id);
    auto split_bucket = split_guard.AsMut<ExtendibleHTableBucketPage<K, V, KC>>();
    // 当且仅当split_bucket和bucket都不为空时，才能跳出merge循环
    if (!split_bucket->IsEmpty() && !bucket->IsEmpty()) {
      return;
    }
    // 3.Merge the split bucket into the bucket
    int size = split_bucket->Size();
    for (int i = 0; i < size; ++i) {
      std::pair<K, V> entry = split_bucket->EntryAt(i);
      bucket->Insert(entry.first, entry.second, cmp_);
    }
    split_bucket->Clear();
    split_guard.Drop();
    // 4.Update the directory
    page_id_t bucket_page_id = directory->GetBucketPageId(bucket_idx);
    directory->DecrLocalDepth(bucket_idx);
    uint32_t local_depth = directory->GetLocalDepth(bucket_idx);

    // 5.Update the pointers to the split page
    uint32_t idx_diff = 1 << local_depth;
    for (int i = bucket_idx - idx_diff; i >= 0; i -= idx_diff) {
      directory->SetBucketPageId(i, bucket_page_id);
      directory->SetLocalDepth(i, local_depth);
    }
    for (int i = bucket_idx + idx_diff; i < static_cast<int>(directory->Size()); i += idx_diff) {
      directory->SetBucketPageId(i, bucket_page_id);
      directory->SetLocalDepth(i, local_depth);
    }
  }
}
template <typename K, typename V, typename KC>
auto DiskExtendibleHashTable<K, V, KC>::InsertToNewDirectory(ExtendibleHTableHeaderPage *header, uint32_t directory_idx,
                                                             uint32_t hash, const K &key, const V &value) -> bool {
  return false;
}

template <typename K, typename V, typename KC>
auto DiskExtendibleHashTable<K, V, KC>::InsertToNewBucket(ExtendibleHTableDirectoryPage *directory, uint32_t bucket_idx,
                                                          const K &key, const V &value) -> bool {
  return false;
}

template <typename K, typename V, typename KC>
void DiskExtendibleHashTable<K, V, KC>::UpdateDirectoryMapping(ExtendibleHTableDirectoryPage *directory,
                                                               uint32_t new_bucket_idx, page_id_t new_bucket_page_id,
                                                               uint32_t new_local_depth, uint32_t local_depth_mask) {
  throw NotImplementedException("DiskExtendibleHashTable is not implemented");
}

/*****************************************************************************
 * REMOVE
 *****************************************************************************/
template <typename K, typename V, typename KC>
auto DiskExtendibleHashTable<K, V, KC>::Remove(const K &key, Transaction *transaction) -> bool {
  if (header_page_id_ == INVALID_PAGE_ID) {
    return false;
  }
  WritePageGuard header_guard = bpm_->FetchPageWrite(header_page_id_);
  auto header = header_guard.AsMut<ExtendibleHTableHeaderPage>();
  // Get the directory page id, Drop the header page and fetch the directory page
  uint32_t directory_index = header->HashToDirectoryIndex(Hash(key));
  page_id_t directory_page_id = header->GetDirectoryPageId(directory_index);

  if (directory_page_id == INVALID_PAGE_ID) {
    header_guard.Drop();
    return false;
  }
  // Get the bucket page id and fetch the bucket page
  WritePageGuard directory_guard = bpm_->FetchPageWrite(directory_page_id);
  auto directory = directory_guard.AsMut<ExtendibleHTableDirectoryPage>();
  header_guard.Drop();
  uint32_t bucket_index = directory->HashToBucketIndex(Hash(key));
  page_id_t bucket_page_id = directory->GetBucketPageId(bucket_index);

  // Fetch the bucket page
  WritePageGuard bucket_guard = bpm_->FetchPageWrite(bucket_page_id);
  auto bucket = bucket_guard.AsMut<ExtendibleHTableBucketPage<K, V, KC>>();

  // Remove the key-value pair from the bucket
  if (!bucket->Remove(key, cmp_)) {
    return false;
  }
  // Try to merge bucket
  MergeBucket(directory, bucket, bucket_index);
  while (directory->CanShrink()) {
    if (directory->GetGlobalDepth() == 0) {
      break;
    }
    directory->DecrGlobalDepth();
  }
  return true;
}

template class DiskExtendibleHashTable<int, int, IntComparator>;
template class DiskExtendibleHashTable<GenericKey<4>, RID, GenericComparator<4>>;
template class DiskExtendibleHashTable<GenericKey<8>, RID, GenericComparator<8>>;
template class DiskExtendibleHashTable<GenericKey<16>, RID, GenericComparator<16>>;
template class DiskExtendibleHashTable<GenericKey<32>, RID, GenericComparator<32>>;
template class DiskExtendibleHashTable<GenericKey<64>, RID, GenericComparator<64>>;
}  // namespace bustub
