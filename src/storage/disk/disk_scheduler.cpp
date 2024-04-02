//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// disk_scheduler.cpp
//
// Identification: src/storage/disk/disk_scheduler.cpp
//
// Copyright (c) 2015-2023, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "storage/disk/disk_scheduler.h"
#include <optional>
#include "common/exception.h"
#include "storage/disk/disk_manager.h"

namespace bustub {

DiskScheduler::DiskScheduler(DiskManager *disk_manager, int num_workers)  // request_queue_初始化分配空间需要确定大小
    : disk_manager_(disk_manager), num_workers_(num_workers), request_queue_(num_workers) {
  // TODO(P1): remove this line after you have implemented the disk scheduler API
  // throw NotImplementedException(
  //     "DiskScheduler is not implemented yet. If you have finished implementing the disk scheduler, please remove the
  //     " "throw exception line in `disk_scheduler.cpp`.");

  // Spawn the background thread
  for (size_t thread_id = 0; thread_id < num_workers_; thread_id++) {
    background_thread_.emplace_back([&, thread_id] { StartWorkerThread(thread_id); });
  }
}

DiskScheduler::~DiskScheduler() {
  // Put a `std::nullopt` in the queue to signal to exit the loop
  for (size_t thread_id = 0; thread_id < background_thread_.size(); thread_id++) {
    request_queue_[thread_id].Put(std::nullopt);
  }
  for (auto &thread : background_thread_) {
    if (thread.joinable()) {
      thread.join();
    }
  }
}

void DiskScheduler::Schedule(DiskRequest r) {
  size_t thread_id = Hash(r.page_id_);
  request_queue_[thread_id].Put(std::move(r));
}

void DiskScheduler::StartWorkerThread(size_t thread_id) {
  // 无限循环执行函数
  while (true) {
    // 从队列中取出一个请求
    auto request = request_queue_[thread_id].Get();
    // 如果请求为空，退出循环
    if (!request.has_value()) {
      break;
    }
    auto page_data = request.value().data_;
    page_id_t page_id = request.value().page_id_;
    // 处理请求
    if (request->is_write_) {
      disk_manager_->WritePage(page_id, page_data);
    } else {
      disk_manager_->ReadPage(page_id, page_data);
    }
    // 通知请求完成
    request.value().callback_.set_value(true);
  }
}
}  // namespace bustub
