/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/copp/CmdConfigCopp.h"

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/cli/fboss2/CmdHandler.cpp"

#include <fmt/format.h>
#include <folly/Conv.h>
#include <thrift/lib/cpp/util/EnumUtils.h>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include "fboss/cli/fboss2/commands/config/copp/CoppUtils.h"
#include "fboss/cli/fboss2/gen-cpp2/cli_metadata_types.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

namespace facebook::fboss {

namespace {

// Sub-command tokens accepted after `config copp queue <id>`.
constexpr std::string_view kSubCmdName = "name";
constexpr std::string_view kSubCmdRateLimit = "rate-limit";
constexpr std::string_view kRateUnitKbps = "kbps";
constexpr std::string_view kRateUnitPps = "pps";

using copp_queue::parseQueueId;

// Literal tokens accepted between the reason name and queue id in
// `config copp reason <reason-name> queue <id>`.
constexpr std::string_view kSubCmdQueue = "queue";

int32_t parseRateMax(const std::string& s, std::string_view unit) {
  int32_t parsed = 0;
  try {
    parsed = folly::to<int32_t>(s);
  } catch (const folly::ConversionError&) {
    throw std::invalid_argument(
        fmt::format(
            "rate-limit {} value must be an integer, got '{}'", unit, s));
  }
  if (parsed < 0) {
    throw std::invalid_argument(
        fmt::format(
            "rate-limit {} value must be non-negative, got {}", unit, parsed));
  }
  return parsed;
}

// Find the cpuQueues[] entry with matching id, or append a fresh one.
// A new entry uses thrift defaults (streamType=UNICAST, unset weight); this
// is only reached when the user targets a queue id that does not exist yet
// in the session config, which is rare in practice — platforms ship with a
// populated cpuQueues list.
cfg::PortQueue& findOrCreateCpuQueue(cfg::SwitchConfig& swConfig, int16_t id) {
  auto& queues = *swConfig.cpuQueues();
  auto it = copp_queue::findQueue(queues, id);
  if (it != queues.end()) {
    return *it;
  }
  cfg::PortQueue q;
  q.id() = id;
  queues.push_back(q);
  return queues.back();
}

void applyRateLimit(cfg::PortQueue& q, CoppQueueArgs::Op op, int32_t max) {
  cfg::Range range;
  range.minimum() = 0;
  range.maximum() = max;
  cfg::PortQueueRate rate;
  if (op == CoppQueueArgs::Op::RATE_LIMIT_KBPS) {
    rate.kbitsPerSec() = range;
  } else {
    rate.pktsPerSec() = range;
  }
  q.portQueueRate() = rate;
}

} // namespace

CoppQueueArgs::CoppQueueArgs(std::vector<std::string> v) {
  if (v.empty()) {
    throw std::invalid_argument(
        "Expected <id> [<sub-cmd> <value>] where <sub-cmd> is one of: "
        "name <string>, rate-limit kbps <max>, rate-limit pps <max>");
  }
  queueId_ = parseQueueId(v[0], "queue");

  if (v.size() == 1) {
    op_ = Op::NONE;
  } else if (v[1] == kSubCmdName) {
    if (v.size() != 3) {
      throw std::invalid_argument(
          fmt::format(
              "'name' requires exactly one <string> value, got {} arg(s)",
              v.size() - 2));
    }
    if (v[2].empty()) {
      throw std::invalid_argument("name <string> must be non-empty");
    }
    op_ = Op::NAME;
    name_ = v[2];
  } else if (v[1] == kSubCmdRateLimit) {
    if (v.size() != 4) {
      throw std::invalid_argument(
          fmt::format(
              "'rate-limit' requires '{}|{}' <max>, got {} arg(s)",
              kRateUnitKbps,
              kRateUnitPps,
              v.size() - 2));
    }
    if (v[2] == kRateUnitKbps) {
      op_ = Op::RATE_LIMIT_KBPS;
    } else if (v[2] == kRateUnitPps) {
      op_ = Op::RATE_LIMIT_PPS;
    } else {
      throw std::invalid_argument(
          fmt::format(
              "rate-limit unit must be '{}' or '{}', got '{}'",
              kRateUnitKbps,
              kRateUnitPps,
              v[2]));
    }
    rateMax_ = parseRateMax(v[3], v[2]);
  } else {
    throw std::invalid_argument(
        fmt::format(
            "Unknown queue sub-command '{}'. Valid: {}, {}",
            v[1],
            kSubCmdName,
            kSubCmdRateLimit));
  }

  data_ = std::move(v);
}

CoppReasonArgs::CoppReasonArgs(std::vector<std::string> v) {
  if (v.size() != 3) {
    throw std::invalid_argument(
        fmt::format(
            "Expected <reason-name> {} <id>, got {} argument(s)",
            kSubCmdQueue,
            v.size()));
  }
  cfg::PacketRxReason reason = copp_reason::parseReason(v[0]);
  if (v[1] != kSubCmdQueue) {
    throw std::invalid_argument(
        fmt::format(
            "Expected '{}' between reason and queue id, got '{}'",
            kSubCmdQueue,
            v[1]));
  }
  reason_ = reason;
  queueId_ = parseQueueId(v[2], "reason");
  data_ = std::move(v);
}

namespace {

std::string applyCpuQueueConfig(
    cfg::SwitchConfig& swConfig,
    const CoppQueueArgs& args) {
  auto& q = findOrCreateCpuQueue(swConfig, args.getQueueId());
  switch (args.getOp()) {
    case CoppQueueArgs::Op::NONE:
      return fmt::format("Ensured queue {} exists", args.getQueueId());
    case CoppQueueArgs::Op::NAME:
      q.name() = args.getName();
      return fmt::format(
          "Set queue {} name to '{}'", args.getQueueId(), args.getName());
    case CoppQueueArgs::Op::RATE_LIMIT_KBPS:
      applyRateLimit(q, args.getOp(), args.getRateMax());
      return fmt::format(
          "Set queue {} rate-limit kbps max to {}",
          args.getQueueId(),
          args.getRateMax());
    case CoppQueueArgs::Op::RATE_LIMIT_PPS:
      applyRateLimit(q, args.getOp(), args.getRateMax());
      return fmt::format(
          "Set queue {} rate-limit pps max to {}",
          args.getQueueId(),
          args.getRateMax());
  }
  throw std::runtime_error("Unhandled queue op");
}

std::string applyReasonConfig(
    cfg::SwitchConfig& swConfig,
    const CoppReasonArgs& args) {
  if (!swConfig.cpuTrafficPolicy().has_value()) {
    swConfig.cpuTrafficPolicy() = cfg::CPUTrafficPolicyConfig{};
  }
  auto& policy = *swConfig.cpuTrafficPolicy();
  if (!policy.rxReasonToQueueOrderedList().has_value()) {
    policy.rxReasonToQueueOrderedList() =
        std::vector<cfg::PacketRxReasonToQueue>{};
  }
  auto& list = *policy.rxReasonToQueueOrderedList();
  const auto reasonName = apache::thrift::util::enumNameSafe(args.getReason());
  for (auto& entry : list) {
    if (*entry.rxReason() == args.getReason()) {
      *entry.queueId() = args.getQueueId();
      return fmt::format(
          "Updated reason {} -> queue {}", reasonName, args.getQueueId());
    }
  }
  cfg::PacketRxReasonToQueue newEntry;
  newEntry.rxReason() = args.getReason();
  newEntry.queueId() = args.getQueueId();
  list.push_back(newEntry);
  return fmt::format(
      "Mapped reason {} -> queue {}", reasonName, args.getQueueId());
}

} // namespace

CmdConfigCoppQueueTraits::RetType CmdConfigCoppQueue::queryClient(
    const HostInfo& /* hostInfo */,
    const ObjectArgType& args) {
  auto& session = ConfigSession::getInstance();
  auto msg = applyCpuQueueConfig(*session.getAgentConfig().sw(), args);
  // cpuQueues edits are processed hitlessly by
  // SaiHostifManager::processHostifDelta -> changeCpuQueue -> QueueManager.
  session.saveConfig(cli::ServiceType::AGENT, cli::ConfigActionLevel::HITLESS);
  return msg;
}

void CmdConfigCoppQueue::printOutput(const RetType& logMsg) {
  std::cout << logMsg << std::endl;
}

CmdConfigCoppReasonTraits::RetType CmdConfigCoppReason::queryClient(
    const HostInfo& /* hostInfo */,
    const ObjectArgType& args) {
  auto& session = ConfigSession::getInstance();
  auto msg = applyReasonConfig(*session.getAgentConfig().sw(), args);
  // rxReason -> queue mapping edits are processed hitlessly by
  // SaiHostifManager::processHostifDelta (trap-group update path).
  session.saveConfig(cli::ServiceType::AGENT, cli::ConfigActionLevel::HITLESS);
  return msg;
}

void CmdConfigCoppReason::printOutput(const RetType& logMsg) {
  std::cout << logMsg << std::endl;
}

// Explicit template instantiations
template void CmdHandler<CmdConfigCopp, CmdConfigCoppTraits>::run();
template void CmdHandler<CmdConfigCoppQueue, CmdConfigCoppQueueTraits>::run();
template void CmdHandler<CmdConfigCoppReason, CmdConfigCoppReasonTraits>::run();

} // namespace facebook::fboss
