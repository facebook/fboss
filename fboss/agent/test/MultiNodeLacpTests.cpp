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
#include <memory>
#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/ApplyThriftConfig.h"
#include "fboss/agent/LacpMachines.h"
#include "fboss/agent/LinkAggregationManager.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwTestEcmpUtils.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/hw/test/HwTestTrunkUtils.h"
#include "fboss/agent/hw/test/LoadBalancerUtils.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/PortDescriptor.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/MultiNodeTest.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/agent/test/TrunkUtils.h"

#include "common/process/Process.h"
#include "folly/IPAddressV4.h"
#include "folly/IPAddressV6.h"
#include "folly/MacAddress.h"

using namespace facebook::fboss;

DECLARE_bool(enable_lacp);

using utility::addAggPort;
namespace {
constexpr int kMaxRetries{60};
constexpr int kRemoteSwitchMaxRetries{150};
constexpr int kBaseVlanId{2000};
constexpr int kLacpLongTimeout{30};
constexpr int kPortsPerAggPort{2};
constexpr int kBaseAggId{500};
} // unnamed namespace

class MultiNodeLacpTest : public MultiNodeTest {
 public:
  void SetUp() override {
    MultiNodeTest::SetUp();
    if (isDUT()) {
      verifyInitialState();
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
    auto config = utility::multiplePortsPerVlanConfig(
        platform()->getHwSwitch(),
        testPorts(),
        cfg::PortLoopbackMode::NONE,
        true, /* interfaceHasSubnet */
        false, /* setInterfaceMac */
        kBaseVlanId,
        kPortsPerAggPort /*numPortsPerVlan*/);
    auto idx = 0;
    auto portList = testPorts();
    for (const auto& aggId : getAggPorts()) {
      addAggPort(aggId, {portList[idx++], portList[idx++]}, &config, rate);
    }
    config.loadBalancers_ref() =
        utility::getEcmpFullTrunkHalfHashConfig(platform());
    config.staticRoutesWithNhops_ref()->resize(2);
    config.staticRoutesWithNhops_ref()[0].prefix_ref() = "::/0";
    for (const auto& entry : getNeighborIpAddrs<folly::IPAddressV6>()) {
      config.staticRoutesWithNhops_ref()[0].nexthops_ref()->push_back(
          entry.str());
    }
    config.staticRoutesWithNhops_ref()[1].prefix_ref() = "0.0.0.0/0";
    for (const auto& entry : getNeighborIpAddrs<folly::IPAddressV4>()) {
      config.staticRoutesWithNhops_ref()[1].nexthops_ref()->push_back(
          entry.str());
    }
    return config;
  }

  template <typename AddrT>
  std::vector<AddrT> getNeighborIpAddrs() const {
    if constexpr (std::is_same_v<AddrT, folly::IPAddressV6>) {
      return {AddrT(isDUT() ? "1::1" : "1::")};
    } else {
      return {AddrT(isDUT() ? "1.0.0.2" : "1.0.0.1")};
    }
  }

  std::vector<AggregatePortID> getAggPorts() const {
    return {AggregatePortID(kBaseAggId)};
  }

  // Waits for Aggregate port to be up
  void waitForAggPortStatus(AggregatePortID aggId, bool portStatus) const {
    auto aggPortUp = [&](const std::shared_ptr<SwitchState>& state) {
      const auto& aggPorts = state->getAggregatePorts();
      if (aggPorts && aggPorts->getAggregatePort(aggId) &&
          aggPorts->getAggregatePort(aggId)->isUp() == portStatus) {
        return true;
      }
      return false;
    };
    EXPECT_TRUE(waitForSwitchStateCondition(
        aggPortUp, isDUT() ? kMaxRetries : kRemoteSwitchMaxRetries));
  }

  // Verify that LACP converged on all member ports
  void verifyLacpState() const {
    for (const auto& aggId : getAggPorts()) {
      const auto& aggPort =
          sw()->getState()->getAggregatePorts()->getAggregatePort(aggId);
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
  }
  void verifyReachability() const {
    const auto dstIpV4s = getNeighborIpAddrs<folly::IPAddressV4>();
    const auto dstIpV6s = getNeighborIpAddrs<folly::IPAddressV6>();

    auto verifyNeighborEntries = [=](const auto& aggId,
                                     const auto& vlanId,
                                     const auto& dstIpV4,
                                     const auto& dstIpV6) {
      checkNeighborResolved(
          folly::IPAddress(dstIpV4), vlanId, PortDescriptor(aggId));
      checkNeighborResolved(
          folly::IPAddress(dstIpV6), vlanId, PortDescriptor(aggId));
    };
    auto verify = [=](const auto& aggId,
                      const auto& dstIpV4,
                      const auto& dstIpV6,
                      const auto& vlanId) {
      std::string pingCmd = "ping -c 5 ";
      std::string resultStr;
      std::string errStr;
      EXPECT_TRUE(facebook::process::Process::execShellCmd(
          pingCmd + dstIpV4.str(), &resultStr, &errStr));
      EXPECT_TRUE(facebook::process::Process::execShellCmd(
          pingCmd + dstIpV6.str(), &resultStr, &errStr));
      // Verify neighbor entries
      verifyNeighborEntries(aggId, vlanId, dstIpV4, dstIpV6);
    };
    auto idx = 0;
    for (const auto& aggId : getAggPorts()) {
      verify(
          aggId,
          dstIpV4s[idx],
          dstIpV6s[idx],
          VlanID(*getConfigWithAggPort().interfaces_ref()[idx].vlanID_ref()));
      idx++;
    }
  }

  void verifyInitialState() {
    // Wait for AggPort
    for (const auto& aggId : getAggPorts()) {
      waitForAggPortStatus(aggId, true);
    }

    // verify lacp state information
    verifyLacpState();
    verifyReachability();
  }

  AggregatePort::SubportsConstRange getSubPorts(AggregatePortID aggId) {
    const auto& aggPort =
        sw()->getState()->getAggregatePorts()->getAggregatePort(aggId);
    EXPECT_NE(aggPort, nullptr);
    return aggPort->sortedSubports();
  }

  ParticipantInfo::Port getRemotePortID(AggregatePortID aggId, PortID portId) {
    const auto& aggPort =
        sw()->getState()->getAggregatePorts()->getAggregatePort(aggId);
    EXPECT_NE(aggPort, nullptr);
    return aggPort->getPartnerState(portId).port;
  }

  void createL3DataplaneFlood() {
    disableTTLDecrementsForRoute<folly::IPAddressV6>({folly::IPAddressV6(), 0});
    disableTTLDecrementsForRoute<folly::IPAddressV4>({folly::IPAddressV4(), 0});
    if (isDUT()) {
      auto state = sw()->getState();
      auto vlan = (*state->getVlans()->begin())->getID();
      auto srcMac = state->getInterfaces()->getInterfaceInVlan(vlan)->getMac();
      auto destMac =
          getNeighborEntry(
              getNeighborIpAddrs<folly::IPAddressV6>()[0],
              state->getInterfaces()->getInterfaceInVlan(vlan)->getID())
              .first;
      for (const auto& sendV6 : {true, false}) {
        utility::pumpTraffic(
            sendV6,
            sw()->getHw(),
            destMac,
            (*sw()->getState()->getVlans()->begin())->getID(),
            std::nullopt,
            255,
            srcMac);
      }
    }
  }
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
    const auto aggPort = getAggPorts()[0];
    // Local port flap
    XLOG(DBG2) << "Disable an Agg member port";
    const auto& subPorts = getSubPorts(aggPort);
    EXPECT_NE(subPorts.size(), 0);
    const auto testPort = subPorts.front().portID;
    setPortStatus(testPort, false);

    // wait for LACP protocol to react
    waitForAggPortStatus(aggPort, false);

    XLOG(DBG2) << "Enable Agg member port";
    setPortStatus(testPort, true);

    waitForAggPortStatus(aggPort, true);
    verifyLacpState();

    // Remote port flap
    XLOG(DBG2) << "Disable an Agg member port on remote switch";
    const auto remotePortID = getRemotePortID(aggPort, testPort);
    auto client = getRemoteThriftClient();
    client->sync_setPortState(remotePortID, false);

    // wait for LACP protocol to react
    waitForAggPortStatus(aggPort, false);

    XLOG(DBG2) << "Enable Agg member port on remote switch";
    client->sync_setPortState(remotePortID, true);
    waitForAggPortStatus(aggPort, true);
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
    const auto aggPort = getAggPorts()[0];
    // Wait for AggPort
    waitForAggPortStatus(aggPort, true);

    // verify lacp state information
    verifyLacpState();
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(MultiNodeLacpTest, LacpWarmBoootIsHitless) {
  auto setup = [&]() {
    // Setup() already verified state for DUT
    if (!isDUT()) {
      verifyInitialState();
    }
    createL3DataplaneFlood();
  };

  auto verify = [&]() {
    // When looping packets, few initial packets are marked as indiscards even
    // though route and ARP/ND entries are present. Currently following
    // up with broadcom in a CSP. Disable initial discard check till then.
    if (platform()->getHwSwitch()->getBootType() == BootType::WARM_BOOT) {
      assertNoInDiscards();
    }
    for (const auto& aggId : getAggPorts()) {
      const auto countInSw = sw()->getState()
                                 ->getAggregatePorts()
                                 ->getAggregatePort(aggId)
                                 ->subportsCount();
      EXPECT_EQ(
          utility::getTrunkMemberCountInHw(sw()->getHw(), aggId, countInSw),
          countInSw);
    }
  };
  verifyAcrossWarmBoots(setup, verify);
}

class MultiNodeDisruptiveTest : public MultiNodeLacpTest {
 public:
  void SetUp() override {
    MultiNodeTest::SetUp();
  }
};
// Stop sending LACP on one port and verify
// that remote side times out
TEST_F(MultiNodeDisruptiveTest, LacpTimeout) {
  auto verify = [=]() {
    if (isDUT()) {
      const auto aggPortId = getAggPorts()[0];
      // stop LACP on one of the member ports
      const auto& subPortRange = getSubPorts(aggPortId);
      EXPECT_GE(subPortRange.size(), 2);
      auto lagMgr = sw()->getLagManager();
      lagMgr->stopLacpOnSubPort(subPortRange.back().portID);
      auto remoteLacpTimeout =
          [this, &aggPortId](const std::shared_ptr<SwitchState>& state) {
            const auto& localPort = getSubPorts(aggPortId).front().portID;
            const auto& aggPort =
                state->getAggregatePorts()->getAggregatePort(aggPortId);
            const auto& remoteState = aggPort->getPartnerState(localPort).state;
            const auto flagsToCheck = LacpState::IN_SYNC |
                LacpState::COLLECTING | LacpState::DISTRIBUTING;
            return ((remoteState & flagsToCheck) == 0);
          };
      // Remote side LACP should timeout on second member and
      // bring down both members due to minlink violation after 3 timeouts
      EXPECT_TRUE(
          waitForSwitchStateCondition(remoteLacpTimeout, 4 * kLacpLongTimeout));
    }
  };
  verifyAcrossWarmBoots(verify);
}
