//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// lru_k_replacer.cpp
//
// Identification: src/buffer/lru_k_replacer.cpp
//
// Copyright (c) 2015-2022, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "buffer/lru_k_replacer.h"
#include <cstddef>
#include <exception>
#include <mutex>
#include <shared_mutex>
#include <stdexcept>
#include "common/config.h"
#include "common/exception.h"
#include "common/macros.h"

namespace bustub {
const size_t INF = std::numeric_limits<size_t>::max();
LRUKNode::LRUKNode(frame_id_t frame_id, size_t k) : k_(k), cur_size_(0), fid_(frame_id) {}

auto LRUKNode::GetKTimestamp() const -> size_t {
  if (cur_size_ < k_) {
    return INF;
  }
  auto it = history_.end();
  // for (size_t i = 0; i < k_; i++) {
  //   it++;
  // }
  std::advance(it, -k_);
  BUSTUB_ASSERT(it != history_.end(), "There is not enough history to get kth timestamp.");
  size_t timestamp = *it;
  return timestamp;
}

auto LRUKNode::GetLastTimestamp() const -> size_t {
  if (history_.empty()) {
    return 0;
  }
  return history_.back();
}

auto LRUKNode::AddHistory(size_t timestamp) -> void {
  history_.push_back(timestamp);
  cur_size_++;
}

LRUKReplacer::LRUKReplacer(size_t num_frames, size_t k) : replacer_size_(num_frames), k_(k) {}

auto LRUKReplacer::Evict(frame_id_t *frame_id) -> bool {
  // 加锁
  std::lock_guard<std::mutex> lock(latch_);

  std::vector<LRUKNode> knodes;    // 存放大于K次访问的节点
  std::vector<LRUKNode> infnodes;  // 存放backward k-distance为无穷大的节点
  size_t max_k_distance = 0;       // backward k-distance最大值
  size_t evict_frame_id = -1;      // 要被evict的frame_id

  for (const auto &node : node_store_) {
    if (node.second.IsEvictable()) {
      if (node.second.GetSize() >= k_) {
        knodes.push_back(node.second);
      } else {
        infnodes.push_back(node.second);
      }
    }
  }

  if (infnodes.empty()) {
    if (knodes.empty()) {
      *frame_id = evict_frame_id;
      return false;
    }
    // 从knodes中选择backward k-distance最大的节点
    for (const auto &node : knodes) {
      size_t k_timestamp = node.GetKTimestamp();
      size_t k_distance = current_timestamp_ - k_timestamp;
      if (k_distance > max_k_distance) {
        max_k_distance = k_distance;
        evict_frame_id = node.GetFrameId();
      }
    }
    *frame_id = evict_frame_id;

    // 消除node_store_中的对应项，减小curr_size_
    node_store_.erase(evict_frame_id);
    curr_size_--;
    return true;
  }

  // 从infnodes中选离最后一次访问最远的结点
  for (const auto &node : infnodes) {
    size_t last_timestamp = node.GetLastTimestamp();
    size_t k_distance = current_timestamp_ - last_timestamp;
    if (k_distance > max_k_distance) {
      max_k_distance = k_distance;
      evict_frame_id = node.GetFrameId();
    }
  }
  *frame_id = evict_frame_id;
  // 消除node_store_中的对应项，减小curr_size_
  node_store_.erase(evict_frame_id);
  curr_size_--;
  return true;
}

void LRUKReplacer::RecordAccess(frame_id_t frame_id, [[maybe_unused]] AccessType access_type) {
  // 加锁
  std::lock_guard<std::mutex> lock(latch_);
  if (frame_id < 0 || frame_id >= static_cast<int32_t>(replacer_size_)) {
    throw std::runtime_error("frame_id is out of range.");
  }

  BUSTUB_ASSERT(frame_id < static_cast<int32_t>(replacer_size_) && frame_id >= 0, "frame_id is out of range.");
  auto it = node_store_.find(frame_id);

  if (it == node_store_.end()) {
    // 此时不存在LRUKNode，需要新建并添加到node_store_
    LRUKNode new_node(frame_id, k_);
    new_node.AddHistory(current_timestamp_);
    node_store_.insert({frame_id, new_node});
  } else {
    // 此时已经存在LRUKNode，想history_中添加新项
    it->second.AddHistory(current_timestamp_);
  }
  // 最后增加current_timestamp_
  AddTimestamp();
}

void LRUKReplacer::SetEvictable(frame_id_t frame_id, bool set_evictable) {
  // 加锁
  std::lock_guard<std::mutex> lock(latch_);
  if (frame_id < 0 || frame_id >= static_cast<int32_t>(replacer_size_)) {
    throw std::runtime_error("frame_id is out of range.");
  }

  BUSTUB_ASSERT(frame_id < static_cast<int32_t>(replacer_size_) && frame_id >= 0, "frame_id is out of range.");
  auto it = node_store_.find(frame_id);

  if (set_evictable) {
    if (!it->second.IsEvictable()) {
      curr_size_++;
    }
    it->second.SetEvictable(true);
  } else {
    if (it->second.IsEvictable()) {
      curr_size_--;
    }
    it->second.SetEvictable(false);
  }
}

void LRUKReplacer::Remove(frame_id_t frame_id) {
  // 加锁
  std::lock_guard<std::mutex> lock(latch_);
  if (frame_id < 0 || frame_id >= static_cast<int32_t>(replacer_size_)) {
    throw std::runtime_error("frame_id is out of range.");
  }

  BUSTUB_ASSERT(frame_id < static_cast<int32_t>(replacer_size_) && frame_id >= 0, "frame_id is out of range.");
  auto it = node_store_.find(frame_id);
  if (!it->second.IsEvictable()) {
    throw std::runtime_error("Remove a non-evitable frame");
  }
  // remove可驱逐的frame
  node_store_.erase(it->first);
  curr_size_--;
}

auto LRUKReplacer::Size() -> size_t {
  // 加锁
  std::lock_guard<std::mutex> lock(latch_);
  return curr_size_;
}

}  // namespace bustub
