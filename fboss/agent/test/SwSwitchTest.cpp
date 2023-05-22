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

#include "fboss/agent/ArpHandler.h"
#include "fboss/agent/FbossHwUpdateError.h"
#include "fboss/agent/NeighborUpdater.h"
#include "fboss/agent/PortStats.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/state/ArpTable.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/Vlan.h"
#include "fboss/agent/test/CounterCache.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/TestUtils.h"

#include <folly/IPAddressV4.h>
#include <folly/IPAddressV6.h>
#include <folly/MacAddress.h>

#include <algorithm>

using namespace facebook::fboss;
using folly::IPAddressV4;
using folly::IPAddressV6;
using folly::MacAddress;
using std::string;
using ::testing::_;
using ::testing::ByRef;
using ::testing::Eq;
using ::testing::Return;

namespace {
HwSwitchMatcher scope() {
  return HwSwitchMatcher{std::unordered_set<SwitchID>{SwitchID(0)}};
}
} // namespace

class SwSwitchTest : public ::testing::Test {
 public:
  void SetUp() override {
    // Setup a default state object
    auto state = testStateA();
    state->publish();
    handle = createTestHandle(state);
    sw = handle->getSw();
    sw->initialConfigApplied(std::chrono::steady_clock::now());
    waitForStateUpdates(sw);
  }

  void TearDown() override {
    sw = nullptr;
    handle.reset();
  }

  SwSwitch* sw{nullptr};
  std::unique_ptr<HwTestHandle> handle{nullptr};
};

TEST_F(SwSwitchTest, GetPortStats) {
  // get port5 portStats for the first time
  EXPECT_EQ(sw->stats()->getPortStats()->size(), 0);
  auto portStats = sw->portStats(PortID(5));
  EXPECT_EQ(sw->stats()->getPortStats()->size(), 1);
  EXPECT_EQ(
      portStats->getPortName(), sw->getState()->getPort(PortID(5))->getName());

  // get port5 directly from PortStatsMap
  portStats = sw->portStats(PortID(5));
  EXPECT_EQ(sw->stats()->getPortStats()->size(), 1);
  EXPECT_EQ(
      portStats->getPortName(), sw->getState()->getPort(PortID(5))->getName());

  // get port0
  portStats = sw->portStats(PortID(0));
  EXPECT_EQ(sw->stats()->getPortStats()->size(), 2);
  EXPECT_EQ(portStats->getPortName(), "port0");
}
ACTION(ThrowException) {
  throw std::exception();
}

TEST_F(SwSwitchTest, UpdateStatsExceptionCounter) {
  CounterCache counters(sw);

  MockHwSwitch* hw = static_cast<MockHwSwitch*>(sw->getHw());
  EXPECT_CALL(*hw, updateStatsImpl(sw->stats()))
      .Times(1)
      .WillRepeatedly(ThrowException());
  sw->updateStats();

  counters.update();
  counters.checkDelta(
      SwitchStats::kCounterPrefix + "update_stats_exceptions.sum.60", 1);
}

TEST_F(SwSwitchTest, TestStateNonCoalescing) {
  const PortID kPort1{1};
  const VlanID kVlan1{1};

  auto verifyReachableCnt = [kVlan1, this](int expectedReachableNbrCnt) {
    auto getReachableCount = [](auto nbrTable) {
      auto reachableCnt = 0;
      for (auto iter : std::as_const(*nbrTable)) {
        auto entry = iter.second;
        if (entry->getState() == NeighborState::REACHABLE) {
          ++reachableCnt;
        }
      }
      return reachableCnt;
    };
    auto arpTable =
        sw->getState()->getMultiSwitchVlans()->getNode(kVlan1)->getArpTable();
    auto ndpTable =
        sw->getState()->getMultiSwitchVlans()->getNode(kVlan1)->getNdpTable();
    auto reachableCnt =
        getReachableCount(arpTable) + getReachableCount(ndpTable);
    EXPECT_EQ(expectedReachableNbrCnt, reachableCnt);
  };
  // No neighbor entries expected
  verifyReachableCnt(0);
  auto origState = sw->getState();
  auto bringPortsUpUpdateFn = [](const std::shared_ptr<SwitchState>& state) {
    return bringAllPortsUp(state);
  };
  sw->updateState("Bring Ports Up", bringPortsUpUpdateFn);
  sw->getNeighborUpdater()->receivedArpMine(
      kVlan1,
      IPAddressV4("10.0.0.2"),
      MacAddress("01:02:03:04:05:06"),
      PortDescriptor(kPort1),
      ArpOpCode::ARP_OP_REPLY);
  sw->getNeighborUpdater()->receivedNdpMine(
      kVlan1,
      IPAddressV6("2401:db00:2110:3001::0002"),
      MacAddress("01:02:03:04:05:06"),
      PortDescriptor(kPort1),
      ICMPv6Type::ICMPV6_TYPE_NDP_NEIGHBOR_ADVERTISEMENT,
      0);
  sw->getNeighborUpdater()->waitForPendingUpdates();
  waitForStateUpdates(sw);
  // 2 neighbor entries expected
  verifyReachableCnt(2);
  // Now flap the port. This should schedule non coalescing updates.
  sw->linkStateChanged(kPort1, false);
  sw->linkStateChanged(kPort1, true);
  waitForStateUpdates(sw);
  // Neighbor purge is scheduled on BG thread. Wait for it to be scheduled.
  waitForBackgroundThread(sw);
  // And wait for purge to happen. Test ensures that
  // purge is not skipped due to port down/up being coalesced
  waitForStateUpdates(sw);
  sw->getNeighborUpdater()->waitForPendingUpdates();
  // Wait for static mac entries to be purged in response to
  // neighbor getting pruned
  waitForStateUpdates(sw);

  // 0 neighbor entries expected, i.e. entries must be purged
  verifyReachableCnt(0);
}

TEST_F(SwSwitchTest, VerifyIsValidStateUpdate) {
  ON_CALL(*getMockHw(sw), isValidStateUpdate(_))
      .WillByDefault(testing::Return(true));

  // ACL with a qualifier should succeed validation
  auto stateV0 = std::make_shared<SwitchState>();
  stateV0->publish();

  auto stateV1 = stateV0->clone();
  auto acls = stateV1->getAcls()->modify(&stateV1);

  auto aclEntry0 = std::make_shared<AclEntry>(0, std::string("acl0"));
  aclEntry0->setDscp(0x24);
  acls->addNode(aclEntry0, scope());

  stateV1->publish();

  EXPECT_TRUE(sw->isValidStateUpdate(StateDelta(stateV0, stateV1)));

  // ACL without any qualifier should fail validation
  auto stateV2 = stateV0->clone();
  auto acls2 = stateV2->getAcls()->modify(&stateV2);

  auto aclEntry1 = std::make_shared<AclEntry>(0, std::string("acl1"));
  acls2->addNode(aclEntry1, scope());

  stateV2->publish();

  EXPECT_FALSE(sw->isValidStateUpdate(StateDelta(stateV0, stateV2)));

  // PortQueue with valid WRED probability
  auto stateV3 = stateV0->clone();
  auto portMap0 = stateV3->getPorts()->modify(&stateV3);
  state::PortFields portFields0;
  portFields0.portId() = PortID(0);
  portFields0.portName() = "port0";
  auto port0 = std::make_shared<Port>(std::move(portFields0));
  auto portQueue0 = std::make_shared<PortQueue>(static_cast<uint8_t>(0));
  cfg::ActiveQueueManagement aqm0;
  cfg::LinearQueueCongestionDetection lqcd0;
  lqcd0.minimumLength() = 0;
  lqcd0.maximumLength() = 0;
  lqcd0.probability() = 50;
  aqm0.detection()->linear_ref() = lqcd0;
  aqm0.behavior() = cfg::QueueCongestionBehavior::EARLY_DROP;
  portQueue0->resetAqms({aqm0});
  std::vector<std::shared_ptr<PortQueue>> portQueues = {portQueue0};
  port0->resetPortQueues(portQueues);
  portMap0->addNode(port0, scope());

  stateV3->publish();

  EXPECT_TRUE(sw->isValidStateUpdate(StateDelta(stateV0, stateV3)));

  // PortQueue with invalid ECN probability
  auto stateV4 = stateV0->clone();
  auto portMap1 = stateV4->getPorts()->modify(&stateV4);
  state::PortFields portFields1;
  portFields1.portId() = PortID(1);
  portFields1.portName() = "port1";
  auto port1 = std::make_shared<Port>(std::move(portFields1));
  auto portQueue1 = std::make_shared<PortQueue>(static_cast<uint8_t>(1));
  cfg::ActiveQueueManagement aqm1;
  cfg::LinearQueueCongestionDetection lqcd1;
  lqcd1.minimumLength() = 0;
  lqcd1.maximumLength() = 0;
  lqcd1.probability() = 50;
  aqm1.detection()->linear_ref() = lqcd1;
  aqm1.behavior() = cfg::QueueCongestionBehavior::ECN;
  portQueue1->resetAqms({aqm1});
  portQueues = {portQueue1};
  port1->resetPortQueues(portQueues);
  portMap1->addNode(port1, scope());

  stateV4->publish();

  EXPECT_FALSE(sw->isValidStateUpdate(StateDelta(stateV0, stateV4)));
}

TEST_F(SwSwitchTest, gracefulExit) {
  auto bringPortsUpUpdateFn = [](const std::shared_ptr<SwitchState>& state) {
    return bringAllPortsUp(state);
  };
  sw->updateStateBlocking("Bring Ports Up", bringPortsUpUpdateFn);
  sw->gracefulExit();
}

TEST_F(SwSwitchTest, overlappingUpdatesWithExit) {
  auto bringPortsUpUpdateFn = [](const std::shared_ptr<SwitchState>& state) {
    return bringAllPortsUp(state);
  };
  sw->updateStateBlocking("Bring Ports Up", bringPortsUpUpdateFn);
  std::atomic<bool> done{false};
  std::thread nonCoaelescingUpdates([this, &done] {
    const PortID kPort1{1};
    while (!done) {
      // Now flap the port. This should schedule non coalescing updates.
      sw->linkStateChanged(kPort1, false);
      sw->linkStateChanged(kPort1, true);
    }
  });
  std::thread blockingUpdates([this, &done] {
    while (!done) {
      auto updateOperStateFn = [](const std::shared_ptr<SwitchState>& state) {
        const PortID kPort2{2};
        std::shared_ptr<SwitchState> newState(state);
        auto* port = newState->getPorts()->getNodeIf(kPort2).get();
        port = port->modify(&newState, scope());
        // Transition up<->down
        port->setOperState(!port->isUp());
        return newState;
      };
      sw->updateStateBlocking("Flap port 2", updateOperStateFn);
    }
  });

  sw->gracefulExit();
  done = true;
  nonCoaelescingUpdates.join();
  blockingUpdates.join();
}
