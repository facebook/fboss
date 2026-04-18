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
#include <folly/String.h>
#include <thrift/lib/cpp/util/EnumUtils.h>
#include <cctype>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include "fboss/cli/fboss2/gen-cpp2/cli_metadata_types.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

namespace facebook::fboss {

namespace {

// Sub-command tokens accepted after `config copp cpu-queue <id>`.
constexpr std::string_view kSubCmdName = "name";
constexpr std::string_view kSubCmdRateLimit = "rate-limit";
constexpr std::string_view kRateUnitKbps = "kbps";
constexpr std::string_view kRateUnitPps = "pps";

// Literal tokens accepted between the reason name and queue id in
// `config copp reason <reason-name> queue <id>`.
constexpr std::string_view kSubCmdQueue = "queue";

// CPU queue IDs are a small platform-bounded set; reject anything that is
// clearly out of range before we construct a PortQueue. The actual per-ASIC
// cap is enforced by the agent (SaiHostifManager::getMaxCpuQueues) at apply
// time.
constexpr int16_t kMaxCpuQueueId = 255;

// Normalize a user-typed reason name: uppercase + dashes->underscores, so
// that "arp", "ARP", "bgp-v6", "bgpv6" all match the cfg::PacketRxReason
// enum names ("ARP", "BGPV6", ...).
std::string normalizeReason(const std::string& v) {
  std::string out;
  out.reserve(v.size());
  for (unsigned char c : v) {
    out.push_back(c == '-' ? '_' : std::toupper(c));
  }
  return out;
}

std::string validReasonNames() {
  std::vector<std::string> names;
  for (auto value : apache::thrift::TEnumTraits<cfg::PacketRxReason>::values) {
    names.push_back(apache::thrift::util::enumNameSafe(value));
  }
  return folly::join(", ", names);
}

int16_t parseQueueId(const std::string& s, std::string_view context) {
  int16_t parsed = 0;
  try {
    parsed = folly::to<int16_t>(s);
  } catch (const folly::ConversionError&) {
    throw std::invalid_argument(
        fmt::format("Queue ID ({}) must be an integer, got '{}'", context, s));
  }
  if (parsed < 0 || parsed > kMaxCpuQueueId) {
    throw std::invalid_argument(
        fmt::format(
            "Queue ID ({}) must be in [0, {}], got {}",
            context,
            kMaxCpuQueueId,
            parsed));
  }
  return parsed;
}

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
  for (auto& q : queues) {
    if (*q.id() == id) {
      return q;
    }
  }
  cfg::PortQueue q;
  q.id() = id;
  queues.push_back(q);
  return queues.back();
}

void applyRateLimit(cfg::PortQueue& q, CoppCpuQueueArgs::Op op, int32_t max) {
  cfg::Range range;
  range.minimum() = 0;
  range.maximum() = max;
  cfg::PortQueueRate rate;
  if (op == CoppCpuQueueArgs::Op::RATE_LIMIT_KBPS) {
    rate.kbitsPerSec() = range;
  } else {
    rate.pktsPerSec() = range;
  }
  q.portQueueRate() = rate;
}

} // namespace

CoppCpuQueueArgs::CoppCpuQueueArgs(std::vector<std::string> v) {
  if (v.empty()) {
    throw std::invalid_argument(
        "Expected <id> [<sub-cmd> <value>] where <sub-cmd> is one of: "
        "name <string>, rate-limit kbps <max>, rate-limit pps <max>");
  }
  queueId_ = parseQueueId(v[0], "cpu-queue");

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
            "Unknown cpu-queue sub-command '{}'. Valid: {}, {}",
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
  cfg::PacketRxReason reason{};
  if (!apache::thrift::TEnumTraits<cfg::PacketRxReason>::findValue(
          normalizeReason(v[0]), &reason)) {
    throw std::invalid_argument(
        fmt::format(
            "Unknown reason name '{}'. Valid: {}", v[0], validReasonNames()));
  }
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
    const CoppCpuQueueArgs& args) {
  auto& q = findOrCreateCpuQueue(swConfig, args.getQueueId());
  switch (args.getOp()) {
    case CoppCpuQueueArgs::Op::NONE:
      return fmt::format("Ensured cpu-queue {} exists", args.getQueueId());
    case CoppCpuQueueArgs::Op::NAME:
      q.name() = args.getName();
      return fmt::format(
          "Set cpu-queue {} name to '{}'", args.getQueueId(), args.getName());
    case CoppCpuQueueArgs::Op::RATE_LIMIT_KBPS:
      applyRateLimit(q, args.getOp(), args.getRateMax());
      return fmt::format(
          "Set cpu-queue {} rate-limit kbps max to {}",
          args.getQueueId(),
          args.getRateMax());
    case CoppCpuQueueArgs::Op::RATE_LIMIT_PPS:
      applyRateLimit(q, args.getOp(), args.getRateMax());
      return fmt::format(
          "Set cpu-queue {} rate-limit pps max to {}",
          args.getQueueId(),
          args.getRateMax());
  }
  throw std::runtime_error("Unhandled cpu-queue op");
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

CmdConfigCoppCpuQueueTraits::RetType CmdConfigCoppCpuQueue::queryClient(
    const HostInfo& /* hostInfo */,
    const ObjectArgType& args) {
  auto& session = ConfigSession::getInstance();
  auto msg = applyCpuQueueConfig(*session.getAgentConfig().sw(), args);
  // cpuQueues edits are processed hitlessly by
  // SaiHostifManager::processHostifDelta -> changeCpuQueue -> QueueManager.
  session.saveConfig(cli::ServiceType::AGENT, cli::ConfigActionLevel::HITLESS);
  return msg;
}

void CmdConfigCoppCpuQueue::printOutput(const RetType& logMsg) {
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
template void
CmdHandler<CmdConfigCoppCpuQueue, CmdConfigCoppCpuQueueTraits>::run();
template void CmdHandler<CmdConfigCoppReason, CmdConfigCoppReasonTraits>::run();

} // namespace facebook::fboss
