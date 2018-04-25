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

namespace facebook { namespace fboss {

BcmPortQueueConfig BcmPortQueueManager::getCurrentQueueSettings() {
  QueueConfig unicastQueues;
  for (int i = 0; i < cosQueueGports_.unicast.size(); i++) {
    unicastQueues.push_back(getCurrentQueueSettings(
      cfg::StreamType::UNICAST, cosQueueGports_.unicast.at(i), i));
  }
  QueueConfig multicastQueues;
  for (int i = 0; i < cosQueueGports_.multicast.size(); i++) {
    multicastQueues.push_back(getCurrentQueueSettings(
      cfg::StreamType::MULTICAST, cosQueueGports_.multicast.at(i), i));
  }
  return BcmPortQueueConfig(std::move(unicastQueues),
                            std::move(multicastQueues));
}

int BcmPortQueueManager::getQueueSize(cfg::StreamType streamType) {
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
    cfg::StreamType streamType, int queueID) {
  if (streamType == cfg::StreamType::UNICAST) {
    return cosQueueGports_.unicast.at(queueID);
  } else if (streamType == cfg::StreamType::MULTICAST) {
    return cosQueueGports_.multicast.at(queueID);
  }
  throw FbossError(
    "Failed to retrieve queue gport because unknown StreamType: ",
    cfg::_StreamType_VALUES_TO_NAMES.find(streamType)->second);
}
}} // facebook::fboss
