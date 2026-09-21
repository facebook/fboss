/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include <memory>
#include <mutex>

namespace facebook::fboss {

class SaiApiLock {
  struct ScopedApiLock {
    ScopedApiLock(std::mutex& m, bool noopLock) : mutex(m), noopLock(noopLock) {
      if (!noopLock) {
        mutex.lock();
      }
    }
    ~ScopedApiLock() {
      if (!noopLock) {
        mutex.unlock();
      }
    }
    std::mutex& mutex;
    bool noopLock{false};
  };

 public:
  static std::shared_ptr<SaiApiLock> getInstance();
  void setAdaptorIsThreadSafe(bool isThreadSafe) {
    adaptorIsThreadSafe_ = isThreadSafe;
  }
  ScopedApiLock lock() const {
    return {mutex_, adaptorIsThreadSafe_};
  }

 private:
  bool adaptorIsThreadSafe_{false};
  mutable std::mutex mutex_;
};
} // namespace facebook::fboss
