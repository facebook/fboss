/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/delete/copp/cpu_queue/CmdDeleteCoppCpuQueue.h"

#include "fboss/cli/fboss2/CmdHandler.cpp"

#include <fmt/format.h>
#include <folly/String.h>
#include <thrift/lib/cpp/util/EnumUtils.h>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/cli/fboss2/commands/config/copp/CoppUtils.h"
#include "fboss/cli/fboss2/gen-cpp2/cli_metadata_types.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

namespace facebook::fboss {

namespace {

// Collect human-readable descriptions of every config entry that still
// points at cpu-queue `queueId`: rxReason -> queue mappings (both the
// ordered list and the deprecated map form) and matchToAction actions
// carrying a queue id (send-to-queue, user-defined-trap).
std::vector<std::string> findQueueReferences(
    const cfg::SwitchConfig& swConfig,
    int16_t queueId) {
  std::vector<std::string> references;
  if (!swConfig.cpuTrafficPolicy().has_value()) {
    return references;
  }
  const auto& policy = *swConfig.cpuTrafficPolicy();
  if (policy.rxReasonToQueueOrderedList().has_value()) {
    for (const auto& entry : *policy.rxReasonToQueueOrderedList()) {
      if (*entry.queueId() == queueId) {
        references.push_back(
            fmt::format(
                "reason {}",
                apache::thrift::util::enumNameSafe(*entry.rxReason())));
      }
    }
  }
  // The deprecated rxReasonToCPUQueue map is still honored by the agent as
  // a fallback when the ordered list is unset, so a queue referenced only
  // there is still live.
  if (policy.rxReasonToCPUQueue().has_value()) {
    for (const auto& [rxReason, mappedQueueId] : *policy.rxReasonToCPUQueue()) {
      if (mappedQueueId == queueId) {
        references.push_back(
            fmt::format(
                "reason {} (deprecated rxReasonToCPUQueue map)",
                apache::thrift::util::enumNameSafe(rxReason)));
      }
    }
  }
  if (policy.trafficPolicy().has_value()) {
    for (const auto& mta : *policy.trafficPolicy()->matchToAction()) {
      const auto& action = *mta.action();
      if (action.sendToQueue().has_value() &&
          *action.sendToQueue()->queueId() == queueId) {
        references.push_back(
            fmt::format("matcher '{}' send-to-queue", *mta.matcher()));
      }
      if (action.userDefinedTrap().has_value() &&
          *action.userDefinedTrap()->queueId() == queueId) {
        references.push_back(
            fmt::format("matcher '{}' user-defined-trap", *mta.matcher()));
      }
    }
  }
  return references;
}

} // namespace

CoppCpuQueueDeleteArgs::CoppCpuQueueDeleteArgs(std::vector<std::string> v) {
  if (v.size() != 1) {
    throw std::invalid_argument(
        fmt::format("Expected exactly one <id>, got {}", v.size()));
  }
  queueId_ = copp_cpu_queue::parseQueueId(v[0], "cpu-queue");
  data_ = std::move(v);
}

CmdDeleteCoppCpuQueueTraits::RetType CmdDeleteCoppCpuQueue::queryClient(
    const HostInfo& /* hostInfo */,
    const ObjectArgType& args) {
  auto& session = ConfigSession::getInstance();
  auto& swConfig = *session.getAgentConfig().sw();

  auto& queues = *swConfig.cpuQueues();
  const auto queueId = args.getQueueId();
  auto it = copp_cpu_queue::findCpuQueue(queues, queueId);
  if (it == queues.end()) {
    throw std::runtime_error(
        fmt::format("No cpu-queue {} in the config", queueId));
  }

  const auto references = findQueueReferences(swConfig, queueId);
  if (!references.empty()) {
    throw std::runtime_error(
        fmt::format(
            "Cannot delete cpu-queue {}: still referenced by {}. "
            "Delete those references first.",
            queueId,
            folly::join(", ", references)));
  }
  queues.erase(it);

  session.saveConfig(cli::ServiceType::AGENT, cli::ConfigActionLevel::HITLESS);
  return fmt::format("Deleted cpu-queue {}", queueId);
}

void CmdDeleteCoppCpuQueue::printOutput(const RetType& logMsg) {
  std::cout << logMsg << std::endl;
}

// Explicit template instantiation
template void
CmdHandler<CmdDeleteCoppCpuQueue, CmdDeleteCoppCpuQueueTraits>::run();

} // namespace facebook::fboss
