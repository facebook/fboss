/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <gtest/gtest.h>

#include "fboss/agent/PortUpdateHandler.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/PortMap.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/CounterCache.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/TestUtils.h"

using namespace facebook::fboss;
using std::string;

namespace {

HwSwitchMatcher scope() {
  return HwSwitchMatcher{std::unordered_set<SwitchID>{SwitchID(0)}};
}

class PortUpdateHandlerTest : public ::testing::Test {
 public:
  void SetUp() override {
    handle = createTestHandle();
    sw = handle->getSw();
    initState = testStateA();

    addState = testStateA();
    // add port 21 which uses VLAN 1
    registerPort(addState, PortID(21), "port21", scope());
    addState->getVlans()->getVlanIf(VlanID(1))->addPort(PortID(21), false);
    deltaAdd = std::make_shared<StateDelta>(initState, addState);
    deltaRemove = std::make_shared<StateDelta>(addState, initState);

    // rename all port name fron portX to eth1/X/1
    initPorts = initState->getPorts();
    newPorts = std::make_shared<PortMap>();
    for (const auto& origPort : std::as_const(*initPorts)) {
      auto newPort = origPort.second->clone();
      newPort->setName(
          folly::to<string>("eth1/", origPort.second->getID(), "/1"));
      newPort->setOperState(true);
      newPorts->addPort(newPort);
    }
    auto changedState = initState->clone();
    changedState->resetPorts(newPorts);
    deltaChange = std::make_shared<StateDelta>(initState, changedState);

    portUpdateHandler = std::make_unique<PortUpdateHandler>(sw);
  }

  void expectPortCounterExist(
      CounterCache& counters,
      std::shared_ptr<PortMap> ports) {
    for (const auto& port : std::as_const(*ports)) {
      EXPECT_TRUE(counters.checkExist(port.second->getName() + ".up"));
      EXPECT_EQ(
          counters.value(port.second->getName() + ".up"), port.second->isUp());
    }
  }

  void expectPortCounterNotExist(
      CounterCache& counters,
      std::shared_ptr<PortMap> ports) {
    for (const auto& port : std::as_const(*ports)) {
      EXPECT_FALSE(counters.checkExist(port.second->getName() + ".up"));
    }
  }

  void TearDown() override {
    sw = nullptr;
  }

  SwSwitch* sw{nullptr};
  std::unique_ptr<HwTestHandle> handle{nullptr};
  std::shared_ptr<SwitchState> initState;
  std::shared_ptr<SwitchState> addState;
  std::shared_ptr<PortMap> initPorts;
  std::shared_ptr<PortMap> newPorts;
  std::shared_ptr<StateDelta> deltaAdd;
  std::shared_ptr<StateDelta> deltaRemove;
  std::shared_ptr<StateDelta> deltaChange;
  std::unique_ptr<PortUpdateHandler> portUpdateHandler;
};

TEST_F(PortUpdateHandlerTest, PortAdded) {
  // // Cache the current stats
  CounterCache counters(sw);
  EXPECT_EQ(sw->stats()->getPortStats()->size(), 0);

  portUpdateHandler->stateUpdated(*std::make_shared<StateDelta>(
      std::make_shared<SwitchState>(), initState));
  EXPECT_EQ(sw->stats()->getPortStats()->size(), 20);

  portUpdateHandler->stateUpdated(*deltaAdd);
  // should be 21 PortStats now
  EXPECT_EQ(sw->stats()->getPortStats()->size(), 21);

  counters.update();
  // make sure original PortStats still exist
  expectPortCounterExist(counters, initPorts);
  // make sure PortStats for port 21 is also created
  // EXPECT_TRUE(counters.checkExist("port21.link_state.flap.sum"));
  EXPECT_TRUE(counters.checkExist("port21.up"));
  EXPECT_EQ(counters.value("port21.up"), 0);
}

TEST_F(PortUpdateHandlerTest, PortRemoved) {
  // // Cache the current stats
  CounterCache counters(sw);
  EXPECT_EQ(sw->stats()->getPortStats()->size(), 0);

  portUpdateHandler->stateUpdated(
      *std::make_shared<StateDelta>(std::make_shared<SwitchState>(), addState));
  EXPECT_EQ(sw->stats()->getPortStats()->size(), 21);

  portUpdateHandler->stateUpdated(*deltaRemove);
  // should be 20 PortStats now
  EXPECT_EQ(sw->stats()->getPortStats()->size(), 20);

  counters.update();
  // make sure original PortStats still exist
  expectPortCounterExist(counters, initPorts);
  // make sure PortStats for port 21 no longer exist
  EXPECT_FALSE(counters.checkExist("port21.up"));
}

TEST_F(PortUpdateHandlerTest, PortChanged) {
  // // Cache the current stats
  CounterCache counters(sw);
  EXPECT_EQ(sw->stats()->getPortStats()->size(), 0);

  portUpdateHandler->stateUpdated(*std::make_shared<StateDelta>(
      std::make_shared<SwitchState>(), initState));
  EXPECT_EQ(sw->stats()->getPortStats()->size(), 20);

  portUpdateHandler->stateUpdated(*deltaChange);
  // should be 20 PortStats now
  EXPECT_EQ(sw->stats()->getPortStats()->size(), 20);

  counters.update();
  // make sure PortStats with the new name is created
  expectPortCounterExist(counters, newPorts);
  expectPortCounterNotExist(counters, initPorts);
}
} // unnamed namespace
