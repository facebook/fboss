// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/TestEnsembleIf.h"

#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/SwitchState.h"

#include <folly/gen/Base.h>

namespace facebook::fboss {

std::vector<PortID> TestEnsembleIf::masterLogicalPortIds(
    const std::set<cfg::PortType>& filter) const {
  auto portIDs = masterLogicalPortIds();
  std::vector<PortID> filteredPortIDs;

  folly::gen::from(portIDs) |
      folly::gen::filter([this, filter](const auto& portID) {
        if (filter.empty()) {
          // if no filter is requested, allow all
          return true;
        }
        auto portType =
            getProgrammedState()->getPorts()->getPort(portID)->getPortType();

        return filter.find(portType) != filter.end();
      }) |
      folly::gen::appendTo(filteredPortIDs);

  return filteredPortIDs;
}
} // namespace facebook::fboss
