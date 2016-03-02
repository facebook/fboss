/*
 * Copyright (c) 2004-present, Facebook, Inc.
 * Copyright (c) 2016, Cavium, Inc.
 * All rights reserved.
 * 
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 * 
 */
#include "fboss/agent/ApplyThriftConfig.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/state/DeltaFunctions.h"
#include "fboss/agent/state/NodeMapDelta.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/PortMap.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/hw/sai/SaiTestUtils.h"
#include "fboss/agent/platforms/sai/SaiPlatform.h"

#include <gtest/gtest.h>

using namespace facebook::fboss;
using std::make_pair;
using std::make_shared;
using std::shared_ptr;

TEST(Port, applyConfig) {
  SaiPlatform platform;
  auto stateV0 = make_shared<SwitchState>();
  stateV0->registerPort(PortID(1), "port1");
  auto portV0 = stateV0->getPort(PortID(1));
  EXPECT_EQ(0, portV0->getGeneration());
  EXPECT_FALSE(portV0->isPublished());
  EXPECT_EQ(PortID(1), portV0->getID());
  EXPECT_EQ("port1", portV0->getName());
  EXPECT_EQ(cfg::PortState::DOWN, portV0->getState());
  Port::VlanMembership emptyVlans;
  EXPECT_EQ(emptyVlans, portV0->getVlans());

  portV0->publish();
  EXPECT_TRUE(portV0->isPublished());

  cfg::SwitchConfig config;
  config.ports.resize(1);
  config.ports[0].logicalID = 1;
  config.ports[0].state = cfg::PortState::UP;
  config.vlans.resize(2);
  config.vlans[0].id = 2;
  config.vlans[1].id = 5;
  config.vlanPorts.resize(2);
  config.vlanPorts[0].logicalPort = 1;
  config.vlanPorts[0].vlanID = 2;
  config.vlanPorts[0].emitTags = false;
  config.vlanPorts[1].logicalPort = 1;
  config.vlanPorts[1].vlanID = 5;
  config.vlanPorts[1].emitTags = true;

  auto stateV1 = publishAndApplyConfig(stateV0, &config, &platform);
  auto portV1 = stateV1->getPort(PortID(1));
  ASSERT_NE(nullptr, portV1);
  EXPECT_NE(portV0, portV1);

  EXPECT_EQ(PortID(1), portV1->getID());
  EXPECT_EQ("port1", portV1->getName());
  EXPECT_EQ(1, portV1->getGeneration());
  EXPECT_EQ(cfg::PortState::UP, portV1->getState());
  EXPECT_FALSE(portV1->isPublished());
  Port::VlanMembership expectedVlans;
  expectedVlans.insert(make_pair(VlanID(2), Port::VlanInfo(false)));
  expectedVlans.insert(make_pair(VlanID(5), Port::VlanInfo(true)));
  EXPECT_EQ(expectedVlans, portV1->getVlans());

  // Applying the same config again should result in no changes
  EXPECT_EQ(nullptr, publishAndApplyConfig(stateV1, &config, &platform));

  // Applying the same config with a new VLAN list should result in changes
  config.vlanPorts.resize(1);
  config.vlanPorts[0].logicalPort = 1;
  config.vlanPorts[0].vlanID = 2021;
  config.vlanPorts[0].emitTags = false;

  Port::VlanMembership expectedVlansV2;
  expectedVlansV2.insert(make_pair(VlanID(2021), Port::VlanInfo(false)));
  auto stateV2 = publishAndApplyConfig(stateV1, &config, &platform);
  auto portV2 = stateV2->getPort(PortID(1));
  ASSERT_NE(nullptr, portV2);
  EXPECT_NE(portV1, portV2);

  EXPECT_EQ(PortID(1), portV2->getID());
  EXPECT_EQ("port1", portV2->getName());
  EXPECT_EQ(2, portV2->getGeneration());
  EXPECT_EQ(cfg::PortState::UP, portV2->getState());
  EXPECT_FALSE(portV2->isPublished());
  EXPECT_EQ(expectedVlansV2, portV2->getVlans());

  // Applying the same config with a different speed should result in changes
  config.ports[0].speed = cfg::PortSpeed::GIGE;

  auto stateV3 = publishAndApplyConfig(stateV2, &config, &platform);
  auto portV3 = stateV3->getPort(PortID(1));
  ASSERT_NE(nullptr, portV3);
  EXPECT_NE(portV2, portV3);
  EXPECT_EQ(cfg::PortSpeed::GIGE, portV3->getSpeed());

  // Attempting to apply a config with a non-existent PortID should fail.
  config.ports[0].logicalID = 2;
  EXPECT_THROW(publishAndApplyConfig(stateV3, &config, &platform), FbossError);
}

TEST(Port, initDefaultConfig) {
  SaiPlatform platform;
  PortID portID(1);
  auto state = make_shared<SwitchState>();
  state->registerPort(portID, "port1");

  // Applying an empty config should result in no changes.
  cfg::SwitchConfig config;
  EXPECT_EQ(nullptr, publishAndApplyConfig(state, &config, &platform));

  // Adding a port entry in the config and initializing it with
  // initDefaultConfig() should also result in no changes.
  config.ports.resize(1);
  state->getPort(portID)->initDefaultConfig(&config.ports[0]);
  EXPECT_EQ(nullptr, publishAndApplyConfig(state, &config, &platform));
}

TEST(PortMap, registerPorts) {
  auto ports = make_shared<PortMap>();
  EXPECT_EQ(0, ports->getGeneration());
  EXPECT_FALSE(ports->isPublished());
  EXPECT_EQ(0, ports->numPorts());

  ports->registerPort(PortID(1), "port1");
  ports->registerPort(PortID(2), "port2");
  ports->registerPort(PortID(3), "port3");
  ports->registerPort(PortID(4), "port4");
  EXPECT_EQ(4, ports->numPorts());

  auto port1 = ports->getPort(PortID(1));
  auto port2 = ports->getPort(PortID(2));
  auto port3 = ports->getPort(PortID(3));
  auto port4 = ports->getPort(PortID(4));
  EXPECT_EQ(PortID(1), port1->getID());
  EXPECT_EQ("port1", port1->getName());
  EXPECT_EQ(PortID(4), port4->getID());
  EXPECT_EQ("port4", port4->getName());

  // Attempting to register a duplicate port ID should fail
  EXPECT_THROW(ports->registerPort(PortID(2), "anotherPort2"),
               FbossError);

  // Registering non-sequential IDs should work
  ports->registerPort(PortID(10), "port10");
  EXPECT_EQ(5, ports->numPorts());
  auto port10 = ports->getPort(PortID(10));
  EXPECT_EQ(PortID(10), port10->getID());
  EXPECT_EQ("port10", port10->getName());

  // Getting non-existent ports should fail
  EXPECT_THROW(ports->getPort(PortID(0)), FbossError);
  EXPECT_THROW(ports->getPort(PortID(7)), FbossError);
  EXPECT_THROW(ports->getPort(PortID(300)), FbossError);

  // Publishing the PortMap should also mark all ports as published
  ports->publish();
  EXPECT_TRUE(ports->isPublished());
  EXPECT_TRUE(port1->isPublished());
  EXPECT_TRUE(port2->isPublished());
  EXPECT_TRUE(port3->isPublished());
  EXPECT_TRUE(port4->isPublished());
  EXPECT_TRUE(port10->isPublished());

  // Attempting to register new ports after the PortMap has been published
  // should crash.
  ASSERT_DEATH(ports->registerPort(PortID(5), "port5"),
               "Check failed: !isPublished()");
}

/*
 * Test that forEachChanged(StateDelta::getPortsDelta(), ...) invokes the
 * callback for the specified list of changed ports.
 */
void checkChangedPorts(const shared_ptr<PortMap> &oldPorts,
                       const shared_ptr<PortMap> &newPorts,
                       const std::set<uint16_t> changedIDs) {
  auto oldState = make_shared<SwitchState>();
  oldState->resetPorts(oldPorts);
  auto newState = make_shared<SwitchState>();
  newState->resetPorts(newPorts);

  std::set<uint16_t> invokedPorts;
  StateDelta delta(oldState, newState);
  DeltaFunctions::forEachChanged(delta.getPortsDelta(),
                                 [&] (const shared_ptr<Port> &oldPort,
  const shared_ptr<Port> &newPort) {
    EXPECT_EQ(oldPort->getID(), newPort->getID());
    EXPECT_NE(oldPort, newPort);

    auto ret = invokedPorts.insert(oldPort->getID());
    EXPECT_TRUE(ret.second);
  });

  EXPECT_EQ(changedIDs, invokedPorts);
}

TEST(PortMap, applyConfig) {
  SaiPlatform platform;
  auto stateV0 = make_shared<SwitchState>();
  auto portsV0 = stateV0->getPorts();
  portsV0->registerPort(PortID(1), "port1");
  portsV0->registerPort(PortID(2), "port2");
  portsV0->registerPort(PortID(3), "port3");
  portsV0->registerPort(PortID(4), "port4");
  portsV0->publish();
  EXPECT_EQ(0, portsV0->getGeneration());
  auto port1 = portsV0->getPort(PortID(1));
  auto port2 = portsV0->getPort(PortID(2));
  auto port3 = portsV0->getPort(PortID(3));
  auto port4 = portsV0->getPort(PortID(4));

  // Applying an empty config shouldn't change a newly-constructed PortMap
  cfg::SwitchConfig config;
  EXPECT_EQ(nullptr, publishAndApplyConfig(stateV0, &config, &platform));

  // Enable port 2
  config.ports.resize(1);
  config.ports[0].logicalID = 2;
  config.ports[0].state = cfg::PortState::UP;
  auto stateV1 = publishAndApplyConfig(stateV0, &config, &platform);
  auto portsV1 = stateV1->getPorts();
  ASSERT_NE(nullptr, portsV1);
  EXPECT_EQ(1, portsV1->getGeneration());
  EXPECT_EQ(4, portsV1->numPorts());

  // Only port 2 should have changed
  EXPECT_EQ(port1, portsV1->getPort(PortID(1)));
  EXPECT_NE(port2, portsV1->getPort(PortID(2)));
  EXPECT_EQ(port3, portsV1->getPort(PortID(3)));
  EXPECT_EQ(port4, portsV1->getPort(PortID(4)));
  checkChangedPorts(portsV0, portsV1, {2});

  auto newPort2 =  portsV1->getPort(PortID(2));
  EXPECT_EQ(cfg::PortState::UP, newPort2->getState());
  EXPECT_EQ(cfg::PortState::DOWN, port1->getState());
  EXPECT_EQ(cfg::PortState::DOWN, port3->getState());
  EXPECT_EQ(cfg::PortState::DOWN, port4->getState());

  // The new PortMap and port 2 should still be unpublished.
  // The remaining other ports are the same and were previously published
  EXPECT_FALSE(portsV1->isPublished());
  EXPECT_FALSE(newPort2->isPublished());
  EXPECT_TRUE(port1->isPublished());
  // Publish portsV1 now.
  portsV1->publish();
  EXPECT_TRUE(portsV1->isPublished());
  EXPECT_TRUE(newPort2->isPublished());
  EXPECT_TRUE(port1->isPublished());

  // Applying the same config again should do nothing.
  EXPECT_EQ(nullptr, publishAndApplyConfig(stateV1, &config, &platform));

  // Now mark all ports up
  config.ports.resize(4);
  config.ports[0].logicalID = 1;
  config.ports[0].state = cfg::PortState::UP;
  config.ports[1].logicalID = 2;
  config.ports[1].state = cfg::PortState::UP;
  config.ports[2].logicalID = 3;
  config.ports[2].state = cfg::PortState::UP;
  config.ports[3].logicalID = 4;
  config.ports[3].state = cfg::PortState::UP;

  auto stateV2 = publishAndApplyConfig(stateV1, &config, &platform);
  auto portsV2 = stateV2->getPorts();
  ASSERT_NE(nullptr, portsV2);
  EXPECT_EQ(2, portsV2->getGeneration());

  EXPECT_NE(port1, portsV2->getPort(PortID(1)));
  EXPECT_EQ(newPort2, portsV2->getPort(PortID(2)));
  EXPECT_NE(port3, portsV2->getPort(PortID(3)));
  EXPECT_NE(port4, portsV2->getPort(PortID(4)));

  EXPECT_EQ(cfg::PortState::UP, portsV2->getPort(PortID(1))->getState());
  EXPECT_EQ(cfg::PortState::UP, portsV2->getPort(PortID(2))->getState());
  EXPECT_EQ(cfg::PortState::UP, portsV2->getPort(PortID(3))->getState());
  EXPECT_EQ(cfg::PortState::UP, portsV2->getPort(PortID(4))->getState());
  checkChangedPorts(portsV1, portsV2, {1, 3, 4});

  EXPECT_FALSE(portsV2->getPort(PortID(1))->isPublished());
  EXPECT_TRUE(portsV2->getPort(PortID(2))->isPublished());
  EXPECT_FALSE(portsV2->getPort(PortID(3))->isPublished());
  EXPECT_FALSE(portsV2->getPort(PortID(4))->isPublished());
  portsV2->publish();
  EXPECT_TRUE(portsV2->getPort(PortID(1))->isPublished());
  EXPECT_TRUE(portsV2->getPort(PortID(2))->isPublished());
  EXPECT_TRUE(portsV2->getPort(PortID(3))->isPublished());
  EXPECT_TRUE(portsV2->getPort(PortID(4))->isPublished());

  // Applying a config with ports that don't already exist should fail
  config.ports[0].logicalID = 10;
  config.ports[0].state = cfg::PortState::UP;
  EXPECT_THROW(publishAndApplyConfig(stateV2, &config, &platform), FbossError);

  // If we remove port3 from the config, it should be marked down
  config.ports.resize(3);
  config.ports[0].logicalID = 1;
  config.ports[2].logicalID = 4;
  auto stateV3 = publishAndApplyConfig(stateV2, &config, &platform);
  auto portsV3 = stateV3->getPorts();
  ASSERT_NE(nullptr, portsV3);
  EXPECT_EQ(3, portsV3->getGeneration());

  EXPECT_EQ(4, portsV3->numPorts());
  EXPECT_EQ(cfg::PortState::UP, portsV3->getPort(PortID(1))->getState());
  EXPECT_EQ(cfg::PortState::UP, portsV3->getPort(PortID(2))->getState());
  EXPECT_EQ(cfg::PortState::DOWN, portsV3->getPort(PortID(3))->getState());
  EXPECT_EQ(cfg::PortState::UP, portsV3->getPort(PortID(4))->getState());
  checkChangedPorts(portsV2, portsV3, {3});
}

TEST(PortMap, iterateOrder) {
  // The NodeMapDelta::Iterator code assumes that the PortMap iterator walks
  // through the ports in sorted order (sorted by PortID).
  //
  // Add a test to ensure that this always remains true.  (If we ever change
  // the underlying map data structure used for PortMap, we will need to update
  // the StateDelta code.)
  auto ports = make_shared<PortMap>();
  ports->registerPort(PortID(99), "a");
  ports->registerPort(PortID(37), "b");
  ports->registerPort(PortID(88), "c");
  ports->registerPort(PortID(4), "d");
  ports->publish();

  auto it = ports->begin();
  ASSERT_NE(ports->end(), it);
  EXPECT_EQ(PortID(4), (*it)->getID());
  EXPECT_EQ("d", (*it)->getName());

  ++it;
  ASSERT_NE(ports->end(), it);
  EXPECT_EQ(PortID(37), (*it)->getID());
  EXPECT_EQ("b", (*it)->getName());

  ++it;
  ASSERT_NE(ports->end(), it);
  EXPECT_EQ(PortID(88), (*it)->getID());
  EXPECT_EQ("c", (*it)->getName());

  ++it;
  ASSERT_NE(ports->end(), it);
  EXPECT_EQ(PortID(99), (*it)->getID());
  EXPECT_EQ("a", (*it)->getName());

  ++it;
  EXPECT_EQ(ports->end(), it);
}
