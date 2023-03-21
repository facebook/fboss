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

#include "fboss/agent/hw/switch_asics/HwAsic.h"

namespace facebook::fboss::utility {

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

void addQueueShaperConfig(
    cfg::SwitchConfig* config,
    const int queueId,
    const uint32_t minKbps,
    const uint32_t maxKbps) {
  cfg::Range kbpsRange;
  kbpsRange.minimum() = minKbps;
  kbpsRange.maximum() = maxKbps;
  auto& queue = config->portQueueConfigs()["queue_config"][queueId];
  queue.portQueueRate() = cfg::PortQueueRate();
  queue.portQueueRate()->kbitsPerSec_ref() = kbpsRange;
}

void addQueueBurstSizeConfig(
    cfg::SwitchConfig* config,
    const int queueId,
    const uint32_t minKbits,
    const uint32_t maxKbits) {
  auto& queue = config->portQueueConfigs()["queue_config"][queueId];
  queue.bandwidthBurstMinKbits() = minKbits;
  queue.bandwidthBurstMaxKbits() = maxKbits;
}

void addQueueEcnConfig(
    cfg::SwitchConfig* config,
    const int queueId,
    const uint32_t minLen,
    const uint32_t maxLen) {
  auto& queue = config->portQueueConfigs()["queue_config"][queueId];
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
    const int probability) {
  auto& queue = config->portQueueConfigs()["queue_config"][queueId];
  if (!queue.aqms().has_value()) {
    queue.aqms() = {};
  }
  queue.aqms()->push_back(kGetWredConfig(minLen, maxLen, probability));
}

void addNetworkAIQueueConfig(
    cfg::SwitchConfig* config,
    cfg::StreamType streamType) {
  std::vector<cfg::PortQueue> portQueues;

  cfg::PortQueue queue0;
  queue0.id() = kNetworkAIMonitoringQueueId;
  queue0.name() = "queue0.monitoring";
  queue0.streamType() = streamType;
  queue0.scheduling() = cfg::QueueScheduling::STRICT_PRIORITY;
  portQueues.push_back(queue0);

  cfg::PortQueue queue1;
  queue0.id() = kNetworkAIRdmaQueueId;
  queue0.name() = "queue6.rdma";
  queue0.streamType() = streamType;
  queue0.scheduling() = cfg::QueueScheduling::STRICT_PRIORITY;
  portQueues.push_back(queue1);

  cfg::PortQueue queue2;
  queue0.id() = kNetworkAINCQueueId;
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
    bool addWredConfig) {
  std::vector<cfg::PortQueue> voqConfig;
  for (auto queueId = kOlympicSilverQueueId; queueId <= kOlympicNCQueueId;
       queueId++) {
    cfg::PortQueue queue;
    *queue.id() = queueId;
    queue.streamType() = streamType;
    queue.name() = folly::to<std::string>("queue", queueId);
    *queue.scheduling() = cfg::QueueScheduling::INTERNAL;

    if (queueId == kOlympicEcn1QueueId) {
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
void addOlympicQueueConfig(
    cfg::SwitchConfig* config,
    cfg::StreamType streamType,
    const HwAsic* asic,
    bool addWredConfig) {
  std::vector<cfg::PortQueue> portQueues;

  cfg::PortQueue queue0;
  *queue0.id() = kOlympicSilverQueueId;
  queue0.name() = "queue0.silver";
  queue0.streamType() = streamType;
  *queue0.scheduling() = cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN;
  queue0.weight() = kOlympicSilverWeight;
  if (asic->scalingFactorBasedDynamicThresholdSupported()) {
    queue0.scalingFactor() = cfg::MMUScalingFactor::ONE;
  }
  if (!asic->mmuQgroupsEnabled()) {
    queue0.reservedBytes() = 3328;
  }
  portQueues.push_back(queue0);

  cfg::PortQueue queue1;
  *queue1.id() = kOlympicGoldQueueId;
  queue1.name() = "queue1.gold";
  queue1.streamType() = streamType;
  *queue1.scheduling() = cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN;
  queue1.weight() = kOlympicGoldWeight;
  if (asic->scalingFactorBasedDynamicThresholdSupported()) {
    queue1.scalingFactor() = cfg::MMUScalingFactor::EIGHT;
  }
  if (!asic->mmuQgroupsEnabled()) {
    queue1.reservedBytes() = 9984;
  }
  portQueues.push_back(queue1);

  cfg::PortQueue queue2;
  *queue2.id() = kOlympicEcn1QueueId;
  queue2.name() = "queue2.ecn1";
  queue2.streamType() = streamType;
  *queue2.scheduling() = cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN;
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
  *queue4.id() = kOlympicBronzeQueueId;
  queue4.name() = "queue4.bronze";
  queue4.streamType() = streamType;
  *queue4.scheduling() = cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN;
  queue4.weight() = kOlympicBronzeWeight;
  portQueues.push_back(queue4);

  cfg::PortQueue queue6;
  *queue6.id() = kOlympicICPQueueId;
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
  *queue7.id() = kOlympicNCQueueId;
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
    addVoqQueueConfig(config, streamType, addWredConfig);
  }
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
  queue0.id() = kOlympicSilverQueueId;
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
  queue2.id() = kOlympicEcn1QueueId;
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

const std::map<int, std::string>& kOlympicAllSPQueueIdToQueueName() {
  static const std::map<int, std::string> queueIdToQueueName = {
      {kOlympicAllSPNCNFQueueId, "queue0.ncnf"},
      {kOlympicAllSPBronzeQueueId, "queue1.bronze"},
      {kOlympicAllSPSilverQueueId, "queue2.silver"},
      {kOlympicAllSPGoldQueueId, "queue3.gold"},
      {kOlympicAllSPICPQueueId, "queeu6.icp"},
      {kOlympicAllSPNCQueueId, "queue7.nc"},
  };

  return queueIdToQueueName;
}

void addOlympicAllSPQueueConfig(
    cfg::SwitchConfig* config,
    cfg::StreamType streamType) {
  std::vector<cfg::PortQueue> portQueues;

  for (const auto& [queueId, queueName] : kOlympicAllSPQueueIdToQueueName()) {
    cfg::PortQueue queue;
    queue.id() = queueId;
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

const std::map<int, std::vector<uint8_t>>& kOlympicQueueToDscp() {
  static const std::map<int, std::vector<uint8_t>> queueToDscp = {
      {kOlympicSilverQueueId, {0,  1,  2,  3,  4,  6,  7,  8,  9,  12, 13,
                               14, 15, 40, 41, 42, 43, 44, 45, 46, 47, 49}},

      {kOlympicGoldQueueId, {18, 24, 31, 33, 34, 36, 37, 38, 39}},
      {kOlympicEcn1QueueId, {5}},
      {kOlympicBronzeQueueId, {10, 11, 16, 17, 19, 20, 21, 22, 23, 25, 50, 51,
                               52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63}},
      {kOlympicICPQueueId, {26, 27, 28, 29, 30, 32, 35}},
      {kOlympicNCQueueId, {48}}};
  return queueToDscp;
}

const std::map<int, std::vector<uint8_t>>& kOlympicAllSPQueueToDscp() {
  static const std::map<int, std::vector<uint8_t>> queueToDscp = {
      {kOlympicAllSPNCNFQueueId, {50, 51, 52, 53, 54, 55, 56, 57, 58, 59}},
      {kOlympicAllSPBronzeQueueId,
       {10, 11, 16, 17, 19, 20, 21, 22, 23, 25, 60, 61, 62, 63}},
      {kOlympicAllSPSilverQueueId,
       {0,  1,  2,  3,  4,  6,  7,  8,  9,  12, 13,
        14, 15, 40, 41, 42, 43, 44, 45, 46, 47, 49}},
      {kOlympicAllSPGoldQueueId, {18, 24, 31, 33, 34, 36, 37, 38, 39}},
      {kOlympicAllSPICPQueueId, {26, 27, 28, 29, 30, 32, 35}},
      {kOlympicAllSPNCQueueId, {48}}};
  return queueToDscp;
}

const std::map<int, uint8_t>& kOlympicWRRQueueToWeight() {
  static const std::map<int, uint8_t> wrrQueueToWeight = {
      {kOlympicSilverQueueId, kOlympicSilverWeight},
      {kOlympicGoldQueueId, kOlympicGoldWeight},
      {kOlympicEcn1QueueId, kOlympicEcn1Weight},
      {kOlympicBronzeQueueId, kOlympicBronzeWeight},
  };

  return wrrQueueToWeight;
}

const std::vector<int>& kOlympicWRRQueueIds() {
  static const std::vector<int> wrrQueueIds = {
      kOlympicSilverQueueId,
      kOlympicGoldQueueId,
      kOlympicEcn1QueueId,
      kOlympicBronzeQueueId};

  return wrrQueueIds;
}

const std::vector<int>& kOlympicSPQueueIds() {
  static const std::vector<int> spQueueIds = {
      kOlympicICPQueueId, kOlympicNCQueueId};

  return spQueueIds;
}

const std::vector<int>& kOlympicWRRAndICPQueueIds() {
  static const std::vector<int> wrrAndICPQueueIds = {
      kOlympicSilverQueueId,
      kOlympicGoldQueueId,
      kOlympicEcn1QueueId,
      kOlympicBronzeQueueId,
      kOlympicICPQueueId};
  return wrrAndICPQueueIds;
}

const std::vector<int>& kOlympicWRRAndNCQueueIds() {
  static const std::vector<int> wrrAndNCQueueIds = {
      kOlympicSilverQueueId,
      kOlympicGoldQueueId,
      kOlympicEcn1QueueId,
      kOlympicBronzeQueueId,
      kOlympicNCQueueId};
  return wrrAndNCQueueIds;
}

bool isOlympicWRRQueueId(int queueId) {
  return kOlympicWRRQueueToWeight().find(queueId) !=
      kOlympicWRRQueueToWeight().end();
}

const std::vector<int>& kOlympicAllSPQueueIds() {
  static const std::vector<int> queueIds = {
      kOlympicAllSPNCNFQueueId,
      kOlympicAllSPBronzeQueueId,
      kOlympicAllSPSilverQueueId,
      kOlympicAllSPGoldQueueId,
      kOlympicAllSPICPQueueId,
      kOlympicAllSPNCQueueId};

  return queueIds;
}

void addOlympicQosMapsHelper(
    cfg::SwitchConfig& cfg,
    const std::map<int, std::vector<uint8_t>>& queueToDscpMap) {
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
  *cfg.qosPolicies()[0].name() = "olympic";
  cfg.qosPolicies()[0].qosMap() = qosMap;

  cfg::TrafficPolicyConfig dataPlaneTrafficPolicy;
  dataPlaneTrafficPolicy.defaultQosPolicy() = "olympic";
  cfg.dataPlaneTrafficPolicy() = dataPlaneTrafficPolicy;
  cfg::CPUTrafficPolicyConfig cpuConfig;
  cfg::TrafficPolicyConfig cpuTrafficPolicy;
  cpuTrafficPolicy.defaultQosPolicy() = "olympic";
  cpuConfig.trafficPolicy() = cpuTrafficPolicy;
  cfg.cpuTrafficPolicy() = cpuConfig;
}

void addOlympicQosMaps(cfg::SwitchConfig& cfg) {
  addOlympicQosMapsHelper(cfg, kOlympicQueueToDscp());
}

void addOlympicAllSPQosMaps(cfg::SwitchConfig& cfg) {
  addOlympicQosMapsHelper(cfg, kOlympicAllSPQueueToDscp());
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
