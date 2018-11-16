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

#include <queue>
#include <folly/Synchronized.h>
#include <boost/container/flat_map.hpp>

namespace facebook { namespace fboss {

using facebook::stats::MonotonicCounter;

class BcmSwitch;
class StateDelta;

class BcmStatUpdater {
 public:
  explicit BcmStatUpdater(BcmSwitch* hw, bool isAlpmEnabled);
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

  BcmHwTableStats getHwTableStats() {
    return *tableStats_.rlock();
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
  void refreshHwTableStats(const StateDelta& delta);
  void refreshAclStats();

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
  folly::Synchronized<BcmHwTableStats> tableStats_;
  std::queue<BcmAclStatHandle> toBeRemovedAclStats_;
  std::queue<std::pair<std::string, AclCounterDescriptor>> toBeAddedAclStats_;
  folly::Synchronized<
      std::map<AclCounterDescriptor, std::unique_ptr<MonotonicCounter>>>
      aclStats_;
};

}} // facebook::fboss
