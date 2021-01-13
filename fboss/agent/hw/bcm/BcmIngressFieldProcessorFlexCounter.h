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

#include "fboss/agent/hw/bcm/BcmFlexCounter.h"

namespace facebook::fboss {

class BcmIngressFieldProcessorFlexCounter : public BcmFlexCounter {
 public:
  BcmIngressFieldProcessorFlexCounter(
      int unit,
      std::optional<int> groupID,
      std::optional<BcmAclStatHandle> statHandle);
  ~BcmIngressFieldProcessorFlexCounter() = default;

  void attach(BcmAclEntryHandle acl);

  void detach(BcmAclEntryHandle acl) {
    detach(unit_, acl, static_cast<BcmAclStatHandle>(counterID_));
  }

  static void
  detach(int unit, BcmAclEntryHandle acl, BcmAclStatHandle aclStatHandle);

  static std::set<cfg::CounterType> getCounterTypeList(
      int unit,
      uint32_t counterID);

  static int getNumAclStatsInFpGroup(int unit, int gid);

  static void removeAllCountersInFpGroup(int unit, int gid);

  static std::optional<uint32_t>
  getFlexCounterIDFromAttachedAcl(int unit, int groupID, BcmAclEntryHandle acl);

  static BcmTrafficCounterStats getAclTrafficFlexCounterStats(
      int unit,
      BcmAclStatHandle handle,
      const std::vector<cfg::CounterType>& counters);

 private:
  // Forbidden copy constructor and assignment operator
  BcmIngressFieldProcessorFlexCounter(
      BcmIngressFieldProcessorFlexCounter const&) = delete;
  BcmIngressFieldProcessorFlexCounter& operator=(
      BcmIngressFieldProcessorFlexCounter const&) = delete;
};
} // namespace facebook::fboss
