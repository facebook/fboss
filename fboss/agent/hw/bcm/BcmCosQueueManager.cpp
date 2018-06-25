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
    QueueStatCounters& counters) {
  if (type.isScopeQueues()) {
    // TODO(joseph5wu) Right now, we create counters based on queue size, we
    // could have created counters only for the queues we set in the config.
    for (int queue = 0; queue < getNumQueues(type.streamType); queue++) {
      auto countersItr = counters.queues.find(queue);
      // if counter already exists, swap it with a new counter with new name
      auto name = folly::to<std::string>(portName_, ".queue",
                                         queue, ".", type.name);
      if (countersItr != counters.queues.end()) {
        facebook::stats::MonotonicCounter newCounter{
          name, stats::SUM, stats::RATE};
        countersItr->second->swap(newCounter);
      }
      else {
        counters.queues.emplace(
          queue,
          new facebook::stats::MonotonicCounter(name, stats::SUM, stats::RATE));
      }
    }
  }
  if (type.isScopeAggregated()) {
    counters.aggregated = new facebook::stats::MonotonicCounter(
      folly::to<std::string>(portName_, ".", type.name),
      stats::SUM, stats::RATE
    );
  }
}

void BcmCosQueueManager::setupQueueCounter(const BcmCosQueueCounterType& type) {
  // create counters for each type for each queue
  auto curCountersItr = queueCounters_.find(type);
  if (curCountersItr != queueCounters_.end()) {
    auto& counters = curCountersItr->second;
    fillOrReplaceCounter(type, counters);
  } else {
    QueueStatCounters counters;
    fillOrReplaceCounter(type, counters);
    queueCounters_.emplace(type, std::move(counters));
  }
}

void BcmCosQueueManager::setupQueueCounters() {
  for (const auto& type: getQueueCounterTypes()) {
    setupQueueCounter(type);
  }
}

void BcmCosQueueManager::updateQueueStats(
    std::chrono::seconds now,
    HwPortStats* portStats) {
  for (auto cntr: queueCounters_) {
    if (cntr.first.isScopeQueues()) {
      for (auto& countersItr: cntr.second.queues) {
        updateQueueStat(countersItr.first, cntr.first, countersItr.second,
                        now, portStats);
      }
    }
    if (cntr.first.isScopeAggregated()) {
      updateQueueAggregatedStat(cntr.first, cntr.second.aggregated,
                                now, portStats);
    }
  }
}
}} // facebook::fboss
