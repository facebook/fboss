/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/lib/config/AgentConfigUtils.h"

#include <algorithm>
#include <set>
#include <string>

#include "fboss/agent/FbossError.h"
#include "fboss/agent/platforms/common/PlatformMapping.h"

namespace facebook::fboss::utility {

cfg::Port createDefaultPortConfig(
    const PlatformMapping* platformMapping,
    PortID id,
    cfg::PortProfileID profileID,
    int32_t ingressVlan) {
  cfg::Port port;
  const auto& mapping = *platformMapping->getPlatformPort(id).mapping();
  port.name() = *mapping.name();
  port.portType() = *mapping.portType();
  port.scope() = *mapping.scope();
  port.logicalID() = id;
  port.profileID() = profileID;
  port.state() = cfg::PortState::DISABLED;
  port.ingressVlan() = ingressVlan;

  const auto profileConfig = platformMapping->getPortProfileConfig(
      PlatformPortProfileConfigMatcher(profileID, id));
  port.speed() = profileConfig.has_value() ? *profileConfig->speed()
                                           : cfg::PortSpeed::DEFAULT;
  return port;
}

int32_t allocateFreeVlanId(
    const cfg::SwitchConfig& config,
    int32_t minId,
    int32_t maxId) {
  std::set<int32_t> usedIds;
  for (const auto& vlan : *config.vlans()) {
    usedIds.insert(*vlan.id());
  }
  for (const auto& intf : *config.interfaces()) {
    usedIds.insert(*intf.intfID());
  }
  for (int32_t candidate = minId; candidate <= maxId; ++candidate) {
    if (usedIds.find(candidate) == usedIds.end()) {
      return candidate;
    }
  }
  throw FbossError(
      "No free vlan id available in range [", minId, ", ", maxId, "]");
}

int32_t addInterfacePortToConfig(
    cfg::SwitchConfig& config,
    const PlatformMapping* platformMapping,
    PortID id,
    cfg::PortProfileID profileID) {
  const int32_t n = allocateFreeVlanId(config);

  auto port = createDefaultPortConfig(platformMapping, id, profileID, n);
  port.routable() = true;
  config.ports()->push_back(port);

  cfg::Vlan vlan;
  vlan.id() = n;
  vlan.name() = "vlan" + std::to_string(n);
  vlan.routable() = true;
  vlan.recordStats() = true;
  config.vlans()->push_back(vlan);

  cfg::VlanPort vlanPort;
  vlanPort.vlanID() = n;
  vlanPort.logicalPort() = static_cast<int32_t>(id);
  vlanPort.spanningTreeState() = cfg::SpanningTreeState::FORWARDING;
  vlanPort.emitTags() = false;
  config.vlanPorts()->push_back(vlanPort);

  cfg::Interface intf;
  intf.intfID() = n;
  intf.vlanID() = n;
  intf.type() = cfg::InterfaceType::VLAN;
  intf.routerID() = 0;
  intf.scope() = cfg::Scope::LOCAL;
  config.interfaces()->push_back(intf);

  return n;
}

void removePortsFromConfig(
    cfg::SwitchConfig& config,
    const std::set<PortID>& portsToRemove,
    PortRemovalMode mode,
    bool pruneEmptyVlansAndInterfaces) {
  // Phases:
  //  1. Disable mode: just mark the listed ports DISABLED and return.
  //  2. Erase mode: drop the listed ports and their vlanPorts, remembering
  //     which vlans lost a member.
  //  3. If pruneEmptyVlansAndInterfaces: drop each of those vlans that no
  //     surviving vlanPort still uses (except the default vlan), plus the VLAN
  //     interface on each.
  if (portsToRemove.empty()) {
    return;
  }

  // Disable mode: leave the port entries (and their vlanPorts/vlans/interfaces)
  // in place, just mark those ports DISABLED.
  if (mode == PortRemovalMode::Disable) {
    for (auto& p : *config.ports()) {
      if (portsToRemove.count(PortID(*p.logicalID())) > 0) {
        p.state() = cfg::PortState::DISABLED;
      }
    }
    return;
  }

  // Erase mode. Drop the ports.
  auto& ports = *config.ports();
  ports.erase(
      std::remove_if(
          ports.begin(),
          ports.end(),
          [&](const cfg::Port& p) {
            return portsToRemove.count(PortID(*p.logicalID())) > 0;
          }),
      ports.end());

  // Drop the removed ports' vlanPorts, remembering the vlans they were members
  // of -- those are the only vlans that could have become empty.
  std::set<int32_t> maybeEmptyVlans;
  auto& vlanPorts = *config.vlanPorts();
  vlanPorts.erase(
      std::remove_if(
          vlanPorts.begin(),
          vlanPorts.end(),
          [&](const cfg::VlanPort& vp) {
            if (portsToRemove.count(PortID(*vp.logicalPort())) > 0) {
              maybeEmptyVlans.insert(*vp.vlanID());
              return true;
            }
            return false;
          }),
      vlanPorts.end());

  if (!pruneEmptyVlansAndInterfaces) {
    return;
  }

  // A candidate vlan is dropped only if no surviving vlanPort still references
  // it and it is not the default vlan. This leaves originally-empty vlans
  // (never candidates) and vlans a surviving tagged/multi-vlan member still
  // uses, so no vlanPort is ever left pointing at a dropped vlan.
  std::set<int32_t> stillUsedVlans;
  for (const auto& vp : vlanPorts) {
    stillUsedVlans.insert(*vp.vlanID());
  }
  std::set<int32_t> vlansToRemove;
  for (const auto vlanId : maybeEmptyVlans) {
    if (vlanId != *config.defaultVlan() && stillUsedVlans.count(vlanId) == 0) {
      vlansToRemove.insert(vlanId);
    }
  }

  auto& vlans = *config.vlans();
  vlans.erase(
      std::remove_if(
          vlans.begin(),
          vlans.end(),
          [&](const cfg::Vlan& v) { return vlansToRemove.count(*v.id()) > 0; }),
      vlans.end());

  // Drop the VLAN interface sitting on each removed vlan. A port-router
  // (PORT-type) interface is bound to a port via portID (its vlanID field is
  // unset, defaulting to 0), so leave those untouched for now -- this helper
  // only manages VLAN-style configs. TODO: prune port-router interfaces once
  // platform support for them is verified.
  auto& interfaces = *config.interfaces();
  interfaces.erase(
      std::remove_if(
          interfaces.begin(),
          interfaces.end(),
          [&](const cfg::Interface& i) {
            if (i.portID().has_value()) {
              return false; // port-router interface: leave in place for now
            }
            return vlansToRemove.count(*i.vlanID()) > 0;
          }),
      interfaces.end());
}

} // namespace facebook::fboss::utility
