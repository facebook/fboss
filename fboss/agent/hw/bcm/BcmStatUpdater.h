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

#include "common/stats/MonotonicCounter.h"
#include "fboss/agent/hw/bcm/BcmTableStats.h"
#include "fboss/agent/hw/bcm/types.h"
#include "fboss/agent/types.h"

#include <boost/container/flat_map.hpp>
#include <folly/Synchronized.h>
#include <queue>

extern "C" {
#include <bcm/port.h>
#include <bcm/types.h>
}

namespace facebook::fboss {

using facebook::stats::MonotonicCounter;
using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::steady_clock;

class BcmSwitch;
class StateDelta;

class BcmStatUpdater {
 public:
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

    void lossOfLock() {
      if (locked_) {
        locked_ = false;
        accuErrorCount_ = 0;
        numLossOfLock_++;
      }
      timeLastCollect_ = steady_clock::now();
    }

    void locked() {
      steady_clock::time_point now = steady_clock::now();
      locked_ = true;
      accuErrorCount_ = 0;
      timeLastLocked_ = now;
      timeLastCollect_ = now;
    }

    void updateLaneStats(uint32 status) {
      if (!locked_) {
        locked();
        return;
      }
      steady_clock::time_point now = steady_clock::now();
      accuErrorCount_ += status;

      milliseconds duration =
          std::chrono::duration_cast<std::chrono::milliseconds>(
              now - timeLastCollect_);
      // There shouldn't be a case where duration would be 0.
      // But just add a check here to be safe.
      if (duration.count() == 0) {
        return;
      }
      double ber = (status * 1000) / (laneRate_ * duration.count());
      if (ber > maxBer_) {
        maxBer_ = ber;
      }
      timeLastCollect_ = now;
    }

    PrbsLaneStats getPrbsLaneStats() const {
      PrbsLaneStats prbsLaneStats = PrbsLaneStats();
      steady_clock::time_point now = steady_clock::now();
      *prbsLaneStats.laneId_ref() = laneId_;
      *prbsLaneStats.locked_ref() = locked_;
      if (!locked_) {
        *prbsLaneStats.ber_ref() = 0.;
      } else {
        milliseconds duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                timeLastCollect_ - timeLastLocked_);
        if (duration.count() == 0) {
          *prbsLaneStats.ber_ref() = 0.;
        } else {
          *prbsLaneStats.ber_ref() =
              (accuErrorCount_ * 1000) / (laneRate_ * duration.count());
        }
      }
      *prbsLaneStats.maxBer_ref() = maxBer_;
      *prbsLaneStats.numLossOfLock_ref() = numLossOfLock_;
      *prbsLaneStats.timeSinceLastLocked_ref() =
          (timeLastLocked_ == steady_clock::time_point())
          ? 0
          : std::chrono::duration_cast<std::chrono::seconds>(
                now - timeLastLocked_)
                .count();
      *prbsLaneStats.timeSinceLastClear_ref() =
          std::chrono::duration_cast<std::chrono::seconds>(
              now - timeLastCleared_)
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
  using LanePrbsStatsTable = std::vector<LanePrbsStatsEntry>;

  explicit BcmStatUpdater(BcmSwitch* hw);
  ~BcmStatUpdater() {}

  /* Thread safety:
   *  Accessing Bcm* data structures from stats and update thread is racy:
   *  One thread is iterating through the objects while the other is adding or
   *  removing objects:
   *    - The stats collection thread (read-only) is being called periodically.
   *    - The update thread would only add/remove objects on config change,
   *      which does not happen very often.
   *
   *  The idea here is to accumulate the changes in separate data structures
   *  (toBe* functions) and apply them all at once in the 'refresh()' function.
   *  Note that this class does not store Bcm* objects, but rather cache
   *  the ids needed to collect the stats.
   *
   *  'refresh()' and 'updateStats()' are using folly::Synchronize to ensure
   *  thread safety.
   */

  MonotonicCounter* getCounterIf(
      BcmAclStatHandle handle,
      cfg::CounterType type);
  size_t getCounterCount() const;

  /* Functions to be called during state update */
  void toBeAddedAclStat(
      BcmAclStatHandle handle,
      const std::string& name,
      const std::vector<cfg::CounterType>& counterTypes);
  void toBeRemovedAclStat(BcmAclStatHandle handle);
  void refreshPostBcmStateChange(const StateDelta& delta);

  /* Functions to be called during stats collection (UpdateStatsThread) */
  void updateStats();

  void clearPortStats(const std::unique_ptr<std::vector<int32_t>>& ports);

  std::vector<PrbsLaneStats> getPortAsicPrbsStats(int32_t portId);
  void clearPortAsicPrbsStats(int32_t portId);

  HwResourceStats getHwTableStats() {
    return *resourceStats_.rlock();
  }

 private:
  void updateAclStat(
      int unit,
      BcmAclStatHandle handle,
      cfg::CounterType counterType,
      std::chrono::seconds now,
      MonotonicCounter* counter);

  std::string counterTypeToString(cfg::CounterType type);

  void updateAclStats();
  void updateHwTableStats();
  void updatePrbsStats();
  void refreshHwTableStats(const StateDelta& delta);
  void refreshAclStats();
  void refreshPrbsStats(const StateDelta& delta);

  double calculateLaneRate(std::shared_ptr<Port> swPort);

  BcmSwitch* hw_;
  std::unique_ptr<BcmHwTableStatManager> bcmTableStatsManager_;

  /* ACL stats */
  struct AclCounterDescriptor {
    AclCounterDescriptor(BcmAclStatHandle hdl, cfg::CounterType type)
        : handle(hdl), counterType(type) {}
    bool operator<(const AclCounterDescriptor& d) const {
      return std::tie(handle, counterType) < std::tie(d.handle, d.counterType);
    }
    BcmAclStatHandle handle;
    cfg::CounterType counterType;
  };
  folly::Synchronized<HwResourceStats> resourceStats_;
  std::queue<BcmAclStatHandle> toBeRemovedAclStats_;
  std::queue<std::pair<std::string, AclCounterDescriptor>> toBeAddedAclStats_;
  folly::Synchronized<
      std::map<AclCounterDescriptor, std::unique_ptr<MonotonicCounter>>>
      aclStats_;
  folly::Synchronized<std::map<int32_t, LanePrbsStatsTable>> portAsicPrbsStats_;
}; // namespace facebook::fboss

} // namespace facebook::fboss
