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

#include <fb303/ExportedHistogramMap.h>
#include <fb303/ExportedHistogramMapImpl.h>
#include <fb303/ExportedStatMapImpl.h>
#include <fb303/ServiceData.h>
#include <folly/Range.h>

namespace facebook {
namespace stats {

class MonotonicCounter {
 public:
  MonotonicCounter(
      folly::StringPiece name,
      fb303::ExportType,
      fb303::ExportType) {
    auto statMap = facebook::fb303::fbData->getStatMap();
    stat_ = statMap->getLockableStatNoExport(name);
    name_ = name;
  }
  void updateValue(std::chrono::seconds now, int64_t value) {
    auto guard = stat_.lock();
    if (!init_) {
      prevValue_ = value;
      init_ = true;
      return;
    }
    if (prevValue_ > value) {
      delta_ = 0;
    } else {
      delta_ = value - prevValue_;
    }
    prevValue_ = value;
    stat_.addValueLocked(guard, fb303::ExportedStat::TimePoint(now), delta_);
  }
  void swap(MonotonicCounter& counter) {}
  int64_t get() const {
    return delta_;
  }
  const std::string& getName() const {
    return name_;
  }

 private:
  bool init_{false};
  int64_t prevValue_{0};
  int64_t delta_{0};
  std::string name_;
  facebook::fb303::ExportedStatMapImpl::LockableStat stat_;
};
} // namespace stats
} // namespace facebook
