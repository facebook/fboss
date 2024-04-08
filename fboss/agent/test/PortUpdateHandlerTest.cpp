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
#include "fboss/lib/CommonUtils.h"

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
    addState->getVlans()->getNodeIf(VlanID(1))->addPort(PortID(21), false);
    deltaAdd = std::make_shared<StateDelta>(initState, addState);
    deltaRemove = std::make_shared<StateDelta>(addState, initState);

    // rename all port name fron portX to eth1/X/1
    initPorts = initState->getPorts();
    newPorts = std::make_shared<MultiSwitchPortMap>();
    for (const auto& origPortMap : std::as_const(*initPorts)) {
      for (const auto& origPort : std::as_const(*origPortMap.second)) {
        auto newPort = origPort.second->clone();
        newPort->setName(
            folly::to<string>("eth1/", origPort.second->getID(), "/1"));
        newPort->setOperState(true);
        newPorts->addNode(newPort, sw->getScopeResolver()->scope(newPort));
      }
    }
    auto changedState = initState->clone();
    changedState->resetPorts(newPorts);
    deltaChange = std::make_shared<StateDelta>(initState, changedState);

    portUpdateHandler = std::make_unique<PortUpdateHandler>(sw);
  }

  void expectPortCounterExist(
      CounterCache& counters,
      std::shared_ptr<MultiSwitchPortMap> ports) {
    for (const auto& portMap : std::as_const(*ports)) {
      for (const auto& port : std::as_const(*portMap.second)) {
        EXPECT_TRUE(counters.checkExist(port.second->getName() + ".up"));
        EXPECT_EQ(
            counters.value(port.second->getName() + ".up"),
            port.second->isUp());
      }
    }
  }

  void expectPortCounterNotExist(
      CounterCache& counters,
      std::shared_ptr<MultiSwitchPortMap> ports) {
    for (const auto& portMap : std::as_const(*ports)) {
      for (const auto& port : std::as_const(*portMap.second)) {
        EXPECT_FALSE(counters.checkExist(port.second->getName() + ".up"));
      }
    }
  }

  void TearDown() override {
    sw = nullptr;
  }

  SwSwitch* sw{nullptr};
  std::unique_ptr<HwTestHandle> handle{nullptr};
  std::shared_ptr<SwitchState> initState;
  std::shared_ptr<SwitchState> addState;
  std::shared_ptr<MultiSwitchPortMap> initPorts;
  std::shared_ptr<MultiSwitchPortMap> newPorts;
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

template <typename SwitchTypeT>
class PortUpdateHandlerLoopDetectionTest : public ::testing::Test {
 public:
  static auto constexpr switchType = SwitchTypeT::switchType;
  std::shared_ptr<SwitchState> initState() const {
    return testStateAWithPortsUp(switchType);
  }
  void SetUp() override {
    handle = createTestHandle(initState());
    sw = handle->getSw();
  }
  void TearDown() override {
    sw = nullptr;
  }
  int switchId() const {
    return 2;
  }
  cfg::PortState getLoopedPortExpectedState() const {
    return switchType == cfg::SwitchType::FABRIC ? cfg::PortState::DISABLED
                                                 : cfg::PortState::ENABLED;
  }
  PortID kPortId() const {
    return PortID(sw->getState()->getPorts()->getAllNodes()->begin()->first);
  }
  void checkPortState(cfg::PortState expectedState) const {
    WITH_RETRIES({
      auto ports = sw->getState()->getPorts()->getAllNodes();
      for (const auto& [id, port] : *ports) {
        if (PortID(id) == kPortId()) {
          EXPECT_EVENTUALLY_EQ(port->getAdminState(), expectedState);
        } else {
          EXPECT_EVENTUALLY_EQ(port->getAdminState(), cfg::PortState::ENABLED);
        }
      }
    });
  }
  std::map<PortID, multiswitch::FabricConnectivityDelta> makeConnectivity(
      FabricEndpoint endpoint) const {
    multiswitch::FabricConnectivityDelta delta;
    delta.newConnectivity() = endpoint;
    std::map<PortID, multiswitch::FabricConnectivityDelta> connectivity;
    connectivity.insert({kPortId(), delta});
    return connectivity;
  }
  SwSwitch* sw{nullptr};
  std::unique_ptr<HwTestHandle> handle{nullptr};
};

TYPED_TEST_SUITE(PortUpdateHandlerLoopDetectionTest, DsfSwitchTypeTestTypes);

TYPED_TEST(PortUpdateHandlerLoopDetectionTest, createLoop) {
  FabricEndpoint endpoint;
  endpoint.isAttached() = true;
  endpoint.switchId() = this->switchId();
  endpoint.portId() = this->kPortId();
  this->sw->linkConnectivityChanged(this->makeConnectivity(endpoint));
  this->checkPortState(this->getLoopedPortExpectedState());
}

} // unnamed namespace
