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
#include "fboss/agent/test/TestUtils.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/mock/MockPlatform.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/PortQueue.h"
#include "fboss/agent/state/SwitchState.h"

#include <gtest/gtest.h>

using namespace facebook::fboss;
using std::make_pair;
using std::make_shared;
using std::shared_ptr;

namespace {
cfg::ActiveQueueManagement getEarlyDropAqmConfig() {
  cfg::ActiveQueueManagement earlyDropAQM;
  cfg::LinearQueueCongestionDetection earlyDropLQCD;
  earlyDropLQCD.minimumLength = 208;
  earlyDropLQCD.maximumLength = 416;
  earlyDropAQM.detection.set_linear(earlyDropLQCD);
  earlyDropAQM.behavior = cfg::QueueCongestionBehavior::EARLY_DROP;
  return earlyDropAQM;
}

cfg::ActiveQueueManagement getECNAqmConfig() {
  cfg::ActiveQueueManagement ecnAQM;
  cfg::LinearQueueCongestionDetection ecnLQCD;
  ecnLQCD.minimumLength = 624;
  ecnLQCD.maximumLength = 624;
  ecnAQM.detection.set_linear(ecnLQCD);
  ecnAQM.behavior = cfg::QueueCongestionBehavior::ECN;
  return ecnAQM;
}

cfg::SwitchConfig generateTestConfig() {
  cfg::SwitchConfig config;
  config.ports.resize(1);
  config.ports[0].logicalID = 1;
  config.ports[0].name_ref().value_unchecked() = "port1";
  config.ports[0].state = cfg::PortState::ENABLED;
  // we just need to test the any queue and set every setting
  cfg::PortQueue queue1;
  queue1.id = 1;
  queue1.name_ref().value_unchecked() = "queue1";
  queue1.streamType = cfg::StreamType::UNICAST;
  queue1.scheduling = cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN;
  queue1.weight_ref() = 9;
  queue1.scalingFactor_ref() = cfg::MMUScalingFactor::EIGHT;
  queue1.reservedBytes_ref() = 19968;
  queue1.sharedBytes_ref() = 19968;
  queue1.packetsPerSec_ref() = 100;
  queue1.aqms_ref().value_unchecked().push_back(getEarlyDropAqmConfig());
  queue1.aqms_ref().value_unchecked().push_back(getECNAqmConfig());
  queue1.__isset.aqms = true;

  config.ports[0].queues.push_back(queue1);
  return config;
}

PortQueue* generatePortQueue() {
  PortQueue* pqObject = new PortQueue(static_cast<uint8_t>(5));
  pqObject->setScheduling(cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN);
  pqObject->setStreamType(cfg::StreamType::UNICAST);
  pqObject->setWeight(5);
  pqObject->setReservedBytes(1000);
  pqObject->setScalingFactor(cfg::MMUScalingFactor::ONE);
  pqObject->setName("queue0");
  pqObject->setPacketsPerSec(200);
  pqObject->setSharedBytes(10000);
  std::vector<cfg::ActiveQueueManagement> aqms;
  aqms.push_back(getEarlyDropAqmConfig());
  aqms.push_back(getECNAqmConfig());
  pqObject->resetAqms(aqms);
  return pqObject;
}

PortQueue* generateProdPortQueue() {
  PortQueue* pqObject = new PortQueue(static_cast<uint8_t>(0));
  pqObject->setWeight(1);
  pqObject->setStreamType(cfg::StreamType::UNICAST);
  pqObject->setReservedBytes(3328);
  pqObject->setScheduling(cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN);
  pqObject->setScalingFactor(cfg::MMUScalingFactor::ONE);
  return pqObject;
}

PortQueue* generateProdCPUPortQueue() {
  PortQueue* pqObject = new PortQueue(static_cast<uint8_t>(1));
  pqObject->setName("cpuQueue-default");
  pqObject->setStreamType(cfg::StreamType::MULTICAST);
  pqObject->setWeight(1);
  pqObject->setScheduling(cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN);
  pqObject->setPacketsPerSec(200);
  pqObject->setReservedBytes(1000);
  pqObject->setSharedBytes(10000);
  return pqObject;
}

PortQueue* generateDefaultPortQueue() {
  // Most of the queues in our system are using default values
  PortQueue* pqObject = new PortQueue(static_cast<uint8_t>(1));
  return pqObject;
}

constexpr int kStateTestDefaultNumPortQueues = 4;
std::shared_ptr<SwitchState> applyInitConfig() {
  auto stateV0 = make_shared<SwitchState>();
  stateV0->registerPort(PortID(1), "port1");
  auto port0 = stateV0->getPort(PortID(1));
  QueueConfig initialQueues;
  for (uint8_t i = 0; i < kStateTestDefaultNumPortQueues; i++) {
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
    generatePortQueue(), generateProdPortQueue(), generateProdCPUPortQueue(),
    generateDefaultPortQueue()};

  for (const auto* pqObject: queues) {
    auto serialized = pqObject->toFollyDynamic();
    auto deserialized = PortQueue::fromFollyDynamic(serialized);
    EXPECT_EQ(*pqObject, *deserialized);
  }
}

TEST(PortQueue, serializationBadForm) {
  auto pqObject = generatePortQueue();

  auto serialized = pqObject->toFollyDynamic();

  auto noBehavior = serialized;
  noBehavior["aqms"][0].erase("behavior");
  EXPECT_THROW(PortQueue::fromFollyDynamic(noBehavior), std::exception);

  auto noDetection = serialized;
  noDetection["aqms"][0].erase("detection");
  EXPECT_THROW(PortQueue::fromFollyDynamic(noDetection), std::exception);

  auto noLinearMaximumThreshold = serialized;
  noLinearMaximumThreshold["aqms"][0]["detection"]["linear"].erase(
    "maximumLength");
  EXPECT_THROW(
      PortQueue::fromFollyDynamic(noLinearMaximumThreshold), std::exception);

  auto noLinearMinimumThreshold = serialized;
  noLinearMinimumThreshold["aqms"][0]["detection"]["linear"].erase("minimumLength");
  EXPECT_THROW(
      PortQueue::fromFollyDynamic(noLinearMinimumThreshold), std::exception);
}

TEST(PortQueue, stateDelta) {
  auto platform = createMockPlatform();
  auto stateV0 = applyInitConfig();

  cfg::SwitchConfig config;
  config.ports.resize(1);
  config.ports[0].logicalID = 1;
  config.ports[0].name_ref().value_unchecked() = "port1";
  config.ports[0].state = cfg::PortState::ENABLED;
  for (int i = 0; i < kStateTestDefaultNumPortQueues; i++) {
    cfg::PortQueue queue;
    queue.id = i;
    queue.weight_ref() = i;
    config.ports[0].queues.push_back(queue);
  }

  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  EXPECT_NE(nullptr, stateV1);
  auto queues1 = stateV1->getPort(PortID(1))->getPortQueues();
  EXPECT_EQ(kStateTestDefaultNumPortQueues, queues1.size());

  config.ports[0].queues[0].weight_ref().value_unchecked() = 5;

  auto stateV2 = publishAndApplyConfig(stateV1, &config, platform.get());
  EXPECT_NE(nullptr, stateV2);
  auto queues2 = stateV2->getPort(PortID(1))->getPortQueues();
  EXPECT_EQ(kStateTestDefaultNumPortQueues, queues2.size());
  EXPECT_EQ(5, queues2.at(0)->getWeight());

  config.ports[0].queues.pop_back();
  auto stateV3 = publishAndApplyConfig(stateV2, &config, platform.get());
  auto queues3 = stateV3->getPort(PortID(1))->getPortQueues();
  EXPECT_EQ(kStateTestDefaultNumPortQueues, queues3.size());
  EXPECT_EQ(1, queues3.at(3)->getWeight());

  cfg::PortQueue queueExtra;
  queueExtra.id = 11;
  queueExtra.weight_ref() = 5;
  config.ports[0].queues.push_back(queueExtra);
  EXPECT_THROW(publishAndApplyConfig(stateV3, &config, platform.get()),
      FbossError);
}

TEST(PortQueue, aqmState) {
  auto platform = createMockPlatform();
  auto stateV0 = applyInitConfig();

  cfg::SwitchConfig config;
  config.ports.resize(1);
  config.ports[0].logicalID = 1;
  config.ports[0].name_ref().value_unchecked() = "port1";
  config.ports[0].state = cfg::PortState::ENABLED;
  cfg::PortQueue queue;
  queue.id = 0;
  queue.weight_ref().value_unchecked() = 1;
  queue.aqms_ref().value_unchecked().push_back(getEarlyDropAqmConfig());
  queue.__isset.aqms = true;
  config.ports[0].queues.push_back(queue);

  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  EXPECT_NE(nullptr, stateV1);
  auto queues1 = stateV1->getPort(PortID(1))->getPortQueues();
  // change one queue, won't affect the other queues
  EXPECT_EQ(kStateTestDefaultNumPortQueues, queues1.size());
  std::vector<cfg::ActiveQueueManagement> aqms;
  aqms.push_back(getEarlyDropAqmConfig());
  EXPECT_TRUE(comparePortQueueAQMs(queues1.at(0)->getAqms(), aqms));
}

TEST(PortQueue, aqmBadState) {
  auto platform = createMockPlatform();
  auto stateV0 = applyInitConfig();

  cfg::SwitchConfig config;
  config.ports.resize(1);
  config.ports[0].logicalID = 1;
  config.ports[0].name_ref().value_unchecked() = "port1";
  config.ports[0].state = cfg::PortState::ENABLED;
  cfg::PortQueue queue;
  queue.id = 0;
  queue.weight_ref().value_unchecked() = 1;

  // create bad ECN AQM state w/o specifying thresholds
  cfg::ActiveQueueManagement ecnAQM;
  ecnAQM.behavior = cfg::QueueCongestionBehavior::ECN;
  queue.aqms_ref().value_unchecked().push_back(getEarlyDropAqmConfig());
  queue.aqms_ref().value_unchecked().push_back(ecnAQM);
  queue.__isset.aqms = true;

  config.ports[0].queues.push_back(queue);

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

    // reset reservedBytes
    config.ports[0].queues[0].__isset.reservedBytes = false;

    auto stateV2 = publishAndApplyConfig(stateV1, &config, platform.get());
    EXPECT_TRUE(stateV2 != nullptr);
    auto queues2 = stateV2->getPort(PortID(1))->getPortQueues();
    EXPECT_FALSE(queues2.at(0)->getReservedBytes().hasValue());
  }
  {
    auto config = generateTestConfig();
    auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
    EXPECT_NE(nullptr, stateV1);

    // reset scalingFactor
    config.ports[0].queues[0].__isset.scalingFactor = false;

    auto stateV2 = publishAndApplyConfig(stateV1, &config, platform.get());
    EXPECT_TRUE(stateV2 != nullptr);
    auto queues2 = stateV2->getPort(PortID(1))->getPortQueues();
    EXPECT_FALSE(queues2.at(0)->getScalingFactor().hasValue());
  }
  {
    auto config = generateTestConfig();
    auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
    EXPECT_NE(nullptr, stateV1);

    // reset aqm
    config.ports[0].queues[0].aqms_ref().value_unchecked().clear();
    config.ports[0].queues[0].__isset.aqms = false;

    auto stateV2 = publishAndApplyConfig(stateV1, &config, platform.get());
    EXPECT_TRUE(stateV2 != nullptr);
    auto queues2 = stateV2->getPort(PortID(1))->getPortQueues();
    EXPECT_TRUE(queues2.at(0)->getAqms().empty());
  }
}
