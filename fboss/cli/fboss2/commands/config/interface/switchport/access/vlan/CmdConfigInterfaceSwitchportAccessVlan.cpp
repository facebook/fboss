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

#include <folly/Conv.h>
#include "fboss/cli/fboss2/session/ConfigSession.h"

namespace facebook::fboss {

CmdConfigInterfaceSwitchportAccessVlanTraits::RetType
CmdConfigInterfaceSwitchportAccessVlan::queryClient(
    const HostInfo& hostInfo,
    const utils::InterfaceList& interfaces,
    const CmdConfigInterfaceSwitchportAccessVlanTraits::ObjectArgType&
        vlanIdValue) {
  if (interfaces.empty()) {
    throw std::invalid_argument("No interface name provided");
  }

  // Extract the VLAN ID (validation already done in VlanIdValue constructor)
  int32_t vlanId = vlanIdValue.getVlanId();

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
  auto& config = ConfigSession::getInstance().getAgentConfig();
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

} // namespace facebook::fboss
