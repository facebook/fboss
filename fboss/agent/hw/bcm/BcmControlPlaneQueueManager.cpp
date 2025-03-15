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

#include <thrift/lib/cpp/util/EnumUtils.h>

#include "fboss/agent/hw/StatsConstants.h"
#include "fboss/agent/hw/bcm/BcmCosQueueManagerUtils.h"
#include "fboss/agent/hw/bcm/BcmEgressQueueFlexCounter.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmPlatform.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"

extern "C" {
#include <bcm/types.h>
}

namespace {
constexpr auto kCPUName = "cpu";
} // namespace

namespace facebook::fboss {

BcmControlPlaneQueueManager::BcmControlPlaneQueueManager(BcmSwitch* hw)
    : BcmCosQueueManager(hw, kCPUName, BCM_GPORT_LOCAL_CPU) {
  maxCPUQueue_ = utility::getMaxCPUQueueSize(hw->getUnit());
  if (auto* flexCounterMgr = hw_->getBcmEgressQueueFlexCounterManager()) {
    flexCounterMgr->attachToCPU();
  }
}

BcmControlPlaneQueueManager::~BcmControlPlaneQueueManager() {
  if (auto* flexCounterMgr = hw_->getBcmEgressQueueFlexCounterManager()) {
    flexCounterMgr->detachFromCPU();
  }
}

const std::vector<BcmCosQueueCounterType>&
BcmControlPlaneQueueManager::getQueueCounterTypes() const {
  static const std::vector<BcmCosQueueCounterType> types = {
      {cfg::StreamType::MULTICAST,
       BcmCosQueueStatType::DROPPED_PACKETS,
       BcmCosQueueCounterScope::QUEUES,
       "in_dropped_pkts"},
      {cfg::StreamType::MULTICAST,
       BcmCosQueueStatType::OUT_PACKETS,
       BcmCosQueueCounterScope::QUEUES,
       "in_pkts"}};
  return types;
}

BcmPortQueueConfig BcmControlPlaneQueueManager::getCurrentQueueSettings()
    const {
  QueueConfig multicastQueues;
  for (int i = 0; i < getNumQueues(cfg::StreamType::MULTICAST); i++) {
    multicastQueues.push_back(
        getCurrentQueueSettings(cfg::StreamType::MULTICAST, i));
  }
  return BcmPortQueueConfig({}, std::move(multicastQueues));
}

bcm_gport_t BcmControlPlaneQueueManager::getQueueGPort(
    cfg::StreamType streamType,
    bcm_cos_queue_t cosQ) const {
  if (!hw_->getPlatform()->getAsic()->isSupported(HwAsic::Feature::L3_QOS)) {
    throw FbossError(
        "Failed to retrieve queue gport because platform doesn't support cosq");
  }
  // cpu only have multicast
  if (streamType == cfg::StreamType::MULTICAST) {
    return cosQueueGports_.multicast.at(cosQ);
  }
  throw FbossError(
      "Failed to retrieve queue gport because unsupported StreamType: ",
      apache::thrift::util::enumNameSafe(streamType));
  ;
}

const PortQueue& BcmControlPlaneQueueManager::getDefaultQueueSettings(
    cfg::StreamType streamType) const {
  return hw_->getPlatform()->getDefaultControlPlaneQueueSettings(streamType);
}

int BcmControlPlaneQueueManager::getNumQueues(
    cfg::StreamType streamType) const {
  // if platform doesn't support cosq, return maxCPUQueue_
  if (!hw_->getPlatform()->getAsic()->isSupported(HwAsic::Feature::L3_QOS)) {
    return maxCPUQueue_;
  }
  // cpu only has multicast
  if (streamType == cfg::StreamType::MULTICAST) {
    return std::min(
        static_cast<int>(cosQueueGports_.multicast.size()), kMaxMCQueueSize);
  }
  throw FbossError(
      "Failed to retrieve queue size because unsupported StreamType: ",
      apache::thrift::util::enumNameSafe(streamType));
}

std::unique_ptr<PortQueue> BcmControlPlaneQueueManager::getCurrentQueueSettings(
    cfg::StreamType streamType,
    bcm_cos_queue_t cosQ) const {
  auto queue = std::make_unique<PortQueue>(static_cast<uint8_t>(cosQ));
  queue->setStreamType(streamType);

  getSchedulingAndWeight(portGport_, cosQ, queue.get());
  // get control value
  getReservedBytes(portGport_, cosQ, queue.get());
  getSharedBytes(portGport_, cosQ, queue.get());

  if (hw_->getPlatform()->getType() != PlatformType::PLATFORM_FAKE_WEDGE40) {
    auto queueGport = getQueueGPort(streamType, cosQ);
    getAlpha(queueGport, cosQ, queue.get());
  }

  getBandwidth(portGport_, cosQ, queue.get());
  return queue;
}

void BcmControlPlaneQueueManager::program(const PortQueue& queue) {
  bcm_cos_queue_t cosQ = queue.getID();

  updateNamedQueue(queue);
  programSchedulingAndWeight(portGport_, cosQ, queue);

  // program control value
  programReservedBytes(portGport_, cosQ, queue);
  programSharedBytes(portGport_, cosQ, queue);
  programBandwidth(portGport_, cosQ, queue);
  if (hw_->getPlatform()->getType() != PlatformType::PLATFORM_FAKE_WEDGE40) {
    auto queueGport = getQueueGPort(queue.getStreamType(), cosQ);
    programAlpha(queueGport, cosQ, queue);
  }
}

std::pair<bcm_gport_t, bcm_cos_queue_t>
BcmControlPlaneQueueManager::getQueueStatIDPair(
    bcm_cos_queue_t cosQ,
    cfg::StreamType streamType) {
  // for cpu queue, we need to check whether platform supports cos. If not, we
  // use cpu gport + queue id to get cos stat; otherwise, we use queue gport +
  // 0 to get cos stat.
  if (hw_->getPlatform()->getAsic()->isSupported(HwAsic::Feature::L3_QOS)) {
    return std::make_pair(getQueueGPort(streamType, cosQ), 0);
  } else {
    return std::make_pair(portGport_, cosQ);
  }
}
} // namespace facebook::fboss
