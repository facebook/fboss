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

#include "fboss/agent/hw/bcm/BcmAclStat.h"
#include "fboss/agent/hw/bcm/BcmFlexCounter.h"

namespace facebook::fboss {

// EM total allocated counter indexes are 9216.
// Hence actionIndex mask is 14 bits
constexpr auto kEMActionIndexObj0Mask = 14;

enum class BcmAclStatType;
class BcmIngressFieldProcessorFlexCounter : public BcmFlexCounter {
 public:
  BcmIngressFieldProcessorFlexCounter(
      int unit,
      std::optional<int> groupID,
      std::optional<BcmAclStatHandle> statHandle,
      BcmAclStatType type);
  ~BcmIngressFieldProcessorFlexCounter() = default;

  void attach(BcmAclEntryHandle acl, BcmAclStatActionIndex actionIndex);

  void detach(BcmAclEntryHandle acl, BcmAclStatActionIndex actionIndex) {
    detach(unit_, acl, static_cast<BcmAclStatHandle>(counterID_), actionIndex);
  }

  static void detach(
      int unit,
      BcmAclEntryHandle acl,
      BcmAclStatHandle aclStatHandle,
      BcmAclStatActionIndex actionIndex);

  static std::set<cfg::CounterType> getCounterTypeList(
      int unit,
      uint32_t counterID);

  static int getNumAclStatsInFpGroup(int unit, int gid);

  static void removeAllCountersInFpGroup(int unit, int gid);

  static std::optional<std::pair<uint32_t, uint32_t>>
  getFlexCounterIDFromAttachedAcl(int unit, int groupID, BcmAclEntryHandle acl);

  static BcmTrafficCounterStats getAclTrafficFlexCounterStats(
      int unit,
      BcmAclStatHandle handle,
      BcmAclStatActionIndex actionIndex,
      const std::vector<cfg::CounterType>& counters);

  void setHwCounterID(uint32_t counterID) {
    counterID_ = counterID;
  }

 private:
  // Forbidden copy constructor and assignment operator
  BcmIngressFieldProcessorFlexCounter(
      BcmIngressFieldProcessorFlexCounter const&) = delete;
  BcmIngressFieldProcessorFlexCounter& operator=(
      BcmIngressFieldProcessorFlexCounter const&) = delete;
  BcmAclStatType statType_;
};
} // namespace facebook::fboss
