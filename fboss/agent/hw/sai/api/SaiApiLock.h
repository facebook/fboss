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
 public:
  static std::shared_ptr<SaiApiLock> getInstance();
  std::lock_guard<std::mutex> lock() const {
    return std::lock_guard<std::mutex>(mutex_);
  }

 private:
  mutable std::mutex mutex_;
};
} // namespace facebook::fboss
