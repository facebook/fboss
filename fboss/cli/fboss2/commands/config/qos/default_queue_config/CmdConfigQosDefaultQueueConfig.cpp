/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/qos/default_queue_config/CmdConfigQosDefaultQueueConfig.h"

#include "fboss/cli/fboss2/CmdHandler.cpp"

#include <fmt/format.h>
#include <cstddef>
#include <cstdint>
#include <iostream>

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/cli/fboss2/commands/config/qos/PortQueueConfigUtils.h"
#include "fboss/cli/fboss2/gen-cpp2/cli_metadata_types.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

namespace facebook::fboss {

CmdConfigQosDefaultQueueConfigTraits::RetType
CmdConfigQosDefaultQueueConfig::queryClient(
    const HostInfo& /* hostInfo */,
    const ObjectArgType& config) {
  auto& session = ConfigSession::getInstance();
  auto& agentConfig = session.getAgentConfig();
  auto& switchConfig = *agentConfig.sw();

  auto& defaultPortQueues = *switchConfig.defaultPortQueues();
  int16_t queueIdVal = config.getQueueId();

  // Edit a local copy; splice back only after all args validate, so a mid-parse
  // throw leaves the shared ConfigSession untouched.
  int existingIdx = -1;
  cfg::PortQueue work;
  for (size_t i = 0; i < defaultPortQueues.size(); ++i) {
    if (*defaultPortQueues[i].id() == queueIdVal) {
      work = defaultPortQueues[i];
      existingIdx = static_cast<int>(i);
      break;
    }
  }
  if (existingIdx < 0) {
    work.id() = queueIdVal;
    work.scheduling() = cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN;
  }

  utils::applyPortQueueConfig(
      work, config.getAttributes(), config.getAqmAttributes());

  if (existingIdx >= 0) {
    defaultPortQueues[existingIdx] = work;
  } else {
    defaultPortQueues.push_back(work);
  }

  session.saveConfig(
      cli::ServiceType::AGENT, cli::ConfigActionLevel::AGENT_COLDBOOT);

  return fmt::format(
      "Successfully configured default-queue-config queue-id {}", queueIdVal);
}

void CmdConfigQosDefaultQueueConfig::printOutput(const RetType& logMsg) {
  std::cout << logMsg << std::endl;
}

template void CmdHandler<
    CmdConfigQosDefaultQueueConfig,
    CmdConfigQosDefaultQueueConfigTraits>::run();

} // namespace facebook::fboss
