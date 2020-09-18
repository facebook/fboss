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

#include <folly/logging/xlog.h>
#include <thrift/lib/cpp/util/EnumUtils.h>

#include "fboss/agent/hw/StatsConstants.h"
#include "fboss/agent/hw/bcm/BcmCosQueueManagerUtils.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmPlatform.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"

extern "C" {
#include <bcm/cosq.h>
}

namespace {
// HACK: there seems to be some bugs in some queue apis related to
// the number of queues. Specifically, if you use the cpu gport +
// queue id to access cpu cosq, it rejects ids above 32. Also the
// calls to get scheduling mode are inconsistent above queue id
// 10. Since we don't use queues above 9, cap the number of
// queues while we work through the issue + upgrade sdk.
constexpr auto kMaxMCQueueSize = 10;
} // namespace

namespace facebook::fboss {

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

BcmControlPlaneQueueManager::BcmControlPlaneQueueManager(
    BcmSwitch* hw,
    const std::string& portName,
    bcm_gport_t portGport)
    : BcmCosQueueManager(hw, portName, portGport) {
  auto rv = bcm_rx_queue_max_get(hw_->getUnit(), &maxCPUQueue_);
  if (rv == BCM_E_UNAVAIL) {
    // T75758668 Temporary hack before Broadcom release fix in next SDK
    XLOG(INFO) << "[HACK] bcm_rx_queue_max_get is unavailable, use "
               << kMaxMCQueueSize;
    maxCPUQueue_ = kMaxMCQueueSize;
  } else {
    bcmCheckError(rv, "failed to get max CPU cos queue number");
  }
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
  ;
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

  getBandwidth(portGport_, cosQ, queue.get());
  return queue;
}

void BcmControlPlaneQueueManager::program(const PortQueue& queue) {
  bcm_cos_queue_t cosQ = queue.getID();

  programSchedulingAndWeight(portGport_, cosQ, queue);

  // program control value
  programReservedBytes(portGport_, cosQ, queue);
  programSharedBytes(portGport_, cosQ, queue);
  programBandwidth(portGport_, cosQ, queue);
}

void BcmControlPlaneQueueManager::updateQueueStat(
    bcm_cos_queue_t cosQ,
    const BcmCosQueueCounterType& type,
    facebook::stats::MonotonicCounter* counter,
    std::chrono::seconds now,
    HwPortStats* /*portStats*/) {
  // for cpu queue, we need to check whether platform supports cos. If not, we
  // use cpu gport + queue id to get cos stat; otherwise, we use queue gport +
  // 0 to get cos stat.
  auto statType = type.statType;
  bcm_gport_t gport = portGport_;
  int specialCosQ = cosQ;
  if (hw_->getPlatform()->getAsic()->isSupported(HwAsic::Feature::L3_QOS)) {
    std::tie(gport, specialCosQ) =
        std::make_pair(getQueueGPort(type.streamType, cosQ), 0);
  }

  uint64_t value;
  auto rv = bcm_cosq_stat_get(
      hw_->getUnit(),
      gport,
      specialCosQ,
      utility::getBcmCosqStatType(statType),
      &value);
  bcmCheckError(
      rv,
      "Unable to get cosq stat ",
      utility::getBcmCosqStatType(statType),
      " for ",
      portName_,
      ", cosQ=",
      cosQ);

  if (counter) {
    counter->updateValue(now, value);
  }

  // HwPortStats only supports for regular port, no need to update portStats
}

} // namespace facebook::fboss
