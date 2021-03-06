/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <folly/init/Init.h>
#include <gtest/gtest.h>
#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/ApplyThriftConfig.h"
#include "fboss/agent/LinkAggregationManager.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/MultiNodeTest.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/agent/test/TrunkUtils.h"

using namespace facebook::fboss;

DEFINE_int32(multiNodeTestPort1, 0, "multinode test port 1");
DEFINE_int32(multiNodeTestPort2, 0, "multinode test port 2");
DEFINE_bool(run_forever, false, "run the test forever");

DECLARE_bool(enable_lacp);

using utility::addAggPort;
namespace {
constexpr int kMaxRetries{60};
constexpr int kBaseVlanId{2000};
constexpr int kLacpLongTimeout{30};
} // unnamed namespace

class MultiNodeLacpTest : public MultiNodeTest {
  void SetUp() override {
    FLAGS_enable_lacp = true;
    MultiNodeTest::SetUp();
  }
  cfg::SwitchConfig initialConfig() const override {
    return getConfigWithAggPort();
  }

 protected:
  cfg::SwitchConfig getConfigWithAggPort(
      cfg::LacpPortRate rate = cfg::LacpPortRate::SLOW) const {
    auto config = utility::oneL3IntfNPortConfig(
        platform()->getHwSwitch(),
        {PortID(FLAGS_multiNodeTestPort1), PortID(FLAGS_multiNodeTestPort2)},
        cfg::PortLoopbackMode::NONE,
        true, /* interfaceHasSubnet */
        kBaseVlanId,
        false, /* optimizePortProfile */
        false /* setInterfaceMac */);
    addAggPort(
        kAggId,
        {FLAGS_multiNodeTestPort1, FLAGS_multiNodeTestPort2},
        &config,
        rate);
    return config;
  }

  // Waits for Aggregate port to be up
  void waitForAggPortStatus(bool portStatus) {
    auto aggPortUp = [&](const std::shared_ptr<SwitchState>& state) {
      const auto& aggPorts = state->getAggregatePorts();
      if (aggPorts && aggPorts->getAggregatePort(kAggId) &&
          aggPorts->getAggregatePort(kAggId)->isUp() == portStatus) {
        return true;
      }
      return false;
    };
    EXPECT_TRUE(waitForSwitchStateCondition(aggPortUp, kMaxRetries));
  }

  // Verify that LACP converged on all member ports
  void verifyLacpState() {
    const auto& aggPort =
        sw()->getState()->getAggregatePorts()->getAggregatePort(kAggId);
    EXPECT_NE(aggPort, nullptr);
    EXPECT_EQ(aggPort->forwardingSubportCount(), aggPort->subportsCount());
    for (const auto& memberAndState : aggPort->subportAndFwdState()) {
      // Verify that member port is enabled
      EXPECT_EQ(memberAndState.second, AggregatePort::Forwarding::ENABLED);
      // Verify that partner has synced
      EXPECT_TRUE(
          aggPort->getPartnerState(memberAndState.first).state &
          LacpState::IN_SYNC);
    }
  }

  AggregatePort::SubportsConstRange getSubPorts() {
    const auto& aggPort =
        sw()->getState()->getAggregatePorts()->getAggregatePort(kAggId);
    EXPECT_NE(aggPort, nullptr);
    return aggPort->sortedSubports();
  }

  ParticipantInfo::Port getRemotePortID(PortID portId) {
    const auto& aggPort =
        sw()->getState()->getAggregatePorts()->getAggregatePort(kAggId);
    EXPECT_NE(aggPort, nullptr);
    return aggPort->getPartnerState(portId).port;
  }

  const facebook::fboss::AggregatePortID kAggId{500};
};

TEST_F(MultiNodeLacpTest, Bringup) {
  // Wait for AggPort
  waitForAggPortStatus(true);

  // verify lacp state information
  verifyLacpState();

  // Do not tear down setup if run_forver flag is true. This is used on
  // remote side of device under test to keep state running till DUT
  // completes tests. Test will be terminated by the starting script
  // by sending a SIGTERM.
  if (FLAGS_run_forever) {
    XLOG(DBG2) << "MultiNodeLacpTest run forever...";
    while (true) {
      sleep(1);
      XLOG_EVERY_MS(DBG2, 5000) << "MultiNodeLacpTest running forever";
    }
  }
}

TEST_F(MultiNodeLacpTest, LinkDown) {
  // Wait for AggPort
  waitForAggPortStatus(true);

  // verify lacp state information
  verifyLacpState();

  XLOG(DBG2) << "Disable an Agg member port";
  const auto& subPorts = getSubPorts();
  EXPECT_NE(subPorts.size(), 0);
  const auto& testPort = subPorts.front().portID;
  setPortStatus(testPort, false);

  // wait for LACP protocol to react
  waitForAggPortStatus(false);

  XLOG(DBG2) << "Enable Agg member port";
  setPortStatus(testPort, true);

  waitForAggPortStatus(true);
  verifyLacpState();
}

TEST_F(MultiNodeLacpTest, RemoteLinkDown) {
  // Wait for AggPort
  waitForAggPortStatus(true);

  // verify lacp state information
  verifyLacpState();

  XLOG(DBG2) << "Disable an Agg member port on remote switch";
  const auto& subPorts = getSubPorts();
  EXPECT_NE(subPorts.size(), 0);
  const auto& remotePortID = getRemotePortID(subPorts.front().portID);
  auto client = getRemoteThriftClient();
  client->sync_setPortState(remotePortID, false);

  // wait for LACP protocol to react
  waitForAggPortStatus(false);

  XLOG(DBG2) << "Enable Agg member port on remote switch";
  client->sync_setPortState(remotePortID, true);
  waitForAggPortStatus(true);
  verifyLacpState();
}

TEST_F(MultiNodeLacpTest, LacpSlowFastInterop) {
  // Change Lacp to slow mode
  auto addAggFastRateFn = [=](const std::shared_ptr<SwitchState>& state) {
    auto newState = state->clone();
    auto config = getConfigWithAggPort(cfg::LacpPortRate::FAST);
    auto endState = applyThriftConfig(newState, &config, platform());
    return endState;
  };

  sw()->updateStateBlocking("Add AggPort fast mode", addAggFastRateFn);

  // Wait for AggPort
  waitForAggPortStatus(true);

  // verify lacp state information
  verifyLacpState();
}

// Stop sending LACP on one port and verify
// that remote side times out
TEST_F(MultiNodeLacpTest, LacpTimeout) {
  waitForAggPortStatus(true);
  // verify lacp state information
  verifyLacpState();

  // stop LACP on one of the member ports
  const auto& subPortRange = getSubPorts();
  EXPECT_GE(subPortRange.size(), 2);
  auto lagMgr = sw()->getLagManager();
  lagMgr->stopLacpOnSubPort(subPortRange.back().portID);

  auto remoteLacpTimeout = [this](const std::shared_ptr<SwitchState>& state) {
    const auto& localPort = getSubPorts().front().portID;
    const auto& aggPort = state->getAggregatePorts()->getAggregatePort(kAggId);
    const auto& remoteState = aggPort->getPartnerState(localPort).state;
    const auto flagsToCheck =
        LacpState::IN_SYNC | LacpState::COLLECTING | LacpState::DISTRIBUTING;
    return ((remoteState & flagsToCheck) == 0);
  };
  // Remote side LACP should timeout on second member and
  // bring down both members due to minlink violation after 3 timeouts
  EXPECT_TRUE(
      waitForSwitchStateCondition(remoteLacpTimeout, 4 * kLacpLongTimeout));
}
