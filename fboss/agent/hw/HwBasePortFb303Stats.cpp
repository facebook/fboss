/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/HwBasePortFb303Stats.h"

#include "fboss/agent/gen-cpp2/switch_config_constants.h"
#include "fboss/agent/hw/CounterUtils.h"
#include "fboss/agent/hw/StatsConstants.h"

#include <fb303/ServiceData.h>
#include <folly/logging/xlog.h>

#include "common/stats/DynamicStats.h"

namespace {

DEFINE_dynamic_quantile_stat(
    buffer_watermark_ucast,
    "buffer_watermark_ucast.{}.{}.{}",
    facebook::fb303::ExportTypeConsts::kNone,
    std::array<double, 1>{{1.0}});

DEFINE_dynamic_quantile_stat(
    egress_gvoq_watermark,
    "egress_gvoq_watermark.{}.{}.{}",
    facebook::fb303::ExportTypeConsts::kNone,
    std::array<double, 1>{{1.0}});

} // unnamed namespace

namespace facebook::fboss {

std::string HwBasePortFb303Stats::statName(
    folly::StringPiece statName,
    folly::StringPiece portName) {
  return folly::to<std::string>(portName, ".", statName);
}

std::string HwBasePortFb303Stats::statName(
    folly::StringPiece statName,
    folly::StringPiece portName,
    int queueId,
    folly::StringPiece queueName) {
  return folly::to<std::string>(
      portName, ".", "queue", queueId, ".", queueName, ".", statName);
}

std::string HwBasePortFb303Stats::statName(
    folly::StringPiece statName,
    folly::StringPiece portName,
    PfcPriority priority) {
  return folly::to<std::string>(portName, ".", statName, ".priority", priority);
}

std::string HwBasePortFb303Stats::pgStatName(
    folly::StringPiece statName,
    folly::StringPiece portName,
    int priority) {
  return folly::to<std::string>(portName, ".", statName, ".pg", priority);
}

int64_t HwBasePortFb303Stats::getCounterLastIncrement(
    folly::StringPiece statKey,
    std::optional<int64_t> defaultVal) const {
  return portCounters_.getCounterLastIncrement(statKey.str(), defaultVal);
}

void HwBasePortFb303Stats::reinitStats(std::optional<std::string> oldPortName) {
  XLOG(DBG2) << "Reinitializing stats for " << portName_;

  for (auto statKey : kPortMonotonicCounterStatKeys()) {
    reinitStat(statKey, portName_, oldPortName);
  }
  for (auto statKey : kPortFb303CounterStatKeys()) {
    // For fb303 - init will happen on the next set. So here we
    // just delete the counter
    utility::deleteCounter(statName(statKey, oldPortName.value_or("")));
  }
  for (auto queueIdAndName : queueId2Name_) {
    for (auto statKey : kQueueMonotonicCounterStatKeys()) {
      auto newStatName = statName(
          statKey, portName_, queueIdAndName.first, queueIdAndName.second);
      std::optional<std::string> oldStatName = oldPortName
          ? std::optional<std::string>(statName(
                statKey,
                *oldPortName,
                queueIdAndName.first,
                queueIdAndName.second))
          : std::nullopt;
      portCounters_.reinitStat(newStatName, oldStatName);
    }
    for (auto statKey : kQueueFb303CounterStatKeys()) {
      utility::deleteCounter(statName(
          statKey,
          oldPortName.value_or(""),
          queueIdAndName.first,
          queueIdAndName.second));
    }
  }
  if (macsecStatsInited_) {
    reinitMacsecStats(oldPortName);
  }
  // Init per priority PFC stats
  for (auto pfcPriority : enabledPfcPriorities_) {
    for (auto statKey : kPfcMonotonicCounterStatKeys()) {
      auto newStatName = statName(statKey, portName_, pfcPriority);
      std::optional<std::string> oldStatName = oldPortName
          ? std::optional<std::string>(
                statName(statKey, *oldPortName, pfcPriority))
          : std::nullopt;
      portCounters_.reinitStat(newStatName, oldStatName);
    }
  }
  if (enabledPfcPriorities_.size()) {
    // If PFC is enabled for priorities, init aggregated port
    // PFC counters as well.
    for (auto statKey : kPfcMonotonicCounterStatKeys()) {
      auto newStatName = statName(statKey, portName_);
      std::optional<std::string> oldStatName = oldPortName
          ? std::optional<std::string>(statName(statKey, *oldPortName))
          : std::nullopt;
      portCounters_.reinitStat(newStatName, oldStatName);
    }
  }
  for (int i = 0; i <= cfg::switch_config_constants::PORT_PG_VALUE_MAX(); ++i) {
    for (auto statKey : kPriorityGroupCounterStatKeys()) {
      auto newStatName = pgStatName(statKey, portName_, i);
      std::optional<std::string> oldStatName = oldPortName
          ? std::optional<std::string>(pgStatName(statKey, *oldPortName, i))
          : std::nullopt;
      portCounters_.reinitStat(newStatName, oldStatName);
    }
  }
}

/*
 * Reinit macsec stats
 */
void HwBasePortFb303Stats::reinitMacsecStats(
    std::optional<std::string> oldPortName) {
  auto reinitStats = [this, &oldPortName](const auto& keys) {
    for (auto statKey : keys) {
      reinitStat(statKey, portName_, oldPortName);
    }
  };
  reinitStats(kInMacsecPortMonotonicCounterStatKeys());
  reinitStats(kOutMacsecPortMonotonicCounterStatKeys());

  macsecStatsInited_ = true;
}
/*
 * Reinit port stat
 */
void HwBasePortFb303Stats::reinitStat(
    folly::StringPiece statKey,
    const std::string& portName,
    std::optional<std::string> oldPortName) {
  portCounters_.reinitStat(
      statName(statKey, portName),
      oldPortName ? std::optional<std::string>(statName(statKey, *oldPortName))
                  : std::nullopt);
}

/*
 * Reinit port queue stat
 */
void HwBasePortFb303Stats::reinitStat(
    folly::StringPiece statKey,
    int queueId,
    std::optional<std::string> oldQueueName) {
  portCounters_.reinitStat(
      statName(statKey, portName_, queueId, queueId2Name_[queueId]),
      oldQueueName ? std::optional<std::string>(
                         statName(statKey, portName_, queueId, *oldQueueName))
                   : std::nullopt);
}

void HwBasePortFb303Stats::queueChanged(
    int queueId,
    const std::string& queueName) {
  auto qitr = queueId2Name_.find(queueId);
  std::optional<std::string> oldQueueName = qitr == queueId2Name_.end()
      ? std::nullopt
      : std::optional<std::string>(qitr->second);
  queueId2Name_[queueId] = queueName;
  for (auto statKey : kQueueMonotonicCounterStatKeys()) {
    reinitStat(statKey, queueId, oldQueueName);
  }
  for (auto statKey : kQueueFb303CounterStatKeys()) {
    utility::deleteCounter(
        statName(statKey, portName_, queueId, queueId2Name_[queueId]));
  }
}

void HwBasePortFb303Stats::queueRemoved(int queueId) {
  for (auto statKey : kQueueMonotonicCounterStatKeys()) {
    portCounters_.removeStat(
        statName(statKey, portName_, queueId, queueId2Name_[queueId]));
  }
  for (auto statKey : kQueueFb303CounterStatKeys()) {
    utility::deleteCounter(
        statName(statKey, portName_, queueId, queueId2Name_[queueId]));
  }
  queueId2Name_.erase(queueId);
}

void HwBasePortFb303Stats::pfcPriorityChanged(
    std::vector<PfcPriority> enabledPriorities) {
  if (enabledPfcPriorities_ == enabledPriorities) {
    // No change in priorities
    return;
  }

  for (auto& pfcPriority : enabledPfcPriorities_) {
    // Remove old priorities stats
    for (auto statKey : kPfcMonotonicCounterStatKeys()) {
      portCounters_.removeStat(statName(statKey, portName_, pfcPriority));
    }
  }
  enabledPfcPriorities_ = std::move(enabledPriorities);

  for (auto pfcPriority : enabledPfcPriorities_) {
    for (auto statKey : kPfcMonotonicCounterStatKeys()) {
      portCounters_.reinitStat(
          statName(statKey, portName_, pfcPriority), std::nullopt);
    }
  }
  if (enabledPfcPriorities_.size()) {
    // If PFC is enabled for priorities, init aggregated port
    // PFC counters as well.
    for (auto statKey : kPfcMonotonicCounterStatKeys()) {
      portCounters_.reinitStat(statName(statKey, portName_), std::nullopt);
    }
  } else {
    for (auto statKey : kPfcMonotonicCounterStatKeys()) {
      portCounters_.removeStat(statName(statKey, portName_));
    }
  }
}

void HwBasePortFb303Stats::updateLeakyBucketFlapCnt(int cnt) {
  auto now = duration_cast<std::chrono::seconds>(
      std::chrono::system_clock::now().time_since_epoch());
  updateStat(now, kLeakyBucketFlapCnt(), cnt);
}

void HwBasePortFb303Stats::updateStat(
    const std::chrono::seconds& now,
    folly::StringPiece statKey,
    int queueId,
    int64_t val) {
  portCounters_.updateStat(
      now, statName(statKey, portName_, queueId, queueId2Name_[queueId]), val);
}

void HwBasePortFb303Stats::updateStat(
    const std::chrono::seconds& now,
    folly::StringPiece statKey,
    int64_t val) {
  portCounters_.updateStat(now, statName(statKey, portName_), val);
}

void HwBasePortFb303Stats::updateStat(
    const std::chrono::seconds& now,
    folly::StringPiece statKey,
    PfcPriority priority,
    int64_t val) {
  portCounters_.updateStat(now, statName(statKey, portName_, priority), val);
}

void HwBasePortFb303Stats::updatePgStat(
    const std::chrono::seconds& now,
    folly::StringPiece statKey,
    int pg,
    int64_t val) {
  portCounters_.updateStat(now, pgStatName(statKey, portName_, pg), val);
}

void HwBasePortFb303Stats::updateQueueWatermarkStats(
    const std::map<int16_t, int64_t>& queueWatermarks) const {
  for (const auto& [queueId, queueName] : queueId2Name_) {
    auto watermarkItr = queueWatermarks.find(queueId);
    CHECK(watermarkItr != queueWatermarks.end());
    STATS_buffer_watermark_ucast.addValue(
        watermarkItr->second,
        portName_,
        folly::to<std::string>("queue", queueId),
        queueName);
  }
}

void HwBasePortFb303Stats::updateEgressGvoqWatermarkStats(
    const std::map<int16_t, int64_t>& gvoqWatermarks) const {
  for (const auto& [queueId, queueName] : queueId2Name_) {
    auto watermarkItr = gvoqWatermarks.find(queueId);
    CHECK(watermarkItr != gvoqWatermarks.end());
    STATS_egress_gvoq_watermark.addValue(
        watermarkItr->second,
        portName_,
        folly::to<std::string>("queue", queueId),
        queueName);
  }
}
} // namespace facebook::fboss
