/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
// Copyright 2014-present Facebook. All Rights Reserved.
#pragma once
#include <folly/TokenBucket.h>
#include <thrift/lib/cpp2/server/PreprocessFunctions.h>
#include <atomic>

namespace facebook::fboss {

class ThriftMethodRateLimit {
  /*
   * Send heartbeat at regular interval to thread.  Measure delay between
   * time we expect heartbeat to be processed vs. time actually processed,
   * and record it to ods.
   */
 public:
  using PopulateCounterFunc = std::function<void(
      const std::string& /*method name*/,
      uint64_t /*method deny count*/,
      uint64_t /*agg deny count*/)>;

  explicit ThriftMethodRateLimit(
      const std::map<std::string, double>& methood2QpsLimit,
      bool shadowMode = false,
      const PopulateCounterFunc& populateCounterFunc = nullptr) {
    for (const auto& it : methood2QpsLimit) {
      method2QpsLimitAndTokenBucket_.emplace(
          it.first, std::make_pair(it.second, folly::DynamicTokenBucket(0)));
      method2DenyCounter_[it.first] = 0;
    }
    shadowMode_ = shadowMode;
    populateCounterFunc_ = populateCounterFunc;
    aggDenyCounter_ = 0;
  }

  bool isQpsLimitExceeded(const std::string& method);
  void incrementDenyCounter(const std::string& method);
  double getQpsLimit(const std::string& method);
  bool getShadowMode() {
    return shadowMode_;
  }
  uint64_t getDenyCounter(const std::string& method) {
    auto it = method2DenyCounter_.find(method);
    return it != method2DenyCounter_.end()
        ? it->second.load(std::memory_order_relaxed)
        : 0;
  }
  uint64_t getAggDenyCounter() {
    return aggDenyCounter_.load(std::memory_order_relaxed);
  }

  static apache::thrift::PreprocessFunc getThriftMethodRateLimitPreprocessFunc(
      std::shared_ptr<ThriftMethodRateLimit> rateLimiter);

 private:
  std::unordered_map<std::string, std::pair<double, folly::DynamicTokenBucket>>
      method2QpsLimitAndTokenBucket_;
  std::unordered_map<std::string, std::atomic<uint64_t>> method2DenyCounter_;
  std::atomic<uint64_t> aggDenyCounter_;
  bool shadowMode_;
  PopulateCounterFunc populateCounterFunc_{nullptr};
};

} // namespace facebook::fboss
