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
using std::chrono::steady_clock;

class LanePrbsStatsEntry {
 public:
  LanePrbsStatsEntry(int32_t laneId, int32_t gportId, double laneRate)
      : laneId_(laneId), gportId_(gportId), laneRate_(laneRate) {
    timeLastCleared_ = steady_clock::now();
    timeLastLocked_ = steady_clock::time_point();
  }

  int32_t getLaneId() const {
    return laneId_;
  }

  int32_t getGportId() const {
    return gportId_;
  }

  double getLaneRate() const {
    return laneRate_;
  }

  void handleOk() {
    steady_clock::time_point now = steady_clock::now();
    locked_ = true;
    accuErrorCount_ = 0;
    if (!locked_) {
      timeLastLocked_ = now;
    }
    timeLastCollect_ = now;
  }

  void handleLockWithErrors(uint32_t num_errors) {
    steady_clock::time_point now = steady_clock::now();
    locked_ = true;
    accuErrorCount_ += num_errors;
    milliseconds duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            now - timeLastCollect_);
    if (duration.count() == 0) {
      return;
    }
    double ber = (num_errors * 1000) / (laneRate_ * duration.count());
    if (ber > maxBer_) {
      maxBer_ = ber;
    }
    timeLastCollect_ = now;
  }

  void handleNotLocked() {
    locked_ = false;
    if (locked_) {
      numLossOfLock_++;
    }
  }

  void handleLossOfLock() {
    steady_clock::time_point now = steady_clock::now();
    locked_ = true;
    numLossOfLock_++;
    timeLastLocked_ = now;
  }

  phy::PrbsLaneStats getPrbsLaneStats() const {
    phy::PrbsLaneStats prbsLaneStats = phy::PrbsLaneStats();
    steady_clock::time_point now = steady_clock::now();
    *prbsLaneStats.laneId() = laneId_;
    *prbsLaneStats.locked() = locked_;
    if (!locked_) {
      *prbsLaneStats.ber() = 0.;
    } else {
      milliseconds duration =
          std::chrono::duration_cast<std::chrono::milliseconds>(
              timeLastCollect_ - timeLastLocked_);
      if (duration.count() == 0) {
        *prbsLaneStats.ber() = 0.;
      } else {
        *prbsLaneStats.ber() =
            (accuErrorCount_ * 1000) / (laneRate_ * duration.count());
      }
    }
    *prbsLaneStats.maxBer() = maxBer_;
    *prbsLaneStats.numLossOfLock() = numLossOfLock_;
    *prbsLaneStats.timeSinceLastLocked() =
        (timeLastLocked_ == steady_clock::time_point())
        ? 0
        : std::chrono::duration_cast<std::chrono::seconds>(
              now - timeLastLocked_)
              .count();
    *prbsLaneStats.timeSinceLastClear() =
        std::chrono::duration_cast<std::chrono::seconds>(now - timeLastCleared_)
            .count();
    return prbsLaneStats;
  }

  void clearLaneStats() {
    accuErrorCount_ = 0;
    maxBer_ = -1.;
    numLossOfLock_ = 0;
    timeLastLocked_ = locked_ ? timeLastCollect_ : steady_clock::time_point();
    timeLastCleared_ = steady_clock::now();
  }

 private:
  const int32_t laneId_ = -1;
  const int32_t gportId_ = -1;
  const double laneRate_ = 0.;
  bool locked_ = false;
  int64_t accuErrorCount_ = 0;
  double maxBer_ = -1.;
  int32_t numLossOfLock_ = 0;
  steady_clock::time_point timeLastLocked_;
  steady_clock::time_point timeLastCleared_;
  steady_clock::time_point timeLastCollect_;
};
} // namespace facebook::fboss
