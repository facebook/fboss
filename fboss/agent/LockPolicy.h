/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <mutex>

namespace facebook::fboss {

class CoarseGrainedLockPolicy {
 public:
  explicit CoarseGrainedLockPolicy(std::mutex& mutex) : lock_(mutex) {}
  const std::lock_guard<std::mutex>& lock() const {
    return lock_;
  }

 private:
  std::lock_guard<std::mutex> lock_;
};

class FineGrainedLockPolicy {
 public:
  explicit FineGrainedLockPolicy(std::mutex& m) : mutex_(m) {}
  std::lock_guard<std::mutex> lock() const {
    return std::lock_guard<std::mutex>(mutex_);
  }

 private:
  std::mutex& mutex_;
};
} // namespace facebook::fboss
