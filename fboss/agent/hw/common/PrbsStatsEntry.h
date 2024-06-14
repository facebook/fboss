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

#include "fboss/agent/if/gen-cpp2/ctrl_types.h"

#include <stdint.h>

namespace facebook::fboss {

using std::chrono::milliseconds;
using std::chrono::system_clock;

class PrbsStatsEntry {
 public:
  PrbsStatsEntry(int32_t laneId, int32_t gportId, double rate)
      : laneId_(laneId), gportId_(gportId), rate_(rate) {
    timeLastCleared_ = system_clock::now();
    timeLastLocked_ = system_clock::time_point();
  }

  PrbsStatsEntry(int32_t portId, double rate) : portId_(portId), rate_(rate) {
    timeLastCleared_ = system_clock::now();
    timeLastLocked_ = system_clock::time_point();
  }

  int32_t getLaneId() const {
    return laneId_;
  }

  int32_t getGportId() const {
    return gportId_;
  }

  int32_t getPortId() const {
    return portId_;
  }

  double getRate() const {
    return rate_;
  }

  void setRate(double rate) {
    rate_ = rate;
  }

  void handleOk() {
    handleLockWithErrors(0);
  }

  void handleLockWithErrors(uint32_t num_errors) {
    if (!locked_) {
      handleLossOfLock();
      return;
    }
    system_clock::time_point now = system_clock::now();
    accuErrorCount_ += num_errors;
    auto startTime = std::max(timeLastCleared_, timeLastCollect_);
    milliseconds duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime);
    if (duration.count() == 0) {
      return;
    }
    double ber = (num_errors * 1000) / (rate_ * duration.count());
    if (ber > maxBer_) {
      maxBer_ = ber;
    }
    timeLastCollect_ = now;
  }

  void handleNotLocked() {
    system_clock::time_point now = system_clock::now();
    if (locked_) {
      locked_ = false;
      accuErrorCount_ = 0;
      maxBer_ = -1.;
      numLossOfLock_++;
    }
    timeLastCollect_ = now;
  }

  void handleLossOfLock() {
    system_clock::time_point now = system_clock::now();
    locked_ = true;
    accuErrorCount_ = 0;
    maxBer_ = -1.;
    numLossOfLock_++;
    timeLastLocked_ = now;
    timeLastCollect_ = now;
  }

  phy::PrbsLaneStats getPrbsStats() const {
    phy::PrbsLaneStats prbsLaneStats = phy::PrbsLaneStats();
    system_clock::time_point now = system_clock::now();
    *prbsLaneStats.laneId() = laneId_;
    *prbsLaneStats.locked() = locked_;
    if (!locked_) {
      *prbsLaneStats.ber() = 0.;
    } else {
      auto startTime = std::max(timeLastCleared_, timeLastLocked_);
      milliseconds duration =
          std::chrono::duration_cast<std::chrono::milliseconds>(
              timeLastCollect_ - startTime);
      if (duration.count() == 0) {
        *prbsLaneStats.ber() = 0.;
      } else {
        *prbsLaneStats.ber() =
            (accuErrorCount_ * 1000) / (rate_ * duration.count());
      }
    }
    *prbsLaneStats.maxBer() = maxBer_;
    *prbsLaneStats.numLossOfLock() = numLossOfLock_;
    *prbsLaneStats.timeSinceLastLocked() =
        (timeLastLocked_ == system_clock::time_point())
        ? 0
        : std::chrono::duration_cast<std::chrono::seconds>(
              now - timeLastLocked_)
              .count();
    *prbsLaneStats.timeSinceLastClear() =
        std::chrono::duration_cast<std::chrono::seconds>(now - timeLastCleared_)
            .count();
    *prbsLaneStats.timeCollected() =
        std::chrono::duration_cast<std::chrono::seconds>(
            timeLastCollect_.time_since_epoch())
            .count();
    return prbsLaneStats;
  }

  void clearPrbsStats() {
    accuErrorCount_ = 0;
    maxBer_ = -1.;
    numLossOfLock_ = 0;
    timeLastLocked_ = locked_ ? timeLastCollect_ : system_clock::time_point();
    timeLastCleared_ = system_clock::now();
  }

 private:
  const int32_t laneId_ = -1;
  const int32_t gportId_ = -1;
  const int32_t portId_ = -1;
  double rate_ = 0.;
  bool locked_ = false;
  int64_t accuErrorCount_ = 0;
  // maxBer_ is the maximum BER seen since last lock
  double maxBer_ = -1.;
  int32_t numLossOfLock_ = 0;
  system_clock::time_point timeLastLocked_;
  system_clock::time_point timeLastCleared_;
  system_clock::time_point timeLastCollect_;
};
} // namespace facebook::fboss
