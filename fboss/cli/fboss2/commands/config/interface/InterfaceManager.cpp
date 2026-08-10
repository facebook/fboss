/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/interface/InterfaceManager.h"

#include <folly/String.h>
#include <algorithm>
#include <cstdint>
#include <set>
#include <string>
#include <vector>
#include "fboss/agent/FbossError.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

namespace {

// Port name for error messages, falling back to the logical ID for the
// unnamed ports some configs carry.
std::string portLabel(const cfg::Port& port) {
  return port.name().has_value() ? *port.name()
                                 : std::to_string(*port.logicalID());
}

// Ids of the tunnels using `intfId` as their underlay interface.
std::vector<std::string> tunnelsOnUnderlayIntf(
    const cfg::SwitchConfig& swConfig,
    int32_t intfId) {
  std::vector<std::string> ids;
  if (swConfig.ipInIpTunnels().has_value()) {
    for (const auto& tunnel : *swConfig.ipInIpTunnels()) {
      if (*tunnel.underlayIntfID() == intfId) {
        ids.push_back(*tunnel.ipInIpTunnelId());
      }
    }
  }
  if (swConfig.srv6Tunnels().has_value()) {
    for (const auto& tunnel : *swConfig.srv6Tunnels()) {
      if (*tunnel.underlayIntfID() == intfId) {
        ids.push_back(*tunnel.srv6TunnelId());
      }
    }
  }
  return ids;
}

// Names of the enabled ports that are members of `vlanId`. Membership comes
// from vlanPorts, matching how ThriftConfigApplier builds its port -> vlan map.
std::vector<std::string> enabledMemberPorts(
    const cfg::SwitchConfig& swConfig,
    int32_t vlanId,
    const std::set<PortID>& portsBeingDeleted) {
  std::set<int32_t> memberPorts;
  for (const auto& vlanPort : *swConfig.vlanPorts()) {
    if (*vlanPort.vlanID() == vlanId) {
      memberPorts.insert(*vlanPort.logicalPort());
    }
  }

  std::vector<std::string> names;
  for (const auto& port : *swConfig.ports()) {
    // A port that is itself being deleted in the same command does not keep
    // the VLAN alive, so it must not block the interface delete.
    if (portsBeingDeleted.count(PortID(*port.logicalID())) > 0) {
      continue;
    }
    if (memberPorts.count(*port.logicalID()) > 0 &&
        *port.state() == cfg::PortState::ENABLED) {
      names.push_back(portLabel(port));
    }
  }
  return names;
}

// True when a VLAN interface other than those in `goingAway` still covers
// `vlanId`, so the VLAN keeps an interface after the delete.
bool vlanKeepsAnInterface(
    const cfg::SwitchConfig& swConfig,
    int32_t vlanId,
    const std::set<InterfaceID>& goingAway) {
  return std::any_of(
      swConfig.interfaces()->cbegin(),
      swConfig.interfaces()->cend(),
      [vlanId, &goingAway](const cfg::Interface& intf) {
        return *intf.type() == cfg::InterfaceType::VLAN &&
            *intf.vlanID() == vlanId &&
            goingAway.count(InterfaceID(*intf.intfID())) == 0;
      });
}

// Throws if removing `intf` — as part of removing all of `goingAway` — would
// dangle a reference or produce a config the agent cannot apply.
void checkDeletable(
    const cfg::SwitchConfig& swConfig,
    const cfg::Interface& intf,
    const std::set<InterfaceID>& goingAway,
    const std::set<PortID>& portsBeingDeleted) {
  const auto id = *intf.intfID();

  if (*intf.type() == cfg::InterfaceType::PORT) {
    throw FbossError(
        "Cannot delete interface ",
        id,
        ": it is the port router interface for port ",
        intf.portID().has_value() ? std::to_string(*intf.portID()) : "<unset>",
        ". Deleting it would leave that port without an interface, which the "
        "agent cannot run with. Delete the port itself instead.");
  }

  auto tunnels = tunnelsOnUnderlayIntf(swConfig, id);
  if (!tunnels.empty()) {
    throw FbossError(
        "Cannot delete interface ",
        id,
        ": it is the underlay interface for tunnel(s): ",
        folly::join(", ", tunnels),
        ". Delete the tunnel(s) first.");
  }

  if (*intf.type() != cfg::InterfaceType::VLAN) {
    return;
  }
  const auto vlanId = *intf.vlanID();
  if (vlanKeepsAnInterface(swConfig, vlanId, goingAway)) {
    return;
  }
  auto enabledPorts = enabledMemberPorts(swConfig, vlanId, portsBeingDeleted);
  if (!enabledPorts.empty()) {
    throw FbossError(
        "Cannot delete interface ",
        id,
        ": it is the only interface for VLAN ",
        vlanId,
        ", which still has enabled member port(s): ",
        folly::join(", ", enabledPorts),
        ". Disable or unbind those ports, or delete the whole VLAN with "
        "'delete vlan ",
        vlanId,
        "'.");
  }
}

} // namespace

void InterfaceManager::deleteInterfaces(
    cfg::SwitchConfig& swConfig,
    const std::set<InterfaceID>& intfIds,
    const std::set<PortID>& portsBeingDeleted) {
  auto& interfaces = *swConfig.interfaces();

  // Check everything before touching anything, so a refusal anywhere in the
  // set leaves the config exactly as it was.
  for (const auto& intfId : intfIds) {
    const auto id = static_cast<int32_t>(intfId);
    auto it = std::find_if(
        interfaces.cbegin(), interfaces.cend(), [id](const cfg::Interface& i) {
          return *i.intfID() == id;
        });
    if (it == interfaces.cend()) {
      throw FbossError("Interface ", id, " does not exist");
    }
    checkDeletable(swConfig, *it, intfIds, portsBeingDeleted);
  }

  // Safe to remove. A VLAN's intfID is a back-pointer carrying no
  // configuration of its own, so it is cleared rather than refused.
  for (auto& vlan : *swConfig.vlans()) {
    if (vlan.intfID().has_value() &&
        intfIds.count(InterfaceID(*vlan.intfID())) > 0) {
      vlan.intfID().reset();
    }
  }

  interfaces.erase(
      std::remove_if(
          interfaces.begin(),
          interfaces.end(),
          [&intfIds](const cfg::Interface& intf) {
            return intfIds.count(InterfaceID(*intf.intfID())) > 0;
          }),
      interfaces.end());
}

} // namespace facebook::fboss
