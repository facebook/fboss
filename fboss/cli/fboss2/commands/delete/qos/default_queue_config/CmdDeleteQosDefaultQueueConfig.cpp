/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/delete/qos/default_queue_config/CmdDeleteQosDefaultQueueConfig.h"

#include "fboss/cli/fboss2/CmdHandler.cpp"

#include <fmt/format.h>
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <stdexcept>

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/cli/fboss2/gen-cpp2/cli_metadata_types.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

namespace facebook::fboss {

CmdDeleteQosDefaultQueueConfigTraits::RetType
CmdDeleteQosDefaultQueueConfig::queryClient(
    const HostInfo& /* hostInfo */,
    const ObjectArgType& queueId) {
  auto& session = ConfigSession::getInstance();
  auto& agentConfig = session.getAgentConfig();
  auto& switchConfig = *agentConfig.sw();

  auto& defaultPortQueues = *switchConfig.defaultPortQueues();
  int16_t queueIdVal = queueId.getQueueId();

  auto it = std::find_if(
      defaultPortQueues.begin(),
      defaultPortQueues.end(),
      [queueIdVal](const auto& queueConfig) {
        return *queueConfig.id() == queueIdVal;
      });

  if (it == defaultPortQueues.end()) {
    throw std::runtime_error(
        fmt::format(
            "No default-queue-config with queue-id {} exists", queueIdVal));
  }

  defaultPortQueues.erase(it);

  session.saveConfig(
      cli::ServiceType::AGENT, cli::ConfigActionLevel::AGENT_COLDBOOT);

  return fmt::format(
      "Successfully deleted default-queue-config queue-id {}", queueIdVal);
}

void CmdDeleteQosDefaultQueueConfig::printOutput(const RetType& logMsg) {
  std::cout << logMsg << std::endl;
}

template void CmdHandler<
    CmdDeleteQosDefaultQueueConfig,
    CmdDeleteQosDefaultQueueConfigTraits>::run();

} // namespace facebook::fboss
