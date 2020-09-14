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
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/BcmWarmBootCache.h"
#include "fboss/agent/state/AclEntry.h"

#include <boost/container/flat_map.hpp>

extern "C" {
#include <bcm/field.h>
}

namespace facebook::fboss {

using facebook::fboss::bcmCheckError;

BcmAclStat::BcmAclStat(
    BcmSwitch* hw,
    int gid,
    const std::vector<cfg::CounterType>& counters)
    : hw_(hw) {
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

BcmAclStat::~BcmAclStat() {
  auto rv = bcm_field_stat_destroy(hw_->getUnit(), handle_);
  bcmLogFatal(rv, hw_, "Failed to destroy stat, stat_id=", handle_);
}

bool BcmAclStat::isStateSame(
    const BcmSwitch* hw,
    BcmAclStatHandle statHandle,
    cfg::TrafficCounter& counter) {
  std::vector<bcm_field_stat_t> expectedCounterTypes;
  for (auto type : *counter.types_ref()) {
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
      HwAsic::AsicType::ASIC_TYPE_TRIDENT2) {
    return std::includes(
        counterTypes.begin(),
        counterTypes.end(),
        expectedCounterTypes.begin(),
        expectedCounterTypes.end());
  }
  return counterTypes == expectedCounterTypes;
}

} // namespace facebook::fboss
