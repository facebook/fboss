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

  // Update ingressVlan for all resolved ports
  for (const utils::Intf& intf : interfaces) {
    cfg::Port* port = intf.getPort();
    if (port) {
      port->ingressVlan() = vlanId;
    }
  }

  // Save the updated config
  ConfigSession::getInstance().saveConfig();

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
