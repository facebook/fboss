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
#include "fboss/agent/hw/mock/MockPlatform.h"
#include "fboss/agent/state/AggregatePort.h"
#include "fboss/agent/state/AggregatePortMap.h"
#include "fboss/agent/state/DeltaFunctions.h"
#include "fboss/agent/state/NodeMapDelta.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/TestUtils.h"

#include <gtest/gtest.h>
#include <algorithm>
#include <vector>

using namespace facebook::fboss;
using std::make_shared;
using std::shared_ptr;

/* Some tests below are structured as follows:
 * baseState --[apply "baseConfig"]--> startState --[apply "config"]--> endState
 * In these tests, what's being tested is the transition from startState and
 * endSate.
 * baseConfig is used simply as a convenient way to transition baseState to
 * startState.
 */

void checkAggPort(
    shared_ptr<AggregatePort> aggPort,
    AggregatePortID expectedId,
    std::string expectedName,
    std::string expectedDescription,
    std::vector<uint32_t> expectedSubports,
    uint32_t expectedGeneration) {
  ASSERT_NE(nullptr, aggPort);
  EXPECT_EQ(expectedId, aggPort->getID());
  EXPECT_EQ(expectedName, aggPort->getName());
  EXPECT_EQ(expectedDescription, aggPort->getDescription());
  EXPECT_EQ(expectedGeneration, aggPort->getGeneration());

  EXPECT_EQ(
      aggPort->subportsCount(),
      std::distance(expectedSubports.begin(), expectedSubports.end()));

  ASSERT_TRUE(
      std::is_sorted(expectedSubports.cbegin(), expectedSubports.cend()));
  auto expectedSubportsBegin = expectedSubports.cbegin();
  auto expectedSubportsBeyond = expectedSubports.cend();

  ASSERT_TRUE(std::is_sorted(
      aggPort->sortedSubports().cbegin(), aggPort->sortedSubports().cend()));
  auto subportsBegin = aggPort->sortedSubports().cbegin();
  auto subportsBeyond = aggPort->sortedSubports().cend();

  auto equivalent = [](
      PortID actual, PortID expected) -> testing::AssertionResult {
    if (actual == expected) {
      return testing::AssertionSuccess();
    }

    if (actual < expected) {
      return testing::AssertionFailure() << actual << " unexpected";
    } else {
      ASSERT(actual > expected);
      return testing::AssertionFailure() << expected << " missing";
    }
  };

  while (subportsBegin != subportsBeyond &&
         expectedSubportsBegin != expectedSubportsBeyond) {
    auto actual = *subportsBegin;
    auto expected = PortID(*expectedSubportsBegin);

    EXPECT_TRUE(equivalent(actual, expected));

    if (actual < expected) {
      ++subportsBegin;
    } else if (actual > expected) {
      ++expectedSubportsBegin;
    } else {
      ++subportsBegin;
      ++expectedSubportsBegin;
    }
  }

  for (; subportsBegin < subportsBeyond; ++subportsBegin) {
    EXPECT_TRUE(testing::AssertionFailure() << *subportsBegin << " unexpected");
  }
  for (; expectedSubportsBegin < expectedSubportsBeyond;
       ++expectedSubportsBegin) {
    EXPECT_TRUE(
        testing::AssertionFailure() << *expectedSubportsBegin << " expected");
  }
}

TEST(AggregatePort, singleTrunkWithOnePhysicalPort) {
  MockPlatform platform;
  auto startState = make_shared<SwitchState>();
  startState->registerPort(PortID(1), "port1");

  // This config has an aggregate port comprised of a single physical port
  cfg::SwitchConfig config;
  config.ports.resize(1);
  config.ports[0].logicalID = 1;
  config.ports[0].state = cfg::PortState::UP;

  config.vlans.resize(1);
  config.vlans[0].id = 1000;
  config.vlans[0].name = "vlan1000";
  config.vlans[0].routable = true;

  config.interfaces.resize(1);
  config.interfaces[0].intfID = 1000;
  config.interfaces[0].vlanID = 1000;
  config.interfaces[0].ipAddresses.resize(2);
  config.interfaces[0].ipAddresses[0] = "1.2.3.4/24";
  config.interfaces[0].ipAddresses[1] = "2a03:2880:10:1f07:face:b00c:0:0/96";

  config.vlanPorts.resize(1);
  config.vlanPorts[0].logicalPort = 1;
  config.vlanPorts[0].vlanID = 1000;
  config.vlanPorts[0].emitTags = false;

  config.__isset.aggregatePorts = true;
  config.aggregatePorts.resize(1);
  config.aggregatePorts[0].key = 1;
  config.aggregatePorts[0].name = "port-channel";
  config.aggregatePorts[0].description = "single bundle";
  config.aggregatePorts[0].physicalPorts.resize(1);
  config.aggregatePorts[0].physicalPorts[0] = 1;

  auto endState = publishAndApplyConfig(startState, &config, &platform);
  ASSERT_NE(nullptr, endState);
  auto aggPort =
      endState->getAggregatePorts()->getAggregatePortIf(AggregatePortID(1));
  ASSERT_NE(nullptr, aggPort);

  checkAggPort(
      aggPort, AggregatePortID(1), "port-channel", "single bundle", {1}, 0);
}

TEST(AggregatePort, singleTrunkWithTwoPhysicalPorts) {
  MockPlatform platform;
  auto baseState = make_shared<SwitchState>();
  baseState->registerPort(PortID(1), "port1");
  baseState->registerPort(PortID(2), "port2");

  cfg::SwitchConfig baseConfig;
  baseConfig.ports.resize(2);
  baseConfig.ports[0].logicalID = 1;
  baseConfig.ports[0].state = cfg::PortState::UP;
  baseConfig.ports[1].logicalID = 2;
  baseConfig.ports[1].state = cfg::PortState::UP;

  baseConfig.vlans.resize(1);
  baseConfig.vlans[0].id = 1000;
  baseConfig.vlans[0].name = "vlan1000";
  baseConfig.vlans[0].routable = true;

  baseConfig.interfaces.resize(1);
  baseConfig.interfaces[0].intfID = 1000;
  baseConfig.interfaces[0].vlanID = 1000;
  baseConfig.interfaces[0].ipAddresses.resize(2);
  baseConfig.interfaces[0].ipAddresses[0] = "1.2.3.4/24";
  baseConfig.interfaces[0].ipAddresses[1] =
      "2a03:2880:10:1f07:face:b00c:0:0/96";

  baseConfig.vlanPorts.resize(2);
  baseConfig.vlanPorts[0].logicalPort = 1;
  baseConfig.vlanPorts[0].vlanID = 1000;
  baseConfig.vlanPorts[0].emitTags = false;
  baseConfig.vlanPorts[1].logicalPort = 2;
  baseConfig.vlanPorts[1].vlanID = 1000;
  baseConfig.vlanPorts[1].emitTags = false;

  auto startState = publishAndApplyConfig(baseState, &baseConfig, &platform);
  ASSERT_NE(nullptr, startState);

  // This config has an aggregate port comprised of two physical ports
  auto config = baseConfig;
  config.__isset.aggregatePorts = true;
  config.aggregatePorts.resize(1);
  config.aggregatePorts[0].key = 1;
  config.aggregatePorts[0].name = "port-channel";
  config.aggregatePorts[0].description = "double bundle";
  config.aggregatePorts[0].physicalPorts.resize(2);
  config.aggregatePorts[0].physicalPorts[0] = 1;
  config.aggregatePorts[0].physicalPorts[1] = 2;

  auto endState = publishAndApplyConfig(startState, &config, &platform);
  ASSERT_NE(nullptr, endState);
  auto aggPort =
      endState->getAggregatePorts()->getAggregatePortIf(AggregatePortID(1));
  ASSERT_NE(nullptr, aggPort);
  checkAggPort(
      aggPort, AggregatePortID(1), "port-channel", "double bundle", {1, 2}, 0);
}

TEST(AggregatePort, singleTrunkIdempotence) {
  MockPlatform platform;
  auto baseState = make_shared<SwitchState>();
  baseState->registerPort(PortID(1), "port1");
  baseState->registerPort(PortID(2), "port2");

  // This config has an aggregate port comprised of two physical ports
  cfg::SwitchConfig baseConfig;
  baseConfig.ports.resize(2);
  baseConfig.ports[0].logicalID = 1;
  baseConfig.ports[0].state = cfg::PortState::UP;
  baseConfig.ports[1].logicalID = 2;
  baseConfig.ports[1].state = cfg::PortState::UP;

  baseConfig.vlans.resize(1);
  baseConfig.vlans[0].id = 1000;
  baseConfig.vlans[0].name = "vlan1000";
  baseConfig.vlans[0].routable = true;

  baseConfig.interfaces.resize(1);
  baseConfig.interfaces[0].intfID = 1000;
  baseConfig.interfaces[0].vlanID = 1000;
  baseConfig.interfaces[0].ipAddresses.resize(2);
  baseConfig.interfaces[0].ipAddresses[0] = "1.2.3.4/24";
  baseConfig.interfaces[0].ipAddresses[1] =
      "2a03:2880:10:1f07:face:b00c:0:0/96";

  baseConfig.vlanPorts.resize(2);
  baseConfig.vlanPorts[0].logicalPort = 1;
  baseConfig.vlanPorts[0].vlanID = 1000;
  baseConfig.vlanPorts[0].emitTags = false;
  baseConfig.vlanPorts[1].logicalPort = 2;
  baseConfig.vlanPorts[1].vlanID = 1000;
  baseConfig.vlanPorts[1].emitTags = false;

  baseConfig.__isset.aggregatePorts = true;
  baseConfig.aggregatePorts.resize(1);
  baseConfig.aggregatePorts[0].key = 1;
  baseConfig.aggregatePorts[0].name = "port-channel";
  baseConfig.aggregatePorts[0].description = "double bundle";
  baseConfig.aggregatePorts[0].physicalPorts.resize(2);
  baseConfig.aggregatePorts[0].physicalPorts[0] = 1;
  baseConfig.aggregatePorts[0].physicalPorts[1] = 2;

  auto startState = publishAndApplyConfig(baseState, &baseConfig, &platform);
  ASSERT_NE(nullptr, startState);

  // config is the same as baseConfig, but the order of physical ports in the
  // sole aggregate port has been reversed. We expect this to be a no-op because
  // subports are sorted in ThriftConfigApplier.
  auto config = baseConfig;
  std::swap(
      config.aggregatePorts[0].physicalPorts[0],
      config.aggregatePorts[0].physicalPorts[1]);

  EXPECT_EQ(nullptr, publishAndApplyConfig(startState, &config, &platform));
}

TEST(AggregatePort, singleTrunkWithoutPhysicalPorts) {
  MockPlatform platform;
  auto baseState = make_shared<SwitchState>();
  baseState->registerPort(PortID(1), "port1");
  baseState->registerPort(PortID(2), "port2");

  // This config has an aggregate port comprised of two physical ports
  cfg::SwitchConfig baseConfig;
  baseConfig.ports.resize(2);
  baseConfig.ports[0].logicalID = 1;
  baseConfig.ports[0].state = cfg::PortState::UP;
  baseConfig.ports[1].logicalID = 2;
  baseConfig.ports[1].state = cfg::PortState::UP;

  baseConfig.vlans.resize(1);
  baseConfig.vlans[0].id = 1000;
  baseConfig.vlans[0].name = "vlan1000";
  baseConfig.vlans[0].routable = true;

  baseConfig.interfaces.resize(1);
  baseConfig.interfaces[0].intfID = 1000;
  baseConfig.interfaces[0].vlanID = 1000;
  baseConfig.interfaces[0].ipAddresses.resize(2);
  baseConfig.interfaces[0].ipAddresses[0] = "1.2.3.4/24";
  baseConfig.interfaces[0].ipAddresses[1] =
      "2a03:2880:10:1f07:face:b00c:0:0/96";

  baseConfig.vlanPorts.resize(2);
  baseConfig.vlanPorts[0].logicalPort = 1;
  baseConfig.vlanPorts[0].vlanID = 1000;
  baseConfig.vlanPorts[0].emitTags = false;
  baseConfig.vlanPorts[1].logicalPort = 2;
  baseConfig.vlanPorts[1].vlanID = 1000;
  baseConfig.vlanPorts[1].emitTags = false;

  baseConfig.__isset.aggregatePorts = true;
  baseConfig.aggregatePorts.resize(1);
  baseConfig.aggregatePorts[0].key = 1;
  baseConfig.aggregatePorts[0].name = "port-channel";
  baseConfig.aggregatePorts[0].description = "double bundle";
  baseConfig.aggregatePorts[0].physicalPorts.resize(2);
  baseConfig.aggregatePorts[0].physicalPorts[0] = 1;
  baseConfig.aggregatePorts[0].physicalPorts[1] = 2;

  auto startState = publishAndApplyConfig(baseState, &baseConfig, &platform);
  ASSERT_NE(nullptr, startState);

  // This config config has a single aggregate port without any constituent
  // physical ports.
  auto config = baseConfig;
  config.aggregatePorts[0].description = "empty bundle";
  config.aggregatePorts[0].physicalPorts.resize(0);

  auto endState = publishAndApplyConfig(startState, &config, &platform);
  ASSERT_NE(nullptr, endState);
  auto aggPort =
      endState->getAggregatePorts()->getAggregatePortIf(AggregatePortID(1));
  ASSERT_NE(nullptr, aggPort);
  checkAggPort(
      aggPort, AggregatePortID(1), "port-channel", "empty bundle", {}, 1);
}

TEST(AggregatePort, noTrunk) {
  MockPlatform platform;
  auto baseState = make_shared<SwitchState>();
  baseState->registerPort(PortID(1), "port1");
  baseState->registerPort(PortID(2), "port2");

  // This config has an aggregate port comprised of two physical ports
  cfg::SwitchConfig baseConfig;
  baseConfig.ports.resize(2);
  baseConfig.ports[0].logicalID = 1;
  baseConfig.ports[0].state = cfg::PortState::UP;
  baseConfig.ports[1].logicalID = 2;
  baseConfig.ports[1].state = cfg::PortState::UP;

  baseConfig.vlans.resize(1);
  baseConfig.vlans[0].id = 1000;
  baseConfig.vlans[0].name = "vlan1000";
  baseConfig.vlans[0].routable = true;

  baseConfig.interfaces.resize(1);
  baseConfig.interfaces[0].intfID = 1000;
  baseConfig.interfaces[0].vlanID = 1000;
  baseConfig.interfaces[0].ipAddresses.resize(2);
  baseConfig.interfaces[0].ipAddresses[0] = "1.2.3.4/24";
  baseConfig.interfaces[0].ipAddresses[1] =
      "2a03:2880:10:1f07:face:b00c:0:0/96";

  baseConfig.vlanPorts.resize(2);
  baseConfig.vlanPorts[0].logicalPort = 1;
  baseConfig.vlanPorts[0].vlanID = 1000;
  baseConfig.vlanPorts[0].emitTags = false;
  baseConfig.vlanPorts[1].logicalPort = 2;
  baseConfig.vlanPorts[1].vlanID = 1000;
  baseConfig.vlanPorts[1].emitTags = false;

  baseConfig.__isset.aggregatePorts = true;
  baseConfig.aggregatePorts.resize(1);
  baseConfig.aggregatePorts[0].key = 1;
  baseConfig.aggregatePorts[0].name = "port-channel";
  baseConfig.aggregatePorts[0].description = "double bundle";
  baseConfig.aggregatePorts[0].physicalPorts.resize(2);
  baseConfig.aggregatePorts[0].physicalPorts[0] = 1;
  baseConfig.aggregatePorts[0].physicalPorts[1] = 2;

  auto startState = publishAndApplyConfig(baseState, &baseConfig, &platform);
  ASSERT_NE(nullptr, startState);

  // This config has no aggregate ports
  auto config = baseConfig;
  config.aggregatePorts.resize(0);

  auto endState = publishAndApplyConfig(startState, &config, &platform);
  ASSERT_NE(nullptr, endState);
  auto allAggPorts = endState->getAggregatePorts();
  EXPECT_EQ(allAggPorts->begin(), allAggPorts->end());
}

// TODO(samank): factor out into TestUtils
/*
 * Test that forEachChanged(StateDelta::getAggregatePortsDelta(), ...) invokes
 * the callback for the specified list of changed AggregatePorts.
 */
void checkChangedAggPorts(
    const shared_ptr<AggregatePortMap>& oldAggPorts,
    const shared_ptr<AggregatePortMap>& newAggPorts,
    const std::set<uint16_t> changedIDs,
    const std::set<uint16_t> addedIDs,
    const std::set<uint16_t> removedIDs) {
  auto oldState = make_shared<SwitchState>();
  oldState->resetAggregatePorts(oldAggPorts);
  auto newState = make_shared<SwitchState>();
  newState->resetAggregatePorts(newAggPorts);

  std::set<uint16_t> foundChanged;
  std::set<uint16_t> foundAdded;
  std::set<uint16_t> foundRemoved;
  StateDelta delta(oldState, newState);
  DeltaFunctions::forEachChanged(
      delta.getAggregatePortsDelta(),
      [&](const shared_ptr<AggregatePort>& oldAggPort,
          const shared_ptr<AggregatePort>& newAggPort) {
        EXPECT_EQ(oldAggPort->getID(), newAggPort->getID());
        EXPECT_NE(oldAggPort, newAggPort);

        // TODO(samank): why is there no cast necessary to go from return type
        // of getID() to uint16_t?
        auto ret = foundChanged.insert(oldAggPort->getID());
        EXPECT_TRUE(ret.second);
      },
      [&](const shared_ptr<AggregatePort>& aggPort) {
        auto ret = foundAdded.insert(aggPort->getID());
        EXPECT_TRUE(ret.second);
      },
      [&](const shared_ptr<AggregatePort>& aggPort) {
        auto ret = foundRemoved.insert(aggPort->getID());
        EXPECT_TRUE(ret.second);
      });

  EXPECT_EQ(changedIDs, foundChanged);
  EXPECT_EQ(addedIDs, foundAdded);
  EXPECT_EQ(removedIDs, foundRemoved);
}

TEST(AggregatePort, multiTrunkAdd) {
  MockPlatform platform;

  auto startState = testStateA();
  auto startAggPorts = startState->getAggregatePorts();

  // Construct a config that will bundle ports 1-10 into a trunk port and
  // ports 11-20 into a second trunk port. We accomplish this by modifying
  // a base config corresponding to startState.
  auto config = testConfigA();
  config.aggregatePorts.resize(2);
  config.aggregatePorts[0].key = 55;
  config.aggregatePorts[0].name = "lag55";
  config.aggregatePorts[0].description = "upwards facing link-bundle";
  config.aggregatePorts[0].physicalPorts.resize(10);
  config.aggregatePorts[0].physicalPorts = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  config.aggregatePorts[1].key = 155;
  config.aggregatePorts[1].name = "lag155";
  config.aggregatePorts[1].description = "downwards facing link-bundle";
  config.aggregatePorts[1].physicalPorts.resize(10);
  config.aggregatePorts[1].physicalPorts = {
      11, 12, 13, 14, 15, 16, 17, 18, 19, 20};

  auto endState = publishAndApplyConfig(startState, &config, &platform);
  ASSERT_NE(nullptr, endState);
  auto endAggPorts = endState->getAggregatePorts();
  ASSERT_NE(nullptr, endAggPorts);
  EXPECT_EQ(1, endAggPorts->getGeneration());
  EXPECT_EQ(2, endAggPorts->size());

  // Check the settings for the newly created AggregatePort 55
  checkAggPort(
      endAggPorts->getAggregatePortIf(AggregatePortID(55)),
      AggregatePortID(55),
      "lag55",
      "upwards facing link-bundle",
      {1, 2, 3, 4, 5, 6, 7, 8, 9, 10},
      0);

  // Check the settings for the newly created AggregatePort 155
  checkAggPort(
      endAggPorts->getAggregatePortIf(AggregatePortID(155)),
      AggregatePortID(155),
      "lag155",
      "downwards facing link-bundle",
      {11, 12, 13, 14, 15, 16, 17, 18, 19, 20},
      0);

  // getAggregatePortIf() should return null on a non-existent AggregatePort
  EXPECT_EQ(nullptr, endAggPorts->getAggregatePortIf(AggregatePortID(1234)));

  checkChangedAggPorts(startAggPorts, endAggPorts, {}, {55, 155}, {});
}

TEST(AggregatePort, multiTrunkIdempotence) {
  MockPlatform platform;

  auto startState = testStateA();

  // Construct a config that will bundle ports 1-10 into a trunk port and
  // ports 11-20 into a second trunk port. We accomplish this by modifying
  // a base config corresponding to startState.
  auto config = testConfigA();
  config.aggregatePorts.resize(2);
  config.aggregatePorts[0].key = 55;
  config.aggregatePorts[0].name = "lag55";
  config.aggregatePorts[0].description = "upwards facing link-bundle";
  config.aggregatePorts[0].physicalPorts.resize(10);
  config.aggregatePorts[0].physicalPorts = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  config.aggregatePorts[1].key = 155;
  config.aggregatePorts[1].name = "lag155";
  config.aggregatePorts[1].description = "downwards facing link-bundle";
  config.aggregatePorts[1].physicalPorts.resize(10);
  config.aggregatePorts[1].physicalPorts = {
      11, 12, 13, 14, 15, 16, 17, 18, 19, 20};

  auto endState = publishAndApplyConfig(startState, &config, &platform);
  ASSERT_NE(nullptr, endState);

  // Applying the same config again should result in no change
  EXPECT_EQ(nullptr, publishAndApplyConfig(endState, &config, &platform));
}

TEST(AggregatePort, multiTrunkAddAndChange) {
  MockPlatform platform;

  auto baseState = testStateA();

  // Construct a config that will bundle ports 1-10 into a trunk port and
  // ports 11-20 into a second trunk port. We accomplish this by modifying
  // a base config corresponding to startingState.
  auto baseConfig = testConfigA();
  baseConfig.aggregatePorts.resize(2);
  baseConfig.aggregatePorts[0].key = 55;
  baseConfig.aggregatePorts[0].name = "lag55";
  baseConfig.aggregatePorts[0].description = "upwards facing link-bundle";
  baseConfig.aggregatePorts[0].physicalPorts.resize(10);
  baseConfig.aggregatePorts[0].physicalPorts = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  baseConfig.aggregatePorts[1].key = 155;
  baseConfig.aggregatePorts[1].name = "lag155";
  baseConfig.aggregatePorts[1].description = "downwards facing link-bundle";
  baseConfig.aggregatePorts[1].physicalPorts.resize(10);
  baseConfig.aggregatePorts[1].physicalPorts = {
      11, 12, 13, 14, 15, 16, 17, 18, 19, 20};

  auto startState = publishAndApplyConfig(baseState, &baseConfig, &platform);
  ASSERT_NE(nullptr, startState);
  auto startAggPorts = startState->getAggregatePorts();

  // Split each trunk port into two separate trunk ports. The "upwards facing
  // link-bundle" will now have a left and right orientation. Likewise with the
  // "downwards facing link-bundle".
  auto config = baseConfig;
  config.aggregatePorts.resize(4);
  config.aggregatePorts[0].description = "up & leftwards facing link-bundle";
  config.aggregatePorts[0].physicalPorts.resize(5);
  config.aggregatePorts[0].physicalPorts = {1, 2, 3, 4, 5};

  config.aggregatePorts[1].description = "down & leftwards facing link-bundle";
  config.aggregatePorts[1].physicalPorts.resize(5);
  config.aggregatePorts[1].physicalPorts = {11, 12, 13, 14, 15};

  config.aggregatePorts[2].key = 40;
  config.aggregatePorts[2].name = "lag40";
  config.aggregatePorts[2].description = "up & rightwards facing link-bundle";
  config.aggregatePorts[2].physicalPorts.resize(5);
  config.aggregatePorts[2].physicalPorts = {6, 7, 8, 9, 10};

  config.aggregatePorts[3].key = 90;
  config.aggregatePorts[3].name = "lag90";
  config.aggregatePorts[3].description = "down & rightwards facing link-bundle";
  config.aggregatePorts[3].physicalPorts.resize(5);
  config.aggregatePorts[3].physicalPorts = {16, 17, 18, 19, 20};

  auto endState = publishAndApplyConfig(startState, &config, &platform);
  ASSERT_NE(nullptr, endState);
  auto endAggPorts = endState->getAggregatePorts();
  ASSERT_NE(nullptr, endAggPorts);
  EXPECT_EQ(2, endAggPorts->getGeneration());
  EXPECT_EQ(4, endAggPorts->size());

  checkChangedAggPorts(startAggPorts, endAggPorts, {55, 155}, {40, 90}, {});

  // Check the settings for the newly created AggregatePort 40
  checkAggPort(
      endAggPorts->getAggregatePortIf(AggregatePortID(40)),
      AggregatePortID(40),
      "lag40",
      "up & rightwards facing link-bundle",
      {6, 7, 8, 9, 10},
      0);

  // Check the settings for the newly created AggregatePort 90
  checkAggPort(
      endAggPorts->getAggregatePortIf(AggregatePortID(90)),
      AggregatePortID(90),
      "lag90",
      "down & rightwards facing link-bundle",
      {16, 17, 18, 19, 20},
      0);

  // Check the settings for the newly modified AggregatePort 55
  checkAggPort(
      endAggPorts->getAggregatePortIf(AggregatePortID(55)),
      AggregatePortID(55),
      "lag55",
      "up & leftwards facing link-bundle",
      {1, 2, 3, 4, 5},
      1);

  // Check the settings for the newly modified AggregatePort 155
  checkAggPort(
      endAggPorts->getAggregatePortIf(AggregatePortID(155)),
      AggregatePortID(155),
      "lag155",
      "down & leftwards facing link-bundle",
      {11, 12, 13, 14, 15},
      1);
}

TEST(AggregatePort, multiTrunkRemove) {
  MockPlatform platform;

  auto baseState = testStateA();

  // Construct a config that will bundle ports 6-10 into trunk port 40,
  // ports 16-20 into trunk port 90, ports 1-5 into trunk port 55, and
  // ports 11-15 into trunk port 155. We accomplish this by modifying
  // a base config corresponding to state0.
  auto baseConfig = testConfigA();
  baseConfig.aggregatePorts.resize(4);
  baseConfig.aggregatePorts[0].key = 55;
  baseConfig.aggregatePorts[0].name = "lag55";
  baseConfig.aggregatePorts[0].description =
      "up & leftwards facing link-bundle";
  baseConfig.aggregatePorts[0].physicalPorts.resize(5);
  baseConfig.aggregatePorts[0].physicalPorts = {1, 2, 3, 4, 5};
  baseConfig.aggregatePorts[1].key = 155;
  baseConfig.aggregatePorts[1].name = "lag155";
  baseConfig.aggregatePorts[1].description =
      "down & leftwards facing link-bundle";
  baseConfig.aggregatePorts[1].physicalPorts.resize(5);
  baseConfig.aggregatePorts[1].physicalPorts = {11, 12, 13, 14, 15};
  baseConfig.aggregatePorts[2].key = 40;
  baseConfig.aggregatePorts[2].name = "lag40";
  baseConfig.aggregatePorts[2].description =
      "up & rightwards facing link-bundle";
  baseConfig.aggregatePorts[2].physicalPorts.resize(5);
  baseConfig.aggregatePorts[2].physicalPorts = {6, 7, 8, 9, 10};
  baseConfig.aggregatePorts[3].key = 90;
  baseConfig.aggregatePorts[3].name = "lag90";
  baseConfig.aggregatePorts[3].description =
      "down & rightwards facing link-bundle";
  baseConfig.aggregatePorts[3].physicalPorts.resize(5);
  baseConfig.aggregatePorts[3].physicalPorts = {16, 17, 18, 19, 20};

  auto startState = publishAndApplyConfig(baseState, &baseConfig, &platform);
  ASSERT_NE(nullptr, startState);
  auto startAggPorts = startState->getAggregatePorts();
  ASSERT_NE(nullptr, startAggPorts);

  // Remove all trunk ports with a leftwards orientation, ie. AggregatePort 55
  // and 155
  auto config = baseConfig;
  config.aggregatePorts[0] = config.aggregatePorts[2];
  config.aggregatePorts[1] = config.aggregatePorts[3];
  config.aggregatePorts.resize(2);

  auto endState = publishAndApplyConfig(startState, &config, &platform);
  ASSERT_NE(nullptr, endState);
  auto endAggPorts = endState->getAggregatePorts();
  ASSERT_NE(nullptr, endAggPorts);
  EXPECT_EQ(2, endAggPorts->getGeneration());
  EXPECT_EQ(2, endAggPorts->size());

  checkChangedAggPorts(startAggPorts, endAggPorts, {}, {}, {55, 155});

  // Check AggregatePort 55 has been removed
  EXPECT_EQ(nullptr, endAggPorts->getAggregatePortIf(AggregatePortID(55)));

  // Check AggregatePort 155 has been removed
  EXPECT_EQ(nullptr, endAggPorts->getAggregatePortIf(AggregatePortID(155)));

  // Check AggregatePort 40 has not been modified
  EXPECT_EQ(
      startAggPorts->getAggregatePortIf(AggregatePortID(40)),
      endAggPorts->getAggregatePortIf(AggregatePortID(40)));

  // Check AggregatePort 90 has not been modified
  EXPECT_EQ(
      startAggPorts->getAggregatePortIf(AggregatePortID(90)),
      endAggPorts->getAggregatePortIf(AggregatePortID(90)));
}
