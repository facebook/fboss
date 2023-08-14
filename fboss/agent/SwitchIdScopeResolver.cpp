// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/SwitchIdScopeResolver.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/state/AclTableGroup.h"
#include "fboss/agent/state/AggregatePort.h"
#include "fboss/agent/state/ForwardingInformationBaseMap.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/LabelForwardingEntry.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/PortDescriptor.h"
#include "fboss/agent/state/SflowCollector.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/SystemPort.h"
#include "fboss/agent/state/Vlan.h"

namespace facebook::fboss {

SwitchIdScopeResolver::SwitchIdScopeResolver(
    const std::map<int64_t, cfg::SwitchInfo>& switchIdToSwitchInfo)
    : switchIdToSwitchInfo_(switchIdToSwitchInfo) {
  auto voqSwitchIds = getSwitchIdsOfType(cfg::SwitchType::VOQ);
  auto npuSwitchIds = getSwitchIdsOfType(cfg::SwitchType::NPU);
  if (voqSwitchIds.size() && npuSwitchIds.size()) {
    throw FbossError(
        " Only one of "
        "voq, npu switch types can be present in a chassis");
  }
  if (voqSwitchIds.size() || npuSwitchIds.size()) {
    l3SwitchMatcher_ = std::make_unique<HwSwitchMatcher>(
        voqSwitchIds.size() ? voqSwitchIds : npuSwitchIds);
  }
  if (voqSwitchIds.size()) {
    voqSwitchMatcher_ = std::make_unique<HwSwitchMatcher>(voqSwitchIds);
  }
  std::unordered_set<SwitchID> allSwitchIds;
  for (const auto& switchIdAndInfo : switchIdToSwitchInfo_) {
    allSwitchIds.insert(SwitchID(switchIdAndInfo.first));
  }
  if (allSwitchIds.size()) {
    allSwitchMatcher_ = std::make_unique<HwSwitchMatcher>(allSwitchIds);
  }
}

std::unordered_set<SwitchID> SwitchIdScopeResolver::getSwitchIdsOfType(
    cfg::SwitchType type) const {
  std::unordered_set<SwitchID> ids;
  for (const auto& switchIdAndInfo : switchIdToSwitchInfo_) {
    if (switchIdAndInfo.second.switchType() == type) {
      ids.insert(SwitchID(switchIdAndInfo.first));
    }
  }
  return ids;
}

void SwitchIdScopeResolver::checkL3() const {
  if (!l3SwitchMatcher_) {
    throw FbossError(" One or more l3 switchIds must be set to get l3 scope");
  }
}
const HwSwitchMatcher& SwitchIdScopeResolver::l3SwitchMatcher() const {
  checkL3();
  return *l3SwitchMatcher_;
}

const HwSwitchMatcher& SwitchIdScopeResolver::allSwitchMatcher() const {
  if (!allSwitchMatcher_) {
    throw FbossError(
        "One or more all switchIds must be set to get allSwitch scope");
  }
  return *allSwitchMatcher_;
}

void SwitchIdScopeResolver::checkVoq() const {
  if (!voqSwitchMatcher_) {
    throw FbossError(" One or more voq switchIds must be set to get voq scope");
  }
}
const HwSwitchMatcher& SwitchIdScopeResolver::voqSwitchMatcher() const {
  checkVoq();
  return *voqSwitchMatcher_;
}

HwSwitchMatcher SwitchIdScopeResolver::scope(PortID portId) const {
  for (const auto& switchIdAndSwitchInfo : switchIdToSwitchInfo_) {
    if (portId >=
            PortID(*switchIdAndSwitchInfo.second.portIdRange()->minimum()) &&
        portId <=
            PortID(*switchIdAndSwitchInfo.second.portIdRange()->maximum())) {
      return HwSwitchMatcher(std::unordered_set<SwitchID>(
          {SwitchID(switchIdAndSwitchInfo.first)}));
    }
  }
  throw FbossError("No switch found for port ", portId);
}

HwSwitchMatcher SwitchIdScopeResolver::scope(
    const std::vector<PortID>& portIds) const {
  std::unordered_set<SwitchID> switchIds;
  for (const auto& portId : portIds) {
    auto portSwitchIds = scope(portId).switchIds();
    switchIds.insert(portSwitchIds.begin(), portSwitchIds.end());
  }
  return HwSwitchMatcher(switchIds);
}

HwSwitchMatcher SwitchIdScopeResolver::scope(
    const std::shared_ptr<Port>& port) const {
  return scope(port->getID());
}

HwSwitchMatcher SwitchIdScopeResolver::scope(const cfg::Port& port) const {
  return scope(PortID(*port.logicalID()));
}

HwSwitchMatcher SwitchIdScopeResolver::scope(
    const cfg::AggregatePort& aggPort) const {
  checkL3();
  std::unordered_set<SwitchID> switchIds;
  for (const auto& subport : *aggPort.memberPorts()) {
    auto subPortSwitchIds = scope(PortID(*subport.memberPortID())).switchIds();
    switchIds.insert(subPortSwitchIds.begin(), subPortSwitchIds.end());
  }
  return HwSwitchMatcher(switchIds);
}

HwSwitchMatcher SwitchIdScopeResolver::scope(
    const std::shared_ptr<AggregatePort>& aggPort) const {
  checkL3();
  std::unordered_set<SwitchID> switchIds;
  for (const auto& subport : aggPort->sortedSubports()) {
    auto portId = subport.portID;
    auto subPortSwitchIds = scope(portId).switchIds();
    switchIds.insert(subPortSwitchIds.begin(), subPortSwitchIds.end());
  }
  return HwSwitchMatcher(switchIds);
}

HwSwitchMatcher SwitchIdScopeResolver::scope(SystemPortID sysPortId) const {
  auto sysPortInt = static_cast<int64_t>(sysPortId);
  for (const auto& [id, info] : switchIdToSwitchInfo_) {
    if (!info.systemPortRange().has_value()) {
      continue;
    }
    if (sysPortInt >= *info.systemPortRange()->minimum() &&
        sysPortInt <= *info.systemPortRange()->maximum()) {
      return HwSwitchMatcher(std::unordered_set<SwitchID>({SwitchID(id)}));
    }
  }

  // This is a non local sys port. So it maps to all local voq switchIds
  return voqSwitchMatcher();
}

HwSwitchMatcher SwitchIdScopeResolver::scope(
    const std::shared_ptr<SystemPort>& sysPort) const {
  return scope(sysPort->getID());
}

const HwSwitchMatcher SwitchIdScopeResolver::scope(
    const std::shared_ptr<Vlan>& vlan) const {
  // TODO - restrict vlan scope to L3 switches
  // Currently we create psuedo vlans on fabric switches
  if (vlan->getPorts().empty()) {
    // VLANs corresponding to loopback intfs have no ports
    // associated with them. Also Psuedo vlans created
    // on fabric switches don't have ports associated with them.
    return *allSwitchMatcher_;
  }
  std::unordered_set<SwitchID> switchIds;
  for (const auto& port : vlan->getPorts()) {
    auto portSwitchIds = scope(PortID(port.first)).switchIds();
    switchIds.insert(portSwitchIds.begin(), portSwitchIds.end());
  }
  return HwSwitchMatcher(switchIds);
}

HwSwitchMatcher SwitchIdScopeResolver::scope(
    const std::shared_ptr<AclTableGroup>& /*aclTableGroup*/) const {
  return l3SwitchMatcher();
}

HwSwitchMatcher SwitchIdScopeResolver::scope(
    const cfg::AclTableGroup& /*aclTableGroup*/) const {
  return l3SwitchMatcher();
}

HwSwitchMatcher SwitchIdScopeResolver::scope(
    const std::shared_ptr<Interface>& intf,
    const std::shared_ptr<SwitchState>& state) const {
  switch (intf->getType()) {
    case cfg::InterfaceType::SYSTEM_PORT:
      return scope(SystemPortID(static_cast<int64_t>(intf->getID())));
    case cfg::InterfaceType::VLAN:
      return scope(
          state->getVlans()->getNode(VlanID(static_cast<int>(intf->getID()))));
  }
  throw FbossError(
      "Unexpected interface type: ", static_cast<int>(intf->getType()));
}

HwSwitchMatcher SwitchIdScopeResolver::scope(
    const std::shared_ptr<Interface>& intf,
    const cfg::SwitchConfig& cfg) const {
  return scope(intf->getType(), intf->getID(), cfg);
}

HwSwitchMatcher SwitchIdScopeResolver::scope(
    const cfg::InterfaceType& type,
    const InterfaceID& interfaceId,
    const cfg::SwitchConfig& cfg) const {
  switch (type) {
    case cfg::InterfaceType::SYSTEM_PORT:
      return scope(SystemPortID(static_cast<int64_t>(interfaceId)));
    case cfg::InterfaceType::VLAN: {
      int vlanId(static_cast<int>(interfaceId));
      auto vitr = std::find_if(
          cfg.vlans()->cbegin(),
          cfg.vlans()->cend(),
          [vlanId](const auto& vlan) { return vlan.id() == vlanId; });
      if (vitr == cfg.vlans()->cend()) {
        throw FbossError("No vlan found for : ", vlanId);
      }
      Vlan::MemberPorts vlanMembers;
      for (const auto& vlanPort : *cfg.vlanPorts()) {
        if (vlanPort.vlanID() == vlanId) {
          vlanMembers.emplace(std::make_pair(*vlanPort.logicalPort(), true));
        }
      }
      return scope(std::make_shared<Vlan>(&*vitr, vlanMembers));
    }
  }
  throw FbossError("Unexpected interface type: ", static_cast<int>(type));
}

HwSwitchMatcher SwitchIdScopeResolver::scope(
    const std::shared_ptr<ForwardingInformationBaseContainer>& /*fibs*/) const {
  return l3SwitchMatcher();
}

HwSwitchMatcher SwitchIdScopeResolver::scope(
    const std::shared_ptr<LabelForwardingEntry>& /*entry*/) const {
  return l3SwitchMatcher();
}

const HwSwitchMatcher& SwitchIdScopeResolver::scope(
    const std::shared_ptr<SflowCollector>& /*entry*/) const {
  return l3SwitchMatcher();
}

const HwSwitchMatcher& SwitchIdScopeResolver::scope(
    const std::shared_ptr<SwitchSettings>& /*s*/) const {
  throw FbossError("Scope is per HwSwitch, implemented during config apply");
}

const HwSwitchMatcher& SwitchIdScopeResolver::scope(
    const cfg::SflowCollector& /*entry*/) const {
  return l3SwitchMatcher();
}

HwSwitchMatcher SwitchIdScopeResolver::scope(
    const std::shared_ptr<SwitchState>& state,
    const boost::container::flat_set<PortDescriptor>& ports) const {
  std::unordered_set<SwitchID> switchIds;

  for (auto port : ports) {
    auto matcher = scope(state, port);
    for (auto switchId : matcher.switchIds()) {
      switchIds.insert(switchId);
    }
  }
  return HwSwitchMatcher(switchIds);
}

HwSwitchMatcher SwitchIdScopeResolver::scope(
    const std::shared_ptr<SwitchState>& state,
    const PortDescriptor& portDesc) const {
  switch (portDesc.type()) {
    case PortDescriptor::PortType::PHYSICAL:
      return scope(state, portDesc.phyPortID());

    case PortDescriptor::PortType::AGGREGATE:
      return scope(state, portDesc.aggPortID());

    case PortDescriptor::PortType::SYSTEM_PORT:
      return scope(portDesc.sysPortID());
  }
  throw FbossError("unknown port type");
}

HwSwitchMatcher SwitchIdScopeResolver::scope(
    const std::shared_ptr<SwitchState>& state,
    const PortID& portId) const {
  auto port = state->getPorts()->getNode(portId);
  return scope(port);
}

HwSwitchMatcher SwitchIdScopeResolver::scope(
    const std::shared_ptr<SwitchState>& state,
    const AggregatePortID& aggPortId) const {
  auto aggPport = state->getAggregatePorts()->getNode(aggPortId);
  return scope(aggPport);
}

} // namespace facebook::fboss
