/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmControlPlaneQueueManager.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/BcmPlatform.h"

namespace facebook { namespace fboss {
BcmControlPlaneQueueManager::BcmControlPlaneQueueManager(
    BcmSwitch* hw,
    const std::string& portName,
    opennsl_gport_t portGport)
  : BcmCosQueueManager(hw, portName, portGport) {}

std::shared_ptr<PortQueue> BcmControlPlaneQueueManager::getCurrentQueueSettings(
    cfg::StreamType /*streamType*/,
    opennsl_cos_queue_t /*cosQ*/) const {
  return std::shared_ptr<PortQueue>{};
}

void BcmControlPlaneQueueManager::program(
    const std::shared_ptr<PortQueue>& /*queue*/) {}

void BcmControlPlaneQueueManager::updateQueueStat(
    opennsl_cos_queue_t /*cosQ*/,
    const BcmCosQueueCounterType& /*type*/,
    facebook::stats::MonotonicCounter* /*counter*/,
    std::chrono::seconds /*now*/,
    HwPortStats* /*portStats*/) {}

int BcmControlPlaneQueueManager::getNumQueues(
    cfg::StreamType streamType) const {
  // if platform doesn't support cosq, return maxCPUQueue_
  if (!hw_->getPlatform()->isCosSupported()) {
    return maxCPUQueue_;
  }
  // cpu only has multicast
  if (streamType == cfg::StreamType::MULTICAST) {
    return cosQueueGports_.multicast.size();
  }
  throw FbossError(
      "Failed to retrieve queue size because unsupported StreamType: ",
      cfg::_StreamType_VALUES_TO_NAMES.find(streamType)->second);
}

}} //facebook::fboss
