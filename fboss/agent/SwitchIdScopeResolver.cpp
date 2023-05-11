// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/SwitchIdScopeResolver.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/SystemPort.h"
#include "fboss/agent/state/Vlan.h"

namespace facebook::fboss {

SwitchIdScopeResolver::SwitchIdScopeResolver(
    const std::map<int64_t, cfg::SwitchInfo>& switchIdToSwitchInfo)
    : switchIdToSwitchInfo_(switchIdToSwitchInfo) {
  auto voqSwitchIds = getSwitchIdsOfType(cfg::SwitchType::VOQ);
  auto npuSwitchIds = getSwitchIdsOfType(cfg::SwitchType::NPU);
  CHECK(voqSwitchIds.empty() || npuSwitchIds.empty())
      << " Only one of "
         "voq, npu switch types can be present in a chassis";
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

const HwSwitchMatcher& SwitchIdScopeResolver::l3SwitchMatcher() const {
  CHECK(l3SwitchMatcher_)
      << " One or more l3 switchIds must be set to get l3 scope";
  return *l3SwitchMatcher_;
}

const HwSwitchMatcher& SwitchIdScopeResolver::allSwitchMatcher() const {
  CHECK(allSwitchMatcher_)
      << " One or more all switchIds must be set to get allSwitch scope";
  return *allSwitchMatcher_;
}

const HwSwitchMatcher& SwitchIdScopeResolver::voqSwitchMatcher() const {
  CHECK(voqSwitchMatcher_)
      << " One or more voq switchIds must be set to get voq scope";
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
    const cfg::AggregatePort& aggPort) const {
  std::unordered_set<SwitchID> switchIds;
  for (const auto& subport : *aggPort.memberPorts()) {
    auto subPortSwitchIds = scope(PortID(*subport.memberPortID())).switchIds();
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
  std::unordered_set<SwitchID> switchIds;
  for (const auto& port : vlan->getPorts()) {
    auto portSwitchIds = scope(PortID(port.first)).switchIds();
    switchIds.insert(portSwitchIds.begin(), portSwitchIds.end());
  }
  return HwSwitchMatcher(switchIds);
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
} // namespace facebook::fboss
