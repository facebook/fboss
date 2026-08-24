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
#include <algorithm>
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
#include "fboss/cli/fboss2/utils/PortQueueConfigUtils.h"

namespace facebook::fboss {

namespace {

// Sub-command tokens accepted after `config copp queue <id>`.
constexpr std::string_view kSubCmdName = "name";
constexpr std::string_view kSubCmdRateLimit = "rate-limit";
constexpr std::string_view kRateUnitKbps = "kbps";
constexpr std::string_view kRateUnitPps = "pps";

using copp_queue::parseQueueId;

// Literal tokens accepted in
// `config copp reason <reason-name> queue <id> [order <n>]`.
constexpr std::string_view kSubCmdQueue = "queue";
constexpr std::string_view kSubCmdOrder = "order";

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

void applyRateLimit(
    cfg::PortQueue& q,
    CoppQueueArgs::RateUnit unit,
    int32_t max) {
  cfg::Range range;
  range.minimum() = 0;
  range.maximum() = max;
  cfg::PortQueueRate rate;
  if (unit == CoppQueueArgs::RateUnit::KBPS) {
    rate.kbitsPerSec() = range;
  } else {
    rate.pktsPerSec() = range;
  }
  q.portQueueRate() = rate;
}

// The SAI scheduler profile carries the WRR weight as a uint8
// (SaiSchedulerManager::makeSchedulerAttributes), so anything outside
// [1, 255] cannot be programmed. utils::applyPortQueueConfig only checks
// non-negativity, so enforce the CPU-queue cap here.
constexpr int32_t kMinWrrWeight = 1;
constexpr int32_t kMaxWrrWeight = 255;

} // namespace

CoppQueueArgs::CoppQueueArgs(std::vector<std::string> v) {
  if (v.empty()) {
    throw std::invalid_argument(
        "Expected <id> [<attr> <value> ...] where <attr> is one of: "
        "name <string>, rate-limit <kbps|pps> <max>, " +
        utils::validQueueAttrs());
  }
  queueId_ = parseQueueId(v[0], "queue");

  // Same walk as utils::QueueIdAndAttributes: <attr> <value> pairs, with two
  // copp-specific extras (name, rate-limit — the latter consumes unit + max)
  // and active-queue-management consuming every remaining token. Attribute
  // names destined for utils::applyPortQueueConfig are validated there.
  for (size_t i = 1; i < v.size();) {
    const auto& attr = v[i];
    if (attr == "active-queue-management" || attr == "aqm") {
      aqmAttributes_.assign(v.begin() + i + 1, v.end());
      break;
    }
    if (attr == kSubCmdName) {
      if (i + 1 >= v.size() || v[i + 1].empty()) {
        throw std::invalid_argument("name <string> must be non-empty");
      }
      name_ = v[i + 1];
      i += 2;
      continue;
    }
    if (attr == kSubCmdRateLimit) {
      if (i + 2 >= v.size()) {
        throw std::invalid_argument(
            fmt::format(
                "'rate-limit' requires '{}|{}' <max>",
                kRateUnitKbps,
                kRateUnitPps));
      }
      const auto& unit = v[i + 1];
      RateUnit rateUnit{};
      if (unit == kRateUnitKbps) {
        rateUnit = RateUnit::KBPS;
      } else if (unit == kRateUnitPps) {
        rateUnit = RateUnit::PPS;
      } else {
        throw std::invalid_argument(
            fmt::format(
                "rate-limit unit must be '{}' or '{}', got '{}'",
                kRateUnitKbps,
                kRateUnitPps,
                unit));
      }
      rateLimit_ = {rateUnit, parseRateMax(v[i + 2], unit)};
      i += 3;
      continue;
    }
    if (i + 1 >= v.size()) {
      throw std::invalid_argument(
          fmt::format("Attribute '{}' requires a value.", attr));
    }
    attributes_.emplace_back(attr, v[i + 1]);
    i += 2;
  }

  data_ = std::move(v);
}

CoppReasonArgs::CoppReasonArgs(std::vector<std::string> v) {
  if (v.size() != 3 && v.size() != 5) {
    throw std::invalid_argument(
        fmt::format(
            "Expected <reason-name> {} <id> [{} <n>], got {} argument(s)",
            kSubCmdQueue,
            kSubCmdOrder,
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
  if (v.size() == 5) {
    if (v[3] != kSubCmdOrder) {
      throw std::invalid_argument(
          fmt::format(
              "Expected '{}' after the queue id, got '{}'",
              kSubCmdOrder,
              v[3]));
    }
    int32_t parsed = 0;
    try {
      parsed = folly::to<int32_t>(v[4]);
    } catch (const folly::ConversionError&) {
      throw std::invalid_argument(
          fmt::format("order value must be an integer, got '{}'", v[4]));
    }
    if (parsed < 0) {
      throw std::invalid_argument(
          fmt::format("order value must be non-negative, got {}", parsed));
    }
    order_ = static_cast<size_t>(parsed);
  }
  data_ = std::move(v);
}

namespace {

std::string applyCpuQueueConfig(
    cfg::SwitchConfig& swConfig,
    const CoppQueueArgs& args) {
  auto& q = findOrCreateCpuQueue(swConfig, args.getQueueId());
  if (!args.hasEdits()) {
    return fmt::format("Ensured queue {} exists", args.getQueueId());
  }
  // Apply onto a copy so a rejected attribute leaves the queue untouched
  // (same transactional pattern as the qos queue-config commands).
  auto work = q;
  if (args.getName().has_value()) {
    work.name() = *args.getName();
  }
  if (args.getRateLimit().has_value()) {
    applyRateLimit(
        work, args.getRateLimit()->first, args.getRateLimit()->second);
  }
  if (!args.getAttributes().empty() || !args.getAqmAttributes().empty()) {
    utils::applyPortQueueConfig(
        work, args.getAttributes(), args.getAqmAttributes());
    for (const auto& [attr, value] : args.getAttributes()) {
      if (attr == "weight" &&
          (*work.weight() < kMinWrrWeight || *work.weight() > kMaxWrrWeight)) {
        throw std::invalid_argument(
            fmt::format(
                "weight must be in [{}, {}], got {}",
                kMinWrrWeight,
                kMaxWrrWeight,
                *work.weight()));
      }
    }
  }
  q = std::move(work);
  // Echo the applied tokens back (everything after <id>).
  const auto tokens = args.data();
  return fmt::format(
      "Updated queue {}: {}",
      args.getQueueId(),
      folly::join(" ", tokens.begin() + 1, tokens.end()));
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

  auto it = std::find_if(
      list.begin(), list.end(), [&args](const cfg::PacketRxReasonToQueue& e) {
        return *e.rxReason() == args.getReason();
      });
  const bool existed = it != list.end();

  cfg::PacketRxReasonToQueue entry;
  entry.rxReason() = args.getReason();
  entry.queueId() = args.getQueueId();

  if (!args.getOrder().has_value()) {
    // No order given: an existing reason keeps its position, a new one
    // appends.
    if (existed) {
      *it = entry;
      return fmt::format(
          "Updated reason {} -> queue {}", reasonName, args.getQueueId());
    }
    list.push_back(entry);
    return fmt::format(
        "Mapped reason {} -> queue {}", reasonName, args.getQueueId());
  }

  // The list is position-sensitive; place the entry at the requested 0-based
  // index. The index refers to the final list, so an existing entry doesn't
  // count towards the bound (it is removed before reinsertion).
  const size_t order = *args.getOrder();
  const size_t maxOrder = existed ? list.size() - 1 : list.size();
  if (order > maxOrder) {
    throw std::invalid_argument(
        fmt::format(
            "order {} is out of range: highest valid position is {}",
            order,
            maxOrder));
  }
  if (existed) {
    list.erase(it);
  }
  list.insert(list.begin() + order, entry);
  return fmt::format(
      "{} reason {} -> queue {} at position {}",
      existed ? "Updated" : "Mapped",
      reasonName,
      args.getQueueId(),
      order);
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
