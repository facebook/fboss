/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/interface/CmdConfigInterfaceQueuingPolicy.h"

#include <fmt/format.h>
#include <folly/String.h>
#include <iostream>
#include <ostream>
#include <stdexcept>
#include <string>
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"
#include "fboss/cli/fboss2/utils/InterfaceList.h"

namespace facebook::fboss {

CmdConfigInterfaceQueuingPolicyTraits::RetType
CmdConfigInterfaceQueuingPolicy::queryClient(
    const HostInfo& /* hostInfo */,
    const utils::InterfaceList& interfaces,
    const ObjectArgType& policyNameArg) {
  if (interfaces.empty()) {
    throw std::invalid_argument("No interface name provided");
  }

  if (policyNameArg.data().empty()) {
    throw std::invalid_argument("No queuing policy name provided");
  }

  std::string policyName = policyNameArg.data()[0];

  auto& session = ConfigSession::getInstance();
  auto& agentConfig = session.getAgentConfig();
  auto& switchConfig = *agentConfig.sw();

  // Check that the policy exists in portQueueConfigs
  const auto& portQueueConfigs = *switchConfig.portQueueConfigs();
  if (portQueueConfigs.find(policyName) == portQueueConfigs.end()) {
    throw std::invalid_argument(
        fmt::format("Queuing policy '{}' does not exist.", policyName));
  }

  // Update portQueueConfigName for all resolved ports
  for (const utils::Intf& intf : interfaces) {
    cfg::Port* port = intf.getPort();
    if (!port) {
      throw std::invalid_argument(
          fmt::format("Interface '{}' is not a physical port.", intf.name()));
    }
    port->portQueueConfigName() = policyName;
  }

  // Save the updated config
  session.saveConfig();

  std::string interfaceList = folly::join(", ", interfaces.getNames());
  return fmt::format(
      "Successfully set queuing-policy '{}' for interface(s) {}",
      policyName,
      interfaceList);
}

void CmdConfigInterfaceQueuingPolicy::printOutput(
    const CmdConfigInterfaceQueuingPolicyTraits::RetType& logMsg) {
  std::cout << logMsg << std::endl;
}

} // namespace facebook::fboss
