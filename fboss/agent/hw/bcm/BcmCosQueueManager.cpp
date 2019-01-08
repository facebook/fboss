/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmCosQueueManager.h"

namespace facebook { namespace fboss {


void BcmCosQueueManager::fillOrReplaceCounter(
    const BcmCosQueueCounterType& type,
    QueueStatCounters& counters,
    const folly::Optional<QueueConfig>& queueConfig) {
  if (type.isScopeQueues()) {
    // TODO(joseph5wu) Right now, we create counters based on queue size, we
    // could have created counters only for the queues we set in the config.
    for (int queue = 0; queue < getNumQueues(type.streamType); queue++) {
      auto countersItr = counters.queues.find(queue);
      /*
       * if a queue name is specified in the config, use it in the statName,
       * else just use queue ID.
       */
      std::string name;

      if (queueConfig.hasValue() &&
          queueConfig.value().at(queue)->getName().hasValue()) {
        name = folly::to<std::string>(
            portName_,
            ".queue",
            queue,
            ".",
            queueConfig.value().at(queue)->getName().value(),
            ".",
            type.name);
      } else {
        name =
            folly::to<std::string>(portName_, ".queue", queue, ".", type.name);
      }

      // if counter already exists, swap it with a new counter with new name
      if (countersItr != counters.queues.end()) {
        facebook::stats::MonotonicCounter newCounter{
          name, stats::SUM, stats::RATE};
        countersItr->second->swap(newCounter);
      }
      else {
        counters.queues.emplace(
            queue,
            std::make_unique<facebook::stats::MonotonicCounter>(
                name, stats::SUM, stats::RATE));
      }
    }
  }
  if (type.isScopeAggregated()) {
    counters.aggregated = std::make_unique<facebook::stats::MonotonicCounter>(
        folly::to<std::string>(portName_, ".", type.name),
        stats::SUM,
        stats::RATE);
  }
}

void BcmCosQueueManager::setupQueueCounter(
    const BcmCosQueueCounterType& type,
    const folly::Optional<QueueConfig>& queueConfig) {
  // create counters for each type for each queue
  auto curCountersItr = queueCounters_.find(type);
  if (curCountersItr != queueCounters_.end()) {
    auto& counters = curCountersItr->second;
    fillOrReplaceCounter(type, counters, queueConfig);
  } else {
    QueueStatCounters counters;
    fillOrReplaceCounter(type, counters, queueConfig);
    queueCounters_.emplace(type, std::move(counters));
  }
}

void BcmCosQueueManager::setupQueueCounters(
    const folly::Optional<QueueConfig>& queueConfig) {
  for (const auto& type : getQueueCounterTypes()) {
    setupQueueCounter(type, queueConfig);
  }
}

void BcmCosQueueManager::updateQueueStats(
    std::chrono::seconds now,
    HwPortStats* portStats) {
  for (const auto& cntr: queueCounters_) {
    if (cntr.first.isScopeQueues()) {
      for (auto& countersItr: cntr.second.queues) {
        updateQueueStat(
            countersItr.first,
            cntr.first,
            countersItr.second.get(),
            now,
            portStats);
      }
    }
    if (cntr.first.isScopeAggregated()) {
      updateQueueAggregatedStat(
          cntr.first, cntr.second.aggregated.get(), now, portStats);
    }
  }
}
}} // facebook::fboss
