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
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmSdkVer.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/tests/BcmSwitchEnsemble.h"
#include "fboss/agent/hw/bcm/tests/BcmTestUtils.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/test/TestEnsembleIf.h"

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

bool validateFlowSetTable(
    const facebook::fboss::HwSwitch* hw,
    const bool expectFlowsetSizeZero) {
  bool isVerified = true;

  XLOG(DBG2) << "validateFlowSetTable with "
             << "expectFlowsetSizeZero: " << expectFlowsetSizeZero;
#if (                                      \
    defined(BCM_SDK_VERSION_GTE_6_5_26) && \
    !defined(BCM_SDK_VERSION_GTE_6_5_28))
  int maxEntries = 0;
  const auto bcmSwitch = static_cast<const BcmSwitch*>(hw);

  int rv = bcm_switch_object_count_get(
      bcmSwitch->getUnit(), bcmSwitchObjectEcmpDynamicFlowSetMax, &maxEntries);
  bcmCheckError(rv, "Failed to get bcmSwitchObjectEcmpDynamicFlowSetMax");

  int usedEntries = 0;
  rv = bcm_switch_object_count_get(
      bcmSwitch->getUnit(),
      bcmSwitchObjectEcmpDynamicFlowSetUsed,
      &usedEntries);
  bcmCheckError(rv, "Failed to get bcmSwitchObjectEcmpDynamicFlowSetUsed");

  int freeEntries = 0;
  rv = bcm_switch_object_count_get(
      bcmSwitch->getUnit(),
      bcmSwitchObjectEcmpDynamicFlowSetFree,
      &freeEntries);
  bcmCheckError(rv, "Failed to get bcmSwitchObjectEcmpDynamicFlowSetFree");
  if (expectFlowsetSizeZero) {
    CHECK_EQ(freeEntries, 0);
  }

  if (maxEntries != KMaxFlowsetTableSize) {
    XLOG(ERR) << "Max Entries are not as expected: " << maxEntries;
    isVerified = false;
  }

  if ((maxEntries - usedEntries) != freeEntries) {
    XLOG(ERR) << "Free entries: " << freeEntries
              << ", unexpected when looking at the used entries : "
              << usedEntries << " , max entries: " << maxEntries;
    isVerified = false;
  }
#endif
  return isVerified;
}

bool verifyEcmpForFlowletSwitching(
    const facebook::fboss::HwSwitch* hw,
    const folly::CIDRNetwork& prefix,
    const cfg::FlowletSwitchingConfig& flowletCfg,
    const cfg::PortFlowletConfig& cfg,
    const bool flowletEnable) {
  const auto bcmSwitch = static_cast<const BcmSwitch*>(hw);
  auto ecmp = getEgressIdForRoute(bcmSwitch, prefix.first, prefix.second, kRid);
  bcm_l3_egress_ecmp_t existing;
  bcm_l3_egress_ecmp_t_init(&existing);
  existing.ecmp_intf = ecmp;
  existing.flags |= BCM_L3_WITH_ID;
  int pathsInHwCount;
  bool isVerified = true;
  bcm_l3_ecmp_get(bcmSwitch->getUnit(), &existing, 0, nullptr, &pathsInHwCount);

  uint32 flowletTableSize = static_cast<uint16>(*flowletCfg.flowletTableSize());
  if (flowletEnable) {
    auto dynamicMode = getFlowletDynamicMode(*flowletCfg.switchingMode());
    if ((existing.dynamic_mode != dynamicMode) ||
        (existing.dynamic_age != *flowletCfg.inactivityIntervalUsecs()) ||
        (existing.dynamic_size != flowletTableSize)) {
      XLOG(DBG2) << "Existing mode: " << existing.dynamic_mode
                 << ", Configured mode: " << dynamicMode
                 << ", Existing age: " << existing.dynamic_age
                 << ", Configured age: "
                 << *flowletCfg.inactivityIntervalUsecs()
                 << ", Existing size: " << existing.dynamic_size
                 << ", Configured size: " << flowletTableSize;
      isVerified = false;
    }
  } else {
    if ((existing.dynamic_mode != BCM_L3_ECMP_DYNAMIC_MODE_DISABLED) ||
        (existing.dynamic_size != 0)) {
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

bool verifyEcmpForNonFlowlet(
    const facebook::fboss::HwSwitch* hw,
    const folly::CIDRNetwork& prefix,
    const bool expectFlowsetFree) {
  const auto bcmSwitch = static_cast<const BcmSwitch*>(hw);
  auto ecmp = getEgressIdForRoute(bcmSwitch, prefix.first, prefix.second, kRid);
  bcm_l3_egress_ecmp_t existing;
  bcm_l3_egress_ecmp_t_init(&existing);
  existing.ecmp_intf = ecmp;
  existing.flags |= BCM_L3_WITH_ID;
  int pathsInHwCount;
  bcm_l3_ecmp_get(bcmSwitch->getUnit(), &existing, 0, nullptr, &pathsInHwCount);
  // Ecmp Id should be greater than or equal to max dlb Ecmp Id
  CHECK_GE(ecmp, kDlbEcmpMaxId);
  // Check all the flowlet configs are disabled
  CHECK_EQ(existing.dynamic_mode, BCM_L3_ECMP_DYNAMIC_MODE_DISABLED);
  CHECK_EQ(existing.dynamic_age, 0);
  CHECK_EQ(existing.dynamic_size, 0);
  int freeEntries = 0;
  bcm_switch_object_count_get(
      bcmSwitch->getUnit(),
      bcmSwitchObjectEcmpDynamicFlowSetFree,
      &freeEntries);
  if (expectFlowsetFree) {
    CHECK_GE(freeEntries, 2048);
  } else {
    CHECK_EQ(freeEntries, 0);
  }
  return true;
}

bool validatePortFlowletQuality(
    const facebook::fboss::HwSwitch* hw,
    const PortID& portId,
    const cfg::PortFlowletConfig& cfg,
    bool /* enable */) {
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

void setEcmpMemberStatus(const TestEnsembleIf* ensemble) {
  const auto bcmEnsemble = dynamic_cast<const BcmSwitchEnsemble*>(ensemble);
  auto bcmSwitch = bcmEnsemble->getHwSwitch();
  setEcmpDynamicMemberUp(bcmSwitch);
}

int getL3EcmpDlbFailPackets(const TestEnsembleIf* ensemble) {
  const auto bcmEnsemble = dynamic_cast<const BcmSwitchEnsemble*>(ensemble);
  auto bcmSwitch = bcmEnsemble->getHwSwitch();
  return bcmSwitch->getHwFlowletStats().l3EcmpDlbFailPackets().value();
}
} // namespace facebook::fboss::utility
