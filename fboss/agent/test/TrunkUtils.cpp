/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/test/TrunkUtils.h"

#include "fboss/agent/ApplyThriftConfig.h"
#include "fboss/agent/state/AggregatePort.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss::utility {

cfg::AggregatePortMember makePortMember(int32_t port) {
  static auto constexpr kAggPriority = 32768;
  cfg::AggregatePortMember aggMember;
  *aggMember.memberPortID_ref() = port;
  *aggMember.priority_ref() = kAggPriority;
  return aggMember;
}

void addAggPort(
    int key,
    const std::vector<int32_t>& ports,
    cfg::SwitchConfig* config) {
  // Create agg port with requisite members
  static constexpr auto kAggPortName = "AGG";
  cfg::AggregatePort aggPort;
  *aggPort.key_ref() = key;
  *aggPort.name_ref() = kAggPortName;
  *aggPort.description_ref() = kAggPortName;
  for (auto port : ports) {
    aggPort.memberPorts_ref()->push_back(makePortMember(port));
  }
  config->aggregatePorts_ref()->push_back(aggPort);
  // Set VLAN for all members to be the same
  std::set<uint32_t> memberPorts(ports.begin(), ports.end());
  std::optional<int32_t> aggVlan;
  for (auto& vlanPort : *config->vlanPorts_ref()) {
    if (memberPorts.find(*vlanPort.logicalPort_ref()) != memberPorts.end()) {
      if (!aggVlan) {
        aggVlan = *vlanPort.vlanID_ref();
      }
      *vlanPort.vlanID_ref() = aggVlan.value();
    }
  }
  // Set ingress VLAN for all members to be the same
  for (auto& port : *config->ports_ref()) {
    if (memberPorts.find(*port.logicalID_ref()) != memberPorts.end()) {
      *port.ingressVlan_ref() = aggVlan.value();
    }
  }
}

std::shared_ptr<SwitchState> enableTrunkPorts(
    std::shared_ptr<SwitchState> curState) {
  auto newState{curState};
  for (auto aggPortOld : *newState->getAggregatePorts()) {
    auto aggPort = aggPortOld->modify(&newState);
    for (auto subPort : aggPort->sortedSubports()) {
      aggPort->setForwardingState(
          subPort.portID, AggregatePort::Forwarding::ENABLED);
    }
  }
  return newState;
}

std::shared_ptr<SwitchState> setTrunkMinLinkCount(
    std::shared_ptr<SwitchState> curState,
    uint8_t minlinks) {
  auto newState{curState};
  for (auto aggPortOld : *newState->getAggregatePorts()) {
    auto aggPort = aggPortOld->modify(&newState);
    aggPort->setMinimumLinkCount(minlinks);
  }
  return newState;
}

std::shared_ptr<SwitchState> disableTrunkPort(
    std::shared_ptr<SwitchState> curState,
    const AggregatePortID& aggId,
    const facebook::fboss::PortID& portId) {
  auto newState{curState};
  auto aggPortOld = newState->getAggregatePorts()->getAggregatePortIf(aggId);
  auto aggPort = aggPortOld->modify(&newState);
  aggPort->setForwardingState(portId, AggregatePort::Forwarding::DISABLED);
  return newState;
}

} // namespace facebook::fboss::utility
