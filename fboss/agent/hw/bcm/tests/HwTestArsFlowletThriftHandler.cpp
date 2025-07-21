// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.
#include "fboss/agent/hw/bcm/BcmEcmpUtils.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/test/HwTestThriftHandler.h"

extern "C" {
#include <bcm/l3.h>
#include <bcm/switch.h>
}

namespace facebook::fboss::utility {

const RouterID kRid(0);

bool HwTestThriftHandler::verifyEcmpForFlowletSwitchingHandler(
    std::unique_ptr<CIDRNetwork> ip,
    std::unique_ptr<::facebook::fboss::state::SwitchSettingsFields> settings,
    bool flowletEnabled) {
  const auto bcmSwitch = static_cast<const BcmSwitch*>(hwSwitch_);

  // Convert CIDRNetwork to folly::CIDRNetwork
  folly::CIDRNetwork follyPrefix{
      folly::IPAddress(*ip->IPAddress()), static_cast<uint8_t>(*ip->mask())};

  auto ecmp = getEgressIdForRoute(
      bcmSwitch, follyPrefix.first, follyPrefix.second, kRid);
  bcm_l3_egress_ecmp_t existing;
  bcm_l3_egress_ecmp_t_init(&existing);
  existing.ecmp_intf = ecmp;
  existing.flags |= BCM_L3_WITH_ID;
  int pathsInHwCount;
  bool isVerified = true;
  bcm_l3_ecmp_get(bcmSwitch->getUnit(), &existing, 0, nullptr, &pathsInHwCount);

  // Extract flowlet configuration from settings
  auto flowletCfg = settings->flowletSwitchingConfig();
  uint32 flowletTableSize =
      flowletCfg ? static_cast<uint16>(*flowletCfg->flowletTableSize()) : 0;

  if (flowletEnabled && flowletCfg) {
    auto dynamicMode = getFlowletDynamicMode(*flowletCfg->switchingMode());
    if ((existing.dynamic_mode != dynamicMode) ||
        (existing.dynamic_age != *flowletCfg->inactivityIntervalUsecs()) ||
        (existing.dynamic_size != flowletTableSize)) {
      XLOG(ERR)
          << "verifyEcmpForFlowletSwitching: Flowlet configuration mismatch - "
          << "dynamic_mode expected: " << dynamicMode
          << ", actual: " << existing.dynamic_mode << ", dynamic_age expected: "
          << *flowletCfg->inactivityIntervalUsecs()
          << ", actual: " << existing.dynamic_age
          << ", dynamic_size expected: " << flowletTableSize
          << ", actual: " << existing.dynamic_size;
      isVerified = false;
    }
  } else {
    if ((existing.dynamic_mode != BCM_L3_ECMP_DYNAMIC_MODE_DISABLED) ||
        (existing.dynamic_size != 0)) {
      XLOG(ERR)
          << "verifyEcmpForFlowletSwitching: Flowlet should be disabled but found - "
          << "dynamic_mode: " << existing.dynamic_mode
          << " (expected: " << BCM_L3_ECMP_DYNAMIC_MODE_DISABLED << "), "
          << "dynamic_size: " << existing.dynamic_size << " (expected: 0)";
      isVerified = false;
    }
  }
  if ((ecmp > 200128) || (ecmp < 200000)) {
    XLOG(ERR)
        << "verifyEcmpForFlowletSwitching: ECMP ID out of expected range - "
        << "ecmp: " << ecmp << " (expected range: 200000-200128)";
    isVerified = false;
  }

  auto ecmp_members = getEcmpGroupInHw(bcmSwitch, ecmp, pathsInHwCount);
  for (const auto& ecmp_member : ecmp_members) {
    int status = -1;
    bcm_l3_egress_ecmp_member_status_get(
        bcmSwitch->getUnit(), ecmp_member, &status);
    if (flowletEnabled) {
      if (status < BCM_L3_ECMP_DYNAMIC_MEMBER_HW) {
        XLOG(ERR)
            << "verifyEcmpForFlowletSwitching: ECMP member status insufficient for flowlet - "
            << "member: " << ecmp_member << ", status: " << status
            << " (expected >= " << BCM_L3_ECMP_DYNAMIC_MEMBER_HW << ")";
        isVerified = false;
      }
    }
  }
  return isVerified;
}

bool HwTestThriftHandler::validateFlowSetTable(
    const bool expectFlowsetSizeZero) {
  bool isVerified = true;

  XLOG(DBG2) << "validateFlowSetTable with "
             << "expectFlowsetSizeZero: " << expectFlowsetSizeZero;
#if (                                      \
    defined(BCM_SDK_VERSION_GTE_6_5_26) && \
    !defined(BCM_SDK_VERSION_GTE_6_5_28))

  int maxEntries = 0;
  const auto bcmSwitch = static_cast<const BcmSwitch*>(hwSwitch_);

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

  XLOG(DBG2) << "version check passed - maxEntries: " << maxEntries
             << ", usedEntries: " << usedEntries
             << ", freeEntries: " << freeEntries;

  if (expectFlowsetSizeZero) {
    CHECK_EQ(freeEntries, 0);
  }

  if (maxEntries != 32768) {
    XLOG(ERR) << "Max Entries are not as expected: " << maxEntries;
    isVerified = false;
  }

  if ((maxEntries - usedEntries) != freeEntries) {
    XLOG(ERR) << "Free entries: " << freeEntries
              << ", unexpected when looking at the used entries : "
              << usedEntries << " , max entries: " << maxEntries;
    isVerified = false;
  }
#else
  XLOG(DBG2)
      << "version check failed - required BCM_SDK_VERSION_GTE_6_5_26 && !BCM_SDK_VERSION_GTE_6_5_28";
#endif
  return isVerified;
}

} // namespace facebook::fboss::utility
