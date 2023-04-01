// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/TestEnsembleIf.h"

#include "fboss/agent/Platform.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/SwitchState.h"

#include <folly/gen/Base.h>

namespace facebook::fboss {

std::vector<PortID> TestEnsembleIf::masterLogicalPortIds(
    const std::set<cfg::PortType>& filter) const {
  auto portIDs = masterLogicalPortIds();
  std::vector<PortID> filteredPortIDs;
  auto platformPorts = getHwSwitch()->getPlatform()->getPlatformPorts();

  folly::gen::from(portIDs) |
      folly::gen::filter([&platformPorts, filter](const auto& portID) {
        if (filter.empty()) {
          // if no filter is requested, allow all
          return true;
        }
        auto portItr = platformPorts.find(static_cast<int>(portID));
        if (portItr == platformPorts.end()) {
          return false;
        }
        auto portType = *portItr->second.mapping()->portType();

        return filter.find(portType) != filter.end();
      }) |
      folly::gen::appendTo(filteredPortIDs);

  return filteredPortIDs;
}
} // namespace facebook::fboss
