// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/TestEnsembleIf.h"

#include "fboss/agent/TxPacket.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/state/SwitchState.h"

#include <folly/gen/Base.h>

namespace facebook::fboss {

void TestEnsembleIf::sendPacketSwitchedAsync(
    std::unique_ptr<TxPacket> pkt,
    const std::optional<SwitchID>& /*switchId*/) {
  // Default: single-switch ensembles send switched without switch targeting.
  sendPacketAsync(std::move(pkt));
}

std::vector<PortID> TestEnsembleIf::masterLogicalPortIdsImpl(
    const std::set<cfg::PortType>& portTypesFilter,
    const std::set<SwitchID>& switchIdFilter) const {
  auto portIDs = getAllMasterLogicalPortIds();
  // Bind by const-ref: getPlatformPorts() returns a reference to the platform
  // mapping's stable port map. Copying it here is both wasteful and unsafe at
  // scale -- this runs on every masterLogicalPortIds() call, including hot
  // paths like getSwitchIdUnderTest() invoked per packet.
  const auto& platformPorts = getPlatformPorts();
  std::vector<PortID> filteredPortIDs;

  // The raw list from getAllMasterLogicalPortIds() stays full; every
  // masterLogicalPortIds(...) view is computed (filtered + capped) here, so the
  // cap is honored uniformly across all overloads. This is what trims the ports
  // a test (and its initial config) uses, replacing init-time pruning of the
  // master list.
  std::map<std::pair<SwitchID, cfg::PortType>, size_t> keptBySwitchAndType;
  for (auto portID : portIDs) {
    auto portItr = platformPorts.find(static_cast<int>(portID));
    std::optional<cfg::PortType> portType;
    if (portItr != platformPorts.end()) {
      portType = *portItr->second.mapping()->portType();
    }

    // Port-type filter (empty filter == all types).
    if (!portTypesFilter.empty() &&
        (!portType.has_value() || !portTypesFilter.contains(*portType))) {
      continue;
    }

    auto portSwitchId = scopeResolver().scope(portID).switchId();
    // Switch filter (empty filter == all switches).
    if (!switchIdFilter.empty() && !switchIdFilter.contains(portSwitchId)) {
      continue;
    }

    // Per-switch per-type cap, applied uniformly to every view.
    if (portType.has_value()) {
      if (auto cap = getMaxRequiredPorts(*portType)) {
        auto& kept = keptBySwitchAndType[{portSwitchId, *portType}];
        if (kept >= *cap) {
          continue;
        }
        ++kept;
      }
    }

    filteredPortIDs.push_back(portID);
  }

  return filteredPortIDs;
}

std::vector<const HwAsic*> TestEnsembleIf::getL3Asics() const {
  auto l3Asics = getHwAsicTable()->getL3Asics();
  CHECK(!l3Asics.empty()) << " No l3 asics found";
  return l3Asics;
}

int TestEnsembleIf::getNumL3Asics() const {
  return getHwAsicTable()->getL3Asics().size();
}

std::vector<SystemPortID> TestEnsembleIf::masterLogicalSysPortIds() const {
  std::vector<SystemPortID> sysPorts;
  for (const auto& asic : getHwAsicTable()->getL3Asics()) {
    if (asic->getSwitchType() != cfg::SwitchType::VOQ) {
      continue;
    }
    auto switchId = asic->getSwitchId();
    CHECK(switchId.has_value());
    for (auto port : masterLogicalPortIds({cfg::PortType::INTERFACE_PORT})) {
      if (scopeResolver().scope(port).switchId() == SwitchID(*switchId)) {
        sysPorts.push_back(getSystemPortID(
            PortID(port), getProgrammedState(), SwitchID(*switchId)));
      }
    }
  }
  return sysPorts;
}
} // namespace facebook::fboss
