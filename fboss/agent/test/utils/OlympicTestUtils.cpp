/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/test/utils/OlympicTestUtils.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/test/utils/TrafficPolicyTestUtils.h"

#include "fboss/agent/hw/switch_asics/HwAsic.h"

namespace facebook::fboss::utility {

int getOlympicQueueId(OlympicQueueType queueType) {
  switch (queueType) {
    case OlympicQueueType::SILVER:
      return kOlympicSilverQueueId;
    case OlympicQueueType::GOLD:
      return kOlympicGoldQueueId;
    case OlympicQueueType::ECN1:
      return kOlympicEcn1QueueId;
    case OlympicQueueType::BRONZE:
      return kOlympicBronzeQueueId;
    case OlympicQueueType::ICP:
      return kOlympicICPQueueId;
    case OlympicQueueType::NC:
      return kOlympicNCQueueId;
  }
  throw FbossError("Invalid olympic queue type ", queueType);
}

int getOlympicV2QueueId(OlympicV2QueueType queueType) {
  switch (queueType) {
    case OlympicV2QueueType::NCNF:
      return kOlympicAllSPNCNFQueueId;
    case OlympicV2QueueType::BRONZE:
      return kOlympicAllSPBronzeQueueId;
    case OlympicV2QueueType::SILVER:
      return kOlympicAllSPSilverQueueId;
    case OlympicV2QueueType::GOLD:
      return kOlympicAllSPGoldQueueId;
    case OlympicV2QueueType::ICP:
      return kOlympicAllSPICPQueueId;
    case OlympicV2QueueType::NC:
      return kOlympicAllSPNCQueueId;
  }
  throw FbossError("Invalid all SP olympic queue type ", queueType);
}

int getNetworkAIQueueId(NetworkAIQueueType queueType) {
  switch (queueType) {
    case NetworkAIQueueType::MONITORING:
      return kNetworkAIMonitoringQueueId;
    case NetworkAIQueueType::RDMA:
      return kNetworkAIRdmaQueueId;
    case NetworkAIQueueType::NC:
      return kNetworkAINCQueueId;
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

int getTrafficClassToCpuEgressQueueId(const HwAsic* hwAsic, int trafficClass) {
  if (hwAsic->getAsicType() == cfg::AsicType::ASIC_TYPE_JERICHO3) {
    // Jericho3 only has two egress queues for cpu port and recycle port
    // match default/low/med to queue 0, high to 1
    return trafficClass == kOlympicNCQueueId ? 1 : 0;
  }
  // same one-to-one tc to queue mapping for other platforms
  return trafficClass;
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
  queue0.id() = getNetworkAIQueueId(NetworkAIQueueType::MONITORING);
  queue0.name() = "queue0.monitoring";
  queue0.streamType() = streamType;
  queue0.scheduling() = cfg::QueueScheduling::STRICT_PRIORITY;
  portQueues.push_back(queue0);

  cfg::PortQueue queue1;
  queue0.id() = getNetworkAIQueueId(NetworkAIQueueType::RDMA);
  queue0.name() = "queue6.rdma";
  queue0.streamType() = streamType;
  queue0.scheduling() = cfg::QueueScheduling::STRICT_PRIORITY;
  portQueues.push_back(queue1);

  cfg::PortQueue queue2;
  queue0.id() = getNetworkAIQueueId(NetworkAIQueueType::NC);
  queue0.name() = "queue7.nc";
  queue0.streamType() = streamType;
  queue0.scheduling() = cfg::QueueScheduling::STRICT_PRIORITY;
  portQueues.push_back(queue2);

  config->portQueueConfigs()["queue_config"] = portQueues;
  for (auto& port : *config->ports()) {
    port.portQueueConfigName() = "queue_config";
  }
}

void addVoqAqmConfig(
    cfg::SwitchConfig* config,
    cfg::StreamType streamType,
    const HwAsic* asic,
    bool addWredConfig,
    bool addEcnConfig) {
  std::vector<cfg::PortQueue> voqConfig;
  std::vector<OlympicQueueType> kQueueTypes{
      OlympicQueueType::SILVER,
      OlympicQueueType::GOLD,
      OlympicQueueType::ECN1,
      OlympicQueueType::ICP,
      OlympicQueueType::NC};
  for (auto queueType : kQueueTypes) {
    auto queueId = getOlympicQueueId(queueType);
    cfg::PortQueue queue;
    *queue.id() = queueId;
    queue.streamType() = streamType;
    queue.name() = folly::to<std::string>("queue", queueId);
    *queue.scheduling() = cfg::QueueScheduling::INTERNAL;

    if (asic->scalingFactorBasedDynamicThresholdSupported()) {
      queue.scalingFactor() = cfg::MMUScalingFactor::ONE;
    }
    queue.reservedBytes() = 1500; // Set to possible MTU!

    if (queueId == getOlympicQueueId(OlympicQueueType::ECN1)) {
      queue.aqms() = {};
      if (addEcnConfig) {
        queue.aqms()->push_back(kGetOlympicEcnConfig());
      }
      if (addWredConfig) {
        queue.aqms()->push_back(kGetWredConfig());
      }
    }
    voqConfig.push_back(queue);
  }
  config->defaultVoqConfig() = voqConfig;
}

void addOlympicQueueConfigWithSchedulingHelper(
    cfg::SwitchConfig* config,
    cfg::StreamType streamType,
    const HwAsic* asic,
    bool addWredConfig,
    cfg::QueueScheduling schedType) {
  addOlympicQueueOptionalEcnWredConfigWithSchedulingHelper(
      config, streamType, asic, addWredConfig, true, schedType);
}

// XXX This is FSW config, add RSW config. Prefix queue names with portName
void addOlympicQueueOptionalEcnWredConfigWithSchedulingHelper(
    cfg::SwitchConfig* config,
    cfg::StreamType streamType,
    const HwAsic* asic,
    bool addWredConfig,
    bool addEcnConfig,
    cfg::QueueScheduling schedType) {
  std::vector<cfg::PortQueue> portQueues;

  cfg::PortQueue queue0;
  *queue0.id() = getOlympicQueueId(OlympicQueueType::SILVER);
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
  *queue1.id() = getOlympicQueueId(OlympicQueueType::GOLD);
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
  *queue2.id() = getOlympicQueueId(OlympicQueueType::ECN1);
  queue2.name() = "queue2.ecn1";
  queue2.streamType() = streamType;
  *queue2.scheduling() = schedType;
  queue2.weight() = kOlympicEcn1Weight;
  if (asic->scalingFactorBasedDynamicThresholdSupported()) {
    queue2.scalingFactor() = cfg::MMUScalingFactor::ONE;
  }
  queue2.aqms() = {};
  if (addEcnConfig) {
    queue2.aqms()->push_back(kGetOlympicEcnConfig());
  }
  if (addWredConfig) {
    queue2.aqms()->push_back(kGetWredConfig());
  }
  portQueues.push_back(queue2);

  cfg::PortQueue queue4;
  *queue4.id() = getOlympicQueueId(OlympicQueueType::BRONZE);
  queue4.name() = "queue4.bronze";
  queue4.streamType() = streamType;
  *queue4.scheduling() = schedType;
  queue4.weight() = kOlympicBronzeWeight;
  portQueues.push_back(queue4);

  cfg::PortQueue queue6;
  *queue6.id() = getOlympicQueueId(OlympicQueueType::ICP);
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
  *queue7.id() = getOlympicQueueId(OlympicQueueType::NC);
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
  // For VoQ switches, add AQM config to VoQ as well.
  if (asic->getSwitchType() == cfg::SwitchType::VOQ) {
    addVoqAqmConfig(config, streamType, asic, addWredConfig, addEcnConfig);
  }
}

void addOlympicQueueConfig(
    cfg::SwitchConfig* config,
    cfg::StreamType streamType,
    const HwAsic* asic,
    bool addWredConfig,
    bool addEcnConfig) {
  addOlympicQueueOptionalEcnWredConfigWithSchedulingHelper(
      config,
      streamType,
      asic,
      addWredConfig,
      addEcnConfig,
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
  queue0.id() = getOlympicQueueId(OlympicQueueType::SILVER);
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
  queue2.id() = getOlympicQueueId(OlympicQueueType::ECN1);
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
  // For VoQ switches, add AQM config to VoQ as well.
  if (asic->getSwitchType() == cfg::SwitchType::VOQ) {
    addVoqAqmConfig(
        config,
        streamType,
        asic,
        true /*addWredConfig*/,
        false /*addEcnConfig*/);
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
    queue.id() = getOlympicV2QueueId(queueType);
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

void addOlympicV2WRRQueueConfig(
    cfg::SwitchConfig* config,
    cfg::StreamType streamType,
    const HwAsic* asic,
    bool addWredConfig) {
  std::vector<cfg::PortQueue> portQueues;

  cfg::PortQueue queue0;
  *queue0.id() = getOlympicV2QueueId(OlympicV2QueueType::NCNF);
  queue0.name() = "queue0.ncnf";
  queue0.streamType() = streamType;
  *queue0.scheduling() = cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN;
  queue0.weight() = kOlympicV2NCNFWeight;
  if (asic->scalingFactorBasedDynamicThresholdSupported()) {
    queue0.scalingFactor() = cfg::MMUScalingFactor::ONE;
  }
  if (!asic->mmuQgroupsEnabled()) {
    queue0.reservedBytes() = 3328;
  }
  portQueues.push_back(queue0);

  cfg::PortQueue queue1;
  *queue1.id() = getOlympicV2QueueId(OlympicV2QueueType::BRONZE);
  queue1.name() = "queue1.bronze";
  queue1.streamType() = streamType;
  *queue1.scheduling() = cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN;
  queue1.weight() = kOlympicV2BronzeWeight;
  if (asic->scalingFactorBasedDynamicThresholdSupported()) {
    queue1.scalingFactor() = cfg::MMUScalingFactor::EIGHT;
  }
  if (!asic->mmuQgroupsEnabled()) {
    queue1.reservedBytes() = 9984;
  }
  portQueues.push_back(queue1);

  cfg::PortQueue queue2;
  *queue2.id() = getOlympicV2QueueId(OlympicV2QueueType::SILVER);
  queue2.name() = "queue2.silver";
  queue2.streamType() = streamType;
  *queue2.scheduling() = cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN;
  queue2.weight() = kOlympicV2SilverWeight;
  if (asic->scalingFactorBasedDynamicThresholdSupported()) {
    queue2.scalingFactor() = cfg::MMUScalingFactor::ONE;
  }
  queue2.aqms() = {};
  queue2.aqms()->push_back(kGetOlympicEcnConfig());
  if (addWredConfig) {
    queue2.aqms()->push_back(kGetWredConfig());
  }
  portQueues.push_back(queue2);

  cfg::PortQueue queue3;
  *queue3.id() = getOlympicV2QueueId(OlympicV2QueueType::GOLD);
  queue3.name() = "queue3.gold";
  queue3.streamType() = streamType;
  *queue3.scheduling() = cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN;
  queue3.weight() = kOlympicV2GoldWeight;
  portQueues.push_back(queue3);

  cfg::PortQueue queue6;
  *queue6.id() = getOlympicV2QueueId(OlympicV2QueueType::ICP);
  queue6.name() = "queue6.icp";
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
  *queue7.id() = getOlympicV2QueueId(OlympicV2QueueType::NC);
  queue7.name() = "queue7.nc";
  queue7.streamType() = streamType;
  *queue7.scheduling() = cfg::QueueScheduling::STRICT_PRIORITY;
  portQueues.push_back(queue7);

  config->portQueueConfigs()["queue_config"] = portQueues;
  for (auto& port : *config->ports()) {
    if (*port.portType() == cfg::PortType::INTERFACE_PORT) {
      // Does this check still apply?
      // Apply queue configs on INTERFACE_PORTS only
      port.portQueueConfigName() = "queue_config";
    }
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
      {getOlympicQueueId(OlympicQueueType::SILVER),
       {0,  1,  2,  3,  4,  6,  7,  8,  9,  12, 13,
        14, 15, 40, 41, 42, 43, 44, 45, 46, 47, 49}},
      {getOlympicQueueId(OlympicQueueType::GOLD),
       {18, 24, 31, 33, 34, 36, 37, 38, 39}},
      {getOlympicQueueId(OlympicQueueType::ECN1), {5}},
      {getOlympicQueueId(OlympicQueueType::BRONZE),
       {10, 11, 16, 17, 19, 20, 21, 22, 23, 25, 50, 51,
        52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63}},
      {getOlympicQueueId(OlympicQueueType::ICP), {26, 27, 28, 29, 30, 32, 35}},
      {getOlympicQueueId(OlympicQueueType::NC), {48}}};
  return queueToDscp;
}

const std::map<int, std::vector<uint8_t>> kOlympicV2QueueToDscp(
    const HwAsic* hwAsic) {
  const std::map<int, std::vector<uint8_t>> queueToDscp = {
      {getOlympicV2QueueId(OlympicV2QueueType::NCNF),
       {50, 51, 52, 53, 54, 55, 56, 57, 58, 59}},
      {getOlympicV2QueueId(OlympicV2QueueType::BRONZE),
       {10, 11, 16, 17, 19, 20, 21, 22, 23, 25, 60, 61, 62, 63}},
      {getOlympicV2QueueId(OlympicV2QueueType::SILVER),
       {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  12, 13,
        14, 15, 40, 41, 42, 43, 44, 45, 46, 47, 49}},
      {getOlympicV2QueueId(OlympicV2QueueType::GOLD),
       {18, 24, 31, 33, 34, 36, 37, 38, 39}},
      {getOlympicV2QueueId(OlympicV2QueueType::ICP),
       {26, 27, 28, 29, 30, 32, 35}},
      {getOlympicV2QueueId(OlympicV2QueueType::NC), {48}}};
  return queueToDscp;
}

const std::map<int, uint8_t> kOlympicWRRQueueToWeight(const HwAsic* hwAsic) {
  const std::map<int, uint8_t> wrrQueueToWeight = {
      {getOlympicQueueId(OlympicQueueType::SILVER), kOlympicSilverWeight},
      {getOlympicQueueId(OlympicQueueType::GOLD), kOlympicGoldWeight},
      {getOlympicQueueId(OlympicQueueType::ECN1), kOlympicEcn1Weight},
      {getOlympicQueueId(OlympicQueueType::BRONZE), kOlympicBronzeWeight},
  };
  return wrrQueueToWeight;
}

const std::map<int, uint8_t> kOlympicV2WRRQueueToWeight(const HwAsic* hwAsic) {
  const std::map<int, uint8_t> wrrQueueToWeight = {
      {getOlympicV2QueueId(OlympicV2QueueType::NCNF), kOlympicV2NCNFWeight},
      {getOlympicV2QueueId(OlympicV2QueueType::BRONZE), kOlympicV2BronzeWeight},
      {getOlympicV2QueueId(OlympicV2QueueType::SILVER), kOlympicV2SilverWeight},
      {getOlympicV2QueueId(OlympicV2QueueType::GOLD), kOlympicV2GoldWeight},
  };

  return wrrQueueToWeight;
}

const std::vector<int> kOlympicWRRQueueIds(const HwAsic* hwAsic) {
  const std::vector<int> wrrQueueIds = {
      getOlympicQueueId(OlympicQueueType::SILVER),
      getOlympicQueueId(OlympicQueueType::GOLD),
      getOlympicQueueId(OlympicQueueType::ECN1),
      getOlympicQueueId(OlympicQueueType::BRONZE)};

  return wrrQueueIds;
}

const std::vector<int> kOlympicV2WRRQueueIds(const HwAsic* hwAsic) {
  const std::vector<int> queueIds = {
      getOlympicV2QueueId(OlympicV2QueueType::NCNF),
      getOlympicV2QueueId(OlympicV2QueueType::BRONZE),
      getOlympicV2QueueId(OlympicV2QueueType::SILVER),
      getOlympicV2QueueId(OlympicV2QueueType::GOLD)};

  return queueIds;
}

const std::vector<int> kOlympicSPQueueIds(const HwAsic* hwAsic) {
  const std::vector<int> spQueueIds = {
      getOlympicQueueId(OlympicQueueType::ICP),
      getOlympicQueueId(OlympicQueueType::NC)};

  return spQueueIds;
}

const std::vector<int> kOlympicWRRAndICPQueueIds(const HwAsic* hwAsic) {
  const std::vector<int> wrrAndICPQueueIds = {
      getOlympicQueueId(OlympicQueueType::SILVER),
      getOlympicQueueId(OlympicQueueType::GOLD),
      getOlympicQueueId(OlympicQueueType::ECN1),
      getOlympicQueueId(OlympicQueueType::BRONZE),
      getOlympicQueueId(OlympicQueueType::ICP)};
  return wrrAndICPQueueIds;
}

const std::vector<int> kOlympicWRRAndNCQueueIds(const HwAsic* hwAsic) {
  const std::vector<int> wrrAndNCQueueIds = {
      getOlympicQueueId(OlympicQueueType::SILVER),
      getOlympicQueueId(OlympicQueueType::GOLD),
      getOlympicQueueId(OlympicQueueType::ECN1),
      getOlympicQueueId(OlympicQueueType::BRONZE),
      getOlympicQueueId(OlympicQueueType::NC)};
  return wrrAndNCQueueIds;
}

const std::vector<int> kOlympicAllSPQueueIds(const HwAsic* hwAsic) {
  const std::vector<int> queueIds = {
      getOlympicV2QueueId(OlympicV2QueueType::NCNF),
      getOlympicV2QueueId(OlympicV2QueueType::BRONZE),
      getOlympicV2QueueId(OlympicV2QueueType::SILVER),
      getOlympicV2QueueId(OlympicV2QueueType::GOLD),
      getOlympicV2QueueId(OlympicV2QueueType::ICP),
      getOlympicV2QueueId(OlympicV2QueueType::NC)};

  return queueIds;
}

void addQosMapsHelper(
    cfg::SwitchConfig& cfg,
    const std::map<int, std::vector<uint8_t>>& queueToDscpMap,
    const std::string& qosPolicyName,
    const HwAsic* hwAsic) {
  cfg::QosMap qosMap;
  qosMap.dscpMaps()->resize(queueToDscpMap.size());
  ssize_t qosMapIdx = 0;
  for (const auto& q2dscps : queueToDscpMap) {
    auto [q, dscps] = q2dscps;
    *qosMap.dscpMaps()[qosMapIdx].internalTrafficClass() = q;
    for (auto dscp : dscps) {
      qosMap.dscpMaps()[qosMapIdx].fromDscpToTrafficClass()->push_back(dscp);
    }
    ++qosMapIdx;
  }
  std::map<int16_t, int16_t> tc2Voq;
  for (int q = 0; q <= kOlympicHighestSPQueueId; q++) {
    tc2Voq.emplace(q, q);
    qosMap.trafficClassToQueueId()->emplace(q, q);
  }
  if (hwAsic->isSupported(HwAsic::Feature::VOQ)) {
    qosMap.trafficClassToVoqId() = std::move(tc2Voq);
  }
  cfg.qosPolicies()->resize(1);
  *cfg.qosPolicies()[0].name() = qosPolicyName;
  cfg.qosPolicies()[0].qosMap() = qosMap;

  // configure cpu qos policy
  std::string cpuQosPolicyName = qosPolicyName;
  if (hwAsic->getAsicType() == cfg::AsicType::ASIC_TYPE_JERICHO3) {
    // create and apply a separate qos policy for Jericho3 cpu port
    cpuQosPolicyName = qosPolicyName + "_cpu";
    cfg::QosMap cpuQosMap = qosMap;
    cpuQosMap.trafficClassToQueueId()->clear();
    for (int q = 0; q <= kOlympicHighestSPQueueId; q++) {
      cpuQosMap.trafficClassToQueueId()->emplace(
          q, getTrafficClassToCpuEgressQueueId(hwAsic, q));
    }
    cfg.qosPolicies()->resize(2);
    *cfg.qosPolicies()[1].name() = cpuQosPolicyName;
    cfg.qosPolicies()[1].qosMap() = cpuQosMap;
  }

  cfg::TrafficPolicyConfig dataPlaneTrafficPolicy;
  dataPlaneTrafficPolicy.defaultQosPolicy() = qosPolicyName;
  cfg.dataPlaneTrafficPolicy() = dataPlaneTrafficPolicy;
  cfg::CPUTrafficPolicyConfig cpuConfig;
  cfg::TrafficPolicyConfig cpuTrafficPolicy;
  cpuTrafficPolicy.defaultQosPolicy() = cpuQosPolicyName;
  cpuConfig.trafficPolicy() = cpuTrafficPolicy;
  cfg.cpuTrafficPolicy() = cpuConfig;

  if (hwAsic->getAsicType() == cfg::AsicType::ASIC_TYPE_JERICHO3) {
    // also apply cpu qos policy for recycle port
    PortID kRecyclePort(1);
    overrideQosPolicy(&cfg, kRecyclePort, cpuQosPolicyName);
  }
}

void addOlympicQosMaps(cfg::SwitchConfig& cfg, const HwAsic* hwAsic) {
  addQosMapsHelper(cfg, kOlympicQueueToDscp(hwAsic), "olympic", hwAsic);
}

void addOlympicV2QosMaps(cfg::SwitchConfig& cfg, const HwAsic* hwAsic) {
  addQosMapsHelper(cfg, kOlympicV2QueueToDscp(hwAsic), "olympic_v2", hwAsic);
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
