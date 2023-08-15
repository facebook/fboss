/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/test/dataplane_tests/HwTestOlympicUtils.h"
#include "fboss/agent/FbossError.h"

#include "fboss/agent/hw/switch_asics/HwAsic.h"

namespace facebook::fboss::utility {

int getOlympicQueueId(const HwAsic* hwAsic, OlympicQueueType queueType) {
  bool queueLowerValHighPri = hwAsic->isSupported(
      HwAsic::Feature::QUEUE_PRIORITY_LOWER_VAL_IS_HIGH_PRI);
  switch (queueType) {
    case OlympicQueueType::SILVER:
      return queueLowerValHighPri ? kOlympicSilverQueueId2
                                  : kOlympicSilverQueueId;
    case OlympicQueueType::GOLD:
      return queueLowerValHighPri ? kOlympicGoldQueueId2 : kOlympicGoldQueueId;
    case OlympicQueueType::ECN1:
      return queueLowerValHighPri ? kOlympicEcn1QueueId2 : kOlympicEcn1QueueId;
    case OlympicQueueType::BRONZE:
      return queueLowerValHighPri ? kOlympicBronzeQueueId2
                                  : kOlympicBronzeQueueId;
    case OlympicQueueType::ICP:
      return queueLowerValHighPri ? kOlympicICPQueueId2 : kOlympicICPQueueId;
    case OlympicQueueType::NC:
      return queueLowerValHighPri ? kOlympicNCQueueId2 : kOlympicNCQueueId;
  }
  throw FbossError("Invalid olympic queue type ", queueType);
}

int getOlympicV2QueueId(const HwAsic* hwAsic, OlympicV2QueueType queueType) {
  bool queueLowerValHighPri = hwAsic->isSupported(
      HwAsic::Feature::QUEUE_PRIORITY_LOWER_VAL_IS_HIGH_PRI);
  switch (queueType) {
    case OlympicV2QueueType::NCNF:
      return queueLowerValHighPri ? kOlympicAllSPNCNFQueueId2
                                  : kOlympicAllSPNCNFQueueId;
    case OlympicV2QueueType::BRONZE:
      return queueLowerValHighPri ? kOlympicAllSPBronzeQueueId2
                                  : kOlympicAllSPBronzeQueueId;
    case OlympicV2QueueType::SILVER:
      return queueLowerValHighPri ? kOlympicAllSPSilverQueueId2
                                  : kOlympicAllSPSilverQueueId;
    case OlympicV2QueueType::GOLD:
      return queueLowerValHighPri ? kOlympicAllSPGoldQueueId2
                                  : kOlympicAllSPGoldQueueId;
    case OlympicV2QueueType::ICP:
      return queueLowerValHighPri ? kOlympicAllSPICPQueueId2
                                  : kOlympicAllSPICPQueueId;
    case OlympicV2QueueType::NC:
      return queueLowerValHighPri ? kOlympicAllSPNCQueueId2
                                  : kOlympicAllSPNCQueueId;
  }
  throw FbossError("Invalid all SP olympic queue type ", queueType);
}

int getNetworkAIQueueId(const HwAsic* hwAsic, NetworkAIQueueType queueType) {
  bool queueLowerValHighPri = hwAsic->isSupported(
      HwAsic::Feature::QUEUE_PRIORITY_LOWER_VAL_IS_HIGH_PRI);
  switch (queueType) {
    case NetworkAIQueueType::MONITORING:
      return queueLowerValHighPri ? kNetworkAIMonitoringQueueId2
                                  : kNetworkAIMonitoringQueueId;
    case NetworkAIQueueType::RDMA:
      return queueLowerValHighPri ? kNetworkAIRdmaQueueId2
                                  : kNetworkAIRdmaQueueId;
    case NetworkAIQueueType::NC:
      return queueLowerValHighPri ? kNetworkAINCQueueId2 : kNetworkAINCQueueId;
  }
  throw FbossError("Invalid all network AI queue type ", queueType);
}

cfg::ActiveQueueManagement kGetOlympicEcnConfig(int minLength, int maxLength) {
  cfg::ActiveQueueManagement ecnAQM;
  cfg::LinearQueueCongestionDetection ecnLQCD;
  ecnLQCD.minimumLength() = minLength;
  ecnLQCD.maximumLength() = maxLength;
  ecnAQM.detection()->linear_ref() = ecnLQCD;
  ecnAQM.behavior() = cfg::QueueCongestionBehavior::ECN;
  return ecnAQM;
}

cfg::ActiveQueueManagement
kGetWredConfig(int minLength, int maxLength, int probability) {
  cfg::ActiveQueueManagement wredAQM;
  cfg::LinearQueueCongestionDetection wredLQCD;
  wredLQCD.minimumLength() = minLength;
  wredLQCD.maximumLength() = maxLength;
  wredLQCD.probability() = probability;
  wredAQM.detection()->linear_ref() = wredLQCD;
  wredAQM.behavior() = cfg::QueueCongestionBehavior::EARLY_DROP;
  return wredAQM;
}

cfg::PortQueue&
getPortQueueConfig(cfg::SwitchConfig* config, const int queueId, bool isVoq) {
  auto& queueConfig = isVoq ? *config->defaultVoqConfig()
                            : config->portQueueConfigs()["queue_config"];
  for (auto& queue : queueConfig) {
    if (queue.id() == queueId) {
      return queue;
    }
  }
  throw FbossError("Cannot find queue ID ", queueId, " in config!");
}

void addQueueShaperConfig(
    cfg::SwitchConfig* config,
    const int queueId,
    const uint32_t minKbps,
    const uint32_t maxKbps) {
  cfg::Range kbpsRange;
  kbpsRange.minimum() = minKbps;
  kbpsRange.maximum() = maxKbps;
  auto& queue = getPortQueueConfig(config, queueId, false /* isVoq */);
  queue.portQueueRate() = cfg::PortQueueRate();
  queue.portQueueRate()->kbitsPerSec_ref() = kbpsRange;
}

void addQueueBurstSizeConfig(
    cfg::SwitchConfig* config,
    const int queueId,
    const uint32_t minKbits,
    const uint32_t maxKbits) {
  auto& queue = getPortQueueConfig(config, queueId, false /* isVoq */);
  queue.bandwidthBurstMinKbits() = minKbits;
  queue.bandwidthBurstMaxKbits() = maxKbits;
}

void addQueueEcnConfig(
    cfg::SwitchConfig* config,
    const int queueId,
    const uint32_t minLen,
    const uint32_t maxLen,
    bool isVoq) {
  auto& queue = getPortQueueConfig(config, queueId, isVoq);
  if (!queue.aqms().has_value()) {
    queue.aqms() = {};
  }
  queue.aqms()->push_back(kGetOlympicEcnConfig(minLen, maxLen));
}

void addQueueWredConfig(
    cfg::SwitchConfig* config,
    const int queueId,
    const uint32_t minLen,
    const uint32_t maxLen,
    const int probability,
    bool isVoq) {
  auto& queue = getPortQueueConfig(config, queueId, isVoq);
  if (!queue.aqms().has_value()) {
    queue.aqms() = {};
  }
  queue.aqms()->push_back(kGetWredConfig(minLen, maxLen, probability));
}

void addNetworkAIQueueConfig(
    cfg::SwitchConfig* config,
    cfg::StreamType streamType,
    const HwAsic* hwAsic) {
  std::vector<cfg::PortQueue> portQueues;

  cfg::PortQueue queue0;
  queue0.id() = getNetworkAIQueueId(hwAsic, NetworkAIQueueType::MONITORING);
  queue0.name() = "queue0.monitoring";
  queue0.streamType() = streamType;
  queue0.scheduling() = cfg::QueueScheduling::STRICT_PRIORITY;
  portQueues.push_back(queue0);

  cfg::PortQueue queue1;
  queue0.id() = getNetworkAIQueueId(hwAsic, NetworkAIQueueType::RDMA);
  queue0.name() = "queue6.rdma";
  queue0.streamType() = streamType;
  queue0.scheduling() = cfg::QueueScheduling::STRICT_PRIORITY;
  portQueues.push_back(queue1);

  cfg::PortQueue queue2;
  queue0.id() = getNetworkAIQueueId(hwAsic, NetworkAIQueueType::NC);
  queue0.name() = "queue7.nc";
  queue0.streamType() = streamType;
  queue0.scheduling() = cfg::QueueScheduling::STRICT_PRIORITY;
  portQueues.push_back(queue2);

  config->portQueueConfigs()["queue_config"] = portQueues;
  for (auto& port : *config->ports()) {
    port.portQueueConfigName() = "queue_config";
  }
}

void addVoqQueueConfig(
    cfg::SwitchConfig* config,
    cfg::StreamType streamType,
    const HwAsic* asic,
    bool addWredConfig) {
  std::vector<cfg::PortQueue> voqConfig;
  std::vector<OlympicQueueType> kQueueTypes{
      OlympicQueueType::SILVER,
      OlympicQueueType::GOLD,
      OlympicQueueType::ECN1,
      OlympicQueueType::ICP,
      OlympicQueueType::NC};
  for (auto queueType : kQueueTypes) {
    auto queueId = getOlympicQueueId(asic, queueType);
    cfg::PortQueue queue;
    *queue.id() = queueId;
    queue.streamType() = streamType;
    queue.name() = folly::to<std::string>("queue", queueId);
    *queue.scheduling() = cfg::QueueScheduling::INTERNAL;

    if (asic->scalingFactorBasedDynamicThresholdSupported()) {
      queue.scalingFactor() = cfg::MMUScalingFactor::ONE;
    }
    queue.reservedBytes() = 1500; // Set to possible MTU!

    if (queueId == getOlympicQueueId(asic, OlympicQueueType::ECN1)) {
      queue.aqms() = {};
      queue.aqms()->push_back(kGetOlympicEcnConfig());
      if (addWredConfig) {
        queue.aqms()->push_back(kGetWredConfig());
      }
    }
    voqConfig.push_back(queue);
  }
  config->defaultVoqConfig() = voqConfig;
}

// XXX This is FSW config, add RSW config. Prefix queue names with portName
void addOlympicQueueConfigWithSchedulingHelper(
    cfg::SwitchConfig* config,
    cfg::StreamType streamType,
    const HwAsic* asic,
    bool addWredConfig,
    cfg::QueueScheduling schedType) {
  std::vector<cfg::PortQueue> portQueues;

  cfg::PortQueue queue0;
  *queue0.id() = getOlympicQueueId(asic, OlympicQueueType::SILVER);
  queue0.name() = "queue0.silver";
  queue0.streamType() = streamType;
  *queue0.scheduling() = schedType;
  queue0.weight() = kOlympicSilverWeight;
  if (asic->scalingFactorBasedDynamicThresholdSupported()) {
    queue0.scalingFactor() = cfg::MMUScalingFactor::ONE;
  }
  if (!asic->mmuQgroupsEnabled()) {
    queue0.reservedBytes() = 3328;
  }
  portQueues.push_back(queue0);

  cfg::PortQueue queue1;
  *queue1.id() = getOlympicQueueId(asic, OlympicQueueType::GOLD);
  queue1.name() = "queue1.gold";
  queue1.streamType() = streamType;
  *queue1.scheduling() = schedType;
  queue1.weight() = kOlympicGoldWeight;
  if (asic->scalingFactorBasedDynamicThresholdSupported()) {
    queue1.scalingFactor() = cfg::MMUScalingFactor::EIGHT;
  }
  if (!asic->mmuQgroupsEnabled()) {
    queue1.reservedBytes() = 9984;
  }
  portQueues.push_back(queue1);

  cfg::PortQueue queue2;
  *queue2.id() = getOlympicQueueId(asic, OlympicQueueType::ECN1);
  queue2.name() = "queue2.ecn1";
  queue2.streamType() = streamType;
  *queue2.scheduling() = schedType;
  queue2.weight() = kOlympicEcn1Weight;
  if (asic->scalingFactorBasedDynamicThresholdSupported()) {
    queue2.scalingFactor() = cfg::MMUScalingFactor::ONE;
  }
  queue2.aqms() = {};
  queue2.aqms()->push_back(kGetOlympicEcnConfig());
  if (addWredConfig) {
    queue2.aqms()->push_back(kGetWredConfig());
  }
  portQueues.push_back(queue2);

  cfg::PortQueue queue4;
  *queue4.id() = getOlympicQueueId(asic, OlympicQueueType::BRONZE);
  queue4.name() = "queue4.bronze";
  queue4.streamType() = streamType;
  *queue4.scheduling() = schedType;
  queue4.weight() = kOlympicBronzeWeight;
  portQueues.push_back(queue4);

  cfg::PortQueue queue6;
  *queue6.id() = getOlympicQueueId(asic, OlympicQueueType::ICP);
  queue6.name() = "queue6.platinum";
  queue6.streamType() = streamType;
  *queue6.scheduling() = cfg::QueueScheduling::STRICT_PRIORITY;
  if (!asic->mmuQgroupsEnabled()) {
    queue6.reservedBytes() = 9984;
  }
  if (asic->scalingFactorBasedDynamicThresholdSupported()) {
    queue6.scalingFactor() = cfg::MMUScalingFactor::EIGHT;
  }
  portQueues.push_back(queue6);

  cfg::PortQueue queue7;
  *queue7.id() = getOlympicQueueId(asic, OlympicQueueType::NC);
  queue7.name() = "queue7.network_control";
  queue7.streamType() = streamType;
  *queue7.scheduling() = cfg::QueueScheduling::STRICT_PRIORITY;
  portQueues.push_back(queue7);

  config->portQueueConfigs()["queue_config"] = portQueues;
  for (auto& port : *config->ports()) {
    if (*port.portType() == cfg::PortType::INTERFACE_PORT) {
      // Apply queue configs on INTERFACE_PORTS only
      port.portQueueConfigName() = "queue_config";
    }
  }
  // For VoQ switches, add the default VoQ queue config as well!
  if (asic->getSwitchType() == cfg::SwitchType::VOQ) {
    addVoqQueueConfig(config, streamType, asic, addWredConfig);
  }
}

void addOlympicQueueConfig(
    cfg::SwitchConfig* config,
    cfg::StreamType streamType,
    const HwAsic* asic,
    bool addWredConfig) {
  addOlympicQueueConfigWithSchedulingHelper(
      config,
      streamType,
      asic,
      addWredConfig,
      cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN);
}

// copied from addOlympicQueueConfig. RSWs might need a separate config
void addFswRswAllSPOlympicQueueConfig(
    cfg::SwitchConfig* config,
    cfg::StreamType streamType,
    const HwAsic* asic,
    bool addWredConfig) {
  addOlympicQueueConfigWithSchedulingHelper(
      config,
      streamType,
      asic,
      addWredConfig,
      cfg::QueueScheduling::STRICT_PRIORITY);
}

// Configure two queues (silver and ecn) with the same weight but different drop
// probability. It is used to verify the queue with higher drop probability will
// have more drops and lower watermark.
void addQueueWredDropConfig(
    cfg::SwitchConfig* config,
    cfg::StreamType streamType,
    const HwAsic* asic) {
  std::vector<cfg::PortQueue> portQueues;

  // 256 packets in the test, where each packet has a payload of 7000 bytes
  auto constexpr maxThresh = 7000 * 256;

  cfg::PortQueue queue0;
  queue0.id() = getOlympicQueueId(asic, OlympicQueueType::SILVER);
  queue0.name() = "queue0";
  queue0.streamType() = streamType;
  queue0.scheduling() = cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN;
  queue0.weight() = kOlympicEcn1Weight;
  if (asic->scalingFactorBasedDynamicThresholdSupported()) {
    queue0.scalingFactor() = cfg::MMUScalingFactor::ONE;
  }
  queue0.aqms() = {};
  queue0.aqms()->push_back(kGetWredConfig(1, maxThresh, 0));
  portQueues.push_back(queue0);

  cfg::PortQueue queue2;
  queue2.id() = getOlympicQueueId(asic, OlympicQueueType::ECN1);
  queue2.name() = "queue2";
  queue2.streamType() = streamType;
  queue2.scheduling() = cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN;
  queue2.weight() = kOlympicEcn1Weight;
  if (asic->scalingFactorBasedDynamicThresholdSupported()) {
    queue2.scalingFactor() = cfg::MMUScalingFactor::ONE;
  }
  queue2.aqms() = {};
  queue2.aqms()->push_back(kGetWredConfig(1, maxThresh, 5));
  portQueues.push_back(queue2);

  config->portQueueConfigs()["queue_config"] = portQueues;
  for (auto& port : *config->ports()) {
    port.portQueueConfigName() = "queue_config";
  }
}

const std::map<OlympicV2QueueType, std::string>&
kOlympicV2QueueIdToQueueName() {
  static const std::map<OlympicV2QueueType, std::string> queueIdToQueueName = {
      {OlympicV2QueueType::NCNF, "queue0.ncnf"},
      {OlympicV2QueueType::BRONZE, "queue1.bronze"},
      {OlympicV2QueueType::SILVER, "queue2.silver"},
      {OlympicV2QueueType::GOLD, "queue3.gold"},
      {OlympicV2QueueType::ICP, "queeu6.icp"},
      {OlympicV2QueueType::NC, "queue7.nc"},
  };

  return queueIdToQueueName;
}

void addOlympicAllSPQueueConfig(
    cfg::SwitchConfig* config,
    cfg::StreamType streamType,
    const HwAsic* asic) {
  std::vector<cfg::PortQueue> portQueues;

  for (const auto& [queueType, queueName] : kOlympicV2QueueIdToQueueName()) {
    cfg::PortQueue queue;
    queue.id() = getOlympicV2QueueId(asic, queueType);
    queue.name() = queueName;
    queue.streamType() = streamType;
    queue.scheduling() = cfg::QueueScheduling::STRICT_PRIORITY;
    portQueues.push_back(queue);
  }

  config->portQueueConfigs()["queue_config"] = portQueues;
  for (auto& port : *config->ports()) {
    port.portQueueConfigName() = "queue_config";
  }
}

std::string getOlympicAclNameForDscp(uint8_t dscp) {
  return folly::to<std::string>("olympic_acl_dscp", dscp);
}

std::string getOlympicCounterNameForDscp(uint8_t dscp) {
  return folly::to<std::string>("dscp", dscp, "_counter");
}

const std::map<int, std::vector<uint8_t>> kOlympicQueueToDscp(
    const HwAsic* hwAsic) {
  const std::map<int, std::vector<uint8_t>> queueToDscp = {
      {getOlympicQueueId(hwAsic, OlympicQueueType::SILVER),
       {0,  1,  2,  3,  4,  6,  7,  8,  9,  12, 13,
        14, 15, 40, 41, 42, 43, 44, 45, 46, 47, 49}},
      {getOlympicQueueId(hwAsic, OlympicQueueType::GOLD),
       {18, 24, 31, 33, 34, 36, 37, 38, 39}},
      {getOlympicQueueId(hwAsic, OlympicQueueType::ECN1), {5}},
      {getOlympicQueueId(hwAsic, OlympicQueueType::BRONZE),
       {10, 11, 16, 17, 19, 20, 21, 22, 23, 25, 50, 51,
        52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63}},
      {getOlympicQueueId(hwAsic, OlympicQueueType::ICP),
       {26, 27, 28, 29, 30, 32, 35}},
      {getOlympicQueueId(hwAsic, OlympicQueueType::NC), {48}}};
  return queueToDscp;
}

const std::map<int, std::vector<uint8_t>> kOlympicV2QueueToDscp(
    const HwAsic* hwAsic) {
  const std::map<int, std::vector<uint8_t>> queueToDscp = {
      {getOlympicV2QueueId(hwAsic, OlympicV2QueueType::NCNF),
       {50, 51, 52, 53, 54, 55, 56, 57, 58, 59}},
      {getOlympicV2QueueId(hwAsic, OlympicV2QueueType::BRONZE),
       {10, 11, 16, 17, 19, 20, 21, 22, 23, 25, 60, 61, 62, 63}},
      {getOlympicV2QueueId(hwAsic, OlympicV2QueueType::SILVER),
       {0,  1,  2,  3,  4,  6,  7,  8,  9,  12, 13,
        14, 15, 40, 41, 42, 43, 44, 45, 46, 47, 49}},
      {getOlympicV2QueueId(hwAsic, OlympicV2QueueType::GOLD),
       {18, 24, 31, 33, 34, 36, 37, 38, 39}},
      {getOlympicV2QueueId(hwAsic, OlympicV2QueueType::ICP),
       {26, 27, 28, 29, 30, 32, 35}},
      {getOlympicV2QueueId(hwAsic, OlympicV2QueueType::NC), {48}}};
  return queueToDscp;
}

const std::map<int, uint8_t> kOlympicWRRQueueToWeight(const HwAsic* hwAsic) {
  const std::map<int, uint8_t> wrrQueueToWeight = {
      {getOlympicQueueId(hwAsic, OlympicQueueType::SILVER),
       kOlympicSilverWeight},
      {getOlympicQueueId(hwAsic, OlympicQueueType::GOLD), kOlympicGoldWeight},
      {getOlympicQueueId(hwAsic, OlympicQueueType::ECN1), kOlympicEcn1Weight},
      {getOlympicQueueId(hwAsic, OlympicQueueType::BRONZE),
       kOlympicBronzeWeight},
  };

  return wrrQueueToWeight;
}

const std::vector<int> kOlympicWRRQueueIds(const HwAsic* hwAsic) {
  const std::vector<int> wrrQueueIds = {
      getOlympicQueueId(hwAsic, OlympicQueueType::SILVER),
      getOlympicQueueId(hwAsic, OlympicQueueType::GOLD),
      getOlympicQueueId(hwAsic, OlympicQueueType::ECN1),
      getOlympicQueueId(hwAsic, OlympicQueueType::BRONZE)};

  return wrrQueueIds;
}

const std::vector<int> kOlympicSPQueueIds(const HwAsic* hwAsic) {
  const std::vector<int> spQueueIds = {
      getOlympicQueueId(hwAsic, OlympicQueueType::ICP),
      getOlympicQueueId(hwAsic, OlympicQueueType::NC)};

  return spQueueIds;
}

const std::vector<int> kOlympicWRRAndICPQueueIds(const HwAsic* hwAsic) {
  const std::vector<int> wrrAndICPQueueIds = {
      getOlympicQueueId(hwAsic, OlympicQueueType::SILVER),
      getOlympicQueueId(hwAsic, OlympicQueueType::GOLD),
      getOlympicQueueId(hwAsic, OlympicQueueType::ECN1),
      getOlympicQueueId(hwAsic, OlympicQueueType::BRONZE),
      getOlympicQueueId(hwAsic, OlympicQueueType::ICP)};
  return wrrAndICPQueueIds;
}

const std::vector<int> kOlympicWRRAndNCQueueIds(const HwAsic* hwAsic) {
  const std::vector<int> wrrAndNCQueueIds = {
      getOlympicQueueId(hwAsic, OlympicQueueType::SILVER),
      getOlympicQueueId(hwAsic, OlympicQueueType::GOLD),
      getOlympicQueueId(hwAsic, OlympicQueueType::ECN1),
      getOlympicQueueId(hwAsic, OlympicQueueType::BRONZE),
      getOlympicQueueId(hwAsic, OlympicQueueType::NC)};
  return wrrAndNCQueueIds;
}

const std::vector<int> kOlympicAllSPQueueIds(const HwAsic* hwAsic) {
  const std::vector<int> queueIds = {
      getOlympicV2QueueId(hwAsic, OlympicV2QueueType::NCNF),
      getOlympicV2QueueId(hwAsic, OlympicV2QueueType::BRONZE),
      getOlympicV2QueueId(hwAsic, OlympicV2QueueType::SILVER),
      getOlympicV2QueueId(hwAsic, OlympicV2QueueType::GOLD),
      getOlympicV2QueueId(hwAsic, OlympicV2QueueType::ICP),
      getOlympicV2QueueId(hwAsic, OlympicV2QueueType::NC)};

  return queueIds;
}

void addOlympicQosMapsHelper(
    cfg::SwitchConfig& cfg,
    const std::map<int, std::vector<uint8_t>>& queueToDscpMap,
    const std::string& qosPolicyName) {
  cfg::QosMap qosMap;
  qosMap.dscpMaps()->resize(queueToDscpMap.size());
  ssize_t qosMapIdx = 0;
  for (const auto& q2dscps : queueToDscpMap) {
    auto [q, dscps] = q2dscps;
    *qosMap.dscpMaps()[qosMapIdx].internalTrafficClass() = q;
    for (auto dscp : dscps) {
      qosMap.dscpMaps()[qosMapIdx].fromDscpToTrafficClass()->push_back(dscp);
    }
    qosMap.trafficClassToQueueId()->emplace(q, q);
    ++qosMapIdx;
  }
  cfg.qosPolicies()->resize(1);
  *cfg.qosPolicies()[0].name() = qosPolicyName;
  cfg.qosPolicies()[0].qosMap() = qosMap;

  cfg::TrafficPolicyConfig dataPlaneTrafficPolicy;
  dataPlaneTrafficPolicy.defaultQosPolicy() = qosPolicyName;
  cfg.dataPlaneTrafficPolicy() = dataPlaneTrafficPolicy;
  cfg::CPUTrafficPolicyConfig cpuConfig;
  cfg::TrafficPolicyConfig cpuTrafficPolicy;
  cpuTrafficPolicy.defaultQosPolicy() = qosPolicyName;
  cpuConfig.trafficPolicy() = cpuTrafficPolicy;
  cfg.cpuTrafficPolicy() = cpuConfig;
}

void addOlympicQosMaps(cfg::SwitchConfig& cfg, const HwAsic* hwAsic) {
  addOlympicQosMapsHelper(cfg, kOlympicQueueToDscp(hwAsic), "olympic");
}

void addOlympicV2QosMaps(cfg::SwitchConfig& cfg, const HwAsic* hwAsic) {
  addOlympicQosMapsHelper(cfg, kOlympicV2QueueToDscp(hwAsic), "olympic_v2");
}

int getMaxWeightWRRQueue(const std::map<int, uint8_t>& queueToWeight) {
  auto maxItr = std::max_element(
      queueToWeight.begin(),
      queueToWeight.end(),
      [](const std::pair<int, uint64_t>& p1,
         const std::pair<int, uint64_t>& p2) { return p1.second < p2.second; });

  return maxItr->first;
}

}; // namespace facebook::fboss::utility
