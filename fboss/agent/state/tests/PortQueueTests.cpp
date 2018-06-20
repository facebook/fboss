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

PortQueue* generatePortQueue() {
  PortQueue* pqObject = new PortQueue(5);
  pqObject->setScheduling(cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN);
  pqObject->setStreamType(cfg::StreamType::UNICAST);
  pqObject->setWeight(5);
  pqObject->setReservedBytes(1000);
  pqObject->setScalingFactor(cfg::MMUScalingFactor::ONE);
  pqObject->setName("queue0");
  pqObject->setPacketsPerSec(200);
  pqObject->setSharedBytes(10000);

  cfg::ActiveQueueManagement aqm;
  cfg::LinearQueueCongestionDetection lqcd;
  lqcd.minimumLength = 42;
  lqcd.maximumLength = 42;
  aqm.detection.set_linear(lqcd);
  aqm.behavior.earlyDrop = true;
  aqm.behavior.ecn = true;
  pqObject->setAqm(aqm);
  return pqObject;
}

PortQueue* generateProdPortQueue() {
  PortQueue* pqObject = new PortQueue(0);
  pqObject->setWeight(1);
  pqObject->setStreamType(cfg::StreamType::UNICAST);
  pqObject->setReservedBytes(3328);
  pqObject->setScheduling(cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN);
  pqObject->setScalingFactor(cfg::MMUScalingFactor::ONE);
  return pqObject;
}

PortQueue* generateProdCPUPortQueue() {
  PortQueue* pqObject = new PortQueue(1);
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
  PortQueue* pqObject = new PortQueue(1);
  return pqObject;
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

TEST(PortQueue, serializationBadFrom) {
  auto pqObject = generatePortQueue();

  auto serialized = pqObject->toFollyDynamic();

  auto noBehavior = serialized;
  noBehavior["aqm"].erase("behavior");
  EXPECT_THROW(PortQueue::fromFollyDynamic(noBehavior), std::exception);

  auto noDetection = serialized;
  noDetection["aqm"].erase("detection");
  EXPECT_THROW(PortQueue::fromFollyDynamic(noDetection), std::exception);

  auto noLinearMaximumThreshold = serialized;
  noLinearMaximumThreshold["aqm"]["detection"]["linear"].erase("maximumLength");
  EXPECT_THROW(
      PortQueue::fromFollyDynamic(noLinearMaximumThreshold), std::exception);

  auto noLinearMinimumThreshold = serialized;
  noLinearMinimumThreshold["aqm"]["detection"]["linear"].erase("minimumLength");
  EXPECT_THROW(
      PortQueue::fromFollyDynamic(noLinearMinimumThreshold), std::exception);
}

TEST(PortQueue, stateDelta) {
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();
  stateV0->registerPort(PortID(1), "port1");
  auto port0 = stateV0->getPort(PortID(1));
  QueueConfig initialQueues;
  for (int i = 0; i < 4; i++) {
    auto q = std::make_shared<PortQueue>(i);
    q->setScheduling(cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN);
    q->setWeight(1);
    initialQueues.push_back(q);
  }
  port0->resetPortQueues(initialQueues);
  port0->publish();
  EXPECT_TRUE(port0->isPublished());

  cfg::SwitchConfig config;
  config.ports.resize(1);
  config.ports[0].logicalID = 1;
  config.ports[0].name = "port1";
  config.ports[0].state = cfg::PortState::ENABLED;
  for (int i = 0; i < 4; i++) {
    cfg::PortQueue queue;
    queue.id = i;
    queue.weight = i;
    queue.__isset.weight = true;
    config.ports[0].queues.push_back(queue);
  }

  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  EXPECT_NE(nullptr, stateV1);
  auto queues1 = stateV1->getPort(PortID(1))->getPortQueues();
  EXPECT_EQ(4, queues1.size());

  config.ports[0].queues[0].weight = 5;

  auto stateV2 = publishAndApplyConfig(stateV1, &config, platform.get());
  EXPECT_NE(nullptr, stateV2);
  auto queues2 = stateV2->getPort(PortID(1))->getPortQueues();
  EXPECT_EQ(4, queues2.size());
  EXPECT_EQ(5, queues2.at(0)->getWeight());

  config.ports[0].queues.pop_back();
  auto stateV3 = publishAndApplyConfig(stateV2, &config, platform.get());
  auto queues3 = stateV3->getPort(PortID(1))->getPortQueues();
  EXPECT_EQ(4, queues3.size());
  EXPECT_EQ(1, queues3.at(3)->getWeight());

  cfg::PortQueue queueExtra;
  queueExtra.id = 11;
  queueExtra.weight = 5;
  queueExtra.__isset.weight = true;
  config.ports[0].queues.push_back(queueExtra);
  EXPECT_THROW(publishAndApplyConfig(stateV3, &config, platform.get()),
      FbossError);
}

TEST(PortQueue, aqmState) {
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();
  stateV0->registerPort(PortID(1), "port1");
  auto port0 = stateV0->getPort(PortID(1));
  auto q = std::make_shared<PortQueue>(0);
  q->setScheduling(cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN);
  q->setWeight(1);
  port0->resetPortQueues({q});
  port0->publish();
  EXPECT_TRUE(port0->isPublished());

  cfg::SwitchConfig config;
  config.ports.resize(1);
  config.ports[0].logicalID = 1;
  config.ports[0].name = "port1";
  config.ports[0].state = cfg::PortState::ENABLED;
  cfg::PortQueue queue;
  cfg::ActiveQueueManagement aqm;
  cfg::LinearQueueCongestionDetection lqcd;
  lqcd.minimumLength = 42;
  lqcd.maximumLength = 42;
  aqm.detection.set_linear(lqcd);
  aqm.behavior.earlyDrop = true;
  aqm.behavior.ecn = true;
  queue.id = 0;
  queue.weight = 1;
  queue.aqm = aqm;
  queue.__isset.aqm = true;
  config.ports[0].queues.push_back(queue);

  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  EXPECT_NE(nullptr, stateV1);
  auto queues1 = stateV1->getPort(PortID(1))->getPortQueues();
  EXPECT_EQ(1, queues1.size());
  EXPECT_EQ(42, queues1.at(0)->getAqm()->detection.get_linear().minimumLength);
  EXPECT_EQ(42, queues1.at(0)->getAqm()->detection.get_linear().maximumLength);
  EXPECT_EQ(true, queues1.at(0)->getAqm()->behavior.ecn);
  EXPECT_EQ(true, queues1.at(0)->getAqm()->behavior.earlyDrop);
}

TEST(PortQueue, aqmBadState) {
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();
  stateV0->registerPort(PortID(1), "port1");
  auto port0 = stateV0->getPort(PortID(1));
  auto q = std::make_shared<PortQueue>(0);
  q->setScheduling(cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN);
  q->setWeight(1);
  port0->resetPortQueues({q});
  port0->publish();
  EXPECT_TRUE(port0->isPublished());

  cfg::SwitchConfig config;
  config.ports.resize(1);
  config.ports[0].logicalID = 1;
  config.ports[0].name = "port1";
  config.ports[0].state = cfg::PortState::ENABLED;
  cfg::PortQueue queue;
  cfg::ActiveQueueManagement aqm;
  cfg::LinearQueueCongestionDetection lqcd;
  lqcd.minimumLength = 42;
  lqcd.maximumLength = 42;
  aqm.behavior.earlyDrop = true;
  aqm.behavior.ecn = true;
  queue.id = 0;
  queue.weight = 1;
  queue.aqm = aqm;
  queue.__isset.aqm = true;
  config.ports[0].queues.push_back(queue);

  EXPECT_THROW(
      publishAndApplyConfig(stateV0, &config, platform.get()), FbossError);
}
