/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/ApplyThriftConfig.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/mock/MockPlatform.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/PortQueue.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/TestUtils.h"

#include <gtest/gtest.h>

using namespace facebook::fboss;
using std::make_pair;
using std::make_shared;
using std::shared_ptr;

namespace {

cfg::Range getRange(uint32_t minimum, uint32_t maximum) {
  cfg::Range range;
  range.minimum() = minimum;
  range.maximum() = maximum;

  return range;
}

cfg::PortQueueRate getPortQueueRatePps(uint32_t minimum, uint32_t maximum) {
  cfg::PortQueueRate portQueueRate;
  portQueueRate.pktsPerSec_ref() = getRange(minimum, maximum);

  return portQueueRate;
}

cfg::ActiveQueueManagement getEarlyDropAqmConfig() {
  cfg::ActiveQueueManagement earlyDropAQM;
  cfg::LinearQueueCongestionDetection earlyDropLQCD;
  earlyDropLQCD.minimumLength() = 208;
  earlyDropLQCD.maximumLength() = 416;
  earlyDropLQCD.probability() = 100;
  earlyDropAQM.detection()->linear_ref() = earlyDropLQCD;
  earlyDropAQM.behavior() = cfg::QueueCongestionBehavior::EARLY_DROP;
  return earlyDropAQM;
}

cfg::ActiveQueueManagement getECNAqmConfig() {
  cfg::ActiveQueueManagement ecnAQM;
  cfg::LinearQueueCongestionDetection ecnLQCD;
  ecnLQCD.minimumLength() = 624;
  ecnLQCD.maximumLength() = 624;
  ecnAQM.detection()->linear_ref() = ecnLQCD;
  ecnAQM.behavior() = cfg::QueueCongestionBehavior::ECN;
  return ecnAQM;
}

cfg::PortQueue generateTestQueueConfig(const int id) {
  cfg::PortQueue queue0;
  queue0.id() = id;
  queue0.name() = folly::to<std::string>("queue_", id);
  queue0.streamType() = cfg::StreamType::UNICAST;
  queue0.scheduling() = cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN;
  queue0.weight() = 9;
  queue0.scalingFactor() = cfg::MMUScalingFactor::EIGHT;
  queue0.reservedBytes() = 19968;
  queue0.sharedBytes() = 19968;
  queue0.portQueueRate() = cfg::PortQueueRate();
  queue0.portQueueRate()->pktsPerSec_ref() = getRange(0, 100);
  queue0.aqms() = {};
  queue0.aqms()->push_back(getECNAqmConfig());
  queue0.aqms()->push_back(getEarlyDropAqmConfig());
  return queue0;
}

cfg::SwitchConfig generateTestConfig(int queueCount = 1) {
  cfg::SwitchConfig config;
  config.ports()->resize(1);
  preparedMockPortConfig(config.ports()[0], 1);
  // program multiple queues if needed, settings can be default
  for (int i = 0; i < queueCount; ++i) {
    auto queue0 = generateTestQueueConfig(i);
    config.portQueueConfigs()["queue_config"].push_back(queue0);
  }
  config.ports()[0].portQueueConfigName() = "queue_config";
  return config;
}

cfg::QosPolicy generateQosPolicy(const std::map<uint16_t, uint16_t>& map) {
  cfg::QosPolicy policy;
  cfg::QosMap qosMap;
  for (auto entry : map) {
    qosMap.trafficClassToQueueId()->emplace(entry.first, entry.second);
  }
  *policy.name() = "policy";
  policy.qosMap() = qosMap;
  return policy;
}

cfg::QosPolicy generateQosPolicyWithPfcAndTrafficClass(
    std::map<int16_t, int16_t> trafficClass,
    std::map<int16_t, int16_t> pfcPriority) {
  cfg::QosPolicy policy;
  cfg::QosMap qosMap;
  qosMap.pfcPriorityToQueueId() = pfcPriority;
  qosMap.trafficClassToQueueId() = trafficClass;
  policy.name() = "policy";
  policy.qosMap() = qosMap;
  return policy;
}

PortQueue* generatePortQueue() {
  static PortQueue pqObject(static_cast<uint8_t>(5));
  pqObject.setScheduling(cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN);
  pqObject.setStreamType(cfg::StreamType::UNICAST);
  pqObject.setWeight(5);
  pqObject.setReservedBytes(1000);
  pqObject.setScalingFactor(cfg::MMUScalingFactor::ONE);
  pqObject.setName("queue0");
  pqObject.setPortQueueRate(getPortQueueRatePps(0, 200));
  pqObject.setSharedBytes(10000);
  std::vector<cfg::ActiveQueueManagement> aqms;
  aqms.push_back(getEarlyDropAqmConfig());
  aqms.push_back(getECNAqmConfig());
  pqObject.resetAqms(aqms);
  return &pqObject;
}

PortQueue* generateProdPortQueue() {
  static PortQueue pqObject(static_cast<uint8_t>(0));
  pqObject.setWeight(1);
  pqObject.setStreamType(cfg::StreamType::UNICAST);
  pqObject.setReservedBytes(3328);
  pqObject.setScheduling(cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN);
  pqObject.setScalingFactor(cfg::MMUScalingFactor::ONE);
  return &pqObject;
}

PortQueue* generateProdCPUPortQueue() {
  static PortQueue pqObject(static_cast<uint8_t>(1));
  pqObject.setName("cpuQueue-default");
  pqObject.setStreamType(cfg::StreamType::MULTICAST);
  pqObject.setWeight(1);
  pqObject.setScheduling(cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN);
  pqObject.setPortQueueRate(getPortQueueRatePps(0, 200));
  pqObject.setReservedBytes(1000);
  pqObject.setSharedBytes(10000);
  return &pqObject;
}

PortQueue* generateDefaultPortQueue() {
  // Most of the queues in our system are using default values
  static PortQueue pqObject(static_cast<uint8_t>(1));
  return &pqObject;
}

constexpr int kStateTestNumPortQueues = 4;
std::shared_ptr<SwitchState> applyInitConfig() {
  auto stateV0 = make_shared<SwitchState>();
  stateV0->registerPort(PortID(1), "port1");
  auto port0 = stateV0->getPort(PortID(1));
  QueueConfig initialQueues;
  for (uint8_t i = 0; i < kStateTestNumPortQueues; i++) {
    auto q = std::make_shared<PortQueue>(i);
    q->setScheduling(cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN);
    q->setWeight(1);
    initialQueues.push_back(q);
  }
  port0->resetPortQueues(initialQueues);
  stateV0->publish();
  EXPECT_TRUE(port0->isPublished());
  return stateV0;
}
} // unnamed namespace

TEST(PortQueue, serialization) {
  std::vector<PortQueue*> queues = {
      generatePortQueue(),
      generateProdPortQueue(),
      generateProdCPUPortQueue(),
      generateDefaultPortQueue()};

  for (const auto* pqObject : queues) {
    auto serialized = pqObject->toThrift();
    auto deserialized = std::make_shared<PortQueue>(serialized);
    EXPECT_EQ(*pqObject, *deserialized);
  }
}

TEST(PortQueue, stateDelta) {
  auto platform = createMockPlatform();
  auto stateV0 = applyInitConfig();

  cfg::SwitchConfig config;
  config.ports()->resize(1);
  preparedMockPortConfig(config.ports()[0], 1);
  for (int i = 0; i < kStateTestNumPortQueues; i++) {
    cfg::PortQueue queue;
    queue.id() = i;
    queue.weight() = i;
    config.portQueueConfigs()["queue_config"].push_back(queue);
    config.ports()[0].portQueueConfigName() = "queue_config";
  }

  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  EXPECT_NE(nullptr, stateV1);
  auto queues1 = stateV1->getPort(PortID(1))->getPortQueues();
  EXPECT_EQ(
      platform->getAsic()->getDefaultNumPortQueues(
          cfg::StreamType::UNICAST, false),
      queues1->size());
  // The first kStateTestNumPortQueues should have weight changed
  for (int i = 0; i < kStateTestNumPortQueues; i++) {
    EXPECT_EQ(i, queues1->at(i)->getWeight());
  }
  // The rest queue should be default
  for (int i = kStateTestNumPortQueues;
       i < platform->getAsic()->getDefaultNumPortQueues(
               cfg::StreamType::UNICAST, false);
       i++) {
    auto defaultQ = std::make_shared<PortQueue>(static_cast<uint8_t>(i));
    defaultQ->setStreamType(cfg::StreamType::UNICAST);
    EXPECT_EQ(*defaultQ, *(queues1->at(i)));
  }

  config.portQueueConfigs()["queue_config"][0].weight() = 5;

  auto stateV2 = publishAndApplyConfig(stateV1, &config, platform.get());
  EXPECT_NE(nullptr, stateV2);
  auto queues2 = stateV2->getPort(PortID(1))->getPortQueues();
  EXPECT_EQ(
      platform->getAsic()->getDefaultNumPortQueues(
          cfg::StreamType::UNICAST, false),
      queues2->size());
  EXPECT_EQ(5, queues2->at(0)->getWeight());

  config.portQueueConfigs()["queue_config"].pop_back();
  auto stateV3 = publishAndApplyConfig(stateV2, &config, platform.get());
  auto queues3 = stateV3->getPort(PortID(1))->getPortQueues();
  EXPECT_EQ(
      platform->getAsic()->getDefaultNumPortQueues(
          cfg::StreamType::UNICAST, false),
      queues3->size());
  EXPECT_EQ(1, queues3->at(3)->getWeight());

  cfg::PortQueue queueExtra;
  queueExtra.id() = 11;
  queueExtra.weight() = 5;
  config.portQueueConfigs()["queue_config"].push_back(queueExtra);
  EXPECT_THROW(
      publishAndApplyConfig(stateV3, &config, platform.get()), FbossError);
}

TEST(PortQueue, aqmState) {
  auto platform = createMockPlatform();
  auto stateV0 = applyInitConfig();

  cfg::SwitchConfig config;
  config.ports()->resize(1);
  preparedMockPortConfig(config.ports()[0], 1);
  cfg::PortQueue queue;
  queue.id() = 0;
  queue.weight() = 1;
  queue.aqms() = {};
  queue.aqms()->push_back(getEarlyDropAqmConfig());
  config.portQueueConfigs()["queue_config"].push_back(queue);
  config.ports()[0].portQueueConfigName() = "queue_config";

  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  EXPECT_NE(nullptr, stateV1);
  auto queues1 = stateV1->getPort(PortID(1))->getPortQueues();
  // change one queue, won't affect the other queues
  EXPECT_EQ(
      platform->getAsic()->getDefaultNumPortQueues(
          cfg::StreamType::UNICAST, false),
      queues1->size());
  std::vector<cfg::ActiveQueueManagement> aqms{};
  aqms.resize(1);
  aqms[0] = getEarlyDropAqmConfig();
  EXPECT_EQ(queues1->at(0)->getAqms()->toThrift(), aqms);
}

TEST(PortQueue, aqmBadState) {
  auto platform = createMockPlatform();
  auto stateV0 = applyInitConfig();

  cfg::SwitchConfig config;
  config.ports()->resize(1);
  preparedMockPortConfig(config.ports()[0], 1);
  cfg::PortQueue queue;
  queue.id() = 0;
  queue.weight() = 1;

  // create bad ECN AQM state w/o specifying thresholds
  cfg::ActiveQueueManagement ecnAQM;
  ecnAQM.behavior() = cfg::QueueCongestionBehavior::ECN;
  queue.aqms() = {};
  queue.aqms()->push_back(getEarlyDropAqmConfig());
  queue.aqms()->push_back(ecnAQM);

  config.portQueueConfigs()["queue_config"].push_back(queue);
  config.ports()[0].portQueueConfigName() = "queue_config";

  EXPECT_THROW(
      publishAndApplyConfig(stateV0, &config, platform.get()), FbossError);
}

TEST(PortQueue, resetPartOfConfigs) {
  auto platform = createMockPlatform();
  auto stateV0 = applyInitConfig();

  {
    auto config = generateTestConfig();
    auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
    EXPECT_NE(nullptr, stateV1);
    auto queues1 = stateV1->getPort(PortID(1))->getPortQueues();
    EXPECT_TRUE(queues1->at(0)->getReservedBytes().has_value());

    // reset reservedBytes
    config.portQueueConfigs()["queue_config"][0].reservedBytes().reset();

    auto stateV2 = publishAndApplyConfig(stateV1, &config, platform.get());
    EXPECT_TRUE(stateV2 != nullptr);
    auto queues2 = stateV2->getPort(PortID(1))->getPortQueues();
    EXPECT_FALSE(queues2->at(0)->getReservedBytes().has_value());
  }
  {
    auto config = generateTestConfig();
    auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
    EXPECT_NE(nullptr, stateV1);
    auto queues1 = stateV1->getPort(PortID(1))->getPortQueues();
    EXPECT_TRUE(queues1->at(0)->getScalingFactor().has_value());

    // reset scalingFactor
    config.portQueueConfigs()["queue_config"][0].scalingFactor().reset();

    auto stateV2 = publishAndApplyConfig(stateV1, &config, platform.get());
    EXPECT_TRUE(stateV2 != nullptr);
    auto queues2 = stateV2->getPort(PortID(1))->getPortQueues();
    EXPECT_FALSE(queues2->at(0)->getScalingFactor().has_value());
  }
  {
    auto config = generateTestConfig();
    auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
    EXPECT_NE(nullptr, stateV1);
    auto queues1 = stateV1->getPort(PortID(1))->getPortQueues();
    EXPECT_EQ(2, queues1->at(0)->getAqms()->size());

    // reset aqm
    config.portQueueConfigs()["queue_config"][0].aqms().reset();

    auto stateV2 = publishAndApplyConfig(stateV1, &config, platform.get());
    EXPECT_TRUE(stateV2 != nullptr);
    auto queues2 = stateV2->getPort(PortID(1))->getPortQueues();
    EXPECT_TRUE(
        !queues2->at(0)->getAqms() || queues2->at(0)->getAqms()->empty());
  }
}

TEST(PortQueue, checkSwConfPortQueueMatch) {
  auto platform = createMockPlatform();
  auto stateV0 = applyInitConfig();

  auto config = generateTestConfig();
  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  EXPECT_NE(nullptr, stateV1);
  auto swQueues = stateV1->getPort(PortID(1))->getPortQueues();
  auto& cfgQueue = config.portQueueConfigs()["queue_config"][0];
  EXPECT_TRUE(checkSwConfPortQueueMatch(swQueues->at(0), &cfgQueue));
}

TEST(PortQueue, checkValidPortQueueConfigRef) {
  auto platform = createMockPlatform();
  auto stateV0 = applyInitConfig();

  cfg::SwitchConfig config;
  config.ports()->resize(1);
  preparedMockPortConfig(config.ports()[0], 1);

  cfg::PortQueue queue0;
  queue0.id() = 0;
  queue0.name() = "queue0";
  queue0.streamType() = cfg::StreamType::UNICAST;
  queue0.scheduling() = cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN;

  /*
   * portQueueConfigs map has entry for "queue_config", but the port is
   * referencing invalid entry "queue_config2"
   */
  config.portQueueConfigs()["queue_config"].push_back(queue0);
  config.ports()[0].portQueueConfigName() = "queue_config2";

  EXPECT_THROW(
      publishAndApplyConfig(stateV0, &config, platform.get()), FbossError);
}

TEST(PortQueue, checkNoPortQueueTrafficClass) {
  auto platform = createMockPlatform();
  auto state = applyInitConfig();

  auto config = generateTestConfig();
  state = publishAndApplyConfig(state, &config, platform.get());
  auto swQueues = state->getPort(PortID(1))->getPortQueues();

  EXPECT_FALSE(swQueues->at(0)->getTrafficClass().has_value());
}

TEST(PortQueue, checkPortQueueTrafficClass) {
  auto platform = createMockPlatform();
  auto state = applyInitConfig();
  auto config = generateTestConfig();
  auto policy = generateQosPolicy({{9, 0}});
  config.qosPolicies()->push_back(policy);
  cfg::TrafficPolicyConfig policyConfig;
  policyConfig.defaultQosPolicy() = *policy.name();
  config.dataPlaneTrafficPolicy() = policyConfig;
  state = publishAndApplyConfig(state, &config, platform.get());
  auto swQueues = state->getPort(PortID(1))->getPortQueues();

  EXPECT_EQ(
      swQueues->at(0)->getTrafficClass().value(), static_cast<TrafficClass>(9));
}

TEST(PortQueue, addPortQueueTrafficClass) {
  auto platform = createMockPlatform();
  auto state = applyInitConfig();

  auto config = generateTestConfig();
  state = publishAndApplyConfig(state, &config, platform.get());
  auto swQueues = state->getPort(PortID(1))->getPortQueues();
  EXPECT_FALSE(swQueues->at(0)->getTrafficClass().has_value());

  auto policy = generateQosPolicy({{9, 0}});
  config.qosPolicies()->push_back(policy);
  cfg::TrafficPolicyConfig policyConfig;
  policyConfig.defaultQosPolicy() = *policy.name();
  config.dataPlaneTrafficPolicy() = policyConfig;
  state = publishAndApplyConfig(state, &config, platform.get());
  swQueues = state->getPort(PortID(1))->getPortQueues();
  EXPECT_EQ(
      swQueues->at(0)->getTrafficClass().value(), static_cast<TrafficClass>(9));
}

TEST(PortQueue, updatePortQueueTrafficClass) {
  auto platform = createMockPlatform();
  auto state = applyInitConfig();
  auto config = generateTestConfig();
  auto policy = generateQosPolicy({{9, 0}});
  config.qosPolicies()->push_back(policy);
  cfg::TrafficPolicyConfig policyConfig;
  policyConfig.defaultQosPolicy() = *policy.name();
  config.dataPlaneTrafficPolicy() = policyConfig;
  state = publishAndApplyConfig(state, &config, platform.get());

  config.qosPolicies()[0].qosMap()->trafficClassToQueueId()->clear();
  config.qosPolicies()[0].qosMap()->trafficClassToQueueId()->emplace(7, 0);
  state = publishAndApplyConfig(state, &config, platform.get());
  auto swQueues = state->getPort(PortID(1))->getPortQueues();
  EXPECT_EQ(
      swQueues->at(0)->getTrafficClass().value(), static_cast<TrafficClass>(7));
}

TEST(PortQueue, removePortQueueTrafficClass) {
  auto platform = createMockPlatform();
  auto state = applyInitConfig();
  auto config = generateTestConfig();
  auto policy = generateQosPolicy({{9, 0}});
  config.qosPolicies()->push_back(policy);
  cfg::TrafficPolicyConfig policyConfig;
  policyConfig.defaultQosPolicy() = *policy.name();
  config.dataPlaneTrafficPolicy() = policyConfig;
  state = publishAndApplyConfig(state, &config, platform.get());

  config.qosPolicies()[0].qosMap()->trafficClassToQueueId()->clear();
  state = publishAndApplyConfig(state, &config, platform.get());
  auto swQueues = state->getPort(PortID(1))->getPortQueues();
  EXPECT_FALSE(swQueues->at(0)->getTrafficClass().has_value());
}

TEST(PortQueue, verifyPfcPriorityToQueue) {
  auto platform = createMockPlatform();
  auto state = applyInitConfig();
  auto config = generateTestConfig();

  state = publishAndApplyConfig(state, &config, platform.get());
  auto swQueues = state->getPort(PortID(1))->getPortQueues();

  EXPECT_FALSE(swQueues->at(0)->getPfcPrioritySet().has_value());

  auto policy = generateQosPolicyWithPfcAndTrafficClass({}, {{7, 0}});
  config.qosPolicies()->push_back(policy);

  cfg::TrafficPolicyConfig policyConfig;
  policyConfig.defaultQosPolicy() = *policy.name();
  config.dataPlaneTrafficPolicy() = policyConfig;
  state = publishAndApplyConfig(state, &config, platform.get());
  swQueues = state->getPort(PortID(1))->getPortQueues();

  std::set<PfcPriority> pfcPrisForQueue0;
  pfcPrisForQueue0.insert(static_cast<PfcPriority>(7));
  EXPECT_EQ(swQueues->at(0)->getPfcPrioritySet().value(), pfcPrisForQueue0);

  config.qosPolicies()[0].qosMap()->pfcPriorityToQueueId()->clear();
  state = publishAndApplyConfig(state, &config, platform.get());
  swQueues = state->getPort(PortID(1))->getPortQueues();
  EXPECT_FALSE(swQueues->at(0)->getPfcPrioritySet().has_value());

  config.qosPolicies()[0].qosMap()->pfcPriorityToQueueId()->emplace(2, 0);

  pfcPrisForQueue0.clear();
  pfcPrisForQueue0.insert(static_cast<PfcPriority>(2));

  state = publishAndApplyConfig(state, &config, platform.get());
  swQueues = state->getPort(PortID(1))->getPortQueues();
  EXPECT_EQ(swQueues->at(0)->getPfcPrioritySet().value(), pfcPrisForQueue0);
}

TEST(PortQueue, verifyMultiplePfcPriorityToSingleQueue) {
  auto platform = createMockPlatform();
  auto state = applyInitConfig();
  // generate 2 queues as part of the test config
  auto config = generateTestConfig(2);

  state = publishAndApplyConfig(state, &config, platform.get());
  auto swQueues = state->getPort(PortID(1))->getPortQueues();

  EXPECT_FALSE(swQueues->at(0)->getPfcPrioritySet().has_value());

  // insert pfcPri {2,3,7} for queue 0
  // {4} for queue 1
  auto policy = generateQosPolicyWithPfcAndTrafficClass(
      {}, {{7, 0}, {2, 0}, {3, 0}, {4, 1}});
  config.qosPolicies()->push_back(policy);

  cfg::TrafficPolicyConfig policyConfig;
  policyConfig.defaultQosPolicy() = *policy.name();
  config.dataPlaneTrafficPolicy() = policyConfig;
  state = publishAndApplyConfig(state, &config, platform.get());
  swQueues = state->getPort(PortID(1))->getPortQueues();

  // expect swQueue 0 to have pfcPri {2,3,7}
  std::set<PfcPriority> pfcPrisForQueue0;
  pfcPrisForQueue0.insert(static_cast<PfcPriority>(7));
  pfcPrisForQueue0.insert(static_cast<PfcPriority>(2));
  pfcPrisForQueue0.insert(static_cast<PfcPriority>(3));

  EXPECT_EQ(swQueues->at(0)->getPfcPrioritySet().value(), pfcPrisForQueue0);

  std::set<PfcPriority> pfcPrisForQueue1;
  pfcPrisForQueue1.insert(static_cast<PfcPriority>(4));
  EXPECT_EQ(swQueues->at(1)->getPfcPrioritySet().value(), pfcPrisForQueue1);
}

TEST(PortQueue, verifyPfcPriorityAndTrafficClass) {
  auto platform = createMockPlatform();
  auto state = applyInitConfig();
  auto config = generateTestConfig();

  auto policy = generateQosPolicyWithPfcAndTrafficClass({{7, 0}}, {{2, 0}});
  config.qosPolicies()->push_back(policy);

  cfg::TrafficPolicyConfig policyConfig;
  policyConfig.defaultQosPolicy() = *policy.name();
  config.dataPlaneTrafficPolicy() = policyConfig;
  state = publishAndApplyConfig(state, &config, platform.get());
  auto swQueues = state->getPort(PortID(1))->getPortQueues();

  std::set<PfcPriority> pfcPris;
  pfcPris.insert(static_cast<PfcPriority>(2));

  EXPECT_EQ(swQueues->at(0)->getPfcPrioritySet().value(), pfcPris);
  EXPECT_EQ(
      swQueues->at(0)->getTrafficClass().value(), static_cast<TrafficClass>(7));
}
