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
#include "fboss/agent/hw/switch_asics/HwAsic.h"

using namespace facebook::fboss;

namespace {
const RouterID kRid(0);
} // namespace

namespace facebook::fboss::utility {

void verifyEgressEcmpEthertype(const BcmSwitch* bcmSwitch) {
  uint32 flags = 0;
  int ethertype_max = 2;
  int ethertype_count = 0;
  int ethertype_array[2] = {0, 0};
  bcm_l3_egress_ecmp_ethertype_get(
      bcmSwitch->getUnit(),
      &flags,
      ethertype_max,
      ethertype_array,
      &ethertype_count);
  CHECK_EQ(flags, 0);
  CHECK_EQ(ethertype_count, 2);
  CHECK_EQ(ethertype_array[0], 0x0800);
  CHECK_EQ(ethertype_array[1], 0x86DD);
}

bool validateFlowletSwitchingEnabled(
    const facebook::fboss::HwSwitch* hw,
    const cfg::FlowletSwitchingConfig& flowletCfg) {
  const auto bcmSwitch = static_cast<const BcmSwitch*>(hw);
  utility::assertSwitchControl(bcmSwitchECMPHashSet0Offset, 5);
  utility::assertSwitchControl(bcmSwitchEcmpDynamicHashOffset, 5);
  utility::assertSwitchControl(bcmSwitchHashUseFlowSelEcmpDynamic, 1);
  utility::assertSwitchControl(bcmSwitchMacroFlowEcmpDynamicHashMinOffset, 0);
  utility::assertSwitchControl(bcmSwitchMacroFlowEcmpDynamicHashMaxOffset, 15);
  utility::assertSwitchControl(
      bcmSwitchMacroFlowEcmpDynamicHashStrideOffset, 1);
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
      bcmSwitchEcmpDynamicPhysicalQueuedBytesMinThreshold,
      (*flowletCfg.dynamicQueueMinThresholdBytes() >> 1));
  utility::assertSwitchControl(
      bcmSwitchEcmpDynamicQueuedBytesMaxThreshold,
      *flowletCfg.dynamicQueueMaxThresholdBytes());
  utility::assertSwitchControl(
      bcmSwitchEcmpDynamicPhysicalQueuedBytesMaxThreshold,
      (*flowletCfg.dynamicQueueMaxThresholdBytes() >> 1));
  utility::assertSwitchControl(
      bcmSwitchEcmpDynamicSampleRate, *flowletCfg.dynamicSampleRate());
  utility::assertSwitchControl(
      bcmSwitchEcmpDynamicEgressBytesMinThreshold,
      *flowletCfg.dynamicEgressMinThresholdBytes());
  utility::assertSwitchControl(
      bcmSwitchEcmpDynamicEgressBytesMaxThreshold,
      *flowletCfg.dynamicEgressMaxThresholdBytes());
  utility::assertSwitchControl(
      bcmSwitchEcmpDynamicPhysicalQueuedBytesExponent,
      *flowletCfg.dynamicPhysicalQueueExponent());
  utility::assertSwitchControl(bcmSwitchEcmpDynamicRandomSeed, 0x5555);

  verifyEgressEcmpEthertype(bcmSwitch);
  return true;
}

bool verifyEcmpForFlowletSwitching(
    const facebook::fboss::HwSwitch* hw,
    const folly::CIDRNetwork& prefix,
    const cfg::FlowletSwitchingConfig& flowletCfg,
    const cfg::PortFlowletConfig& cfg,
    const bool flowletEnable,
    const bool expectFlowsetSizeZero) {
  const auto bcmSwitch = static_cast<const BcmSwitch*>(hw);
  auto ecmp = getEgressIdForRoute(bcmSwitch, prefix.first, prefix.second, kRid);
  bcm_l3_egress_ecmp_t existing;
  bcm_l3_egress_ecmp_t_init(&existing);
  existing.ecmp_intf = ecmp;
  existing.flags |= BCM_L3_WITH_ID;
  int pathsInHwCount;
  bool isVerified = true;
  bcm_l3_ecmp_get(bcmSwitch->getUnit(), &existing, 0, nullptr, &pathsInHwCount);
  const int flowletTableSize = getFlowletSizeWithScalingFactor(
      *flowletCfg.flowletTableSize(), pathsInHwCount, *flowletCfg.maxLinks());
  if (flowletEnable && flowletTableSize > 0) {
    if ((existing.dynamic_mode != BCM_L3_ECMP_DYNAMIC_MODE_NORMAL) ||
        (existing.dynamic_age != *flowletCfg.inactivityIntervalUsecs()) ||
        (existing.dynamic_size != flowletTableSize) ||
        (expectFlowsetSizeZero != 0)) {
      isVerified = false;
    }
  } else {
    if ((existing.dynamic_mode != BCM_L3_ECMP_DYNAMIC_MODE_DISABLED) ||
        (expectFlowsetSizeZero != 1) ||
        (existing.dynamic_size != flowletTableSize)) {
      isVerified = false;
    }
  }
  if ((ecmp > 200128) || (ecmp < 200000)) {
    isVerified = false;
  }

  auto ecmp_members = getEcmpGroupInHw(bcmSwitch, ecmp, pathsInHwCount);
  for (const auto& ecmp_member : ecmp_members) {
    int status = -1;
    bcm_l3_egress_ecmp_member_status_get(
        bcmSwitch->getUnit(), ecmp_member, &status);
    if (flowletEnable) {
      if (status < BCM_L3_ECMP_DYNAMIC_MEMBER_HW) {
        isVerified = false;
      }
    }
    bcm_l3_egress_t egress;
    bcm_l3_egress_t_init(&egress);
    bcm_l3_egress_get(0, ecmp_member, &egress);
    if (flowletEnable &&
        !(hw->getPlatform()->getAsic()->isSupported(
            HwAsic::Feature::FLOWLET_PORT_ATTRIBUTES))) {
      // verify the port flowlet config values only in TH3
      // since this port flowlet configs are set in egress object in TH3
      CHECK_EQ(egress.dynamic_scaling_factor, *cfg.scalingFactor());
      CHECK_EQ(egress.dynamic_load_weight, *cfg.loadWeight());
      CHECK_EQ(egress.dynamic_queue_size_weight, *cfg.queueWeight());
    } else {
      // verify the default values in TH4
      // since this won't be set in egress object in TH4
      CHECK_EQ(egress.dynamic_scaling_factor, -1);
      CHECK_EQ(egress.dynamic_load_weight, -1);
      CHECK_EQ(egress.dynamic_queue_size_weight, -1);
    }
  }
  return isVerified;
}

bool validatePortFlowletQuality(
    const facebook::fboss::HwSwitch* hw,
    const PortID& portId,
    const cfg::PortFlowletConfig& cfg) {
  bcm_l3_ecmp_dlb_port_quality_attr_t attr;
  bcm_l3_ecmp_dlb_port_quality_attr_t_init(&attr);
  const auto bcmSwitch = static_cast<const BcmSwitch*>(hw);
  bcm_l3_ecmp_dlb_port_quality_attr_get(bcmSwitch->getUnit(), portId, &attr);
  if (hw->getPlatform()->getAsic()->getAsicType() !=
      cfg::AsicType::ASIC_TYPE_FAKE) {
    CHECK_EQ(attr.scaling_factor, *cfg.scalingFactor());
    CHECK_EQ(attr.load_weight, *cfg.loadWeight());
    CHECK_EQ(attr.queue_size_weight, *cfg.queueWeight());
  }
  return true;
}

bool validateFlowletSwitchingDisabled(const facebook::fboss::HwSwitch* hw) {
  const auto bcmSwitch = static_cast<const BcmSwitch*>(hw);
  utility::assertSwitchControl(bcmSwitchEcmpDynamicEgressBytesExponent, 0);
  utility::assertSwitchControl(bcmSwitchEcmpDynamicQueuedBytesExponent, 0);
  utility::assertSwitchControl(bcmSwitchEcmpDynamicQueuedBytesMinThreshold, 0);
  utility::assertSwitchControl(
      bcmSwitchEcmpDynamicPhysicalQueuedBytesMinThreshold, 0);
  utility::assertSwitchControl(bcmSwitchEcmpDynamicQueuedBytesMaxThreshold, 0);
  utility::assertSwitchControl(
      bcmSwitchEcmpDynamicPhysicalQueuedBytesMaxThreshold, 0);
  utility::assertSwitchControl(bcmSwitchEcmpDynamicSampleRate, 62500);
  utility::assertSwitchControl(bcmSwitchEcmpDynamicEgressBytesMinThreshold, 0);
  utility::assertSwitchControl(bcmSwitchEcmpDynamicEgressBytesMaxThreshold, 0);
  utility::assertSwitchControl(
      bcmSwitchEcmpDynamicPhysicalQueuedBytesExponent, 0);
  utility::assertSwitchControl(bcmSwitchEcmpDynamicRandomSeed, 0);

  verifyEgressEcmpEthertype(bcmSwitch);
  return true;
}

void setEcmpMemberStatus(const facebook::fboss::HwSwitch* hw) {
  const auto bcmSwitch = static_cast<const BcmSwitch*>(hw);
  auto ecmpMembers = utility::getEcmpMembersInHw(bcmSwitch);
  for (const auto ecmpMember : ecmpMembers) {
    bcm_l3_egress_ecmp_member_status_set(
        bcmSwitch->getUnit(), ecmpMember, BCM_L3_ECMP_DYNAMIC_MEMBER_FORCE_UP);
  }
}

} // namespace facebook::fboss::utility
