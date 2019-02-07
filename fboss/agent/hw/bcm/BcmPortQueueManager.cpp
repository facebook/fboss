/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmPortQueueManager.h"

#include "fboss/agent/hw/bcm/BcmPlatform.h"
#include "fboss/agent/hw/bcm/BcmStatsConstants.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"

#include "folly/logging/xlog.h"

namespace facebook { namespace fboss {
const std::vector<BcmCosQueueCounterType>&
BcmPortQueueManager::getQueueCounterTypes() const {
  static const std::vector<BcmCosQueueCounterType> types = {
      {cfg::StreamType::UNICAST,
       BcmCosQueueStatType::DROPPED_PACKETS,
       BcmCosQueueCounterScope::QUEUES,
       kOutCongestionDiscards()},
      {cfg::StreamType::UNICAST,
       BcmCosQueueStatType::DROPPED_BYTES,
       BcmCosQueueCounterScope::QUEUES,
       kOutCongestionDiscardsBytes()},
      {cfg::StreamType::UNICAST,
       BcmCosQueueStatType::OUT_BYTES,
       BcmCosQueueCounterScope::QUEUES,
       kOutBytes()},
      {cfg::StreamType::UNICAST,
       BcmCosQueueStatType::DROPPED_PACKETS,
       BcmCosQueueCounterScope::AGGREGATED,
       kOutCongestionDiscards()},
      {cfg::StreamType::UNICAST,
       BcmCosQueueStatType::DROPPED_BYTES,
       BcmCosQueueCounterScope::AGGREGATED,
       kOutCongestionDiscardsBytes()},
  };
  return types;
}

/**
 * For regular port queue, we always return # of queues based on how many
 # queue gports we collect from BCM during H/W initializing.
 */
int BcmPortQueueManager::getNumQueues(cfg::StreamType streamType) const {
  if (streamType == cfg::StreamType::UNICAST) {
    return cosQueueGports_.unicast.size();
  } else if (streamType == cfg::StreamType::MULTICAST) {
    return cosQueueGports_.multicast.size();
  }
  throw FbossError(
    "Failed to retrieve queue size because unknown StreamType",
    cfg::_StreamType_VALUES_TO_NAMES.find(streamType)->second);
}

opennsl_gport_t BcmPortQueueManager::getQueueGPort(
    cfg::StreamType streamType, opennsl_cos_queue_t cosQ) const {
  if (streamType == cfg::StreamType::UNICAST) {
    return cosQueueGports_.unicast.at(cosQ);
  } else if (streamType == cfg::StreamType::MULTICAST) {
    return cosQueueGports_.multicast.at(cosQ);
  }
  throw FbossError(
      "Failed to retrieve queue gport because unknown StreamType: ",
      cfg::_StreamType_VALUES_TO_NAMES.find(streamType)->second);
}

BcmPortQueueConfig BcmPortQueueManager::getCurrentQueueSettings() const {
  // Make sure that the QueueConf is in cosQ order
  QueueConfig unicastQueues;
  for (int i = 0; i < cosQueueGports_.unicast.size(); i++) {
    try {
      unicastQueues.push_back(
          getCurrentQueueSettings(cfg::StreamType::UNICAST, i));
    } catch (const facebook::fboss::FbossError& err) {
      XLOG(ERR)
        << "Error in getCurrentQueueSettings - unicast cosq order: "
        << i;
    }
  }
  // Make sure that the QueueConf is in cosQ order
  QueueConfig multicastQueues;
  for (int i = 0; i < cosQueueGports_.multicast.size(); i++) {
    try {
      multicastQueues.push_back(
          getCurrentQueueSettings(cfg::StreamType::MULTICAST, i));
    } catch (const facebook::fboss::FbossError& err) {
      XLOG(ERR)
        << "Error in getCurrentQueueSettings - multicast cosq order: "
        << i;
    }
  }
  return BcmPortQueueConfig(std::move(unicastQueues),
                            std::move(multicastQueues));
}

const PortQueue& BcmPortQueueManager::getDefaultQueueSettings(
    cfg::StreamType streamType) const {
  return hw_->getPlatform()->getDefaultPortQueueSettings(streamType);
}
}} // facebook::fboss
