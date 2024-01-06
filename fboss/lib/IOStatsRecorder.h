/*
 *  Copyright (c) 2018-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include "fboss/lib/if/gen-cpp2/io_stats_types.h"

namespace facebook {
namespace fboss {

class IOStatsRecorder {
  using time_point = std::chrono::steady_clock::time_point;
  using seconds = std::chrono::seconds;

 public:
  IOStatsRecorder() {}
  ~IOStatsRecorder() {}

  void updateReadDownTime();
  void updateWriteDownTime();
  void recordReadSuccess();
  void recordWriteSuccess();
  void recordReadAttempted();
  void recordWriteAttempted();
  void recordReadFailed();
  void recordWriteFailed();

  IOStats getStats();

 private:
  double downTimeLocked(const time_point& lastSuccess) {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<seconds>(now - lastSuccess).count();
  }
  IOStats stats_;
  std::mutex statsMutex_;
  time_point lastSuccessfulRead_;
  time_point lastSuccessfulWrite_;
};

} // namespace fboss
} // namespace facebook
