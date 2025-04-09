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
    const std::shared_ptr<SwitchState>& state,
    const SwitchIdScopeResolver* resolver,
    const HwAsicTable* asicTable) {
  auto vlanID = getVlanIDFromVlanOrIntf<VlanOrIntfT>(vlanOrIntf);
  if (vlanID.has_value() ||
      !asicTable->isFeatureSupportedOnAnyAsic(
          HwAsic::Feature::CPU_TX_PACKET_REQUIRES_VLAN_TAG)) {
    return vlanID;
  }
  // TODO: handle case with multiple switches
  CHECK(!resolver->hasMultipleSwitches());
  CHECK(asicTable->isFeatureSupportedOnAllAsic(
      HwAsic::Feature::CPU_TX_PACKET_REQUIRES_VLAN_TAG));

  static std::optional<VlanID> kVlanID{};
  if (!kVlanID.has_value()) {
    // cache default vid in static variable so only first tx requires lookup.
    // dynamic change of default vid will break this
    auto scope = HwSwitchMatcher(asicTable->getSwitchIDs(
        HwAsic::Feature::CPU_TX_PACKET_REQUIRES_VLAN_TAG));
    auto settings =
        state->getSwitchSettings()->getNodeIf(scope.matcherString());
    kVlanID = getDefaultTxVlanIdIf(settings);
  }
  vlanID = kVlanID;
  CHECK(vlanID.has_value())
      << "vlan id required for tx could not be determined";
  return vlanID;
}

} // namespace facebook::fboss::utility
