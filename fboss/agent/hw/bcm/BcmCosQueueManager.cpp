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

#include "fboss/agent/hw/bcm/BcmPlatform.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/CounterUtils.h"

namespace facebook::fboss {

BcmCosQueueManager::BcmCosQueueManager(
    BcmSwitch* hw,
    const std::string& portName,
    bcm_gport_t portGport)
    : hw_(hw), portName_(portName), portGport_(portGport) {
  if (hw_->getPlatform()->isCosSupported()) {
    // Make sure we get all the queue gports from hardware for portGport
    getCosQueueGportsFromHw();
  }
}

void BcmCosQueueManager::fillOrReplaceCounter(
    const BcmCosQueueCounterType& type,
    QueueStatCounters& counters,
    const std::optional<QueueConfig>& queueConfig) {
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

      if (queueConfig.has_value() && (queueConfig.value().size() != 0) &&
          queueConfig.value().at(queue)->getName().has_value()) {
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

      // if counter already exists, delete it
      if (countersItr != counters.queues.end()) {
        auto oldName = countersItr->second->getName();
        utility::deleteCounter(oldName);
        counters.queues.erase(countersItr);
      }

      counters.queues.emplace(
          queue,
          std::make_unique<facebook::stats::MonotonicCounter>(
              name, fb303::SUM, fb303::RATE));
    }
  }
  if (type.isScopeAggregated()) {
    counters.aggregated = std::make_unique<facebook::stats::MonotonicCounter>(
        folly::to<std::string>(portName_, ".", type.name),
        fb303::SUM,
        fb303::RATE);
  }
}

void BcmCosQueueManager::setupQueueCounter(
    const BcmCosQueueCounterType& type,
    const std::optional<QueueConfig>& queueConfig) {
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
    const std::optional<QueueConfig>& queueConfig) {
  for (const auto& type : getQueueCounterTypes()) {
    setupQueueCounter(type, queueConfig);
  }
}

void BcmCosQueueManager::destroyQueueCounters() {
  std::map<BcmCosQueueCounterType, QueueStatCounters> swapTo;
  queueCounters_.swap(swapTo);

  for (const auto& type : getQueueCounterTypes()) {
    auto it = swapTo.find(type);
    if (it != swapTo.end()) {
      for (auto& item : it->second.queues) {
        if (item.second) {
          utility::deleteCounter(item.second->getName());
        }
      }
      if (it->second.aggregated) {
        utility::deleteCounter(it->second.aggregated->getName());
      }
    }
  }
}

void BcmCosQueueManager::updateQueueStats(
    std::chrono::seconds now,
    HwPortStats* portStats) {
  for (const auto& cntr : queueCounters_) {
    if (cntr.first.isScopeQueues()) {
      for (auto& countersItr : cntr.second.queues) {
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

} // namespace facebook::fboss
