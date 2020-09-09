/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/test/dataplane_tests/HwTest2QueueUtils.h"

#include "fboss/agent/hw/test/TrafficPolicyUtils.h"

#include "fboss/agent/hw/test/ConfigFactory.h"

namespace facebook::fboss::utility {

// XXX This is FSW config, add RSW config. Prefix queue names with portName
// XXX this is 2Q config minus ECN. If needed, add 2Q + ECN config
void add2QueueConfig(cfg::SwitchConfig* config, PortID portID) {
  std::vector<cfg::PortQueue> portQueues;

  cfg::PortQueue queue0;
  queue0.id = k2QueueLowPriQueueId;
  queue0.name_ref() = "queue0.low_pri";
  queue0.streamType = cfg::StreamType::UNICAST;
  queue0.scheduling = cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN;
  queue0.weight_ref() = k2QueueLowPriWeight;
  queue0.scalingFactor_ref() = cfg::MMUScalingFactor::ONE;
  queue0.reservedBytes_ref() = 3328;
  portQueues.push_back(queue0);

  cfg::PortQueue queue1;
  queue1.id = k2QueueHighPriQueueId;
  queue1.name_ref() = "queue1.high_pri";
  queue1.streamType = cfg::StreamType::UNICAST;
  queue1.scheduling = cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN;
  queue1.weight_ref() = k2QueueHighPriWeight;
  queue1.scalingFactor_ref() = cfg::MMUScalingFactor::EIGHT;
  queue1.reservedBytes_ref() = 9984;
  portQueues.push_back(queue1);

  cfg::PortQueue queue7;
  queue7.id = k2QueueNCQueueId;
  queue7.name_ref() = "queue7.network_control";
  queue7.streamType = cfg::StreamType::UNICAST;
  queue7.scheduling = cfg::QueueScheduling::STRICT_PRIORITY;
  portQueues.push_back(queue7);

  config->portQueueConfigs_ref()["queue_config"] = portQueues;
  auto portCfg = std::find_if(
      config->ports_ref()->begin(),
      config->ports_ref()->end(),
      [&portID](auto& port) {
        return PortID(*port.logicalID_ref()) == portID;
      });
  portCfg->portQueueConfigName_ref() = "queue_config";
}

std::string get2QueueAclNameForDscp(uint8_t dscp) {
  return folly::to<std::string>("2queue_acl_dscp", dscp);
}

std::string get2QueueCounterNameForDscp(uint8_t dscp) {
  return folly::to<std::string>("dscp", dscp, "_counter");
}

void add2QueueQosMaps(cfg::SwitchConfig& cfg) {
  cfg::QosMap qosMap;
  auto queueToDscpMap = k2QueueToDscp();
  qosMap.dscpMaps_ref()->resize(queueToDscpMap.size());
  ssize_t qosMapIdx = 0;
  for (const auto& q2dscps : queueToDscpMap) {
    auto [q, dscps] = q2dscps;
    *qosMap.dscpMaps_ref()[qosMapIdx].internalTrafficClass_ref() = q;
    for (auto dscp : dscps) {
      qosMap.dscpMaps_ref()[qosMapIdx].fromDscpToTrafficClass_ref()->push_back(
          dscp);
    }
    qosMap.trafficClassToQueueId_ref()->emplace(q, q);
    ++qosMapIdx;
  }
  cfg.qosPolicies_ref()->resize(1);
  *cfg.qosPolicies_ref()[0].name_ref() = "qp";
  cfg.qosPolicies_ref()[0].qosMap_ref() = qosMap;

  cfg::TrafficPolicyConfig dataPlaneTrafficPolicy;
  dataPlaneTrafficPolicy.defaultQosPolicy_ref() = "qp";
  cfg.dataPlaneTrafficPolicy_ref() = dataPlaneTrafficPolicy;
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

} // namespace facebook::fboss::utility
