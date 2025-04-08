/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/test/utils/QueueTestUtils.h"
#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"

namespace facebook::fboss::utility {

// XXX This is FSW config, add RSW config. Prefix queue names with portName
// XXX this is 2Q config minus ECN. If needed, add 2Q + ECN config
void add2QueueConfig(
    cfg::SwitchConfig* config,
    cfg::StreamType streamType,
    bool scalingFactorSupported) {
  std::vector<cfg::PortQueue> portQueues;

  cfg::PortQueue queue0;
  *queue0.id() = k2QueueLowPriQueueId;
  queue0.name() = "queue0.low_pri";
  queue0.streamType() = streamType;
  *queue0.scheduling() = cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN;
  queue0.weight() = k2QueueLowPriWeight;
  if (scalingFactorSupported) {
    queue0.scalingFactor() = cfg::MMUScalingFactor::ONE;
  }
  queue0.reservedBytes() = 3328;
  portQueues.push_back(queue0);

  cfg::PortQueue queue1;
  *queue1.id() = k2QueueHighPriQueueId;
  queue1.name() = "queue1.high_pri";
  queue1.streamType() = streamType;
  *queue1.scheduling() = cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN;
  queue1.weight() = k2QueueHighPriWeight;
  if (scalingFactorSupported) {
    queue1.scalingFactor() = cfg::MMUScalingFactor::EIGHT;
  }
  queue1.reservedBytes() = 9984;
  portQueues.push_back(queue1);

  cfg::PortQueue queue7;
  *queue7.id() = k2QueueNCQueueId;
  queue7.name() = "queue7.network_control";
  queue7.streamType() = streamType;
  *queue7.scheduling() = cfg::QueueScheduling::STRICT_PRIORITY;
  portQueues.push_back(queue7);

  config->portQueueConfigs()["queue_config"] = portQueues;
  for (auto& port : *config->ports()) {
    port.portQueueConfigName() = "queue_config";
  }
}

std::string get2QueueAclNameForDscp(uint8_t dscp) {
  return folly::to<std::string>("2queue_acl_dscp", dscp);
}

std::string get2QueueCounterNameForDscp(uint8_t dscp) {
  return folly::to<std::string>("dscp", dscp, "_counter");
}

const std::map<int, std::vector<uint8_t>>& k2QueueToDscp() {
  static const std::map<int, std::vector<uint8_t>> queueToDscp = {
      {k2QueueLowPriQueueId,
       {8,  9,  10, 11, 12, 13, 14, 15, 40, 41, 42, 43, 44, 45, 46, 47,
        49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63}},
      {k2QueueHighPriQueueId,
       {0,  1,  2,  3,  4,  6,  7,  16, 17, 18, 19, 20, 21, 22, 23, 24,
        25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39}},
      {k2QueueEcn1QueueId, {5}},
      {k2QueueNCQueueId, {48}},
  };
  return queueToDscp;
}

const std::map<int, uint8_t>& k2QueueWRRQueueToWeight() {
  static const std::map<int, uint8_t> wrrQueueToWeight = {
      {k2QueueLowPriQueueId, k2QueueLowPriWeight},
      {k2QueueHighPriQueueId, k2QueueHighPriWeight},
      {k2QueueEcn1QueueId, k2QueueEcn1Weight},
  };

  return wrrQueueToWeight;
}

const std::vector<int>& k2QueueWRRQueueIds() {
  static const std::vector<int> wrrQueueIds = {
      k2QueueLowPriQueueId,
      k2QueueHighPriQueueId,
  };

  return wrrQueueIds;
}

const std::vector<int>& k2QueueSPQueueIds() {
  static const std::vector<int> spQueueIds = {k2QueueNCQueueId};

  return spQueueIds;
}

const std::vector<int>& k2QueueWRRAndNCQueueIds() {
  static const std::vector<int> wrrAndNCQueueIds = {
      k2QueueLowPriQueueId, k2QueueHighPriQueueId, k2QueueNCQueueId};
  return wrrAndNCQueueIds;
}

bool is2QueueWRRQueueId(int queueId) {
  return k2QueueWRRQueueToWeight().find(queueId) !=
      k2QueueWRRQueueToWeight().end();
}

int getTrafficClassToCpuEgressQueueId(const HwAsic* hwAsic, int trafficClass) {
  if (hwAsic->getAsicType() == cfg::AsicType::ASIC_TYPE_JERICHO3) {
    // Jericho3 only has two egress queues for cpu port and recycle port
    // match default/low/med to queue 0, high to 1
    return trafficClass == 7 ? 1 : 0;
  }
  // same one-to-one tc to queue mapping for other platforms
  return trafficClass;
}

int getTrafficClassToEgressQueueId(const HwAsic* hwAsic, int trafficClass) {
  if (isDualStage3Q2QQos()) {
    if (trafficClass == 7) {
      // tc 7 -> nc egq 7
      return 7;
    } else if (trafficClass == 2) {
      // tc 2 -> rdma egq 2
      return 2;
    }
    // all other tc -> default egq 6
    return 6;
  }
  // same one-to-one tc to queue mapping for other platforms
  return trafficClass;
}

int getAqmGranularThreshold(const HwAsic& asic, int value) {
  return ceil(value / asic.getThresholdGranularity()) *
      asic.getThresholdGranularity();
}

cfg::ActiveQueueManagement
GetEcnConfig(const HwAsic& asic, int minLength, int maxLength) {
  cfg::ActiveQueueManagement ecnAQM;
  cfg::LinearQueueCongestionDetection ecnLQCD;
  ecnLQCD.minimumLength() = getAqmGranularThreshold(asic, minLength);
  ecnLQCD.maximumLength() = getAqmGranularThreshold(asic, maxLength);
  ecnAQM.detection()->linear_ref() = ecnLQCD;
  ecnAQM.behavior() = cfg::QueueCongestionBehavior::ECN;
  return ecnAQM;
}

cfg::ActiveQueueManagement GetWredConfig(
    const HwAsic& asic,
    int minLength,
    int maxLength,
    int probability) {
  cfg::ActiveQueueManagement wredAQM;
  cfg::LinearQueueCongestionDetection wredLQCD;
  wredLQCD.minimumLength() = getAqmGranularThreshold(asic, minLength);
  wredLQCD.maximumLength() = getAqmGranularThreshold(asic, maxLength);
  wredLQCD.probability() = probability;
  wredAQM.detection()->linear_ref() = wredLQCD;
  wredAQM.behavior() = cfg::QueueCongestionBehavior::EARLY_DROP;
  return wredAQM;
}

} // namespace facebook::fboss::utility
