/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/interface/CmdConfigInterfaceQueueConfig.h"

#include "fboss/cli/fboss2/CmdHandler.cpp"

#include <fmt/format.h>
#include <folly/String.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/cli/fboss2/commands/config/qos/PortQueueConfigUtils.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"
#include "fboss/cli/fboss2/utils/InterfaceList.h"

namespace facebook::fboss {

CmdConfigInterfaceQueueConfigTraits::RetType
CmdConfigInterfaceQueueConfig::queryClient(
    const HostInfo& /* hostInfo */,
    const utils::InterfaceList& interfaces,
    const ObjectArgType& nameArg) {
  if (interfaces.empty()) {
    throw std::invalid_argument("No interface name provided");
  }

  const std::string& name = nameArg.getName();
  const bool useDefault = nameArg.isDefault();

  auto& session = ConfigSession::getInstance();
  auto& agentConfig = session.getAgentConfig();
  auto& switchConfig = *agentConfig.sw();

  // `default` is not a portQueueConfigs entry, so there is nothing to look up
  // and nothing to bind: per Port::portQueueConfigName's contract, a port with
  // the field unset already resolves to SwitchConfig::defaultPortQueues.
  // Selecting it therefore clears any existing override rather than writing a
  // name that resolves to nothing.
  if (!useDefault) {
    const auto& portQueueConfigs = *switchConfig.portQueueConfigs();
    if (portQueueConfigs.find(name) == portQueueConfigs.end()) {
      throw std::invalid_argument(
          fmt::format("Queue config '{}' does not exist.", name));
    }
  }

  for (const utils::Intf& intf : interfaces) {
    cfg::Port* port = intf.getPort();
    if (!port) {
      throw std::invalid_argument(
          fmt::format("Interface '{}' is not a physical port.", intf.name()));
    }
    if (useDefault) {
      port->portQueueConfigName().reset();
    } else {
      port->portQueueConfigName() = name;
    }
  }

  session.saveConfig();

  std::string interfaceList = folly::join(", ", interfaces.getNames());
  if (useDefault) {
    return fmt::format(
        "Successfully reset interface(s) {} to the default queue config",
        interfaceList);
  }
  return fmt::format(
      "Successfully set queue-config '{}' for interface(s) {}",
      name,
      interfaceList);
}

void CmdConfigInterfaceQueueConfig::printOutput(
    const CmdConfigInterfaceQueueConfigTraits::RetType& logMsg) {
  std::cout << logMsg << std::endl;
}

// Explicit template instantiation
template void CmdHandler<
    CmdConfigInterfaceQueueConfig,
    CmdConfigInterfaceQueueConfigTraits>::run();

} // namespace facebook::fboss
