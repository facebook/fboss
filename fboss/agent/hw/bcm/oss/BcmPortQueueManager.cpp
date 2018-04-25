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

std::shared_ptr<PortQueue> BcmPortQueueManager::getCurrentQueueSettings(
    cfg::StreamType /*streamType*/, opennsl_gport_t /*queueGport*/,
    int /*queueIdx*/) {
  return std::shared_ptr<PortQueue>{};
}

void BcmPortQueueManager::setupQueue(
  const std::shared_ptr<PortQueue>& /*queue*/) {}

void BcmPortQueueManager::setupQueueCounters(
  const std::vector<BcmPortQueueCounterType>& /*types*/) {}

void BcmPortQueueManager::updateQueueStats(
  std::chrono::seconds /*now*/, HwPortStats* /*portStats*/) {}

void BcmPortQueueManager::fillOrReplaceCounter(
  const BcmPortQueueCounterType& /*type*/, QueueStatCounters& /*counters*/) {}
}} // facebook::fboss
