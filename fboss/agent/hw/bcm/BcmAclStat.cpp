/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmAclStat.h"

#include "fboss/agent/hw/bcm/BcmAddressFBConvertors.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmFieldProcessorFBConvertors.h"
#include "fboss/agent/hw/bcm/BcmFieldProcessorUtils.h"
#include "fboss/agent/hw/bcm/BcmIngressFieldProcessorFlexCounter.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/BcmWarmBootCache.h"

extern "C" {
#include <bcm/field.h>
}

namespace facebook::fboss {

using facebook::fboss::bcmCheckError;

BcmAclStat::BcmAclStat(
    BcmSwitch* hw,
    int gid,
    const std::vector<cfg::CounterType>& counters,
    BcmAclStatType type,
    BcmAclStatActionIndex actionIndex)
    : hw_(hw), statType_(type), actionIndex_(actionIndex) {
  if (hw_->getPlatform()->getAsic()->isSupported(
          HwAsic::Feature::INGRESS_FIELD_PROCESSOR_FLEX_COUNTER)) {
    flexCounter_ = std::make_unique<BcmIngressFieldProcessorFlexCounter>(
        hw_->getUnit(), gid, std::nullopt, statType_);
    handle_ = flexCounter_->getID();
  } else {
    int statHandle;
    std::vector<bcm_field_stat_t> types;
    for (auto counter : counters) {
      types.push_back(utility::cfgCounterTypeToBcmCounterType(counter));
    }
    auto rv = bcm_field_stat_create(
        hw_->getUnit(), gid, types.size(), types.data(), &statHandle);
    bcmCheckError(rv, "Failed to create stat in group=", gid);
    handle_ = statHandle;
  }
}

BcmAclStat::BcmAclStat(
    BcmSwitch* hw,
    BcmAclStatHandle statHandle,
    BcmAclStatType type,
    BcmAclStatActionIndex actionIndex)
    : hw_(hw), handle_(statHandle), statType_(type), actionIndex_(actionIndex) {
  if (hw_->getPlatform()->getAsic()->isSupported(
          HwAsic::Feature::INGRESS_FIELD_PROCESSOR_FLEX_COUNTER)) {
    flexCounter_ = std::make_unique<BcmIngressFieldProcessorFlexCounter>(
        hw_->getUnit(), std::nullopt, statHandle, statType_);
  }
}

BcmAclStat::~BcmAclStat() {
  if (flexCounter_) {
    if (statType_ == BcmAclStatType::EM) {
      // EM stats share flex counter. Set the hwid to 0 so that
      // hw entry is not freed. it will be freed when table is destroyed
      flexCounter_->setHwCounterID(0);
    }
    flexCounter_.reset();
  } else {
    auto rv = bcm_field_stat_destroy(hw_->getUnit(), handle_);
    bcmLogFatal(rv, hw_, "Failed to destroy stat, stat_id=", handle_);
  }
}

void BcmAclStat::destroy(
    const BcmSwitchIf* hw,
    BcmAclStatHandle aclStatHandle) {
  if (hw->getPlatform()->getAsic()->isSupported(
          HwAsic::Feature::INGRESS_FIELD_PROCESSOR_FLEX_COUNTER)) {
    BcmIngressFieldProcessorFlexCounter::destroy(hw->getUnit(), aclStatHandle);
  } else {
    auto rv = bcm_field_stat_destroy(hw->getUnit(), aclStatHandle);
    bcmCheckError(rv, "Failed to destroy stat=", aclStatHandle);
  }
}

bool BcmAclStat::isStateSame(
    const BcmSwitch* hw,
    BcmAclStatHandle statHandle,
    cfg::TrafficCounter& counter) {
  if (hw->getPlatform()->getAsic()->isSupported(
          HwAsic::Feature::INGRESS_FIELD_PROCESSOR_FLEX_COUNTER)) {
    const auto& hwCounterTypes =
        BcmIngressFieldProcessorFlexCounter::getCounterTypeList(
            hw->getUnit(), statHandle);
    for (auto type : *counter.types()) {
      if (hwCounterTypes.find(type) == hwCounterTypes.end()) {
        return false;
      }
    }
    return true;
  }

  std::vector<bcm_field_stat_t> expectedCounterTypes;
  for (auto type : *counter.types()) {
    expectedCounterTypes.push_back(
        utility::cfgCounterTypeToBcmCounterType(type));
  }

  int statSize;
  auto rv = bcm_field_stat_size(hw->getUnit(), statHandle, &statSize);
  bcmCheckError(rv, "Unable to get stat size for acl stat=", statHandle);
  if (statSize < expectedCounterTypes.size()) {
    return false;
  }

  std::vector<bcm_field_stat_t> counterTypes(statSize);
  rv = bcm_field_stat_config_get(
      hw->getUnit(), statHandle, counterTypes.size(), counterTypes.data());
  bcmCheckError(rv, "Unable to get stat types for acl stat=", statHandle);

  std::sort(counterTypes.begin(), counterTypes.end());
  std::sort(expectedCounterTypes.begin(), expectedCounterTypes.end());

  /*
   * The asic used by Wedge40 can't program a subset a counter types, it has to
   * program them all at once. The SDK hides this limitation by storing the
   * counter types in an internal data structure, so if you program a single
   * counter and ask the SDK what counters are being used, it returns a single
   * counter (even if 2 counters were actually programmed).
   * The problem is that when the switch do a warmboot, the SDK's internal state
   * has been cleared and the SDK returns the real values from the HW: 2
   * counters.
   *
   * To overcome this problem on Trident 2, we have to check if the set of
   * expected counters is a subset of the hw counters instead of just checking
   * for equality.
   */
  if (hw->getPlatform()->getAsic()->getAsicType() ==
      cfg::AsicType::ASIC_TYPE_TRIDENT2) {
    return std::includes(
        counterTypes.begin(),
        counterTypes.end(),
        expectedCounterTypes.begin(),
        expectedCounterTypes.end());
  }
  return counterTypes == expectedCounterTypes;
}

void BcmAclStat::attach(BcmAclEntryHandle acl) {
  if (hw_->getPlatform()->getAsic()->isSupported(
          HwAsic::Feature::INGRESS_FIELD_PROCESSOR_FLEX_COUNTER)) {
    flexCounter_->attach(acl, actionIndex_);
  } else {
    auto rv = bcm_field_entry_stat_attach(hw_->getUnit(), acl, handle_);
    bcmCheckError(rv, "Failed to attach stat=", handle_, " to acl=", acl);
  }
}

void BcmAclStat::detach(BcmAclEntryHandle acl) {
  // This function will be called by BcmAclEntry::~BcmAclEntry(),
  // so we can't use HwAsic::isSupported() because destructor is not allowed to
  // call pure virtual method.
  if (flexCounter_) {
    flexCounter_->detach(acl, actionIndex_);
  } else {
    auto rv = bcm_field_entry_stat_detach(hw_->getUnit(), acl, handle_);
    bcmCheckError(rv, "Failed to detach stat=", handle_, " from acl=", acl);
  }
}

void BcmAclStat::detach(
    const BcmSwitchIf* hw,
    BcmAclEntryHandle acl,
    BcmAclStatHandle aclStatHandle,
    BcmAclStatActionIndex actionIndex) {
  if (hw->getPlatform()->getAsic()->isSupported(
          HwAsic::Feature::INGRESS_FIELD_PROCESSOR_FLEX_COUNTER)) {
    BcmIngressFieldProcessorFlexCounter::detach(
        hw->getUnit(), acl, aclStatHandle, actionIndex);
  } else {
    auto rv = bcm_field_entry_stat_detach(hw->getUnit(), acl, aclStatHandle);
    bcmCheckError(
        rv, "Failed to detach stat=", aclStatHandle, " from acl=", acl);
  }
}

int BcmAclStat::getNumAclStatsInFpGroup(const BcmSwitch* hw, int gid) {
  if (hw->getPlatform()->getAsic()->isSupported(
          HwAsic::Feature::INGRESS_FIELD_PROCESSOR_FLEX_COUNTER)) {
    return BcmIngressFieldProcessorFlexCounter::getNumAclStatsInFpGroup(
        hw->getUnit(), gid);
  } else {
    bcm_field_group_status_t status;
    auto rv = bcm_field_group_status_get(hw->getUnit(), gid, &status);
    bcmCheckError(
        rv, "failed to get group status for gid=", folly::to<std::string>(gid));
    return status.counter_count;
  }
}

std::optional<std::pair<BcmAclStatHandle, BcmAclStatActionIndex>>
BcmAclStat::getAclStatHandleFromAttachedAcl(
    const BcmSwitchIf* hw,
    int groupID,
    BcmAclEntryHandle acl) {
  if (hw->getPlatform()->getAsic()->isSupported(
          HwAsic::Feature::INGRESS_FIELD_PROCESSOR_FLEX_COUNTER)) {
    if (auto flexCounterID = BcmIngressFieldProcessorFlexCounter::
            getFlexCounterIDFromAttachedAcl(hw->getUnit(), groupID, acl)) {
      return std::optional<std::pair<BcmAclStatHandle, BcmAclStatActionIndex>>{
          std::make_pair((*flexCounterID).first, (*flexCounterID).second)};
    } else {
      return std::nullopt;
    }
  } else {
    int statHandle;
    auto rv = bcm_field_entry_stat_get(hw->getUnit(), acl, &statHandle);
    if (rv == BCM_E_NOT_FOUND) {
      return std::nullopt;
    }
    bcmCheckError(rv, "Unable to get stat_id of field entry=", acl);
    return std::optional<std::pair<BcmAclStatHandle, BcmAclStatActionIndex>>{
        std::make_pair(statHandle, kDefaultAclActionIndex)};
  }
}
} // namespace facebook::fboss
