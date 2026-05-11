/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/interface/switchport/access/vlan/CmdConfigInterfaceSwitchportAccessVlan.h"

#include <unordered_set>

#include "fboss/cli/fboss2/CmdHandler.cpp"

#include "fboss/cli/fboss2/session/ConfigSession.h"

namespace facebook::fboss {

CmdConfigInterfaceSwitchportAccessVlanTraits::RetType
CmdConfigInterfaceSwitchportAccessVlan::queryClient(
    const HostInfo& /* hostInfo */,
    const utils::InterfaceList& interfaces,
    const CmdConfigInterfaceSwitchportAccessVlanTraits::ObjectArgType&
        vlanIdValue) {
  if (interfaces.empty()) {
    throw std::invalid_argument("No interface name provided");
  }

  // Extract the VLAN ID (validation already done in VlanIdValue constructor)
  int32_t vlanId = vlanIdValue.getVlanId();

  // Validate that the VLAN exists and has an interface before modifying config
  auto& config = ConfigSession::getInstance().getAgentConfig();
  bool vlanExists = false;
  for (const auto& vlan : *config.sw()->vlans()) {
    if (*vlan.id() == vlanId) {
      vlanExists = true;
      break;
    }
  }
  if (!vlanExists) {
    throw std::invalid_argument(
        "VLAN " + std::to_string(vlanId) +
        " does not exist. Create the VLAN first before assigning ports to it.");
  }
  bool isDefaultVlan = *config.sw()->defaultVlan() == vlanId;
  if (!isDefaultVlan) {
    bool vlanHasInterface = false;
    for (const auto& intf : *config.sw()->interfaces()) {
      if (*intf.vlanID() == vlanId) {
        vlanHasInterface = true;
        break;
      }
    }
    if (!vlanHasInterface) {
      throw std::invalid_argument(
          "VLAN " + std::to_string(vlanId) +
          " has no interface. Add an interface to the VLAN before assigning ports to it.");
    }
  }

  // Collect the logical port IDs we need to update
  std::unordered_set<int32_t> portIds;

  // Update ingressVlan for all resolved ports
  for (const utils::Intf& intf : interfaces) {
    cfg::Port* port = intf.getPort();
    if (port) {
      port->ingressVlan() = vlanId;
      portIds.insert(*port->logicalID());
    }
  }

  // Also update the vlanPorts entries for these ports
  auto& vlanPorts = *config.sw()->vlanPorts();
  for (auto& vlanPort : vlanPorts) {
    if (portIds.count(*vlanPort.logicalPort())) {
      vlanPort.vlanID() = vlanId;
    }
  }

  // Save the updated config
  // VLAN changes require agent warmboot as reloadConfig() doesn't apply them
  ConfigSession::getInstance().saveConfig(
      cli::ServiceType::AGENT, cli::ConfigActionLevel::AGENT_WARMBOOT);

  std::string interfaceList = folly::join(", ", interfaces.getNames());
  std::string message = "Successfully set access VLAN for interface(s) " +
      interfaceList + " to " + std::to_string(vlanId);

  return message;
}

void CmdConfigInterfaceSwitchportAccessVlan::printOutput(
    const CmdConfigInterfaceSwitchportAccessVlanTraits::RetType& logMsg) {
  std::cout << logMsg << std::endl;
}

// Explicit template instantiation
template void CmdHandler<
    CmdConfigInterfaceSwitchportAccessVlan,
    CmdConfigInterfaceSwitchportAccessVlanTraits>::run();

} // namespace facebook::fboss
