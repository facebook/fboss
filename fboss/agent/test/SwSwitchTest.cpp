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
#include "fboss/agent/Main.h"
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
using ::testing::Return;

class SwSwitchTest : public ::testing::Test {
 public:
  void SetUp() override {
    // Setup a default state object
    auto state = testStateA();
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

TEST_F(SwSwitchTest, HwRejectsUpdateThenAccepts) {
  CounterCache counters(sw);
  // applied and desired state in sync before we begin
  EXPECT_EQ(sw->getAppliedState(), sw->getDesiredState());
  auto origState = sw->getAppliedState();
  auto newState = bringAllPortsUp(sw->getAppliedState()->clone());
  // Have HwSwitch reject this state update. In current implementation
  // this happens only in case of table overflow. However at the SwSwitch
  // layer we don't care *why* the HwSwitch rejected this update, just
  // that it did
  EXPECT_HW_CALL(sw, stateChanged(_)).WillRepeatedly(Return(origState));
  auto stateUpdateFn = [=](const std::shared_ptr<SwitchState>& /*state*/) {
    return newState;
  };
  sw->updateState("Reject update", stateUpdateFn);
  waitForStateUpdates(sw);
  EXPECT_NE(sw->getAppliedState(), sw->getDesiredState());
  counters.update();
  counters.checkDelta(SwitchStats::kCounterPrefix + "hw_out_of_sync", 1);
  EXPECT_EQ(1, counters.value(SwitchStats::kCounterPrefix + "hw_out_of_sync"));
  // Have HwSwitch now accept this update
  EXPECT_HW_CALL(sw, stateChanged(_)).WillRepeatedly(Return(newState));
  sw->updateState("Accept update", stateUpdateFn);
  waitForStateUpdates(sw);
  EXPECT_EQ(sw->getAppliedState(), sw->getDesiredState());
  counters.update();
  counters.checkDelta(SwitchStats::kCounterPrefix + "hw_out_of_sync", -1);
  EXPECT_EQ(0, counters.value(SwitchStats::kCounterPrefix + "hw_out_of_sync"));
}

TEST_F(SwSwitchTest, TestStateNonCoalescing) {
  const PortID kPort1{1};
  const VlanID kVlan1{1};

  auto verifyReachableCnt = [kVlan1, this](int expectedReachableNbrCnt) {
    auto getReachableCount = [](auto nbrTable) {
      auto reachableCnt = 0;
      for (const auto entry : *nbrTable) {
        if (entry->getState() == NeighborState::REACHABLE) {
          ++reachableCnt;
        }
      }
      return reachableCnt;
    };
    auto arpTable =
        sw->getAppliedState()->getVlans()->getVlan(kVlan1)->getArpTable();
    auto ndpTable =
        sw->getAppliedState()->getVlans()->getVlan(kVlan1)->getNdpTable();
    auto reachableCnt =
        getReachableCount(arpTable) + getReachableCount(ndpTable);
    EXPECT_EQ(expectedReachableNbrCnt, reachableCnt);
  };
  // No neighbor entries expected
  verifyReachableCnt(0);
  auto origState = sw->getAppliedState();
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
  auto aclMap1 = stateV1->getAcls()->modify(&stateV1);

  auto aclEntry0 = std::make_shared<AclEntry>(0, "acl0");
  aclEntry0->setDscp(0x24);
  aclMap1->addNode(aclEntry0);

  stateV1->publish();

  EXPECT_TRUE(sw->isValidStateUpdate(StateDelta(stateV0, stateV1)));

  // ACL without any qualifier should fail validation
  auto stateV2 = stateV0->clone();
  auto aclMap2 = stateV2->getAcls()->modify(&stateV2);

  auto aclEntry1 = std::make_shared<AclEntry>(0, "acl1");
  aclMap2->addNode(aclEntry1);

  stateV2->publish();

  EXPECT_FALSE(sw->isValidStateUpdate(StateDelta(stateV0, stateV2)));
}
