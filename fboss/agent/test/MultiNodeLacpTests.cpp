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
#include <thread>
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
#include "fboss/agent/state/StateUtils.h"
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

 protected:
  cfg::SwitchConfig initialConfig() const override {
    return getConfigWithAggPort();
  }

  void setCmdLineFlagOverrides() const override {
    FLAGS_enable_lacp = true;
    MultiNodeTest::setCmdLineFlagOverrides();
  }

  cfg::SwitchConfig getConfigWithAggPort(
      cfg::LacpPortRate rate = cfg::LacpPortRate::SLOW) const {
    auto config = utility::multiplePortsPerIntfConfig(
        platform()->getHwSwitch(),
        testPorts(),
        {{cfg::PortType::INTERFACE_PORT, cfg::PortLoopbackMode::NONE}},
        true, /* interfaceHasSubnet */
        false, /* setInterfaceMac */
        kBaseVlanId,
        kPortsPerAggPort /*numPortsPerVlan*/);
    auto idx = 0;
    auto portList = testPorts();
    for (const auto& aggId : getAggPorts()) {
      addAggPort(aggId, {portList[idx++], portList[idx++]}, &config, rate);
    }
    config.loadBalancers() =
        utility::getEcmpFullTrunkHalfHashConfig(platform());

    config.staticRoutesWithNhops()->clear();
    setupDefaultRoutes(&config, 2);
    return config;
  }

  void setupDefaultRoutes(cfg::SwitchConfig* config, int numNextHops) const {
    setupRoute(
        config,
        RoutePrefix<folly::IPAddressV4>(folly::IPAddressV4("0.0.0.0"), 0),
        numNextHops);
    setupRoute(
        config,
        RoutePrefix<folly::IPAddressV6>(folly::IPAddressV6("::"), 0),
        numNextHops);
  }

  template <typename AddrT>
  void setupRoute(
      cfg::SwitchConfig* config,
      const RoutePrefix<AddrT>& prefix,
      int numNextHops) const {
    cfg::StaticRouteWithNextHops route{};
    auto prefixStr = prefix.str();
    route.prefix() = prefixStr;
    auto addrs = getNeighborIpAddrs<AddrT>();
    addrs.resize(numNextHops);
    for (auto addr : addrs) {
      route.nexthops()->push_back(addr.str());
    }
    auto iter = config->staticRoutesWithNhops()->begin();
    while (iter != config->staticRoutesWithNhops()->end()) {
      // remove any route with existing prefix
      if (*iter->prefix() == prefixStr) {
        config->staticRoutesWithNhops()->erase(iter);
        break;
      }
      iter++;
    }
    config->staticRoutesWithNhops()->push_back(route);
  }

  template <typename AddrT>
  std::vector<AddrT> getNeighborIpAddrs() const {
    if constexpr (std::is_same_v<AddrT, folly::IPAddressV6>) {
      return {AddrT(isDUT() ? "1::1" : "1::"), AddrT(isDUT() ? "2::1" : "2::")};
    } else {
      return {
          AddrT(isDUT() ? "1.0.0.2" : "1.0.0.1"),
          AddrT(isDUT() ? "2.0.0.2" : "2.0.0.1")};
    }
  }

  std::vector<AggregatePortID> getAggPorts() const {
    return {AggregatePortID(kBaseAggId), AggregatePortID(kBaseAggId + 1)};
  }

  // Waits for Aggregate port to be up
  void waitForAggPortStatus(AggregatePortID aggId, bool portStatus) const {
    auto aggPortUp = [&](const std::shared_ptr<SwitchState>& state) {
      const auto& aggPorts = state->getAggregatePorts();
      if (aggPorts) {
        const auto aggPort = aggPorts->getNode(aggId);
        if (aggPort && aggPort->isUp() == portStatus) {
          return true;
        }
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
          sw()->getState()->getAggregatePorts()->getNode(aggId);
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

  void verifyReachability(
      folly::IPAddress& dstIp,
      VlanID vlan,
      AggregatePortID aggPort) const {
    std::string pingCmd = dstIp.isV4() ? "ping -c 5 " : "ping6 -c 5 ";
    std::string resultStr;
    std::string errStr;
    EXPECT_TRUE(facebook::process::Process::execShellCmd(
        pingCmd + dstIp.str(), &resultStr, &errStr));
    checkNeighborResolved(dstIp, vlan, PortDescriptor(aggPort));
  }

  template <typename AddrT>
  void verifyReachability(const RoutePrefix<AddrT>& prefix) const {
    std::vector<folly::IPAddress> nexthops{};
    auto config = getConfigWithAggPort();
    for (auto route : *config.staticRoutesWithNhops()) {
      if (*route.prefix() == prefix.str()) {
        for (auto nexthop : *route.nexthops()) {
          nexthops.push_back(folly::IPAddress(nexthop));
        }
        break;
      }
    }
    auto vlans = getVlanIDs();
    auto aggPorts = getAggPorts();
    for (auto i = 0; i < nexthops.size(); i++) {
      verifyReachability(nexthops[i], vlans[i], aggPorts[i]);
    }
  }

  std::vector<VlanID> getVlanIDs() const {
    std::vector<VlanID> vlans{};
    for (auto aggPort : getAggPorts()) {
      vlans.push_back(getVlanID(aggPort));
    }
    return vlans;
  }

  VlanID getVlanID(AggregatePortID aggPortID) const {
    auto config = initialConfig();
    PortID memberPort{};
    for (auto aggPort : *config.aggregatePorts()) {
      if (AggregatePortID(*aggPort.key()) != aggPortID) {
        continue;
      }
      for (auto memberPort : *aggPort.memberPorts()) {
        for (auto vlanPort : *config.vlanPorts()) {
          if (*vlanPort.logicalPort() == *memberPort.memberPortID()) {
            return VlanID(*vlanPort.vlanID());
          }
        }
      }
    }
    throw FbossError("VLAN not found for aggPort:", aggPortID);
  }

  void verifyReachability() {
    verifyReachability(
        RoutePrefix<folly::IPAddressV4>({folly::IPAddressV4("0.0.0.0"), 0}));
    verifyReachability(
        RoutePrefix<folly::IPAddressV6>({folly::IPAddressV6("::"), 0}));
  }

  void verifyInitialState() {
    // In a cold boot ports can flap initially. Wait for ports to
    // stabilize state
    if (platform()->getHwSwitch()->getBootType() != BootType::WARM_BOOT) {
      sleep(60);
    }

    // Wait for AggPort
    for (const auto& aggId : getAggPorts()) {
      waitForAggPortStatus(aggId, true);
    }

    // verify lacp state information
    verifyLacpState();
    verifyReachability();
  }

  std::vector<LegacyAggregatePortFields::Subport> getSubPorts(
      AggregatePortID aggId) {
    const auto& aggPort = sw()->getState()->getAggregatePorts()->getNode(aggId);
    EXPECT_NE(aggPort, nullptr);
    return aggPort->sortedSubports();
  }

  ParticipantInfo::Port getRemotePortID(AggregatePortID aggId, PortID portId) {
    const auto& aggPort = sw()->getState()->getAggregatePorts()->getNode(aggId);
    EXPECT_NE(aggPort, nullptr);
    return aggPort->getPartnerState(portId).port;
  }

  void createL3DataplaneFlood() {
    disableTTLDecrementsForRoute<folly::IPAddressV6>({folly::IPAddressV6(), 0});
    disableTTLDecrementsForRoute<folly::IPAddressV4>({folly::IPAddressV4(), 0});
    if (isDUT()) {
      auto state = sw()->getState();
      auto vlan =
          util::getFirstMap(state->getVlans())->cbegin()->second->getID();
      auto srcMac = state->getInterfaces()->getInterfaceInVlan(vlan)->getMac();
      auto destMac =
          getNeighborEntry(
              getNeighborIpAddrs<folly::IPAddressV6>()[0],
              state->getInterfaces()->getInterfaceInVlan(vlan)->getID())
              .first;
      for (const auto& sendV6 : {true, false}) {
        utility::pumpTraffic(
            sendV6, sw()->getHw(), destMac, vlan, std::nullopt, 255, srcMac);
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
    auto ecmpSizeInSw = getAggPorts().size();
    EXPECT_EQ(
        utility::getEcmpSizeInHw(
            sw()->getHw(),
            {folly::IPAddress("::"), 0},
            RouterID(0),
            ecmpSizeInSw),
        ecmpSizeInSw);
    for (const auto& aggId : getAggPorts()) {
      const auto countInSw = sw()->getState()
                                 ->getAggregatePorts()
                                 ->getNode(aggId)
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
      auto remoteLacpTimeout = [this, &aggPortId](
                                   const std::shared_ptr<SwitchState>& state) {
        const auto& subPorts = getSubPorts(aggPortId);
        const auto& localPort = subPorts.front().portID;
        const auto& aggPort = state->getAggregatePorts()->getNode(aggPortId);
        const auto& remoteState = aggPort->getPartnerState(localPort).state;
        const auto flagsToCheck = LacpState::IN_SYNC | LacpState::COLLECTING |
            LacpState::DISTRIBUTING;
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

class MultiNodeRoutingLoop : public MultiNodeLacpTest {
 public:
  void SetUp() override {
    MultiNodeLacpTest::SetUp();
  }

  cfg::SwitchConfig initialConfig() const override {
    auto config = getConfigWithAggPort();

    for (auto& lb : *config.loadBalancers()) {
      if (*lb.id() != cfg::LoadBalancerID::AGGREGATE_PORT) {
        continue;
      }
      // setting up as per prod, DUT is uu and REMOTE is DU
      lb.seed() = isDUT() ? 0x35f25a4a : 0xd277bc20;
    }
    setupRoute(&config, prefix_, 1);
    return config;
  }

  void createL3DataplaneFlood() {
    XLOG(INFO) << "creating data plane flood";
    disableTTLDecrementsForRoute<folly::IPAddressV6>(prefix_);
    auto state = sw()->getState();
    auto vlan = util::getFirstMap(state->getVlans())->cbegin()->second->getID();
    auto srcMac = state->getInterfaces()->getInterfaceInVlan(vlan)->getMac();
    auto destMac =
        getNeighborEntry(
            getNeighborIpAddrs<folly::IPAddressV6>()[0],
            state->getInterfaces()->getInterfaceInVlan(vlan)->getID())
            .first;
    utility::pumpTraffic(
        sw()->getHw(),
        destMac,
        {folly::IPAddress("200::10")},
        {folly::IPAddress("100::10")},
        1024,
        1024,
        1,
        vlan,
        std::nullopt,
        255,
        srcMac);
  }

  const RoutePrefix<folly::IPAddressV6> prefix_{
      folly::IPAddressV6("100::10"),
      126};
};

TEST_F(MultiNodeRoutingLoop, LoopTraffic) {
  auto setup = [=]() {
    verifyInitialState();
    verifyReachability(prefix_);
    if (!isDUT()) {
      createL3DataplaneFlood();
    }
  };
  auto verify = [=]() {
    std::this_thread::sleep_for(3s);
    assertNoInDiscards(0);
  };
  verifyAcrossWarmBoots(setup, verify, setup, verify);
}
