// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/hw/sai/switch/SaiArsManager.h"
#include "fboss/agent/hw/sai/switch/SaiNextHopGroupManager.h"
#include "fboss/agent/hw/sai/switch/SaiPortManager.h"
#include "fboss/agent/hw/sai/switch/SaiRouteManager.h"
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"
#include "fboss/agent/hw/sai/switch/SaiVirtualRouterManager.h"
#include "fboss/agent/hw/test/HwTestThriftHandler.h"

namespace facebook::fboss::utility {

bool verifyArs(
    const HwSwitch* hw,
    const ArsSaiId& arsSaiId,
    const cfg::FlowletSwitchingConfig& cfg) {
  ArsSaiId nullArsSaiId{SAI_NULL_OBJECT_ID};
  if (arsSaiId == nullArsSaiId) {
    XLOG(ERR)
        << "verifyArs: ARS SAI ID is null, cannot verify ARS configuration";
    return false;
  }

#if SAI_API_VERSION >= SAI_VERSION(1, 14, 0)
  auto& arsApi = SaiApiTable::getInstance()->arsApi();
  const auto saiSwitch = static_cast<const SaiSwitch*>(hw);

  auto mode = arsApi.getAttribute(arsSaiId, SaiArsTraits::Attributes::Mode());
  auto switchingMode =
      saiSwitch->managerTable()->arsManager().cfgSwitchingModeToSai(
          *cfg.switchingMode());
  if (switchingMode != mode) {
    XLOG(ERR) << "verifyArs: Switching mode mismatch - expected: "
              << switchingMode << ", actual: " << mode;
    return false;
  }
  if (hw->getPlatform()->getAsic()->getAsicType() !=
      cfg::AsicType::ASIC_TYPE_CHENAB) {
    auto idleTime =
        arsApi.getAttribute(arsSaiId, SaiArsTraits::Attributes::IdleTime());
    if (*cfg.inactivityIntervalUsecs() != idleTime) {
      XLOG(ERR) << "verifyArs: Inactivity interval mismatch - expected: "
                << *cfg.inactivityIntervalUsecs() << ", actual: " << idleTime;
      return false;
    }
    auto maxFlows =
        arsApi.getAttribute(arsSaiId, SaiArsTraits::Attributes::MaxFlows());
    if (*cfg.flowletTableSize() != maxFlows) {
      XLOG(ERR) << "verifyArs: Flowlet table size mismatch - expected: "
                << *cfg.flowletTableSize() << ", actual: " << maxFlows;
      return false;
    }
  }
#endif
  return true;
}
NextHopGroupSaiId getNextHopGroupSaiId(
    const SaiSwitch* saiSwitch,
    const folly::CIDRNetwork& ip) {
  const auto* managerTable = saiSwitch->managerTable();
  const auto* vrHandle =
      managerTable->virtualRouterManager().getVirtualRouterHandle(RouterID(0));
  auto routeEntry = SaiRouteTraits::RouteEntry{
      saiSwitch->getSaiSwitchId(), vrHandle->virtualRouter->adapterKey(), ip};
  const auto* routeHandle =
      managerTable->routeManager().getRouteHandle(routeEntry);
  return static_cast<NextHopGroupSaiId>(
      routeHandle->nextHopGroupHandle()->adapterKey());
}

/**
 * @brief Verifies the port flowlet configuration for a given network prefix
 * @param hw HwSwitch pointer to the hardware switch
 * @param ip Network prefix (folly::CIDRNetwork) to verify flowlet configuration
 * for
 * @param cfg Port flowlet configuration containing scaling factor, load weight,
 * and queue weight
 * @param flowletEnable Boolean indicating whether flowlet switching is enabled
 * @return True if the port flowlet configuration in hardware matches the
 * expected configuration, false otherwise. For SAI implementation, this is a
 * placeholder that always returns true as port flowlet configuration is handled
 * differently in SAI compared to BCM.
 */
bool verifyPortFlowletConfig(
    const HwSwitch* hw,
    const folly::CIDRNetwork& ip,
    const cfg::PortFlowletConfig& cfg,
    const bool flowletEnable) {
  // SAI implementation for port flowlet config verification
  // Currently, this is a placeholder that always returns true
  // as port flowlet configuration is handled differently in SAI
  return true;
}

bool verifyEcmpForFlowletSwitching(
    const HwSwitch* hw,
    const folly::CIDRNetwork& ip,
    const cfg::FlowletSwitchingConfig& flowletCfg,
    const bool flowletEnable) {
  bool isVerified = true;

#if SAI_API_VERSION >= SAI_VERSION(1, 14, 0)
  const auto saiSwitch = static_cast<const SaiSwitch*>(hw);
  auto nextHopGroupSaiId = getNextHopGroupSaiId(saiSwitch, ip);
  auto arsId = SaiApiTable::getInstance()->nextHopGroupApi().getAttribute(
      nextHopGroupSaiId, SaiNextHopGroupTraits::Attributes::ArsObjectId());

  if (flowletEnable) {
    isVerified = (arsId != SAI_NULL_OBJECT_ID);
    if (isVerified) {
      isVerified = verifyArs(hw, static_cast<ArsSaiId>(arsId), flowletCfg);
    }
  } else {
    isVerified = (arsId == SAI_NULL_OBJECT_ID);
  }
#endif

  return isVerified;
}

bool HwTestThriftHandler::verifyEcmpForFlowletSwitchingHandler(
    std::unique_ptr<CIDRNetwork> ip,
    std::unique_ptr<::facebook::fboss::state::SwitchSettingsFields> settings,
    bool flowletEnabled) {
  // Convert CIDRNetwork to folly::CIDRNetwork
  folly::CIDRNetwork follyPrefix{
      folly::IPAddress(*ip->IPAddress()), static_cast<uint8_t>(*ip->mask())};
  // Extract flowlet configuration from settings
  auto flowletCfg = settings->flowletSwitchingConfig();
  return verifyEcmpForFlowletSwitching(
      hwSwitch_, follyPrefix, *flowletCfg, flowletEnabled);
}

bool HwTestThriftHandler::verifyPortFlowletConfig(
    std::unique_ptr<CIDRNetwork> prefix,
    std::unique_ptr<cfg::PortFlowletConfig> cfg,
    bool flowletEnable) {
  // Convert CIDRNetwork to folly::CIDRNetwork
  folly::CIDRNetwork follyPrefix{
      folly::IPAddress(*prefix->IPAddress()),
      static_cast<uint8_t>(*prefix->mask())};

  return ::facebook::fboss::utility::verifyPortFlowletConfig(
      hwSwitch_, follyPrefix, *cfg, flowletEnable);
}

bool HwTestThriftHandler::validateFlowSetTable(
    const bool expectFlowsetSizeZero) {
  // Not applicable for SAI
  return true;
}

} // namespace facebook::fboss::utility
