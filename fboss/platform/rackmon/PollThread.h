// Copyright 2021-present Facebook. All Rights Reserved.
#pragma once
#include <atomic>
#include <condition_variable>
#include <functional>
#include <thread>

namespace rackmon {

using PollThreadTime = std::chrono::seconds;
template <class T>
class PollThread {
  std::mutex eventMutex_{};
  std::condition_variable eventCV_{};
  std::thread threadID_{};
  std::atomic<bool> started_ = false;

  std::function<void(T*)> func_{};
  T* obj_ = nullptr;
  PollThreadTime sleepTime_{5};

  void worker() {
    std::unique_lock lk(eventMutex_);
    while (started_.load()) {
      func_(obj_);
      eventCV_.wait_for(lk, sleepTime_, [this]() { return !started_.load(); });
    }
  }
  void notifyStop() {
    std::unique_lock lk(eventMutex_);
    started_ = false;
    eventCV_.notify_all();
  }

 public:
  PollThread(
      std::function<void(T*)> func,
      T* obj,
      const PollThreadTime& pollInterval)
      : func_(func), obj_(obj), sleepTime_(pollInterval) {}
  ~PollThread() {
    stop();
  }
  void start() {
    if (!started_.load()) {
      started_ = true;
      threadID_ = std::thread(&PollThread::worker, this);
    }
  }
  void stop() {
    if (started_.load()) {
      notifyStop();
      threadID_.join();
    }
  }
};

} // namespace rackmon
