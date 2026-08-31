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
#include "fboss/cli/fboss2/commands/config/vlan/VlanManager.h"
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

// Throws if removing `intf`, as part of removing all interfaces in the same
// command, would dangle a reference or produce a config the agent cannot
// apply.
void checkDeletable(
    const cfg::SwitchConfig& swConfig,
    const cfg::Interface& intf,
    const std::set<PortID>& portsBeingDeleted) {
  const auto id = *intf.intfID();

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

  if (*intf.type() == cfg::InterfaceType::SYSTEM_PORT) {
    // On VOQ/DSF switches the agent derives a system-port interface for every
    // port; erasing the config row leaves the port with a dangling interface
    // ID (and for the inband port, breaks every subsequent config apply).
    throw FbossError(
        "Cannot delete interface ",
        id,
        ": deleting system-port interfaces is not supported.");
  }

  if (*intf.type() == cfg::InterfaceType::PORT) {
    // Port::getInterfaceID() CHECK-fails on a port with an empty interface
    // list, taking the agent down on the first packet routed via the port.
    // Deleting the port itself pulls its router interface along (see
    // queryClient), so the interface may only go when its port goes too.
    const bool portGoesToo = intf.portID().has_value() &&
        portsBeingDeleted.count(PortID(*intf.portID())) > 0;
    if (!portGoesToo) {
      throw FbossError(
          "Cannot delete interface ",
          id,
          ": it is the port router interface for port ",
          intf.portID().has_value() ? std::to_string(*intf.portID())
                                    : "<unset>",
          ". Deleting it would leave that port without an interface, which "
          "the agent cannot run with. Delete the port itself instead.");
    }
    return;
  }

  const auto vlanId = *intf.vlanID();
  // The agent enforces exactly one interface per non-default VLAN, so this
  // interface is its VLAN's only one: the VLAN either cascades away with it
  // or (default VLAN only) survives interface-less. ThriftConfigApplier
  // rejects an interface-less VLAN with "VLAN <id> has no interface, even
  // when corresp port <port> is enabled".
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
  // ingress VLAN, mirroring the referrer check in CmdDeleteVlan.
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
// leaves the config untouched.
//
// A VLAN losing its interface is cascaded away with it, along with its
// VlanPort membership rows and static MAC entries, matching 'delete vlan'.
// Leaving a bare VLAN behind arms a landmine: enabling a member port later
// fails config apply far from the command that caused it. The default VLAN
// (SwitchConfig.defaultVlan) legitimately exists without an interface, so
// only its intfID back-pointer is cleared.
//
// An ACL redirect-nexthop naming one of these interfaces is not a refusal:
// RedirectNextHop.intfID is optional and the agent just disables the ACL when
// no nexthop resolves.
std::set<int32_t> deleteInterfaces(
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
    checkDeletable(swConfig, *it, portsBeingDeleted);
    if (*it->type() == cfg::InterfaceType::VLAN &&
        *it->vlanID() != *swConfig.defaultVlan() &&
        VlanManager::findVlan(swConfig, VlanID(*it->vlanID())) != nullptr) {
      vlansToCascade.insert(*it->vlanID());
    }
  }

  // The default VLAN is the only VLAN that may exist without an interface,
  // so it is the only one that survives its interface's deletion; clear its
  // back-pointer. Non-default VLANs cascade away whole below.
  for (auto& vlan : *swConfig.vlans()) {
    if (*vlan.id() == *swConfig.defaultVlan() && vlan.intfID().has_value() &&
        intfIds.count(InterfaceID(*vlan.intfID())) > 0) {
      vlan.intfID().reset();
    }
  }

  // deleteVlan also drops each cascaded VLAN's interface; the erase below
  // covers the rest (loopbacks, the default VLAN's SVI, port router
  // interfaces going with their port). Deletability was established by
  // checkDeletable above, with this command's own rules (a port going away in
  // the same command does not count as a referrer).
  for (const auto vlanId : vlansToCascade) {
    VlanManager::deleteVlan(swConfig, VlanID(vlanId));
  }
  std::erase_if(interfaces, [&intfIds](const cfg::Interface& intf) {
    return intfIds.count(InterfaceID(*intf.intfID())) > 0;
  });

  return vlansToCascade;
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

  // InterfaceList resolves a bare number as an interface ID, so a
  // whole-interface delete can name the interfaces generated configs leave
  // unnamed. Throws if any name is not found.
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
        // A name resolving to an interface but no port is a portless L3
        // interface (VLAN SVI, loopback); the interfaces a port owns are
        // folded into this set below.
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
    // A port delete implies deleting the interfaces that cannot outlive it:
    // its port router interfaces (bound by portID) and the SVI of any VLAN
    // the port was the last member of (removePortsFromConfig would otherwise
    // prune them with no referrer checks at all). Fold them into
    // interfacesToDelete so both spellings of the same delete go through the
    // same checks and the same cascade reporting.
    if (!portsToDelete.empty()) {
      std::set<int32_t> memberVlansOfDeleted;
      std::set<int32_t> survivingVlans;
      for (const auto& vp : *swConfig.vlanPorts()) {
        if (portsToDelete.count(PortID(*vp.logicalPort())) > 0) {
          memberVlansOfDeleted.insert(*vp.vlanID());
        } else {
          survivingVlans.insert(*vp.vlanID());
        }
      }
      for (const auto& iface : *swConfig.interfaces()) {
        const bool boundToDeletedPort = iface.portID().has_value() &&
            portsToDelete.count(PortID(*iface.portID())) > 0;
        const bool onNewlyEmptyVlan =
            *iface.type() == cfg::InterfaceType::VLAN &&
            *iface.vlanID() != *swConfig.defaultVlan() &&
            memberVlansOfDeleted.count(*iface.vlanID()) > 0 &&
            survivingVlans.count(*iface.vlanID()) == 0;
        if (boundToDeletedPort || onNewlyEmptyVlan) {
          interfacesToDelete.insert(InterfaceID(*iface.intfID()));
        }
      }
    }
    // Interfaces first: deleteInterfaces() validates before it mutates, so
    // running it ahead of removePortsFromConfig keeps a refusal from leaving
    // the session half-mutated. It therefore sees the ports as still present,
    // hence portsToDelete.
    std::set<int32_t> cascadedVlans;
    if (!interfacesToDelete.empty()) {
      cascadedVlans =
          deleteInterfaces(swConfig, interfacesToDelete, portsToDelete);
    }
    if (!portsToDelete.empty()) {
      // SVIs and port router interfaces of the deleted ports are already gone
      // via deleteInterfaces above; the prune here only sweeps L2-only VLANs
      // (no interface) left without a member.
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
