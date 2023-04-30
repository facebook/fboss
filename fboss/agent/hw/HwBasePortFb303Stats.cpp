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

#include "fboss/agent/hw/StatsConstants.h"

#include <folly/logging/xlog.h>

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

int64_t HwBasePortFb303Stats::getCounterLastIncrement(
    folly::StringPiece statKey) const {
  return portCounters_.getCounterLastIncrement(statKey.str());
}

void HwBasePortFb303Stats::reinitStats(std::optional<std::string> oldPortName) {
  XLOG(DBG2) << "Reinitializing stats for " << portName_;

  for (auto statKey : kPortStatKeys()) {
    reinitStat(statKey, portName_, oldPortName);
  }
  for (auto queueIdAndName : queueId2Name_) {
    for (auto statKey : kQueueStatKeys()) {
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
  }
  if (macsecStatsInited_) {
    reinitMacsecStats(oldPortName);
  }
  // Init per priority PFC stats
  for (auto pfcPriority : enabledPfcPriorities_) {
    for (auto statKey : kPfcStatKeys()) {
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
    for (auto statKey : kPfcStatKeys()) {
      auto newStatName = statName(statKey, portName_);
      std::optional<std::string> oldStatName = oldPortName
          ? std::optional<std::string>(statName(statKey, *oldPortName))
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
  reinitStats(kInMacsecPortStatKeys());
  reinitStats(kOutMacsecPortStatKeys());

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
  for (auto statKey : kQueueStatKeys()) {
    reinitStat(statKey, queueId, oldQueueName);
  }
}

void HwBasePortFb303Stats::queueRemoved(int queueId) {
  auto qitr = queueId2Name_.find(queueId);
  for (auto statKey : kQueueStatKeys()) {
    portCounters_.removeStat(
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
    for (auto statKey : kPfcStatKeys()) {
      portCounters_.removeStat(statName(statKey, portName_, pfcPriority));
    }
  }
  enabledPfcPriorities_ = std::move(enabledPriorities);

  for (auto pfcPriority : enabledPfcPriorities_) {
    for (auto statKey : kPfcStatKeys()) {
      portCounters_.reinitStat(
          statName(statKey, portName_, pfcPriority), std::nullopt);
    }
  }
  if (enabledPfcPriorities_.size()) {
    // If PFC is enabled for priorities, init aggregated port
    // PFC counters as well.
    for (auto statKey : kPfcStatKeys()) {
      portCounters_.reinitStat(statName(statKey, portName_), std::nullopt);
    }
  } else {
    for (auto statKey : kPfcStatKeys()) {
      portCounters_.removeStat(statName(statKey, portName_));
    }
  }
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
} // namespace facebook::fboss
