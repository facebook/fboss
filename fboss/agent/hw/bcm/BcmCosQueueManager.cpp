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

#include <folly/logging/xlog.h>
#include <thrift/lib/cpp/util/EnumUtils.h>

#include "fboss/agent/hw/CounterUtils.h"
#include "fboss/agent/hw/bcm/BcmCosQueueFBConvertors.h"
#include "fboss/agent/hw/bcm/BcmCosQueueManagerUtils.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/BcmPlatform.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"

extern "C" {

// TODO (skhare) find better way to identify OpenNSA build
#if (!defined(BCM_VER_MAJOR))
// TODO (skhare) refactor and replace with right OpenNSA API
#define NUM_COS(unit) 8
#else
#include <soc/drv.h>
#endif

#include <bcm/cosq.h>
}

namespace {
// the corresponding bcm_cosq_control_t for uc and mc queues
using facebook::fboss::BcmCosQueueControlType;
using facebook::fboss::cfg::StreamType;
using CosControlTypeAndDescMap =
    std::map<StreamType, std::pair<bcm_cosq_control_t, std::string>>;
static const std::map<BcmCosQueueControlType, CosControlTypeAndDescMap>
    kCosqControlTypeAndDescMap = {
        {BcmCosQueueControlType::ALPHA,
         {{StreamType::UNICAST,
           std::make_pair(bcmCosqControlDropLimitAlpha, "uc alpha")},
          {StreamType::MULTICAST,
           std::make_pair(bcmCosqControlDropLimitAlpha, "mc alpa")}}},
        {BcmCosQueueControlType::RESERVED_BYTES,
         {{StreamType::UNICAST,
           std::make_pair(
               bcmCosqControlEgressUCQueueMinLimitBytes,
               "uc reserved bytes")},
          {StreamType::MULTICAST,
           std::make_pair(
               bcmCosqControlEgressMCQueueMinLimitBytes,
               "mc reserved bytes")}}},
        {BcmCosQueueControlType::SHARED_BYTES,
         {{StreamType::UNICAST,
           std::make_pair(
               bcmCosqControlEgressUCQueueSharedLimitBytes,
               "uc shared bytes")},
          {StreamType::MULTICAST,
           std::make_pair(
               bcmCosqControlEgressMCQueueSharedLimitBytes,
               "mc shared bytes")}}}};

int cosqGportTraverseCallback(
    int unit,
    bcm_gport_t gport,
    int /*numq*/,
    uint32 flags,
    bcm_gport_t queueGport,
    void* userData) {
  auto cosQueueManager =
      static_cast<facebook::fboss::BcmCosQueueManager*>(userData);
  if (gport != cosQueueManager->getPortGport() &&
      !(BCM_GPORT_MODPORT_PORT_GET(gport) == 0 &&
        cosQueueManager->getPortGport() == BCM_GPORT_LOCAL_CPU)) {
    // no-op
    return 0l;
  }
  facebook::fboss::CosQueueGports* info = cosQueueManager->getCosQueueGports();
  if (flags & BCM_COSQ_GPORT_SCHEDULER) {
    info->scheduler = queueGport;
  } else if (flags & BCM_COSQ_GPORT_UCAST_QUEUE_GROUP) {
    bcm_cos_queue_t cosQ = BCM_GPORT_UCAST_QUEUE_GROUP_QID_GET(queueGport);
    // T75758668 some platforms don't support NUM_COS any more.
    // And we can just use queueGport from this callback directly
    if (NUM_COS(unit) == 0 || BCM_COSQ_QUEUE_VALID(unit, cosQ)) {
      info->unicast[cosQ] = queueGport;
    }
  } else if (flags & BCM_COSQ_GPORT_MCAST_QUEUE_GROUP) {
    bcm_cos_queue_t cosQ = BCM_GPORT_MCAST_QUEUE_GROUP_QID_GET(queueGport);
    info->multicast[cosQ] = queueGport;
  }
  return 0l;
};
} // unnamed namespace

namespace facebook::fboss {

BcmCosQueueManager::BcmCosQueueManager(
    BcmSwitch* hw,
    const std::string& portName,
    bcm_gport_t portGport)
    : hw_(hw), portName_(portName), portGport_(portGport) {
  if (hw_->getPlatform()->getAsic()->isSupported(HwAsic::Feature::L3_QOS)) {
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

bcm_cos_queue_t BcmCosQueueManager::getCosQueue(
    cfg::StreamType streamType,
    bcm_gport_t gport) const {
  if (streamType == cfg::StreamType::UNICAST) {
    return BCM_GPORT_UCAST_QUEUE_GROUP_QID_GET(gport);
  } else if (streamType == cfg::StreamType::MULTICAST) {
    return BCM_COSQ_GPORT_MCAST_EGRESS_QUEUE_GET(gport);
  }
  throw FbossError(
      "Failed to get cos queue because unknown StreamType ",
      apache::thrift::util::enumNameSafe(streamType));
  ;
}

int BcmCosQueueManager::getControlValue(
    cfg::StreamType streamType,
    bcm_gport_t gport,
    bcm_cos_queue_t cosQ,
    BcmCosQueueControlType ctrlType) const {
  bcm_cosq_control_t type;
  std::string description;
  std::tie(type, description) =
      kCosqControlTypeAndDescMap.at(ctrlType).at(streamType);

  int value;
  int rv = bcm_cosq_control_get(hw_->getUnit(), gport, cosQ, type, &value);
  bcmCheckError(
      rv, "Unable to get ", description, " for ", portName_, " cosQ ", cosQ);
  return value;
}

void BcmCosQueueManager::programControlValue(
    cfg::StreamType streamType,
    bcm_gport_t gport,
    bcm_cos_queue_t cosQ,
    BcmCosQueueControlType ctrlType,
    int value) {
  bcm_cosq_control_t type;
  std::string description;
  std::tie(type, description) =
      kCosqControlTypeAndDescMap.at(ctrlType).at(streamType);

  int rv = bcm_cosq_control_set(hw_->getUnit(), gport, cosQ, type, value);
  bcmCheckError(
      rv, "Unable to set ", description, " for ", portName_, " cosQ ", cosQ);
}

void BcmCosQueueManager::getSchedulingAndWeight(
    bcm_gport_t gport,
    bcm_cos_queue_t cosQ,
    PortQueue* queue) const {
  int mode, weight;
  int rv =
      bcm_cosq_gport_sched_get(hw_->getUnit(), gport, cosQ, &mode, &weight);
  bcmCheckError(
      rv,
      "Unable to get scheduling mode and weight for ",
      portName_,
      " cosQ ",
      cosQ);

  auto cfgPair =
      utility::bcmSchedulingAndWeightToCfg(std::make_pair(mode, weight));
  queue->setScheduling(cfgPair.first);
  queue->setWeight(cfgPair.second);
}

void BcmCosQueueManager::programSchedulingAndWeight(
    bcm_gport_t gport,
    bcm_cos_queue_t cosQ,
    const PortQueue& queue) {
  auto bcmPair = utility::cfgSchedulingAndWeightToBcm(
      std::make_pair(queue.getScheduling(), queue.getWeight()));
  int rv = bcm_cosq_gport_sched_set(
      hw_->getUnit(), gport, cosQ, bcmPair.first, bcmPair.second);
  bcmCheckError(
      rv,
      "Unable to set scheduling mode and weight for ",
      portName_,
      " cosQ ",
      cosQ);
}

void BcmCosQueueManager::getReservedBytes(
    bcm_gport_t gport,
    bcm_cos_queue_t cosQ,
    PortQueue* queue) const {
  int reservedBytes = getControlValue(
      queue->getStreamType(),
      gport,
      cosQ,
      BcmCosQueueControlType::RESERVED_BYTES);
  const auto& defaultQueueSettings =
      getDefaultQueueSettings(queue->getStreamType());
  if (reservedBytes &&
      reservedBytes != defaultQueueSettings.getReservedBytes().value()) {
    queue->setReservedBytes(reservedBytes);
  }
}

void BcmCosQueueManager::programReservedBytes(
    bcm_gport_t gport,
    bcm_cos_queue_t cosQ,
    const PortQueue& queue) {
  const auto& defaultQueueSettings =
      getDefaultQueueSettings(queue.getStreamType());
  int reservedBytes = defaultQueueSettings.getReservedBytes().value();
  if (queue.getReservedBytes()) {
    reservedBytes = queue.getReservedBytes().value();
  }
  programControlValue(
      queue.getStreamType(),
      gport,
      cosQ,
      BcmCosQueueControlType::RESERVED_BYTES,
      reservedBytes);
}

void BcmCosQueueManager::getSharedBytes(
    bcm_gport_t gport,
    bcm_cos_queue_t cosQ,
    PortQueue* queue) const {
  int sharedBytes = getControlValue(
      queue->getStreamType(),
      gport,
      cosQ,
      BcmCosQueueControlType::SHARED_BYTES);
  const auto& defaultQueueSettings =
      getDefaultQueueSettings(queue->getStreamType());
  if (sharedBytes &&
      sharedBytes != defaultQueueSettings.getSharedBytes().value()) {
    queue->setSharedBytes(sharedBytes);
  }
}

void BcmCosQueueManager::programSharedBytes(
    bcm_gport_t gport,
    bcm_cos_queue_t cosQ,
    const PortQueue& queue) {
  const auto& defaultQueueSettings =
      getDefaultQueueSettings(queue.getStreamType());
  int sharedBytes = defaultQueueSettings.getSharedBytes().value();
  if (queue.getSharedBytes()) {
    sharedBytes = queue.getSharedBytes().value();
  }
  XLOG(DBG1) << "Setting gport " << gport << ", queue "
             << static_cast<int>(queue.getID()) << " shared bytes to "
             << sharedBytes << " bytes";
  programControlValue(
      queue.getStreamType(),
      gport,
      cosQ,
      BcmCosQueueControlType::SHARED_BYTES,
      sharedBytes);
}

void BcmCosQueueManager::getBandwidth(
    bcm_gport_t gport,
    int queueIdx,
    PortQueue* queue) const {
  uint32_t ppsMin, ppsMax, flags;
  int rv = bcm_cosq_gport_bandwidth_get(
      hw_->getUnit(), gport, queueIdx, &ppsMin, &ppsMax, &flags);
  bcmCheckError(
      rv,
      "Unable to get cos queue bandwidth for ",
      portName_,
      " queue ",
      queueIdx);

  const auto& defaultQueueSettings =
      getDefaultQueueSettings(queue->getStreamType());

  // Check if default setting is pps and matches what is configued in
  // hardware
  if (flags == BCM_COSQ_BW_PACKET_MODE &&
      defaultQueueSettings.getPortQueueRate().value().getType() ==
          cfg::PortQueueRate::Type::pktsPerSec &&
      *defaultQueueSettings.getPortQueueRate()
              .value()
              .get_pktsPerSec()
              .minimum_ref() == ppsMin &&
      *defaultQueueSettings.getPortQueueRate()
              .value()
              .get_pktsPerSec()
              .maximum_ref() == ppsMax) {
    XLOG(DBG1) << "Configured pps equals default ppsMin: " << ppsMin
               << " and default ppsMax: " << ppsMax;
    return;
  }

  // Check if default setting is kbps and matches what is configued in hardware
  if (flags != BCM_COSQ_BW_PACKET_MODE &&
      defaultQueueSettings.getPortQueueRate().value().getType() ==
          cfg::PortQueueRate::Type::kbitsPerSec &&
      *defaultQueueSettings.getPortQueueRate()
              .value()
              .get_kbitsPerSec()
              .minimum_ref() == ppsMin &&
      *defaultQueueSettings.getPortQueueRate()
              .value()
              .get_kbitsPerSec()
              .maximum_ref() == ppsMax) {
    XLOG(DBG1) << "Configured kbps equals default ppsMin: " << ppsMin
               << " and default ppsMax: " << ppsMax;
    return;
  }

  cfg::PortQueueRate portQueueRate;

  cfg::Range range;
  *range.minimum_ref() = ppsMin;
  *range.maximum_ref() = ppsMax;

  flags != BCM_COSQ_BW_PACKET_MODE ? portQueueRate.set_kbitsPerSec(range)
                                   : portQueueRate.set_pktsPerSec(range);

  queue->setPortQueueRate(portQueueRate);
}

void BcmCosQueueManager::programBandwidth(
    bcm_gport_t gport,
    int queueIdx,
    const PortQueue& queue) {
  uint32_t rateMin, rateMax, flags;

  const auto& defaultQueueSettings =
      getDefaultQueueSettings(queue.getStreamType());
  auto portQueueRate = defaultQueueSettings.getPortQueueRate().value();

  if (queue.getPortQueueRate()) {
    portQueueRate = queue.getPortQueueRate().value();
  }

  if (portQueueRate.getType() == cfg::PortQueueRate::Type::pktsPerSec) {
    rateMin = *portQueueRate.get_pktsPerSec().minimum_ref();
    rateMax = *portQueueRate.get_pktsPerSec().maximum_ref();
    flags = BCM_COSQ_BW_PACKET_MODE;
  } else { // cfg::PortQueueRate::Type::pktsPerSec
    rateMin = *portQueueRate.get_kbitsPerSec().minimum_ref();
    rateMax = *portQueueRate.get_kbitsPerSec().maximum_ref();
    flags = 0;
  }

  XLOG(DBG1) << "Setting gport " << gport << ", queue "
             << static_cast<int>(queue.getID()) << " bandwidth "
             << ((flags == BCM_COSQ_BW_PACKET_MODE) ? "PacketsPerSec" : "Kbps")
             << " min: " << rateMin << " max: " << rateMax;

  int rv = bcm_cosq_gport_bandwidth_set(
      hw_->getUnit(), gport, queueIdx, rateMin, rateMax, flags);
  bcmCheckError(
      rv, "Unable to set pps/kbps for ", portName_, " queue ", queueIdx);
}

void BcmCosQueueManager::updateQueueAggregatedStat(
    const BcmCosQueueCounterType& type,
    facebook::stats::MonotonicCounter* counter,
    std::chrono::seconds now,
    HwPortStats* portStats) {
  // bcm support passing -1 as cosQ id and using port gport can get
  // all cosQ stats
  auto statType = type.statType;

  uint64_t value;
  auto rv = bcm_cosq_stat_get(
      hw_->getUnit(),
      portGport_,
      -1,
      utility::getBcmCosqStatType(statType),
      &value);
  bcmCheckError(
      rv,
      "Unable to get aggregated cosq stat ",
      utility::getBcmCosqStatType(statType),
      " for ",
      portName_);

  if (counter) {
    counter->updateValue(now, value);
  }

  // right now HwPortStats only supports returning pkt stat for aggregated
  if (statType == BcmCosQueueStatType::DROPPED_PACKETS && portStats) {
    *portStats->outCongestionDiscardPkts__ref() = value;
  }
}

void BcmCosQueueManager::getCosQueueGportsFromHw() {
  auto rv = bcm_cosq_gport_traverse(
      hw_->getUnit(), &cosqGportTraverseCallback, static_cast<void*>(this));
  bcmCheckError(rv, "Failed to traverse queue gports for ", portName_);
  XLOG(INFO) << "getCosQueueGportsFromHw for port: " << portName_
             << ", port gport: " << portGport_
             << ", unicast: " << cosQueueGports_.unicast.size()
             << ", multicast: " << cosQueueGports_.multicast.size();
}

} // namespace facebook::fboss
