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
#include "fboss/agent/FbossError.h"
#include "fboss/agent/state/AggregatePort.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss::utility {

cfg::AggregatePortMember makePortMember(int32_t port, cfg::LacpPortRate rate) {
  static auto constexpr kAggPriority = 32768;
  cfg::AggregatePortMember aggMember;
  *aggMember.memberPortID() = port;
  *aggMember.priority() = kAggPriority;
  aggMember.rate() = rate;
  return aggMember;
}

void addAggPort(
    int key,
    const std::vector<int32_t>& ports,
    cfg::SwitchConfig* config,
    cfg::LacpPortRate rate,
    double minLinkPercentage,
    cfg::AggregatePortType aggregatePortType,
    std::optional<std::string> aggPortName) {
  // Create agg port with requisite members
  static constexpr auto kAggPortName = "AGG";
  cfg::AggregatePort aggPort;
  aggPort.key() = key;
  if (aggPortName.has_value()) {
    aggPort.name() = *aggPortName;
    aggPort.description() = *aggPortName;
  } else {
    aggPort.name() = folly::to<std::string>(kAggPortName, "-", key);
    aggPort.description() = kAggPortName;
  }
  aggPort.minimumCapacity()->set_linkPercentage(minLinkPercentage);
  aggPort.aggregatePortType() = aggregatePortType;
  for (auto port : ports) {
    aggPort.memberPorts()->push_back(makePortMember(port, rate));
  }
  config->aggregatePorts()->push_back(aggPort);
  // Set VLAN for all members to be the same
  std::set<uint32_t> memberPorts(ports.begin(), ports.end());
  std::optional<int32_t> aggVlan;
  for (auto& vlanPort : *config->vlanPorts()) {
    if (memberPorts.contains(vlanPort.logicalPort().value())) {
      if (!aggVlan) {
        aggVlan = vlanPort.vlanID().value();
      }
    }
  }
  // PORT RIF configs (P200/Chenab) use vlanID 0 in vlanPorts; SAI LAG creation
  // requires a real VLAN (e.g. default VLAN 4094 on P200). Fall back to the
  // switch default when members have no explicit VLAN membership.
  if (!aggVlan || *aggVlan == 0) {
    if (config->defaultVlan().has_value() && *config->defaultVlan() != 0) {
      aggVlan = *config->defaultVlan();
    }
  }
  if (!aggVlan || *aggVlan == 0) {
    throw FbossError("No VLAN found for aggregate port in addAggPort");
  }
  for (auto& vlanPort : *config->vlanPorts()) {
    if (memberPorts.contains(vlanPort.logicalPort().value())) {
      vlanPort.vlanID() = *aggVlan;
    }
  }

  if (aggregatePortType == cfg::AggregatePortType::LAG_PORT) {
    // Set ingress VLAN for all members to be the same
    for (auto& port : *config->ports()) {
      if (memberPorts.contains(port.logicalID().value())) {
        port.ingressVlan() = *aggVlan;
      }
    }
  }
}

std::shared_ptr<SwitchState> enableTrunkPorts(
    std::shared_ptr<SwitchState> curState) {
  auto newState{curState};
  for (const auto& [_, aggPorts] :
       std::as_const(*newState->getAggregatePorts())) {
    for (const auto& idAndAggPort : std::as_const(*aggPorts)) {
      auto aggPort = idAndAggPort.second->modify(&newState);
      for (auto subPort : aggPort->sortedSubports()) {
        aggPort->setForwardingState(
            subPort.portID, AggregatePort::Forwarding::ENABLED);
      }
    }
  }
  return newState;
}

std::shared_ptr<SwitchState> disableTrunkPorts(
    std::shared_ptr<SwitchState> curState) {
  auto newState{curState};
  for (const auto& [_, aggPorts] :
       std::as_const(*newState->getAggregatePorts())) {
    for (const auto& idAndAggPort : std::as_const(*aggPorts)) {
      auto aggPort = idAndAggPort.second->modify(&newState);
      for (auto subPort : aggPort->sortedSubports()) {
        aggPort->setForwardingState(
            subPort.portID, AggregatePort::Forwarding::DISABLED);
      }
    }
  }
  return newState;
}

std::shared_ptr<SwitchState> setTrunkMinLinkCount(
    std::shared_ptr<SwitchState> curState,
    uint8_t minlinks) {
  auto newState{curState};
  for (const auto& [_, aggPorts] :
       std::as_const(*newState->getAggregatePorts())) {
    for (const auto& idAndAggPort : std::as_const(*aggPorts)) {
      auto aggPort = idAndAggPort.second->modify(&newState);
      aggPort->setMinimumLinkCount(minlinks);
    }
  }
  return newState;
}

std::shared_ptr<SwitchState> disableTrunkPort(
    std::shared_ptr<SwitchState> curState,
    const AggregatePortID& aggId,
    const facebook::fboss::PortID& portId) {
  auto newState{curState};
  auto aggPortOld = newState->getAggregatePorts()->getNodeIf(aggId);
  auto aggPort = aggPortOld->modify(&newState);
  aggPort->setForwardingState(portId, AggregatePort::Forwarding::DISABLED);
  return newState;
}

} // namespace facebook::fboss::utility
