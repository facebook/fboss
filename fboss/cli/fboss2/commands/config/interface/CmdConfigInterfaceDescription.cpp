/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/interface/CmdConfigInterfaceDescription.h"

#include <folly/Conv.h>
#include "fboss/cli/fboss2/session/ConfigSession.h"

namespace facebook::fboss {

CmdConfigInterfaceDescriptionTraits::RetType
CmdConfigInterfaceDescription::queryClient(
    const HostInfo& hostInfo,
    const utils::InterfaceList& interfaces,
    const ObjectArgType& description) {
  if (interfaces.empty()) {
    throw std::invalid_argument("No interface name provided");
  }

  std::string descriptionStr = description.data()[0];

  // Update description for all resolved ports
  for (const utils::Intf& intf : interfaces) {
    cfg::Port* port = intf.getPort();
    if (port) {
      port->description() = descriptionStr;
    }
  }

  // Save the updated config - description changes are hitless
  ConfigSession::getInstance().saveConfig(
      cli::ServiceType::AGENT, cli::ConfigActionLevel::HITLESS);

  std::string interfaceList = folly::join(", ", interfaces.getNames());
  return "Successfully set description for interface(s) " + interfaceList;
}

void CmdConfigInterfaceDescription::printOutput(const RetType& logMsg) {
  std::cout << logMsg << std::endl;
}

} // namespace facebook::fboss
