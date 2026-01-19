/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/test/utils/HyperPortTestUtils.h"
#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/state/AggregatePort.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss::utility {

std::vector<PortID> getHyperPortMembers(
    const std::shared_ptr<SwitchState>& state,
    PortID port) {
  if (!FLAGS_hyper_port) {
    return {};
  }
  auto aggPort = state->getAggregatePorts()->getNodeIf(
      AggregatePortID(static_cast<uint32_t>(port)));

  if (!aggPort ||
      aggPort->getAggregatePortType() != cfg::AggregatePortType::HYPER_PORT) {
    return {};
  }
  auto subPorts = aggPort->sortedSubports();
  std::vector<PortID> memberIds;
  std::for_each(
      subPorts.begin(), subPorts.end(), [&memberIds](const auto& subPort) {
        memberIds.push_back(subPort.portID);
      });
  return memberIds;
}

} // namespace facebook::fboss::utility
