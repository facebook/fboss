// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/HwAsicTable.h"
#include "fboss/agent/SwitchIdScopeResolver.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/state/SwitchState.h"

#include <optional>

namespace facebook::fboss::utility {

template <typename VlanOrIntfT>
std::optional<VlanID> getVlanIDForTx(
    const std::shared_ptr<VlanOrIntfT>& vlanOrIntf,
    const std::shared_ptr<SwitchSettings>& settings,
    const HwAsic* hwAsic) {
  auto vlanID = getVlanIDFromVlanOrIntf<VlanOrIntfT>(vlanOrIntf);
  if (vlanID.has_value() ||
      !hwAsic->isSupported(HwAsic::Feature::CPU_TX_PACKET_REQUIRES_VLAN_TAG)) {
    return vlanID;
  }

  static std::optional<VlanID> kVlanID{};
  if (!kVlanID.has_value()) {
    // cache default vid in static variable so only first tx requires lookup.
    // dynamic change of default vid will break this
    kVlanID = getDefaultTxVlanIdIf(settings);
  }
  return kVlanID;
}

template <typename VlanOrIntfT>
std::optional<VlanID> getVlanIDForTx(
    const std::shared_ptr<VlanOrIntfT>& vlanOrIntf,
    const std::shared_ptr<SwitchState>& state,
    const SwitchIdScopeResolver* resolver,
    const HwAsicTable* asicTable) {
  if (!asicTable->isFeatureSupportedOnAnyAsic(
          HwAsic::Feature::CPU_TX_PACKET_REQUIRES_VLAN_TAG)) {
    return getVlanIDFromVlanOrIntf<VlanOrIntfT>(vlanOrIntf);
  }
  // TODO: handle case with multiple switches
  CHECK(!resolver->hasMultipleSwitches());
  CHECK(asicTable->isFeatureSupportedOnAllAsic(
      HwAsic::Feature::CPU_TX_PACKET_REQUIRES_VLAN_TAG));

  auto switchIds =
      asicTable->getSwitchIDs(HwAsic::Feature::CPU_TX_PACKET_REQUIRES_VLAN_TAG);
  auto asic = asicTable->getHwAsicIf(*switchIds.begin());
  CHECK(asic);
  auto scope = HwSwitchMatcher(switchIds);
  auto settings = state->getSwitchSettings()->getNodeIf(scope.matcherString());
  auto vlanID = getVlanIDForTx(vlanOrIntf, settings, asic);
  CHECK(vlanID.has_value())
      << "vlan id required for tx could not be determined";
  return vlanID;
}

} // namespace facebook::fboss::utility
