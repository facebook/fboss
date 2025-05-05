/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/test/utils/NetworkAITestUtils.h"
#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/test/utils/AsicUtils.h"
#include "fboss/agent/test/utils/QueueTestUtils.h"
#include "fboss/agent/test/utils/TrafficPolicyTestUtils.h"
#include "fboss/agent/test/utils/VoqTestUtils.h"

namespace facebook::fboss::utility {
const std::map<int, std::vector<uint8_t>> kNetworkAIQueueToDscp() {
  const std::map<int, std::vector<uint8_t>> queueToDscp = {
      {getNetworkAIQueueId(NetworkAIQueueType::DEFAULT),
       {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15,
        16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 31, 33, 34, 36, 37, 38,
        39, 40, 41, 42, 43, 44, 45, 46, 47, 49, 60, 61, 62, 63}},
      {getNetworkAIQueueId(NetworkAIQueueType::RDMA),
       {50, 51, 52, 53, 54, 55, 56, 57, 58, 59}},
      {getNetworkAIQueueId(NetworkAIQueueType::MONITORING),
       {32, 35, 26, 27, 28, 29, 30}},
      {getNetworkAIQueueId(NetworkAIQueueType::NC), {48}}};
  return queueToDscp;
}

void addNetworkAIQosMaps(
    cfg::SwitchConfig& cfg,
    const std::vector<const HwAsic*>& asics) {
  std::string qosPolicyName = "network_ai_v2";
  auto hwAsic = checkSameAndGetAsic(asics);
  cfg::QosMap qosMap;
  qosMap.dscpMaps()->resize(kNetworkAIQueueToDscp().size());
  ssize_t qosMapIdx = 0;
  for (const auto& q2dscps : kNetworkAIQueueToDscp()) {
    auto [q, dscps] = q2dscps;
    *qosMap.dscpMaps()[qosMapIdx].internalTrafficClass() = q;
    for (auto dscp : dscps) {
      qosMap.dscpMaps()[qosMapIdx].fromDscpToTrafficClass()->push_back(dscp);
    }
    ++qosMapIdx;
  }
  std::map<int16_t, int16_t> tc2Voq;
  for (int q = 0; q <= kNetworkAINCQueueId; q++) {
    tc2Voq.emplace(q, getTrafficClassToVoqId(hwAsic, q));
    qosMap.trafficClassToQueueId()->emplace(
        q, getTrafficClassToEgressQueueId(hwAsic, q));
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
    for (int q = 0; q <= kNetworkAINCQueueId; q++) {
      cpuQosMap.trafficClassToQueueId()->emplace(
          q, getTrafficClassToCpuEgressQueueId(hwAsic, q));
    }
    cpuQosMap.trafficClassToVoqId()->clear();
    for (int q = 0; q <= kNetworkAINCQueueId; q++) {
      cpuQosMap.trafficClassToVoqId()->emplace(
          q, getTrafficClassToCpuVoqId(hwAsic, q));
    }
    cfg.qosPolicies()->resize(2);
    *cfg.qosPolicies()[1].name() = cpuQosPolicyName;
    cfg.qosPolicies()[1].qosMap() = cpuQosMap;
  }

  cfg::TrafficPolicyConfig dataPlaneTrafficPolicy;
  dataPlaneTrafficPolicy.defaultQosPolicy() = qosPolicyName;
  cfg.dataPlaneTrafficPolicy() = dataPlaneTrafficPolicy;
  cfg::CPUTrafficPolicyConfig cpuConfig;
  if (cfg.cpuTrafficPolicy()) {
    cpuConfig = *cfg.cpuTrafficPolicy();
  }
  cfg::TrafficPolicyConfig cpuTrafficPolicy;
  if (cpuConfig.trafficPolicy()) {
    cpuTrafficPolicy = *cpuConfig.trafficPolicy();
  }
  cpuTrafficPolicy.defaultQosPolicy() = cpuQosPolicyName;
  cpuConfig.trafficPolicy() = cpuTrafficPolicy;
  cfg.cpuTrafficPolicy() = cpuConfig;

  if (hwAsic->getAsicType() == cfg::AsicType::ASIC_TYPE_JERICHO3) {
    // also apply cpu qos policy for recycle port
    for (const auto& port : *cfg.ports()) {
      if (*port.portType() == cfg::PortType::RECYCLE_PORT ||
          *port.portType() == cfg::PortType::EVENTOR_PORT) {
        XLOG(DBG2) << "override and use rcy qos policy for port "
                   << *port.logicalID();
        overrideQosPolicy(&cfg, *port.logicalID(), cpuQosPolicyName);
      }
    }
  }
}

void addNetworkAIQueueConfig(
    cfg::SwitchConfig* config,
    cfg::StreamType streamType,
    cfg::QueueScheduling schedType,
    const HwAsic* hwAsic,
    bool addWredConfig,
    bool addEcnConfig,
    std::unordered_map<NetworkAIQueueType, cfg::QueueScheduling>
        schedTypeOverride) {
  std::vector<cfg::PortQueue> portQueues;
  cfg::PortQueue queue0;
  queue0.id() = getNetworkAIQueueId(NetworkAIQueueType::RDMA);
  queue0.name() = "queue2.rdma";
  queue0.streamType() = streamType;
  auto it = schedTypeOverride.find(NetworkAIQueueType::RDMA);
  queue0.scheduling() =
      (it == schedTypeOverride.end()) ? schedType : it->second;
  if (schedType == cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN) {
    queue0.weight() = kNetworkAIRdmaWeight;
  }
  queue0.aqms() = {};
  if (addEcnConfig) {
    queue0.aqms()->push_back(GetEcnConfig(*hwAsic));
  }
  if (addWredConfig) {
    queue0.aqms()->push_back(GetWredConfig(*hwAsic));
  }
  portQueues.push_back(queue0);

  cfg::PortQueue queue1;
  queue1.id() = getNetworkAIQueueId(NetworkAIQueueType::MONITORING);
  queue1.name() = "queue6.monitoring";
  queue1.streamType() = streamType;
  it = schedTypeOverride.find(NetworkAIQueueType::MONITORING);
  queue1.scheduling() =
      (it == schedTypeOverride.end()) ? schedType : it->second;
  if (schedType == cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN) {
    queue1.weight() = kNetworkAIMonitoringWeight;
  }
  portQueues.push_back(queue1);

  cfg::PortQueue queue2;
  queue2.id() = getNetworkAIQueueId(NetworkAIQueueType::NC);
  queue2.name() = "queue7.nc";
  queue2.streamType() = streamType;
  it = schedTypeOverride.find(NetworkAIQueueType::NC);
  queue2.scheduling() =
      (it == schedTypeOverride.end()) ? schedType : it->second;
  if (schedType == cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN) {
    queue2.weight() = kNetworkAINcWeight;
  }
  portQueues.push_back(queue2);

  cfg::PortQueue queue3;
  queue3.id() = getNetworkAIQueueId(NetworkAIQueueType::DEFAULT);
  queue3.name() = "queue0.default";
  queue3.streamType() = streamType;
  it = schedTypeOverride.find(NetworkAIQueueType::DEFAULT);
  queue3.scheduling() =
      (it == schedTypeOverride.end()) ? schedType : it->second;
  if (schedType == cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN) {
    queue3.weight() = kNetworkAIDefaultWeight;
  }
  portQueues.push_back(queue3);

  config->defaultPortQueues() = portQueues;

  if (hwAsic->getSwitchType() == cfg::SwitchType::VOQ) {
    addVoqAqmConfig(config, streamType, hwAsic, addWredConfig, addEcnConfig);
  }
}

int getNetworkAIQueueId(NetworkAIQueueType queueType) {
  switch (queueType) {
    case NetworkAIQueueType::MONITORING:
      return kNetworkAIMonitoringQueueId;
    case NetworkAIQueueType::RDMA:
      return kNetworkAIRdmaQueueId;
    case NetworkAIQueueType::NC:
      return kNetworkAINCQueueId;
    case NetworkAIQueueType::DEFAULT:
      return kNetworkAIDefaultQueueId;
  }
  throw FbossError("Invalid all network AI queue type ", queueType);
}

void addVoqAqmConfig(
    cfg::SwitchConfig* config,
    cfg::StreamType streamType,
    const HwAsic* asic,
    bool addWredConfig,
    bool addEcnConfig) {
  auto nameAndDefaultVoqCfg =
      getNameAndDefaultVoqCfg(cfg::PortType::INTERFACE_PORT);
  CHECK(nameAndDefaultVoqCfg.has_value());
  std::vector<cfg::PortQueue> voqs = nameAndDefaultVoqCfg->queueConfig;
  for (auto& voq : voqs) {
    auto voqId = voq.id();
    if (asic->scalingFactorBasedDynamicThresholdSupported()) {
      voq.scalingFactor() = cfg::MMUScalingFactor::ONE;
    }
    voq.reservedBytes() = 1500; // Set to possible MTU!

    if (voqId == getTrafficClassToVoqId(asic, 2)) {
      voq.aqms() = {};
      if (addEcnConfig) {
        voq.aqms()->push_back(GetEcnConfig(*asic));
      }
      if (addWredConfig) {
        voq.aqms()->push_back(GetWredConfig(*asic));
      }
    }
  }
  config->defaultVoqConfig() = voqs;
}

void addEventorVoqConfig(
    cfg::SwitchConfig* config,
    cfg::StreamType streamType) {
  // Eventor port queue config
  cfg::PortQueue queue;
  *queue.id() = 0;
  queue.streamType() = streamType;
  queue.name() = "default";
  *queue.scheduling() = cfg::QueueScheduling::INTERNAL;
  queue.maxDynamicSharedBytes() = 1024 * 1024; // 1MB
  queue.scalingFactor() = cfg::MMUScalingFactor::ONE_HALF;
  std::vector<cfg::PortQueue> eventorVoqConfig{std::move(queue)};
  const std::string kEventorQueueConfigName{"eventor_queue_config"};
  config->portQueueConfigs()[kEventorQueueConfigName] = eventorVoqConfig;
  for (auto& port : *config->ports()) {
    if (*port.portType() == cfg::PortType::EVENTOR_PORT) {
      port.portVoqConfigName() = kEventorQueueConfigName;
    }
  }
}

const std::vector<int> kNetworkAISPQueueIds() {
  const std::vector<int> spQueueIds = {
      getNetworkAIQueueId(NetworkAIQueueType::RDMA),
      getNetworkAIQueueId(NetworkAIQueueType::NC)};

  return spQueueIds;
}

const std::map<int, uint8_t> kNetworkAIWRRQueueToWeight() {
  const std::map<int, uint8_t> wrrQueueToWeight = {
      {getNetworkAIQueueId(NetworkAIQueueType::DEFAULT),
       kNetworkAIDefaultWeight},
      {getNetworkAIQueueId(NetworkAIQueueType::RDMA), kNetworkAIRdmaWeight},
      {getNetworkAIQueueId(NetworkAIQueueType::MONITORING),
       kNetworkAIMonitoringWeight},
      {getNetworkAIQueueId(NetworkAIQueueType::NC), kNetworkAINcWeight},
  };
  const std::map<int, uint8_t> wrrQueueToWeight3Q2Q = {
      {getNetworkAIQueueId(NetworkAIQueueType::RDMA), kNetworkAIRdmaWeight},
      {getNetworkAIQueueId(NetworkAIQueueType::MONITORING),
       kNetworkAIMonitoringWeight},
      {getNetworkAIQueueId(NetworkAIQueueType::NC), kNetworkAINcWeight},
  };
  return isDualStage3Q2QQos() ? wrrQueueToWeight3Q2Q : wrrQueueToWeight;
}

const std::vector<int> kNetworkAIWRRQueueIds() {
  const std::vector<int> wrrQueueIds = {
      getNetworkAIQueueId(NetworkAIQueueType::DEFAULT),
      getNetworkAIQueueId(NetworkAIQueueType::RDMA),
      getNetworkAIQueueId(NetworkAIQueueType::MONITORING),
      getNetworkAIQueueId(NetworkAIQueueType::NC)};
  const std::vector<int> wrrQueueIds3Q2Q = {
      getNetworkAIQueueId(NetworkAIQueueType::RDMA),
      getNetworkAIQueueId(NetworkAIQueueType::MONITORING),
      getNetworkAIQueueId(NetworkAIQueueType::NC)};

  return isDualStage3Q2QQos() ? wrrQueueIds3Q2Q : wrrQueueIds;
}

const std::vector<int> kNetworkAIWRRAndICPQueueIds() {
  const std::vector<int> queueIds = {
      getNetworkAIQueueId(NetworkAIQueueType::DEFAULT),
      getNetworkAIQueueId(NetworkAIQueueType::RDMA),
      getNetworkAIQueueId(NetworkAIQueueType::MONITORING)};
  const std::vector<int> queueIds3Q2Q = {
      getNetworkAIQueueId(NetworkAIQueueType::RDMA),
      getNetworkAIQueueId(NetworkAIQueueType::MONITORING)};

  return isDualStage3Q2QQos() ? queueIds3Q2Q : queueIds;
}

const std::vector<int> kNetworkAIWRRAndNCQueueIds() {
  const std::vector<int> queueIds = {
      getNetworkAIQueueId(NetworkAIQueueType::DEFAULT),
      getNetworkAIQueueId(NetworkAIQueueType::RDMA),
      getNetworkAIQueueId(NetworkAIQueueType::NC)};
  const std::vector<int> queueIds3Q2Q = {
      getNetworkAIQueueId(NetworkAIQueueType::RDMA),
      getNetworkAIQueueId(NetworkAIQueueType::NC)};

  return isDualStage3Q2QQos() ? queueIds3Q2Q : queueIds;
}

}; // namespace facebook::fboss::utility
