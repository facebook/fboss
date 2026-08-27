/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/delete/interface/CmdDeleteInterface.h"
#include "fboss/cli/fboss2/CmdHandler.cpp"

#include <fmt/format.h>
#include <folly/String.h>
#include <algorithm>
#include <iostream>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

#include "fboss/agent/FbossError.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/types.h"
#include "fboss/cli/fboss2/commands/config/interface/InterfaceIpUtils.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/utils/InterfaceList.h"
#include "fboss/lib/config/AgentConfigUtils.h"

namespace facebook::fboss {

namespace {

// Valueless delete attributes: reset a port/interface attribute to default.
// The lldp-expected-* names come from the shared lldpAttrToTag() list so the
// config and delete commands cannot drift apart.
const std::unordered_set<std::string> kValuelessDeleteAttributes = [] {
  std::unordered_set<std::string> attrs = {
      "loopback-mode", "lookup-class", "queue-config"};
  for (const auto& name : lldpAttrNames()) {
    attrs.insert(name);
  }
  return attrs;
}();

// All known attributes: valueless resets + valued IP address removals.
const std::unordered_set<std::string> kKnownDeleteAttributes = [] {
  std::unordered_set<std::string> attrs = kValuelessDeleteAttributes;
  attrs.insert("ip-address");
  attrs.insert("ipv6-address");
  return attrs;
}();

const std::string kValidDeleteAttrs = fmt::format(
    "loopback-mode, lookup-class, queue-config, {}, ip-address, ipv6-address",
    folly::join(", ", lldpAttrNames()));

std::string portLabel(const cfg::Port& port) {
  return port.name().has_value() ? *port.name()
                                 : std::to_string(*port.logicalID());
}

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

// Membership comes from vlanPorts, matching how ThriftConfigApplier builds
// its port -> vlan map.
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

// Ports naming `vlanId` in Port.ingressVlan. The only fallback value for
// ingressVlan is 0, which means routed port, so clearing it would silently
// change the port's L2 mode.
std::vector<std::string> ingressVlanPorts(
    const cfg::SwitchConfig& swConfig,
    int32_t vlanId,
    const std::set<PortID>& portsBeingDeleted) {
  std::vector<std::string> names;
  for (const auto& port : *swConfig.ports()) {
    if (portsBeingDeleted.count(PortID(*port.logicalID())) > 0) {
      continue;
    }
    if (*port.ingressVlan() == vlanId) {
      names.push_back(portLabel(port));
    }
  }
  return names;
}

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

// Throws if removing `intf`, as part of removing all of `goingAway`, would
// dangle a reference or produce a config the agent cannot apply.
void checkDeletable(
    const cfg::SwitchConfig& swConfig,
    const cfg::Interface& intf,
    const std::set<InterfaceID>& goingAway,
    const std::set<PortID>& portsBeingDeleted) {
  const auto id = *intf.intfID();

  if (*intf.type() == cfg::InterfaceType::PORT) {
    // Port::getInterfaceID() CHECK-fails on a port with an empty interface
    // list, taking the agent down on the first packet routed via the port.
    throw FbossError(
        "Cannot delete interface ",
        id,
        ": it is the port router interface for port ",
        intf.portID().has_value() ? std::to_string(*intf.portID()) : "<unset>",
        ". Deleting it would leave that port without an interface, which the "
        "agent cannot run with. Delete the port itself instead.");
  }

  // Tunnel.underlayIntfID is a required field, so there is nothing to clear.
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
  // ThriftConfigApplier rejects such a config with "VLAN <id> has no
  // interface, even when corresp port <port> is enabled".
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
  // The VLAN would be left without an interface, so it is cascaded (see
  // deleteInterfaces below) — unless a port still names it as its untagged
  // ingress VLAN, mirroring the referrer check in VlanManager::deleteVlan.
  if (vlanId != *swConfig.defaultVlan()) {
    auto ingressPorts = ingressVlanPorts(swConfig, vlanId, portsBeingDeleted);
    if (!ingressPorts.empty()) {
      throw FbossError(
          "Cannot delete interface ",
          id,
          ": it is the only interface for VLAN ",
          vlanId,
          ", which would be removed with it but is the ingress VLAN for "
          "port(s): ",
          folly::join(", ", ingressPorts),
          ". Move the port(s) to another VLAN first, or delete the whole "
          "VLAN with 'delete vlan ",
          vlanId,
          "'.");
    }
  }
}

// Removes the given interfaces and the VLAN intfID back-pointers naming them.
//
// Every ID is checked before any of them is removed, so a refused delete
// leaves the config untouched. Checking the set as a whole also means two
// interfaces sharing a VLAN can be deleted together: neither counts as the
// other's surviving cover.
//
// A VLAN losing its last interface is cascaded away with it, along with its
// VlanPort membership rows and static MAC entries, matching 'delete vlan'.
// Leaving a bare VLAN behind arms a landmine: enabling a member port later
// fails config apply far from the command that caused it. The default VLAN
// (SwitchConfig.defaultVlan) legitimately exists without an interface, so
// only its intfID back-pointer is cleared.
//
// An ACL redirect-nexthop naming one of these interfaces is not a refusal:
// RedirectNextHop.intfID is optional and the agent just disables the ACL when
// no nexthop resolves.
std::vector<int32_t> deleteInterfaces(
    cfg::SwitchConfig& swConfig,
    const std::set<InterfaceID>& intfIds,
    const std::set<PortID>& portsBeingDeleted) {
  auto& interfaces = *swConfig.interfaces();

  // An id with no interface is skipped rather than rejected: callers resolve
  // names through InterfaceList first, so it cannot happen here.
  std::set<int32_t> vlansToCascade;
  for (const auto& intfId : intfIds) {
    const auto id = static_cast<int32_t>(intfId);
    auto it = std::find_if(
        interfaces.cbegin(), interfaces.cend(), [id](const cfg::Interface& i) {
          return *i.intfID() == id;
        });
    if (it == interfaces.cend()) {
      continue;
    }
    checkDeletable(swConfig, *it, intfIds, portsBeingDeleted);
    if (*it->type() == cfg::InterfaceType::VLAN &&
        *it->vlanID() != *swConfig.defaultVlan() &&
        !vlanKeepsAnInterface(swConfig, *it->vlanID(), intfIds)) {
      vlansToCascade.insert(*it->vlanID());
    }
  }

  // Safe to remove.
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

  if (!vlansToCascade.empty()) {
    auto& vlans = *swConfig.vlans();
    vlans.erase(
        std::remove_if(
            vlans.begin(),
            vlans.end(),
            [&vlansToCascade](const cfg::Vlan& v) {
              return vlansToCascade.count(*v.id()) > 0;
            }),
        vlans.end());
    auto& vlanPorts = *swConfig.vlanPorts();
    vlanPorts.erase(
        std::remove_if(
            vlanPorts.begin(),
            vlanPorts.end(),
            [&vlansToCascade](const cfg::VlanPort& vp) {
              return vlansToCascade.count(*vp.vlanID()) > 0;
            }),
        vlanPorts.end());
    if (swConfig.staticMacAddrs().has_value()) {
      auto& macs = *swConfig.staticMacAddrs();
      macs.erase(
          std::remove_if(
              macs.begin(),
              macs.end(),
              [&vlansToCascade](const cfg::StaticMacEntry& e) {
                return vlansToCascade.count(*e.vlanID()) > 0;
              }),
          macs.end());
    }
  }

  return {vlansToCascade.begin(), vlansToCascade.end()};
}

} // namespace

InterfaceDeleteConfig::InterfaceDeleteConfig(const std::vector<std::string>& v)
    : InterfaceAttrArgsBase(
          kKnownDeleteAttributes,
          kValuelessDeleteAttributes,
          "delete attribute",
          kValidDeleteAttrs) {
  auto portNames = parseTokens(v);

  // If no known attribute delimited the port list, catch a mistyped attribute:
  // interface names always contain '/' (e.g. "eth1/1/1"); attribute names never
  // do but do contain '-'.
  if (attributes_.empty()) {
    for (const auto& tok : portNames) {
      if (tok.find('-') != std::string::npos &&
          tok.find('/') == std::string::npos) {
        throw std::invalid_argument(
            fmt::format(
                "Unknown delete attribute '{}'. Valid attributes are: {}",
                tok,
                kValidDeleteAttrs));
      }
    }
  }

  // Validate ip-address / ipv6-address values as CIDR networks.
  for (const auto& [attr, value] : attributes_) {
    if (attr == "ip-address" || attr == "ipv6-address") {
      validateInterfaceIpAttr(attr, value);
    }
  }

  // Resolve names to InterfaceList (throws if any name is not found).
  // InterfaceList resolves a bare number as a port logical ID or an interface
  // ID, so a whole-interface delete can name the interfaces that generated
  // configs leave unnamed.
  interfaces_ = utils::InterfaceList(std::move(portNames));
}

CmdDeleteInterfaceTraits::RetType CmdDeleteInterface::queryClient(
    const HostInfo& /* hostInfo */,
    const ObjectArgType& deleteConfig) {
  const auto& interfaces = deleteConfig.getInterfaces();
  const auto& attributes = deleteConfig.getAttributes();

  if (interfaces.empty()) {
    throw std::invalid_argument("No interface name provided");
  }

  // No attributes => delete the whole port(s) / interface(s) from the config.
  if (attributes.empty()) {
    auto& swConfig = *ConfigSession::getInstance().getAgentConfig().sw();
    std::set<PortID> portsToDelete;
    std::set<InterfaceID> interfacesToDelete;
    std::vector<std::string> deletedNames;
    for (const utils::Intf& intf : interfaces) {
      if (const cfg::Port* port = intf.getPort()) {
        portsToDelete.insert(PortID(*port->logicalID()));
      } else if (const cfg::Interface* iface = intf.getInterface()) {
        // A name that resolves to an interface but no port is a portless L3
        // interface (VLAN SVI, loopback), so the interface itself is what gets
        // removed. The interfaces a port owns are pruned by
        // removePortsFromConfig below instead.
        interfacesToDelete.insert(InterfaceID(*iface->intfID()));
      } else {
        continue;
      }
      deletedNames.push_back(intf.name());
    }
    if (portsToDelete.empty() && interfacesToDelete.empty()) {
      throw std::invalid_argument(
          "No port or interface found for the specified name(s)");
    }
    // Interfaces first: deleteInterfaces() validates before it mutates, so
    // running it ahead of removePortsFromConfig keeps a refusal from leaving
    // the session half-mutated. It therefore sees the ports as still present,
    // hence portsToDelete.
    std::vector<int32_t> cascadedVlans;
    if (!interfacesToDelete.empty()) {
      cascadedVlans =
          deleteInterfaces(swConfig, interfacesToDelete, portsToDelete);
    }
    if (!portsToDelete.empty()) {
      utility::removePortsFromConfig(
          swConfig,
          portsToDelete,
          utility::PortRemovalMode::Erase,
          /*pruneEmptyVlansAndInterfaces=*/true);
    }
    // The erases above dangle the raw pointers the cached PortMap holds into
    // swConfig.ports() and swConfig.interfaces().
    ConfigSession::getInstance().rebuildPortMap();
    // HITLESS: the agent's reloadConfig() applies the delta live, as it does
    // for 'config interface <port> profile' and 'delete vlan'.
    ConfigSession::getInstance().saveConfig();
    if (!cascadedVlans.empty()) {
      return fmt::format(
          "Deleted interface(s): {} (also removed VLAN(s) {} left without "
          "an interface)",
          folly::join(", ", deletedNames),
          folly::join(", ", cascadedVlans));
    }
    return fmt::format(
        "Deleted interface(s): {}", folly::join(", ", deletedNames));
  }

  std::vector<std::string> results;
  bool changed = false;

  for (const auto& [attr, value] : attributes) {
    if (attr == "ip-address" || attr == "ipv6-address") {
      // Remove a specific IP address from each interface's ipAddresses list.
      bool expectV6 = (attr == "ipv6-address");
      std::vector<std::string> doneNames;
      std::vector<std::string> missingNames;
      for (const utils::Intf& intf : interfaces) {
        cfg::Interface* iface = intf.getInterface();
        if (!iface) {
          missingNames.push_back(intf.name());
          continue;
        }
        auto& ipAddresses = *iface->ipAddresses();
        auto it = std::find(ipAddresses.begin(), ipAddresses.end(), value);
        if (it != ipAddresses.end()) {
          ipAddresses.erase(it);
          changed = true;
          doneNames.push_back(intf.name());
        } else {
          results.push_back(
              fmt::format(
                  "{} {} not configured on interface {}",
                  expectV6 ? "IPv6 address" : "IP address",
                  value,
                  intf.name()));
        }
      }
      if (!doneNames.empty()) {
        results.push_back(
            fmt::format(
                "Successfully removed {} {} from interface(s): {}",
                expectV6 ? "IPv6 address" : "IP address",
                value,
                folly::join(", ", doneNames)));
      }
      if (!missingNames.empty()) {
        results.push_back(
            fmt::format(
                "No interface config found for: {}",
                folly::join(", ", missingNames)));
      }
    } else {
      // Port-level valueless reset (loopback-mode, lookup-class, queue-config,
      // lldp-expected-*).
      std::vector<std::string> resetNames;
      std::vector<std::string> skippedNames;
      for (const utils::Intf& intf : interfaces) {
        cfg::Port* port = intf.getPort();
        if (!port) {
          skippedNames.push_back(intf.name());
          continue;
        }
        if (attr == "loopback-mode") {
          if (*port->loopbackMode() != cfg::PortLoopbackMode::NONE) {
            port->loopbackMode() = cfg::PortLoopbackMode::NONE;
            changed = true;
          }
        } else if (attr == "lookup-class") {
          if (!port->lookupClasses()->empty()) {
            port->lookupClasses()->clear();
            changed = true;
          }
        } else if (attr == "queue-config") {
          // Same reset as `config interface <intf> queue-config default`: an
          // unset portQueueConfigName resolves to
          // SwitchConfig::defaultPortQueues.
          if (port->portQueueConfigName().has_value()) {
            port->portQueueConfigName().reset();
            changed = true;
          }
        } else if (auto tag = lldpTagForAttr(attr); tag.has_value()) {
          changed |= port->expectedLLDPValues()->erase(*tag) > 0;
        }
        resetNames.push_back(intf.name());
      }

      if (resetNames.empty()) {
        results.push_back(
            fmt::format(
                "Attribute '{}' not reset: no port found for interface(s) {}",
                attr,
                folly::join(", ", skippedNames)));
      } else if (skippedNames.empty()) {
        results.push_back(
            fmt::format(
                "Successfully reset attribute '{}' for interface(s): {}",
                attr,
                folly::join(", ", resetNames)));
      } else {
        results.push_back(
            fmt::format(
                "Successfully reset attribute '{}' for interface(s): {}; "
                "no port found for {}",
                attr,
                folly::join(", ", resetNames),
                folly::join(", ", skippedNames)));
      }
    }
  }

  if (changed) {
    ConfigSession::getInstance().saveConfig();
  }

  return folly::join("\n", results);
}

void CmdDeleteInterface::printOutput(const RetType& logMsg) {
  std::cout << logMsg << std::endl;
}

// Explicit template instantiation
template void CmdHandler<CmdDeleteInterface, CmdDeleteInterfaceTraits>::run();

} // namespace facebook::fboss
