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
#include "fboss/agent/state/PortQueueMap.h"
#include "fboss/agent/state/SwitchState.h"

#include <gtest/gtest.h>

using namespace facebook::fboss;
using std::make_pair;
using std::make_shared;
using std::shared_ptr;
namespace {
}
TEST(PortQueue, stateDelta) {
  auto platform = createMockPlatform();
  auto stateV0 = make_shared<SwitchState>();
  stateV0->registerPort(PortID(1), "port1");
  auto port0 = stateV0->getPort(PortID(1));
  port0->publish();
  EXPECT_TRUE(port0->isPublished());

  cfg::SwitchConfig config;
  config.ports.resize(1);
  config.ports[0].logicalID = 1;
  config.ports[0].name = "port1";
  config.ports[0].state = cfg::PortState::UP;
  for (int i = 0; i < 4; i++) {
    cfg::PortQueue queue;
    queue.id = i;
    queue.weight = i;
    config.ports[0].queues.push_back(queue);
  }

  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  EXPECT_NE(nullptr, stateV1);
  auto queues1 = stateV1->getPort(PortID(1))->getPortQueues();
  ASSERT_NE(nullptr, queues1);
  EXPECT_EQ(4, queues1->size());

  config.ports[0].queues[0].weight = 5;

  auto stateV2 = publishAndApplyConfig(stateV1, &config, platform.get());
  EXPECT_NE(nullptr, stateV2);
  auto queues2 = stateV2->getPort(PortID(1))->getPortQueues();
  EXPECT_NE(nullptr, queues2);
  EXPECT_EQ(4, queues2->size());
  EXPECT_EQ(5, queues2->getQueue(0)->getWeight());

  config.ports[0].queues.pop_back();
  auto stateV3 = publishAndApplyConfig(stateV2, &config, platform.get());
  EXPECT_NE(nullptr, stateV3);
  auto queues3 = stateV3->getPort(PortID(1))->getPortQueues();
  EXPECT_EQ(3, queues3->size());
}
