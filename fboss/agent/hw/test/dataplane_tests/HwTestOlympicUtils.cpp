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

namespace facebook::fboss::utility {

cfg::ActiveQueueManagement kGetOlympicEcnConfig() {
  cfg::ActiveQueueManagement ecnAQM;
  cfg::LinearQueueCongestionDetection ecnLQCD;
  ecnLQCD.minimumLength = 41600;
  ecnLQCD.maximumLength = 41600;
  ecnAQM.detection.set_linear(ecnLQCD);
  ecnAQM.behavior = cfg::QueueCongestionBehavior::ECN;
  return ecnAQM;
}

// XXX This is FSW config, add RSW config. Prefix queue names with portName
void addOlympicQueueConfig(cfg::SwitchConfig* config) {
  std::vector<cfg::PortQueue> portQueues;

  cfg::PortQueue queue0;
  queue0.id = kOlympicSilverQueueId;
  queue0.name_ref() = "queue0.silver";
  queue0.streamType = cfg::StreamType::UNICAST;
  queue0.scheduling = cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN;
  queue0.weight_ref() = kOlympicSilverWeight;
  queue0.scalingFactor_ref() = cfg::MMUScalingFactor::ONE;
  queue0.reservedBytes_ref() = 3328;
  portQueues.push_back(queue0);

  cfg::PortQueue queue1;
  queue1.id = kOlympicGoldQueueId;
  queue1.name_ref() = "queue1.gold";
  queue1.streamType = cfg::StreamType::UNICAST;
  queue1.scheduling = cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN;
  queue1.weight_ref() = kOlympicGoldWeight;
  queue1.scalingFactor_ref() = cfg::MMUScalingFactor::EIGHT;
  queue1.reservedBytes_ref() = 9984;
  portQueues.push_back(queue1);

  cfg::PortQueue queue2;
  queue2.id = kOlympicEcn1QueueId;
  queue2.name_ref() = "queue2.ecn1";
  queue2.streamType = cfg::StreamType::UNICAST;
  queue2.scheduling = cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN;
  queue2.weight_ref() = kOlympicEcn1Weight;
  queue2.scalingFactor_ref() = cfg::MMUScalingFactor::ONE;
  queue2.aqms_ref() = {};
  queue2.aqms_ref()->push_back(kGetOlympicEcnConfig());
  portQueues.push_back(queue2);

  cfg::PortQueue queue4;
  queue4.id = kOlympicBronzeQueueId;
  queue4.name_ref() = "queue4.bronze";
  queue4.streamType = cfg::StreamType::UNICAST;
  queue4.scheduling = cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN;
  queue4.weight_ref() = kOlympicBronzeWeight;
  portQueues.push_back(queue4);

  cfg::PortQueue queue6;
  queue6.id = kOlympicICPQueueId;
  queue6.name_ref() = "queue6.platinum";
  queue6.streamType = cfg::StreamType::UNICAST;
  queue6.scheduling = cfg::QueueScheduling::STRICT_PRIORITY;
  queue6.reservedBytes_ref() = 9984;
  queue6.scalingFactor_ref() = cfg::MMUScalingFactor::EIGHT;
  portQueues.push_back(queue6);

  cfg::PortQueue queue7;
  queue7.id = kOlympicNCQueueId;
  queue7.name_ref() = "queue7.network_control";
  queue7.streamType = cfg::StreamType::UNICAST;
  queue7.scheduling = cfg::QueueScheduling::STRICT_PRIORITY;
  portQueues.push_back(queue7);

  config->portQueueConfigs["queue_config"] = portQueues;
  config->ports[0].portQueueConfigName_ref() = "queue_config";
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
  static const std::vector<int> wrrQueueIds = {kOlympicSilverQueueId,
                                               kOlympicGoldQueueId,
                                               kOlympicEcn1QueueId,
                                               kOlympicBronzeQueueId};

  return wrrQueueIds;
}

const std::vector<int>& kOlympicSPQueueIds() {
  static const std::vector<int> spQueueIds = {kOlympicICPQueueId,
                                              kOlympicNCQueueId};

  return spQueueIds;
}

const std::vector<int>& kOlympicWRRAndICPQueueIds() {
  static const std::vector<int> wrrAndICPQueueIds = {kOlympicSilverQueueId,
                                                     kOlympicGoldQueueId,
                                                     kOlympicEcn1QueueId,
                                                     kOlympicBronzeQueueId,
                                                     kOlympicICPQueueId};
  return wrrAndICPQueueIds;
}

const std::vector<int>& kOlympicWRRAndNCQueueIds() {
  static const std::vector<int> wrrAndNCQueueIds = {kOlympicSilverQueueId,
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

void addOlympicQosMaps(cfg::SwitchConfig& cfg) {
  cfg::QosMap qosMap;
  auto queueToDscpMap = kOlympicQueueToDscp();
  qosMap.dscpMaps.resize(queueToDscpMap.size());
  ssize_t qosMapIdx = 0;
  for (const auto& q2dscps : queueToDscpMap) {
    auto [q, dscps] = q2dscps;
    qosMap.dscpMaps[qosMapIdx].internalTrafficClass = q;
    for (auto dscp : dscps) {
      qosMap.dscpMaps[qosMapIdx].fromDscpToTrafficClass.push_back(dscp);
    }
    qosMap.trafficClassToQueueId.emplace(q, q);
    ++qosMapIdx;
  }
  cfg.qosPolicies.resize(1);
  cfg.qosPolicies[0].name = "qp";
  cfg.qosPolicies[0].qosMap_ref() = qosMap;

  cfg::TrafficPolicyConfig dataPlaneTrafficPolicy;
  dataPlaneTrafficPolicy.defaultQosPolicy_ref() = "qp";
  cfg.dataPlaneTrafficPolicy_ref() = dataPlaneTrafficPolicy;
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
