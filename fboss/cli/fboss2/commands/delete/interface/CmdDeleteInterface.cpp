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
#include "fboss/lib/config/agent/PortConfigUtils.h"

namespace facebook::fboss {

namespace {

// Valueless delete attributes: reset a port/interface attribute to default.
// The lldp-expected-* names come from the shared lldpAttrToTag() list so the
// config and delete commands cannot drift apart.
const std::unordered_set<std::string> kValuelessDeleteAttributes = [] {
  std::unordered_set<std::string> attrs = {
      "description", "loopback-mode", "lookup-class", "mtu", "queue-config"};
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
    "description, loopback-mode, lookup-class, mtu, queue-config, {}, ip-address, ipv6-address",
    folly::join(", ", lldpAttrNames()));

std::string portLabel(const cfg::Port& port) {
  return port.name().has_value() ? *port.name()
                                 : std::to_string(*port.logicalID());
}

// Enabled ports that are members of `vlanId` (per vlanPorts).
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
    // A port deleted in the same command does not count.
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

// Ports whose untagged ingress VLAN (Port.ingressVlan) is `vlanId`.
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

// Throws if deleting `intf` would leave a config the agent rejects.
void checkDeletable(
    const cfg::SwitchConfig& swConfig,
    const cfg::Interface& intf,
    const std::set<PortID>& portsBeingDeleted) {
  const auto id = *intf.intfID();

  if (*intf.type() == cfg::InterfaceType::PORT) {
    // The agent CHECK-fails on a port with no interface
    // (Port::getInterfaceID()), so this may only go together with its port.
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
          "the agent cannot run with. Delete it together with its port.");
    }
    return;
  }

  const auto vlanId = *intf.vlanID();
  // The agent rejects a VLAN with no interface while a member port is
  // enabled ("VLAN <id> has no interface, even when corresp port <port> is
  // enabled").
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
  // A non-default VLAN is removed with its interface (see deleteInterfaces),
  // so, as for 'delete vlan', no port may still use it as its ingress VLAN.
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

// Deletes the given interfaces, checking all of them first so a refused
// delete changes nothing. A non-default VLAN left without an interface is
// deleted too, as 'delete vlan' would; the default VLAN is kept and its
// intfID cleared. Returns the IDs of the VLANs deleted.
std::set<int32_t> deleteInterfaces(
    cfg::SwitchConfig& swConfig,
    const std::set<InterfaceID>& intfIds,
    const std::set<PortID>& portsBeingDeleted) {
  auto& interfaces = *swConfig.interfaces();

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

  for (auto& vlan : *swConfig.vlans()) {
    if (*vlan.id() == *swConfig.defaultVlan() && vlan.intfID().has_value() &&
        intfIds.count(InterfaceID(*vlan.intfID())) > 0) {
      vlan.intfID().reset();
    }
  }

  // deleteVlan() also deletes each VLAN's interface; erase the rest.
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

  // Resolve names to InterfaceList (throws if any is not found).
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
        // interface (VLAN SVI, loopback).
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
    // Interfaces first, so a refusal happens before any port is removed.
    std::set<int32_t> cascadedVlans;
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
    } else if (attr == "mtu") {
      // Interface-level reset: mtu is an optional field, and the agent falls
      // back to Interface::kDefaultMtu when it is unset.
      std::vector<std::string> resetNames;
      std::vector<std::string> missingNames;
      for (const utils::Intf& intf : interfaces) {
        cfg::Interface* iface = intf.getInterface();
        if (!iface) {
          missingNames.push_back(intf.name());
          continue;
        }
        if (iface->mtu().has_value()) {
          iface->mtu().reset();
          changed = true;
        }
        resetNames.push_back(intf.name());
      }
      if (!resetNames.empty()) {
        results.push_back(
            fmt::format(
                "Successfully reset attribute 'mtu' for interface(s): {}",
                folly::join(", ", resetNames)));
      }
      if (!missingNames.empty()) {
        results.push_back(
            fmt::format(
                "No interface config found for: {}",
                folly::join(", ", missingNames)));
      }
    } else {
      // Port-level valueless reset (description, loopback-mode, lookup-class,
      // queue-config, lldp-expected-*).
      std::vector<std::string> resetNames;
      std::vector<std::string> skippedNames;
      for (const utils::Intf& intf : interfaces) {
        cfg::Port* port = intf.getPort();
        if (!port) {
          skippedNames.push_back(intf.name());
          continue;
        }
        if (attr == "description") {
          if (port->description().has_value()) {
            port->description().reset();
            changed = true;
          }
        } else if (attr == "loopback-mode") {
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
