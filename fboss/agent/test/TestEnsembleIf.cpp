// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/TestEnsembleIf.h"

#include "fboss/agent/Platform.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/SwitchState.h"

#include <folly/gen/Base.h>

namespace facebook::fboss {

std::vector<PortID> TestEnsembleIf::masterLogicalPortIdsImpl(
    const std::set<cfg::PortType>& portTypesFilter,
    const std::set<SwitchID>& switchIdFilter) const {
  auto portIDs = masterLogicalPortIds();
  std::vector<PortID> filteredPortIDs;
  auto platformPorts = getPlatformPorts();

  folly::gen::from(portIDs) |
      folly::gen::filter(
          [this, &platformPorts, &portTypesFilter, &switchIdFilter](
              PortID portID) {
            if (portTypesFilter.empty() && switchIdFilter.empty()) {
              // if no filter is requested, allow all
              return true;
            }
            if (portTypesFilter.size()) {
              auto portItr = platformPorts.find(static_cast<int>(portID));
              if (portItr == platformPorts.end()) {
                return false;
              }
              auto portType = *portItr->second.mapping()->portType();

              if (portTypesFilter.find(portType) == portTypesFilter.end()) {
                return false;
              }
            }
            if (switchIdFilter.size()) {
              auto portSwitchId = scopeResolver().scope(portID).switchId();
              if (switchIdFilter.find(portSwitchId) == switchIdFilter.end()) {
                return false;
              }
            }
            return true;
          }) |
      folly::gen::appendTo(filteredPortIDs);

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
    auto sysPortRange = getProgrammedState()
                            ->getDsfNodes()
                            ->getNodeIf(SwitchID(*switchId))
                            ->getSystemPortRange();
    CHECK(sysPortRange.has_value());
    for (auto port : masterLogicalPortIds({cfg::PortType::INTERFACE_PORT})) {
      if (scopeResolver().scope(port).switchId() == SwitchID(*switchId)) {
        sysPorts.push_back(
            SystemPortID(*sysPortRange->minimum() + static_cast<int>(port)));
      }
    }
  }
  return sysPorts;
}
} // namespace facebook::fboss
