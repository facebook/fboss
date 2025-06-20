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

namespace facebook::fboss {

class ThriftMethodRateLimit {
  /*
   * Send heartbeat at regular interval to thread.  Measure delay between
   * time we expect heartbeat to be processed vs. time actually processed,
   * and record it to ods.
   */
 public:
  explicit ThriftMethodRateLimit(
      const std::map<std::string, double>& methood2QpsLimit,
      bool shadowMode = false) {
    for (const auto& it : methood2QpsLimit) {
      method2QpsLimitAndTokenBucket_.emplace(
          it.first, std::make_pair(it.second, folly::DynamicTokenBucket(0)));
    }
    shadowMode_ = shadowMode;
  }

  bool isQpsLimitExceeded(const std::string& method);
  double getQpsLimit(const std::string& method);
  bool getShadowMode() {
    return shadowMode_;
  }

  static apache::thrift::PreprocessFunc getThriftMethodRateLimitPreprocessFunc(
      std::unique_ptr<ThriftMethodRateLimit> rateLimiter);

 private:
  std::unordered_map<std::string, std::pair<double, folly::DynamicTokenBucket>>
      method2QpsLimitAndTokenBucket_;
  bool shadowMode_;
};

} // namespace facebook::fboss
