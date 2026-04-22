/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/interface/switchport/trunk/allowed/vlan/CmdConfigInterfaceSwitchportTrunkAllowedVlan.h"

#include "fboss/cli/fboss2/CmdHandler.cpp"

#include <fmt/format.h>
#include <folly/String.h>
#include <algorithm>
#include <set>
#include "fboss/cli/fboss2/session/ConfigSession.h"

namespace facebook::fboss {

CmdConfigInterfaceSwitchportTrunkAllowedVlanTraits::RetType
CmdConfigInterfaceSwitchportTrunkAllowedVlan::queryClient(
    const HostInfo& /* hostInfo */,
    const utils::InterfaceList& interfaces,
    const CmdConfigInterfaceSwitchportTrunkAllowedVlanTraits::ObjectArgType&
        vlanAction) {
  if (interfaces.empty()) {
    throw std::invalid_argument("No interface name provided");
  }

  auto& session = ConfigSession::getInstance();
  auto& config = session.getAgentConfig();
  auto& swConfig = *config.sw();

  const auto& vlanIds = vlanAction.getVlanIds();
  bool isAdd = vlanAction.isAdd();

  // Get port logical IDs from port names
  std::vector<std::pair<std::string, int32_t>> portNamesAndIds;
  for (const utils::Intf& intf : interfaces) {
    cfg::Port* port = intf.getPort();
    if (port) {
      portNamesAndIds.emplace_back(*port->name(), *port->logicalID());
    }
  }

  if (portNamesAndIds.empty()) {
    throw std::invalid_argument("No valid ports found");
  }

  int changeCount = 0;

  if (isAdd) {
    // Verify all VLANs exist before adding - VLANs must be pre-configured
    // with their corresponding interfaces to avoid TunManager crashes
    std::vector<int32_t> missingVlans;
    for (int32_t vlanId : vlanIds) {
      auto vitr = std::find_if(
          swConfig.vlans()->cbegin(),
          swConfig.vlans()->cend(),
          [vlanId](const auto& vlan) { return *vlan.id() == vlanId; });

      if (vitr == swConfig.vlans()->cend()) {
        missingVlans.push_back(vlanId);
      }
    }

    if (!missingVlans.empty()) {
      std::vector<std::string> missingVlanStrs;
      missingVlanStrs.reserve(missingVlans.size());
      for (int32_t vlanId : missingVlans) {
        missingVlanStrs.push_back(std::to_string(vlanId));
      }
      throw std::invalid_argument(
          fmt::format(
              "VLAN(s) {} do not exist. VLANs must be created with their "
              "corresponding interfaces before adding to trunk ports.",
              folly::join(", ", missingVlanStrs)));
    }

    // Add VlanPort entries for each port and VLAN combination
    for (const auto& portNameAndId : portNamesAndIds) {
      int32_t portLogicalId = portNameAndId.second;
      for (int32_t vlanId : vlanIds) {
        // Check if VlanPort entry already exists
        auto existingEntry = std::find_if(
            swConfig.vlanPorts()->begin(),
            swConfig.vlanPorts()->end(),
            [vlanId, portLogicalId](const auto& vp) {
              return *vp.vlanID() == vlanId &&
                  *vp.logicalPort() == portLogicalId;
            });

        if (existingEntry == swConfig.vlanPorts()->end()) {
          // Create new VlanPort entry with emitTags=true for trunk mode
          cfg::VlanPort newVlanPort;
          newVlanPort.vlanID() = vlanId;
          newVlanPort.logicalPort() = portLogicalId;
          newVlanPort.spanningTreeState() = cfg::SpanningTreeState::FORWARDING;
          newVlanPort.emitTags() = true; // Tagged for trunk ports
          swConfig.vlanPorts()->push_back(std::move(newVlanPort));
          changeCount++;
        }
      }
    }
  } else {
    // Remove: Build a set of (vlanId, portLogicalId) pairs to remove
    std::set<std::pair<int32_t, int32_t>> toRemove;
    for (const auto& portNameAndId : portNamesAndIds) {
      int32_t portLogicalId = portNameAndId.second;
      for (int32_t vlanId : vlanIds) {
        toRemove.insert({vlanId, portLogicalId});
      }
    }

    // Remove matching VlanPort entries
    auto& vlanPorts = *swConfig.vlanPorts();
    size_t originalSize = vlanPorts.size();

    vlanPorts.erase(
        std::remove_if(
            vlanPorts.begin(),
            vlanPorts.end(),
            [&toRemove](const auto& vp) {
              return toRemove.count({*vp.vlanID(), *vp.logicalPort()}) > 0;
            }),
        vlanPorts.end());

    changeCount = static_cast<int>(originalSize - vlanPorts.size());
    // Note: We do not auto-delete VLANs when removing from trunk ports.
    // VLANs must be managed separately since they require corresponding
    // interfaces to be properly configured.
  }

  // Save the updated config
  session.saveConfig();

  std::string interfaceList = folly::join(", ", interfaces.getNames());
  std::vector<std::string> vlanIdStrs;
  vlanIdStrs.reserve(vlanIds.size());
  for (int32_t vlanId : vlanIds) {
    vlanIdStrs.push_back(std::to_string(vlanId));
  }
  std::string vlanList = folly::join(", ", vlanIdStrs);
  std::string action = isAdd ? "added" : "removed";

  if (changeCount == 0) {
    return fmt::format(
        "No changes made - VLAN(s) {} already {} on interface(s) {}",
        vlanList,
        isAdd ? "allowed" : "not associated with",
        interfaceList);
  }

  return fmt::format(
      "Successfully {} {} VLAN-port association(s) for VLAN(s) {} on interface(s) {}",
      action,
      changeCount,
      vlanList,
      interfaceList);
}

void CmdConfigInterfaceSwitchportTrunkAllowedVlan::printOutput(
    const CmdConfigInterfaceSwitchportTrunkAllowedVlanTraits::RetType& logMsg) {
  std::cout << logMsg << std::endl;
}

// Explicit template instantiation
template void CmdHandler<
    CmdConfigInterfaceSwitchportTrunkAllowedVlan,
    CmdConfigInterfaceSwitchportTrunkAllowedVlanTraits>::run();

} // namespace facebook::fboss
