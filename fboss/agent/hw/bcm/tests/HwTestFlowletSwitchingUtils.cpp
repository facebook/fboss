/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/HwTestFlowletSwitchingUtils.h"
#include <fboss/agent/gen-cpp2/switch_config_types.h>
#include "fboss/agent/hw/bcm/BcmEcmpUtils.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/tests/BcmTestUtils.h"

using namespace facebook::fboss;

namespace {
const RouterID kRid(0);
} // namespace

namespace facebook::fboss::utility {

bool validateFlowletSwitchingEnabled(
    const facebook::fboss::HwSwitch* hw,
    const cfg::FlowletSwitchingConfig& flowletCfg) {
  const auto bcmSwitch = static_cast<const BcmSwitch*>(hw);
  utility::assertSwitchControl(bcmSwitchECMPHashSet0Offset, 0x5);
  utility::assertSwitchControl(bcmSwitchEcmpDynamicHashOffset, 0x5);
  utility::assertSwitchControl(
      bcmSwitchEcmpDynamicEgressBytesExponent,
      *flowletCfg.dynamicEgressLoadExponent());
  utility::assertSwitchControl(
      bcmSwitchEcmpDynamicQueuedBytesExponent,
      *flowletCfg.dynamicQueueExponent());
  utility::assertSwitchControl(
      bcmSwitchEcmpDynamicQueuedBytesMinThreshold,
      *flowletCfg.dynamicQueueMinThresholdBytes());
  utility::assertSwitchControl(
      bcmSwitchEcmpDynamicQueuedBytesMaxThreshold,
      *flowletCfg.dynamicQueueMaxThresholdBytes());
  utility::assertSwitchControl(
      bcmSwitchEcmpDynamicSampleRate, *flowletCfg.dynamicSampleRate());
  return true;
}

bool verifyEcmpForFlowletSwitching(
    const facebook::fboss::HwSwitch* hw,
    const folly::CIDRNetwork& prefix,
    const cfg::FlowletSwitchingConfig& flowletCfg) {
  const auto bcmSwitch = static_cast<const BcmSwitch*>(hw);
  auto ecmp = getEgressIdForRoute(bcmSwitch, prefix.first, prefix.second, kRid);
  bcm_l3_egress_ecmp_t existing;
  bcm_l3_egress_ecmp_t_init(&existing);
  existing.ecmp_intf = ecmp;
  existing.flags |= BCM_L3_WITH_ID;
  int pathsInHwCount;
  bcm_l3_ecmp_get(bcmSwitch->getUnit(), &existing, 0, nullptr, &pathsInHwCount);
  CHECK_EQ(existing.dynamic_mode, BCM_L3_ECMP_DYNAMIC_MODE_NORMAL);
  CHECK_EQ(existing.dynamic_age, *flowletCfg.inactivityIntervalUsecs());
  CHECK_EQ(existing.dynamic_size, *flowletCfg.flowletTableSize());
  return true;
}
} // namespace facebook::fboss::utility
