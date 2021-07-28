// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <gtest/gtest.h>
#include "fboss/agent/LldpManager.h"
#include "fboss/agent/PlatformPort.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/hw/gen-cpp2/hardware_stats_types.h"
#include "fboss/agent/hw/test/LoadBalancerUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwTestQosUtils.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/link_tests/LinkTest.h"
#include "fboss/qsfp_service/lib/QsfpCache.h"

using namespace ::testing;
using namespace facebook::fboss;

// Tests that the link comes up after a flap on the ASIC
TEST_F(LinkTest, asicLinkFlap) {
  auto verify = [this]() {
    auto ports = getCabledPorts();
    // Set the port status on all cabled ports to false. The link should go down
    for (const auto& port : ports) {
      setPortStatus(port, false);
    }
    EXPECT_NO_THROW(waitForAllCabledPorts(false));

    // Set the port status on all cabled ports to true. The link should come
    // back up
    for (const auto& port : ports) {
      setPortStatus(port, true);
    }
    EXPECT_NO_THROW(waitForAllCabledPorts(true));
  };

  verifyAcrossWarmBoots([]() {}, verify);
}

TEST_F(LinkTest, getTransceivers) {
  auto allTransceiversPresent = [this]() {
    auto ports = getCabledPorts();
    // Set the port status on all cabled ports to false. The link should go
    // down
    for (const auto& port : ports) {
      auto transceiverId =
          platform()->getPlatformPort(port)->getTransceiverID().value();
      if (!platform()->getQsfpCache()->getIf(transceiverId)) {
        return false;
      }
    }
    return true;
  };

  verifyAcrossWarmBoots(
      []() {},
      [allTransceiversPresent, this]() {
        checkWithRetry(allTransceiversPresent);
      });
}

TEST_F(LinkTest, trafficRxTx) {
  auto packetsFlowingOnAllPorts = [this]() {
    sw()->getLldpMgr()->sendLldpOnAllPorts();
    auto lldpDb = sw()->getLldpMgr()->getDB();
    for (const auto& port : getCabledPorts()) {
      if (!lldpDb->getNeighbors(port).size()) {
        XLOG(INFO) << " No lldp neighbors on : " << port;
        return false;
      }
    }
    return true;
  };

  verifyAcrossWarmBoots(
      []() {},
      [packetsFlowingOnAllPorts, this]() {
        checkWithRetry(packetsFlowingOnAllPorts);
      });
}

TEST_F(LinkTest, warmbootIsHitLess) {
  boost::container::flat_set<PortDescriptor> ecmpPorts;
  auto vlanOwningPorts =
      utility::getPortsWithExclusiveVlanMembership(sw()->getState());
  for (auto port : getCabledPorts()) {
    if (vlanOwningPorts.find(PortDescriptor(port)) != vlanOwningPorts.end()) {
      ecmpPorts.insert(PortDescriptor(port));
    }
  }
  ASSERT_GT(ecmpPorts.size(), 0);
  auto setupTraffic = [this, &ecmpPorts]() {
    sw()->updateStateBlocking("Resolve nhops", [this, ecmpPorts](auto state) {
      utility::EcmpSetupTargetedPorts6 ecmp6(
          state, sw()->getPlatform()->getLocalMac());
      return ecmp6.resolveNextHops(state, ecmpPorts);
    });
    utility::EcmpSetupTargetedPorts6 ecmp6(
        sw()->getState(), sw()->getPlatform()->getLocalMac());
    ecmp6.programRoutes(
        std::make_unique<SwSwitchRouteUpdateWrapper>(sw()->getRouteUpdater()),
        ecmpPorts);
    for (const auto& nextHop : ecmp6.getNextHops()) {
      if (ecmpPorts.find(nextHop.portDesc) != ecmpPorts.end()) {
        utility::disableTTLDecrements(
            sw()->getHw(), ecmp6.getRouterId(), nextHop);
      }
    }
    utility::pumpTraffic(
        true,
        sw()->getHw(),
        sw()->getPlatform()->getLocalMac(),
        (*sw()->getState()->getVlans()->begin())->getID());
    // TODO: Assert that traffic reached a certain rate
  };

  auto verifyInDiscards = [this]() {
    for (auto i = 0; i < 2; ++i) {
      auto portStats = sw()->getHw()->getPortStats();
      for (auto [port, stats] : portStats) {
        auto inDiscards = *stats.inDiscards__ref();
        XLOG(INFO) << "Port: " << port << " in discards: " << inDiscards;
        EXPECT_EQ(inDiscards, 0);
      }
      // Allow for a few rounds of stat collection
      sleep(1);
    }
  };
  verifyAcrossWarmBoots(setupTraffic, verifyInDiscards);
}
