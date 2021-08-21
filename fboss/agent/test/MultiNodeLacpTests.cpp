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
#include "fboss/agent/LacpMachines.h"
#include "fboss/agent/LinkAggregationManager.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/MultiNodeTest.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/agent/test/TrunkUtils.h"

#include "common/process/Process.h"

using namespace facebook::fboss;

DECLARE_bool(enable_lacp);

using utility::addAggPort;
namespace {
constexpr int kMaxRetries{60};
constexpr int kBaseVlanId{2000};
constexpr int kLacpLongTimeout{30};
} // unnamed namespace

class MultiNodeLacpTest : public MultiNodeTest {
 public:
  void SetUp() override {
    MultiNodeTest::SetUp();
    if (isDUT()) {
      // Wait for AggPort
      waitForAggPortStatus(true);

      // verify lacp state information
      verifyLacpState();
    }
  }

 private:
  cfg::SwitchConfig initialConfig() const override {
    return getConfigWithAggPort();
  }

  void setupFlags() const override {
    FLAGS_enable_lacp = true;
    MultiNodeTest::setupFlags();
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
  auto verify = [=]() {
    auto period = PeriodicTransmissionMachine::LONG_PERIOD * 3;
    // ensure that lacp session can stay up post timeout
    XLOG(DBG2) << "Waiting for LACP timeout period";
    std::this_thread::sleep_for(period);
    verifyLacpState();
  };

  verifyAcrossWarmBoots(verify);
}

TEST_F(MultiNodeLacpTest, LinkDown) {
  auto verify = [=]() {
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
  };
  verifyAcrossWarmBoots(verify);
}

TEST_F(MultiNodeLacpTest, RemoteLinkDown) {
  auto verify = [=]() {
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
  };
  verifyAcrossWarmBoots(verify);
}

TEST_F(MultiNodeLacpTest, LacpSlowFastInterop) {
  auto setup = [=]() {
    if (isDUT()) {
      // Change Lacp to fast mode
      sw()->applyConfig(
          "Add AggPort fast mode",
          getConfigWithAggPort(cfg::LacpPortRate::FAST));
    }
  };

  auto verify = [=]() {
    // Wait for AggPort
    waitForAggPortStatus(true);

    // verify lacp state information
    verifyLacpState();
  };
  verifyAcrossWarmBoots(setup, verify);
}

// Stop sending LACP on one port and verify
// that remote side times out
TEST_F(MultiNodeLacpTest, LacpTimeout) {
  if (isDUT()) {
    // stop LACP on one of the member ports
    const auto& subPortRange = getSubPorts();
    EXPECT_GE(subPortRange.size(), 2);
    auto lagMgr = sw()->getLagManager();
    lagMgr->stopLacpOnSubPort(subPortRange.back().portID);
    auto remoteLacpTimeout = [this](const std::shared_ptr<SwitchState>& state) {
      const auto& localPort = getSubPorts().front().portID;
      const auto& aggPort =
          state->getAggregatePorts()->getAggregatePort(kAggId);
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
}

TEST_F(MultiNodeLacpTest, NeighborTest) {
  const auto dstIpV4 = "1.1.1.2";
  const auto dstIpV6 = "1::1";

  auto verifyNeighborEntries = [=]() {
    const auto vlanId =
        VlanID(*getConfigWithAggPort().vlanPorts_ref()[0].vlanID_ref());
    checkNeighborResolved(
        folly::IPAddress(dstIpV4), vlanId, PortDescriptor(kAggId));
    checkNeighborResolved(
        folly::IPAddress(dstIpV6), vlanId, PortDescriptor(kAggId));
  };
  auto verify = [=]() {
    std::string pingCmd = "ping -c 5 ";
    std::string resultStr;
    std::string errStr;
    EXPECT_TRUE(facebook::process::Process::execShellCmd(
        pingCmd + dstIpV4, &resultStr, &errStr));
    EXPECT_TRUE(facebook::process::Process::execShellCmd(
        pingCmd + dstIpV6, &resultStr, &errStr));
    // Verify neighbor entries
    verifyNeighborEntries();
  };
  verifyAcrossWarmBoots(
      []() {},
      verify,
      []() {},
      [verifyNeighborEntries]() { verifyNeighborEntries(); });
}
