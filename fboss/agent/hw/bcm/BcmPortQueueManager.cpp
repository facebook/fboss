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

#include "fboss/agent/hw/StatsConstants.h"
#include "fboss/agent/hw/bcm/BcmCosQueueFBConvertors.h"
#include "fboss/agent/hw/bcm/BcmCosQueueManagerUtils.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmPlatform.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"

#include <folly/logging/xlog.h>
#include <thrift/lib/cpp/util/EnumUtils.h>

extern "C" {
#include <bcm/cosq.h>
#include <bcm/types.h>
}

namespace {
using facebook::fboss::cfg::QueueCongestionBehavior;
static const std::array<QueueCongestionBehavior, 2> kSupportedAQMBehaviors = {
    QueueCongestionBehavior::EARLY_DROP,
    QueueCongestionBehavior::ECN};

} // namespace

namespace facebook::fboss {

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
      {cfg::StreamType::UNICAST,
       BcmCosQueueStatType::OUT_PACKETS,
       BcmCosQueueCounterScope::QUEUES,
       kOutPkts()},
      {cfg::StreamType::UNICAST,
       BcmCosQueueStatType::WRED_DROPPED_PACKETS,
       BcmCosQueueCounterScope::QUEUES,
       kWredDroppedPackets()},
  };

  if (!hw_->getPlatform()->getAsic()->isSupported(
          HwAsic::Feature::OBM_COUNTERS)) {
    return types;
  }

  std::vector<BcmCosQueueCounterType> obmTypes = {
      {cfg::StreamType::UNICAST,
       BcmCosQueueStatType::OBM_LOSSY_HIGH_PRI_DROPPED_PACKETS,
       BcmCosQueueCounterScope::AGGREGATED,
       kObmLossyHighPriDroppedPkts()},
      {cfg::StreamType::UNICAST,
       BcmCosQueueStatType::OBM_LOSSY_HIGH_PRI_DROPPED_BYTES,
       BcmCosQueueCounterScope::AGGREGATED,
       kObmLossyHighPriDroppedBytes()},
      {cfg::StreamType::UNICAST,
       BcmCosQueueStatType::OBM_LOSSY_LOW_PRI_DROPPED_PACKETS,
       BcmCosQueueCounterScope::AGGREGATED,
       kObmLossyLowPriDroppedPkts()},
      {cfg::StreamType::UNICAST,
       BcmCosQueueStatType::OBM_LOSSY_LOW_PRI_DROPPED_BYTES,
       BcmCosQueueCounterScope::AGGREGATED,
       kObmLossyLowPriDroppedBytes()},
      {cfg::StreamType::UNICAST,
       BcmCosQueueStatType::OBM_HIGH_WATERMARK,
       // NOTE: MAX agg is unsupported, resorting to SUM
       BcmCosQueueCounterScope::AGGREGATED,
       kObmHighWatermark()},
  };

  obmTypes.insert(obmTypes.begin(), types.begin(), types.end());
  static const std::vector<BcmCosQueueCounterType> allTypes = {
      obmTypes.begin(), obmTypes.end()};
  return allTypes;
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
      apache::thrift::util::enumNameSafe(streamType));
}

bcm_gport_t BcmPortQueueManager::getQueueGPort(
    cfg::StreamType streamType,
    bcm_cos_queue_t cosQ) const {
  if (streamType == cfg::StreamType::UNICAST) {
    return cosQueueGports_.unicast.at(cosQ);
  } else if (streamType == cfg::StreamType::MULTICAST) {
    return cosQueueGports_.multicast.at(cosQ);
  }
  throw FbossError(
      "Failed to retrieve queue gport because unknown StreamType: ",
      apache::thrift::util::enumNameSafe(streamType));
}

BcmPortQueueConfig BcmPortQueueManager::getCurrentQueueSettings() const {
  // Make sure that the QueueConf is in cosQ order
  QueueConfig unicastQueues;
  for (int i = 0; i < cosQueueGports_.unicast.size(); i++) {
    try {
      unicastQueues.push_back(
          getCurrentQueueSettings(cfg::StreamType::UNICAST, i));
    } catch (const facebook::fboss::FbossError& err) {
      XLOG(ERR) << "Error in getCurrentQueueSettings - unicast cosq order: "
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
      XLOG(ERR) << "Error in getCurrentQueueSettings - multicast cosq order: "
                << i;
    }
  }
  return BcmPortQueueConfig(
      std::move(unicastQueues), std::move(multicastQueues));
}

const PortQueue& BcmPortQueueManager::getDefaultQueueSettings(
    cfg::StreamType streamType) const {
  return hw_->getPlatform()->getDefaultPortQueueSettings(streamType);
}

void BcmPortQueueManager::programEgressDynamicSharedEnabled(
    cfg::StreamType streamType,
    bcm_gport_t gport,
    bcm_cos_queue_t cosQ) const {
  bcm_cosq_control_t cosControl;
  std::string streamTypeStr;

  if (streamType == cfg::StreamType::MULTICAST) {
    cosControl = bcmCosqControlEgressMCSharedDynamicEnable;
    streamTypeStr = "mc";
  } else {
    cosControl = bcmCosqControlEgressUCSharedDynamicEnable;
    streamTypeStr = "uc";
  }

  auto errorDescr = folly::to<std::string>(
      "egress ", streamTypeStr, " shared dynamic enable value");
  int rv = bcm_cosq_control_set(hw_->getUnit(), gport, cosQ, cosControl, 1);

  bcmCheckError(
      rv, "Unable to set ", errorDescr, " for ", portName_, " cosQ ", cosQ);
}

bool BcmPortQueueManager::isEgressDynamicSharedEnabled(
    cfg::StreamType streamType,
    bcm_gport_t gport,
    bcm_cos_queue_t cosQ) const {
  int value = 0;
  std::string streamTypeStr;
  bcm_cosq_control_t cosControl;

  if (streamType == cfg::StreamType::MULTICAST) {
    cosControl = bcmCosqControlEgressMCSharedDynamicEnable;
    streamTypeStr = "mc";
  } else {
    cosControl = bcmCosqControlEgressUCSharedDynamicEnable;
    streamTypeStr = "uc";
  }

  auto errorDescr = folly::to<std::string>(
      "egress ", streamTypeStr, " shared dynamic enable");
  int rv =
      bcm_cosq_control_get(hw_->getUnit(), gport, cosQ, cosControl, &value);
  bcmCheckError(
      rv, "Unable to get ", errorDescr, " for ", portName_, " cosQ ", cosQ);
  return (value != 0);
}

void BcmPortQueueManager::getAlpha(
    bcm_gport_t gport,
    bcm_cos_queue_t cosQ,
    PortQueue* queue) const {
  // check if EgressMC(UC)SharedDynamicEnable is enabled on the HW
  // mostly its programmed as enabled by default during sdk init
  // It causes failures on SIM where default settings may be different
  cfg::MMUScalingFactor scalingFactor;
  const auto& defaultQueueSettings =
      getDefaultQueueSettings(queue->getStreamType());
  if (isEgressDynamicSharedEnabled(queue->getStreamType(), gport, cosQ)) {
    auto alpha = getControlValue(
        queue->getStreamType(), gport, cosQ, BcmCosQueueControlType::ALPHA);
    scalingFactor = utility::bcmAlphaToCfgAlpha(
        static_cast<bcm_cosq_control_drop_limit_alpha_value_e>(alpha));
  } else {
    // pick the default valaue if not programmed
    scalingFactor = defaultQueueSettings.getScalingFactor().value();
  }

  if (scalingFactor != defaultQueueSettings.getScalingFactor().value()) {
    queue->setScalingFactor(scalingFactor);
  }
}

void BcmPortQueueManager::getAqms(
    bcm_gport_t gport,
    bcm_cos_queue_t cosQ,
    PortQueue* queue) const {
  if (queue->getStreamType() != cfg::StreamType::UNICAST) {
    // No AQMs on non unicast queues
    queue->resetAqms({});
    return;
  }

  std::vector<cfg::ActiveQueueManagement> aqms;
  auto defaultAqms = getDefaultQueueSettings(queue->getStreamType()).getAqms();
  // get every profile back
  for (auto behavior : kSupportedAQMBehaviors) {
    bcm_cosq_gport_discard_t discard = utility::initBcmCosqDiscard(behavior);

    auto rv = bcm_cosq_gport_discard_get(hw_->getUnit(), gport, cosQ, &discard);
    bcmCheckError(
        rv,
        "Unable to retrieve cos queue discard for ",
        portName_,
        " cosQ ",
        cosQ,
        " flags ",
        discard.flags);

    for (const auto& defaultAqm : std::as_const(*defaultAqms)) {
      if (defaultAqm->get<switch_config_tags::behavior>() == behavior) {
        // THRIFT_COPY
        auto aqmOpt = utility::bcmAqmToCfgAqm(discard, defaultAqm->toThrift());
        if (aqmOpt.has_value()) {
          aqms.push_back(aqmOpt.value());
        }
      }
    }
  }
  queue->resetAqms(aqms);
}

std::unique_ptr<PortQueue> BcmPortQueueManager::getCurrentQueueSettings(
    cfg::StreamType streamType,
    bcm_cos_queue_t cosQ) const {
  auto queue = std::make_unique<PortQueue>(static_cast<uint8_t>(cosQ));
  queue->setStreamType(streamType);

  auto queueGport = getQueueGPort(streamType, cosQ);

  /*
   * TODO: Scheduling params require using portGport instead of queueGport due
   * to Broadcom bug. Refer T37398023 for details.
   */
  getSchedulingAndWeight(portGport_, cosQ, queue.get());

  // get control value
  getReservedBytes(queueGport, cosQ, queue.get());
  getSharedBytes(queueGport, cosQ, queue.get());
  getAlpha(queueGport, cosQ, queue.get());
  /*
   * Experiments showed that using queueGport to get/set bandwidth worked for
   * TH, but for TH3, it failed for front panel ports (but worked for CPU port).
   * Using portGport + cosQ worked for TH as well as TH3, and for front panel
   * ports as well as CPU port.
   * CS9032705 asks this question to Broadcom.
   */
  getBandwidth(portGport_, cosQ, queue.get());
  getAqms(queueGport, cosQ, queue.get());
  return queue;
}

void BcmPortQueueManager::programAlpha(
    bcm_gport_t gport,
    bcm_cos_queue_t cosQ,
    const PortQueue& queue) {
  const auto& defaultQueueSettings =
      getDefaultQueueSettings(queue.getStreamType());

  // EgressDynamicShared is mostly enabled by SDK during init
  // but in some cases its not. Explicitly enable it always when we set
  // alpha
  // (1) SIM
  // (2) mmu_lossless = 1
  programEgressDynamicSharedEnabled(queue.getStreamType(), gport, cosQ);
  auto alpha = utility::cfgAlphaToBcmAlpha(
      defaultQueueSettings.getScalingFactor().value());
  if (queue.getScalingFactor()) {
    alpha = utility::cfgAlphaToBcmAlpha(queue.getScalingFactor().value());
  }
  programControlValue(
      queue.getStreamType(), gport, cosQ, BcmCosQueueControlType::ALPHA, alpha);
}

void BcmPortQueueManager::programAqm(
    bcm_gport_t gport,
    bcm_cos_queue_t cosQ,
    cfg::QueueCongestionBehavior behavior,
    std::optional<cfg::QueueCongestionDetection> detection) {
  const auto& defaultQueue = getDefaultQueueSettings(cfg::StreamType::UNICAST);
  auto& defaultAqms = defaultQueue.getAqms();
  // NOTE: The following logic only works on Tomahawk (Wedge100).
  //       Trident2 (Wedge40) does not have a drop profile for
  //       ECT_MARKED packets, so enabling ECN on that chip
  //       works differently. It should be sufficient to configure
  //       the TCP profile to have the MARK_CONGESTION flag. Some
  //       additional tuning may be required to properly setup
  //       the "no early drops; yes ecn" case.

  CHECK(defaultAqms);
  std::optional<cfg::ActiveQueueManagement> defaultAqmThrift{};
  for (const auto& defaultAqm : std::as_const(*defaultAqms)) {
    // find in list of active queue management configurations, the configuration
    // for this behavior
    if (defaultAqm->get<switch_config_tags::behavior>() == behavior) {
      // THRIFT_COPY
      defaultAqmThrift = defaultAqm->toThrift();
      break;
    }
  }
  CHECK(defaultAqmThrift.has_value());
  bcm_cosq_gport_discard_t discard =
      utility::cfgAqmToBcmAqm(behavior, detection, (*defaultAqmThrift));

  auto rv = bcm_cosq_gport_discard_set(hw_->getUnit(), gport, cosQ, &discard);
  bcmCheckError(rv, "Unable to configure aqm for ", portName_, "; cosQ ", cosQ);
  auto behaviorS = apache::thrift::util::enumNameSafe(behavior);
  XLOG(DBG2) << "Setup active queue management[" << behaviorS << "] for "
             << portName_ << "; cosQ " << cosQ << "; flags: " << discard.flags
             << "; queue length min threshold: " << discard.min_thresh
             << "; queue length max threshold: " << discard.max_thresh;
}

void BcmPortQueueManager::programTrafficClass(
    bcm_gport_t queueGport,
    bcm_cos_queue_t cosQ,
    int prio) {
  auto rv = bcm_cosq_gport_mapping_set(
      hw_->getUnit(),
      portGport_,
      prio,
      BCM_COSQ_GPORT_UCAST_QUEUE_GROUP,
      queueGport,
      cosQ);
  bcmCheckError(
      rv,
      "failed to set cosq mapping for gport=",
      portGport_,
      ", cosQ=",
      cosQ,
      " with int_prio=",
      prio);
}

void BcmPortQueueManager::programAqms(
    bcm_gport_t gport,
    bcm_cos_queue_t cosQ,
    const PortQueue& queue) {
  if (queue.getStreamType() != cfg::StreamType::UNICAST) {
    XLOG(WARNING) << "Only unicast queue supports AQM";
    return;
  }

  // if the current aqm hasn't changed, no need to re-program
  PortQueue queueInHw(static_cast<uint8_t>(cosQ));
  queueInHw.setStreamType(queue.getStreamType());
  getAqms(gport, cosQ, &queueInHw);
  // THRIFT_COPY
  if (queue.isAqmsSame(&queueInHw)) {
    XLOG(DBG2) << "Desired AQMs are the same as current, no need to re-program";
    return;
  }

  // Following the same logic of all other queue attributes to be programmed
  // into ASIC, if one behavior is flagged, we need to program the threshold for
  // this profile/curve in BRCM; otherwise, we need to clear the existing values
  // in BRCM by using default values to reset this profile/curve.
  // Since we support 2 behavior types(EARLY_DROP and ECN), we should always
  // program 2 discards with the correct flags and threshold.

  // Since BCM_COSQ_DISCARD_ENABLE and BCM_COSQ_DISCARD_MARK_CONGESTION
  // are shared port_discard setting rather than one profile setting, we need
  // program the unset discard value first, and then the one that actually sets
  // threshold and BCM_COSQ_DISCARD_ENABLE. Otherwise, we might reset these
  // two flags(DISCARD_ENABLE and MARK_CONGESTION).
  std::array<bool, 2> filters = {true, false};
  for (bool isReset : filters) {
    for (auto behavior : kSupportedAQMBehaviors) {
      auto detection = queue.findDetectionInAqms(behavior);
      bool isEnabled = detection.has_value();
      if (isReset && !isEnabled) {
        programAqm(gport, cosQ, behavior, std::nullopt);
      } else if (!isReset && isEnabled) {
        CHECK(detection.has_value());
        programAqm(gport, cosQ, behavior, detection);
      }
    }
  }
}

void BcmPortQueueManager::program(const PortQueue& queue) {
  bcm_cos_queue_t cosQ = queue.getID();
  auto streamType = queue.getStreamType();
  bcm_gport_t queueGport = getQueueGPort(streamType, cosQ);

  updateNamedQueue(queue);
  /*
   * TODO: Scheduling params require using portGport instead of queueGport due
   * to Broadcom bug. Refer T37398023 for details.
   */
  programSchedulingAndWeight(portGport_, cosQ, queue);

  // program control value
  programReservedBytes(queueGport, cosQ, queue);
  programSharedBytes(queueGport, cosQ, queue);
  programAlpha(queueGport, cosQ, queue);
  /*
   * Experiments showed that using queueGport to get/set bandwidth worked for
   * TH, but for TH3, it failed for front panel ports (but worked for CPU port).
   * Using portGport + cosQ worked for TH as well as TH3, and for front panel
   * ports as well as CPU port.
   * CS9032705 asks this question to Broadcom.
   */
  programBandwidth(portGport_, cosQ, queue);
  programAqms(queueGport, cosQ, queue);

  if (auto trafficClass = queue.getTrafficClass()) {
    // if traffic class for queue is set, from configuration
    programTrafficClass(queueGport, cosQ, trafficClass.value());
  } else {
    /* no traffic classes provided, use queue id as the traffic class */
    auto prio = CosQToBcmInternalPriority(cosQ);
    programTrafficClass(queueGport, cosQ, prio);
  }
}

std::pair<bcm_gport_t, bcm_cos_queue_t> BcmPortQueueManager::getQueueStatIDPair(
    bcm_cos_queue_t cosQ,
    cfg::StreamType streamType) {
  // for regular port, we always get queue gport from CosQueueGports and use 0
  // as queue id to call bcm_cosq_stat_get
  return std::make_pair(getQueueGPort(streamType, cosQ), 0);
}

// 1:1 mapping: cos <-> internal priority
int BcmPortQueueManager::CosQToBcmInternalPriority(bcm_cos_queue_t cosQ) {
  if (cosQ > BCM_COS_MAX || cosQ < BCM_COS_MIN) {
    throw FbossError("Invalid cosQ: ", cosQ);
  }
  return cosQ;
}

bcm_cos_queue_t BcmPortQueueManager::bcmInternalPriorityToCosQ(int prio) {
  if (prio > BCM_PRIO_MAX || prio < BCM_PRIO_MIN) {
    throw FbossError("Invalid internal piority: ", prio);
  }
  return prio;
}

} // namespace facebook::fboss
