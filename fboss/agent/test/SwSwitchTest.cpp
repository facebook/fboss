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
#include "fboss/agent/MultiSwitchFb303Stats.h"
#include "fboss/agent/NeighborUpdater.h"
#include "fboss/agent/PortStats.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/ValidateStateUpdate.h"
#include "fboss/agent/state/ArpTable.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/StateUtils.h"
#include "fboss/agent/state/SwitchState.h"

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
  aqm0.detection()->linear() = lqcd0;
  aqm0.behavior() = cfg::QueueCongestionBehavior::EARLY_DROP;
  portQueue0->resetAqms({aqm0});
  std::vector<std::shared_ptr<PortQueue>> portQueues = {portQueue0};
  port0->resetPortQueues(portQueues);
  portMap0->addNode(port0, scope());

  stateV3->publish();

  EXPECT_TRUE(sw->isValidStateUpdate(StateDelta(stateV0, stateV3)));

  // PortQueue with valid ECN probability (50%) - valid when
  // ECN_PROBABILISTIC_MARKING supported MockAsic returns true for
  // ECN_PROBABILISTIC_MARKING by default
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
  aqm1.detection()->linear() = lqcd1;
  aqm1.behavior() = cfg::QueueCongestionBehavior::ECN;
  portQueue1->resetAqms({aqm1});
  portQueues = {portQueue1};
  port1->resetPortQueues(portQueues);
  portMap1->addNode(port1, scope());

  stateV4->publish();

  EXPECT_TRUE(sw->isValidStateUpdate(StateDelta(stateV0, stateV4)));

  // PortQueue with ECN probability 100% - always valid regardless of feature
  // support
  auto stateV5 = stateV0->clone();
  auto portMap2 = stateV5->getPorts()->modify(&stateV5);
  state::PortFields portFields2;
  portFields2.portId() = PortID(2);
  portFields2.portName() = "port2";
  auto port2 = std::make_shared<Port>(std::move(portFields2));
  auto portQueue2 = std::make_shared<PortQueue>(static_cast<uint8_t>(0));
  cfg::ActiveQueueManagement aqm2;
  cfg::LinearQueueCongestionDetection lqcd2;
  lqcd2.minimumLength() = 0;
  lqcd2.maximumLength() = 0;
  lqcd2.probability() = 100;
  aqm2.detection()->linear() = lqcd2;
  aqm2.behavior() = cfg::QueueCongestionBehavior::ECN;
  portQueue2->resetAqms({aqm2});
  std::vector<std::shared_ptr<PortQueue>> portQueues2 = {portQueue2};
  port2->resetPortQueues(portQueues2);
  portMap2->addNode(port2, scope());

  stateV5->publish();

  EXPECT_TRUE(sw->isValidStateUpdate(StateDelta(stateV0, stateV5)));

  // PortQueue with ECN probability 0% - invalid (must be >0)
  auto stateV6 = stateV0->clone();
  auto portMap3 = stateV6->getPorts()->modify(&stateV6);
  state::PortFields portFields3;
  portFields3.portId() = PortID(3);
  portFields3.portName() = "port3";
  auto port3 = std::make_shared<Port>(std::move(portFields3));
  auto portQueue3 = std::make_shared<PortQueue>(static_cast<uint8_t>(0));
  cfg::ActiveQueueManagement aqm3;
  cfg::LinearQueueCongestionDetection lqcd3;
  lqcd3.minimumLength() = 0;
  lqcd3.maximumLength() = 0;
  lqcd3.probability() = 0;
  aqm3.detection()->linear() = lqcd3;
  aqm3.behavior() = cfg::QueueCongestionBehavior::ECN;
  portQueue3->resetAqms({aqm3});
  std::vector<std::shared_ptr<PortQueue>> portQueues3 = {portQueue3};
  port3->resetPortQueues(portQueues3);
  portMap3->addNode(port3, scope());

  stateV6->publish();

  EXPECT_FALSE(sw->isValidStateUpdate(StateDelta(stateV0, stateV6)));

  // PortQueue with ECN probability 101% - invalid (must be <=100)
  auto stateV7 = stateV0->clone();
  auto portMap4 = stateV7->getPorts()->modify(&stateV7);
  state::PortFields portFields4;
  portFields4.portId() = PortID(4);
  portFields4.portName() = "port4";
  auto port4 = std::make_shared<Port>(std::move(portFields4));
  auto portQueue4 = std::make_shared<PortQueue>(static_cast<uint8_t>(0));
  cfg::ActiveQueueManagement aqm4;
  cfg::LinearQueueCongestionDetection lqcd4;
  lqcd4.minimumLength() = 0;
  lqcd4.maximumLength() = 0;
  lqcd4.probability() = 101;
  aqm4.detection()->linear() = lqcd4;
  aqm4.behavior() = cfg::QueueCongestionBehavior::ECN;
  portQueue4->resetAqms({aqm4});
  std::vector<std::shared_ptr<PortQueue>> portQueues4 = {portQueue4};
  port4->resetPortQueues(portQueues4);
  portMap4->addNode(port4, scope());

  stateV7->publish();

  EXPECT_FALSE(sw->isValidStateUpdate(StateDelta(stateV0, stateV7)));
}

TEST_F(SwSwitchTest, VerifyEcnValidationWhenFeatureNotSupported) {
  // This test verifies ECN validation when ECN_PROBABILISTIC_MARKING is NOT
  // supported. In this case, only 100% probability is allowed.
  ON_CALL(*getMockHw(sw), isValidStateUpdate(_))
      .WillByDefault(testing::Return(true));

  auto stateV0 = std::make_shared<SwitchState>();
  stateV0->publish();

  // Helper to create a port with ECN configuration
  auto createPortWithEcnProbability =
      [&](PortID portId, const std::string& portName, int probability) {
        state::PortFields portFields;
        portFields.portId() = portId;
        portFields.portName() = portName;
        auto port = std::make_shared<Port>(std::move(portFields));
        auto portQueue = std::make_shared<PortQueue>(static_cast<uint8_t>(0));
        cfg::ActiveQueueManagement aqm;
        cfg::LinearQueueCongestionDetection lqcd;
        lqcd.minimumLength() = 0;
        lqcd.maximumLength() = 0;
        lqcd.probability() = probability;
        aqm.detection()->linear() = lqcd;
        aqm.behavior() = cfg::QueueCongestionBehavior::ECN;
        portQueue->resetAqms({aqm});
        std::vector<std::shared_ptr<PortQueue>> portQueues = {portQueue};
        port->resetPortQueues(portQueues);
        return port;
      };

  // Test 1: ECN probability 50% should be INVALID when feature not supported
  auto port50 = createPortWithEcnProbability(PortID(10), "port10", 50);
  EXPECT_FALSE(hasValidPortQueues(
      port50, false /* isEcnProbabilisticMarkingSupported */));

  // Test 2: ECN probability 100% should be VALID when feature not supported
  auto port100 = createPortWithEcnProbability(PortID(11), "port11", 100);
  EXPECT_TRUE(hasValidPortQueues(
      port100, false /* isEcnProbabilisticMarkingSupported */));

  // Test 3: ECN probability 0% should be INVALID when feature not supported
  auto port0 = createPortWithEcnProbability(PortID(12), "port12", 0);
  EXPECT_FALSE(hasValidPortQueues(
      port0, false /* isEcnProbabilisticMarkingSupported */));

  // Test 4: ECN probability 99% should be INVALID when feature not supported
  auto port99 = createPortWithEcnProbability(PortID(13), "port13", 99);
  EXPECT_FALSE(hasValidPortQueues(
      port99, false /* isEcnProbabilisticMarkingSupported */));
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
      sw->linkStateChanged(kPort1, false, cfg::PortType::INTERFACE_PORT);
      sw->linkStateChanged(kPort1, true, cfg::PortType::INTERFACE_PORT);
    }
  });
  std::thread blockingUpdates([this, &done] {
    while (!done) {
      auto updateOperStateFn = [](const std::shared_ptr<SwitchState>& state) {
        const PortID kPort2{2};
        std::shared_ptr<SwitchState> newState(state);
        auto* port = newState->getPorts()->getNodeIf(kPort2).get();
        port = port->modify(&newState);
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

TEST_F(SwSwitchTest, multiSwitchFb303Stats) {
  std::map<SwitchID, const HwAsic*> asicMap;
  CounterCache counters(sw);
  asicMap[SwitchID(0)] = handle->getPlatform()->getAsic();
  auto multiSwitchStats = std::make_unique<MultiSwitchFb303Stats>(asicMap);
  auto getGlobalStats = [](int64_t val) {
    HwSwitchFb303GlobalStats globalStats;
    globalStats.tx_pkt_allocated() = val;
    globalStats.tx_pkt_freed() = val;
    globalStats.tx_pkt_sent() = val;
    globalStats.tx_pkt_sent_done() = val;
    globalStats.tx_errors() = val;
    globalStats.tx_pkt_allocation_errors() = val;
    globalStats.parity_errors() = val;
    globalStats.parity_corr() = val;
    globalStats.parity_uncorr() = val;
    globalStats.asic_error() = val;
    globalStats.global_drops() = val;
    globalStats.global_reachability_drops() = val;
    globalStats.packet_integrity_drops() = val;
    globalStats.vsq_resource_exhaustion_drops() = val;
    globalStats.dram_enqueued_bytes() = val;
    globalStats.dram_dequeued_bytes() = val;
    globalStats.dram_blocked_time_ns() = val;
    globalStats.dram_quarantined_buffer_count() = val;
    globalStats.fabric_reachability_missing() = val;
    globalStats.fabric_reachability_mismatch() = val;
    globalStats.fabric_connectivity_bogus() = val;
    globalStats.switch_reachability_change() = val;
    return globalStats;
  };
  auto val = 100;
  auto statsUpdate1 = getGlobalStats(val);
  multiSwitchStats->updateStats(statsUpdate1);
  // fb303 stat should have gotten an update
  EXPECT_EQ(
      multiSwitchStats->getHwSwitchFb303GlobalStats().getTxPktAllocCount(), 1);
  counters.update();
  auto checkValues = [&counters](const int64_t expectedVal) {
    EXPECT_EQ(counters.value("mock.tx.pkt.allocated.sum"), expectedVal);
    EXPECT_EQ(counters.value("mock.tx.pkt.freed.sum"), expectedVal);
    EXPECT_EQ(counters.value("mock.tx.pkt.sent.sum"), expectedVal);
    EXPECT_EQ(counters.value("mock.tx.pkt.sent.done.sum"), expectedVal);
    EXPECT_EQ(counters.value("mock.tx.errors.sum"), expectedVal);
    EXPECT_EQ(counters.value("mock.tx.pkt.allocation.errors.sum"), expectedVal);
    EXPECT_EQ(counters.value("mock.parity.errors.sum"), expectedVal);
    EXPECT_EQ(counters.value("mock.parity.corr.sum"), expectedVal);
    EXPECT_EQ(counters.value("mock.parity.uncorr.sum"), expectedVal);
    EXPECT_EQ(counters.value("mock.asic.error.sum"), expectedVal);
    EXPECT_EQ(counters.value("global_drops.sum"), expectedVal);
    EXPECT_EQ(counters.value("global_reachability_drops.sum"), expectedVal);
    EXPECT_EQ(counters.value("packet_integrity_drops.sum"), expectedVal);
    EXPECT_EQ(counters.value("vsq_resource_exhaustion_drops.sum"), expectedVal);
    EXPECT_EQ(counters.value("dram_enqueued_bytes.sum"), expectedVal);
    EXPECT_EQ(counters.value("dram_dequeued_bytes.sum"), expectedVal);
    EXPECT_EQ(counters.value("dram_blocked_time_ns.sum"), expectedVal);
    EXPECT_EQ(counters.value("dram_quarantined_buffer_count.sum"), expectedVal);
    EXPECT_EQ(counters.value("fabric_connectivity_missing"), expectedVal);
    EXPECT_EQ(counters.value("fabric_connectivity_mismatch"), expectedVal);
    EXPECT_EQ(counters.value("fabric_connectivity_bogus"), expectedVal);
    EXPECT_EQ(counters.value("switch_reachability_change.sum"), expectedVal);
  };
  checkValues(val);
  auto updatedVal = 110;
  auto statsUpdate2 = getGlobalStats(updatedVal);
  multiSwitchStats->updateStats(statsUpdate2);
  counters.update();
  checkValues(updatedVal);
  updatedVal = 50;
  auto statsUpdate3 = getGlobalStats(updatedVal);
  multiSwitchStats->updateStats(statsUpdate3);
  counters.update();
  checkValues(updatedVal);
}

TEST_F(SwSwitchTest, swSwitchRunState) {
  auto switchSettings =
      utility::getFirstNodeIf(sw->getState()->getSwitchSettings());
  EXPECT_EQ(switchSettings->getSwSwitchRunState(), SwitchRunState::CONFIGURED);
}

class SwSwitchTestNbrs : public SwSwitchTest {
  void SetUp() override {
    SwSwitchTest::SetUp();
  }
};

TEST_F(SwSwitchTestNbrs, TestStateNonCoalescing) {
  auto sw = this->sw;

  const PortID kPort1{1};
  const VlanID kVlan1{1};
  const InterfaceID kInterfaceID1{1};

  auto verifyReachableCnt =
      [kVlan1, kInterfaceID1, this](int expectedReachableNbrCnt) {
        auto getReachableCount = [](auto nbrTable) {
          auto reachableCnt = 0;
          for (const auto& iter : std::as_const(*nbrTable)) {
            auto entry = iter.second;
            if (entry->getState() == NeighborState::REACHABLE) {
              ++reachableCnt;
            }
          }
          return reachableCnt;
        };

        std::shared_ptr<ArpTable> arpTable;
        std::shared_ptr<NdpTable> ndpTable;

        arpTable = this->sw->getState()
                       ->getInterfaces()
                       ->getNode(kInterfaceID1)
                       ->getArpTable();
        ndpTable = this->sw->getState()
                       ->getInterfaces()
                       ->getNode(kInterfaceID1)
                       ->getNdpTable();

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

  sw->getNeighborUpdater()->receivedArpMineForIntf(
      kInterfaceID1,
      IPAddressV4("10.0.0.2"),
      MacAddress("01:02:03:04:05:06"),
      PortDescriptor(kPort1),
      ArpOpCode::ARP_OP_REPLY);

  sw->getNeighborUpdater()->receivedNdpMineForIntf(
      kInterfaceID1,
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
  sw->linkStateChanged(kPort1, false, cfg::PortType::INTERFACE_PORT);
  sw->linkStateChanged(kPort1, true, cfg::PortType::INTERFACE_PORT);
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

TEST_F(SwSwitchTest, FillFsdbStatsNullShelManager) {
  // This test exercises fillFsdbStats() on an NPU (non-VOQ) switch
  // configuration where shelManager_ is null. Without the null check
  // guard on shelManager_, this would crash with a null pointer
  // dereference.
  multiswitch::HwSwitchStats hwStats;
  sw->updateHwSwitchStats(0, hwStats);
  // Should not crash - shelManager_ is null on NPU switches
  // and the code must guard against that.
  EXPECT_NO_THROW(sw->fillFsdbStats());
}
