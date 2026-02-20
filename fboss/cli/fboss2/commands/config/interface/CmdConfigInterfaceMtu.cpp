/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/interface/CmdConfigInterfaceMtu.h"

#include <folly/Conv.h>
#include "fboss/cli/fboss2/session/ConfigSession.h"

namespace facebook::fboss {

CmdConfigInterfaceMtuTraits::RetType CmdConfigInterfaceMtu::queryClient(
    const HostInfo& hostInfo,
    const utils::InterfaceList& interfaces,
    const CmdConfigInterfaceMtuTraits::ObjectArgType& mtuValue) {
  // Extract the MTU value (validation already done in MtuValue constructor)
  int32_t mtu = mtuValue.getMtu();

  // Update MTU for all resolved interfaces
  for (const utils::Intf& intf : interfaces) {
    cfg::Interface* interface = intf.getInterface();
    if (interface) {
      interface->mtu() = mtu;
    }
  }

  // Save the updated config - MTU changes are hitless
  ConfigSession::getInstance().saveConfig(
      cli::ServiceType::AGENT, cli::ConfigActionLevel::HITLESS);

  std::string interfaceList = folly::join(", ", interfaces.getNames());
  std::string message = "Successfully set MTU for interface(s) " +
      interfaceList + " to " + std::to_string(mtu);

  return message;
}

void CmdConfigInterfaceMtu::printOutput(
    const CmdConfigInterfaceMtuTraits::RetType& logMsg) {
  std::cout << logMsg << std::endl;
}

} // namespace facebook::fboss
