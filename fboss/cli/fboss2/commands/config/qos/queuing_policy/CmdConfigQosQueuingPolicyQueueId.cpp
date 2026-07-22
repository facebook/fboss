/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/qos/queuing_policy/CmdConfigQosQueuingPolicyQueueId.h"

#include "fboss/cli/fboss2/CmdHandler.cpp"

#include <fmt/format.h>
#include <folly/Conv.h>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/cli/fboss2/commands/config/qos/PortQueueConfigUtils.h"
#include "fboss/cli/fboss2/commands/config/qos/queuing_policy/CmdConfigQosQueuingPolicy.h"
#include "fboss/cli/fboss2/gen-cpp2/cli_metadata_types.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

namespace facebook::fboss {

QueueConfig::QueueConfig(std::vector<std::string> v) {
  // Minimum: <queue-id> <attr> <value>
  if (v.empty()) {
    throw std::invalid_argument(
        "Expected: <queue-id> <attr> <value> [<attr> <value> ...] where <attr> is one of: "
        "reserved-bytes, shared-bytes, weight, scaling-factor, scheduling, stream-type, "
        "buffer-pool-name, active-queue-management");
  }

  // Parse the queue ID (first argument). The true upper bound is ASIC
  // dependent; utils::kMaxQueueId is the shared arbitrary-but-high limit.
  queueId_ = folly::to<int16_t>(v[0]);
  if (queueId_ < 0 || queueId_ > utils::kMaxQueueId) {
    throw std::invalid_argument(
        fmt::format(
            "Queue ID must be between 0 and {}, got: {}",
            utils::kMaxQueueId,
            queueId_));
  }
  data_.push_back(v[0]);

  // Parse the remaining arguments
  // Most attributes are simple key-value pairs, but "active-queue-management"
  // has nested sub-attributes that consume all remaining arguments.
  for (size_t i = 1; i < v.size();) {
    const auto& attr = v[i];
    data_.push_back(attr);

    if (attr == "active-queue-management" || attr == "aqm") {
      // Everything after "active-queue-management" is part of the AQM config
      std::vector<std::string> aqmArgs;
      for (size_t j = i + 1; j < v.size(); ++j) {
        aqmArgs.push_back(v[j]);
        data_.push_back(v[j]);
      }
      aqmAttributes_ = std::move(aqmArgs);
      break; // AQM consumes all remaining arguments
    }

    // Regular key-value pair
    if (i + 1 >= v.size()) {
      throw std::invalid_argument(
          fmt::format("Attribute '{}' requires a value.", attr));
    }
    const auto& value = v[i + 1];
    attributes_.emplace_back(attr, value);
    data_.push_back(value);
    i += 2;
  }
}

CmdConfigQosQueuingPolicyQueueIdTraits::RetType
CmdConfigQosQueuingPolicyQueueId::queryClient(
    const HostInfo& /* hostInfo */,
    const QueuingPolicyName& policyName,
    const ObjectArgType& config) {
  auto& session = ConfigSession::getInstance();
  auto& agentConfig = session.getAgentConfig();
  auto& switchConfig = *agentConfig.sw();

  auto& portQueueConfigs = *switchConfig.portQueueConfigs();
  int16_t queueIdVal = config.getQueueId();
  auto& configList = portQueueConfigs[policyName.getName()];

  // Edit a local copy; splice back only after all args validate, so a mid-parse
  // throw leaves the policy's existing queue config untouched.
  int existingIdx = -1;
  cfg::PortQueue work;
  for (size_t i = 0; i < configList.size(); ++i) {
    if (*configList[i].id() == queueIdVal) {
      work = configList[i];
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
    configList[existingIdx] = work;
  } else {
    configList.push_back(work);
  }

  session.saveConfig(
      cli::ServiceType::AGENT, cli::ConfigActionLevel::AGENT_WARMBOOT);

  return fmt::format(
      "Successfully configured queuing-policy '{}' queue-id {}",
      policyName.getName(),
      queueIdVal);
}

void CmdConfigQosQueuingPolicyQueueId::printOutput(const RetType& logMsg) {
  std::cout << logMsg << std::endl;
}

// Explicit template instantiation
template void CmdHandler<
    CmdConfigQosQueuingPolicyQueueId,
    CmdConfigQosQueuingPolicyQueueIdTraits>::run();

} // namespace facebook::fboss
