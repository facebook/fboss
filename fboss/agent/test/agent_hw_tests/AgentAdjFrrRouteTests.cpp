/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/agent/test/utils/LoadBalancerTestUtils.h"

#include <folly/logging/xlog.h>

#include <limits>

namespace facebook::fboss {

class AgentAdjFrrRouteTest : public AgentHwTest {
 protected:
  static constexpr int kMaxLoadBalanceDeviationPct = 25;
  static constexpr size_t kNumRouteNextHops = 5;
  static constexpr size_t kNumRequiredPhyLoopbackPorts = kNumRouteNextHops + 1;

  std::optional<size_t> maxRequiredInterfacePorts() const override {
    return std::nullopt;
  }

  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {
        ProductionFeature::ARS_FLOWLET,
        ProductionFeature::ARS_SPRAY,
        ProductionFeature::ADJACENCY_FRR};
  }

  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto config = utility::onePortPerInterfaceConfig(
        ensemble.getSw(),
        ensemble.masterLogicalPortIds(),
        true /* interfaceHasSubnet */);
    phyLoopbackPortIds_.clear();
    // BRCM switches require PHY loopback for FRR link
    // state detection.
    for (auto& port : *config.ports()) {
      if (*port.speed() == cfg::PortSpeed::EIGHTHUNDREDG) {
        port.loopbackMode() = cfg::PortLoopbackMode::PHY;
        phyLoopbackPortIds_.emplace_back(*port.logicalID());
      }
    }
    utility::addFlowletConfigs(
        config,
        ensemble.masterLogicalPortIds(),
        ensemble.isSai(),
        cfg::SwitchingMode::PER_PACKET_QUALITY);
    config.loadBalancers()->push_back(
        utility::getEcmpFullHashConfig(ensemble.getL3Asics()));
    return config;
  }

  void setCmdLineFlagOverrides() const override {
    AgentHwTest::setCmdLineFlagOverrides();
    FLAGS_flowletSwitchingEnable = true;
  }

  void setupRouteWithPrimaryAndBackupNhops(bool includePrimaryNextHop = true) {
    CHECK_GE(phyLoopbackPortIds_.size(), kNumRequiredPhyLoopbackPorts);
    utility::EcmpSetupTargetedPorts<folly::IPAddressV6> ecmpHelper(
        getProgrammedState(), getSw()->needL2EntryForNeighbor());
    boost::container::flat_set<PortDescriptor> nextHopPorts;
    for (size_t i = 0; i < kNumRouteNextHops; ++i) {
      nextHopPorts.emplace(phyLoopbackPortIds_.at(i));
    }
    applyNewState([&](const std::shared_ptr<SwitchState>& state) {
      return ecmpHelper.resolveNextHops(state, nextHopPorts);
    });

    programRouteWithPrimaryAndBackupNhops(includePrimaryNextHop);
  }

  void programRouteWithPrimaryAndBackupNhops(bool includePrimaryNextHop) {
    utility::EcmpSetupTargetedPorts<folly::IPAddressV6> ecmpHelper(
        getProgrammedState(), getSw()->needL2EntryForNeighbor());
    const auto makeNextHop = [this, &ecmpHelper](
                                 size_t index, NextHopRole role) {
      return UnresolvedNextHop(
          ecmpHelper.ip(PortDescriptor(phyLoopbackPortIds_.at(index))),
          ECMP_WEIGHT,
          std::nullopt,
          std::nullopt,
          std::nullopt,
          std::nullopt,
          {},
          std::nullopt,
          std::nullopt,
          std::nullopt,
          role);
    };

    RouteNextHopSet nextHops{
        makeNextHop(1, NextHopRole::BACKUP),
        makeNextHop(2, NextHopRole::BACKUP),
        makeNextHop(3, NextHopRole::BACKUP),
        makeNextHop(4, NextHopRole::BACKUP),
    };
    if (includePrimaryNextHop) {
      nextHops.emplace(makeNextHop(0, NextHopRole::PRIMARY));
    }
    auto routeUpdater = getSw()->getRouteUpdater();
    routeUpdater.addRoute(
        RouterID(0),
        folly::IPAddressV6("2001::"),
        16,
        ClientID::BGPD,
        RouteNextHopEntry(nextHops, AdminDistance::EBGP));
    routeUpdater.program();
  }

  void restoreNextHop(PortID port) {
    bringUpPort(port);
    utility::EcmpSetupTargetedPorts<folly::IPAddressV6> ecmpHelper(
        getProgrammedState(), getSw()->needL2EntryForNeighbor());
    const boost::container::flat_set<PortDescriptor> nextHopPorts{
        PortDescriptor(port)};
    applyNewState([&](const std::shared_ptr<SwitchState>& state) {
      return ecmpHelper.unresolveNextHops(state, nextHopPorts);
    });
    applyNewState([&](const std::shared_ptr<SwitchState>& state) {
      return ecmpHelper.resolveNextHops(state, nextHopPorts);
    });
  }

  void sendTrafficAndVerifyOutPackets(
      PortID injectionPort,
      const std::vector<PortID>& egressPorts,
      int packetCount,
      const char* egressPortDescription) {
    CHECK(!egressPorts.empty());
    auto getOutPkts = [&egressPorts](const auto& portStats) {
      uint64_t outPkts{0};
      for (auto port : egressPorts) {
        outPkts += *portStats.at(port).outUnicastPkts__ref();
      }
      return outPkts;
    };
    const auto beforePortStats = getLatestPortStats(egressPorts);
    const auto beforeOutPkts = getOutPkts(beforePortStats);

    utility::pumpTraffic(
        true,
        utility::getAllocatePktFn(getAgentEnsemble()),
        utility::getSendPktFunc(getAgentEnsemble()),
        getMacForFirstInterfaceWithPortsForTesting(getProgrammedState()),
        getVlanIDForTx(),
        injectionPort,
        255,
        packetCount);

    WITH_RETRIES({
      const auto afterPortStats = getLatestPortStats(egressPorts);
      const auto afterOutPkts = getOutPkts(afterPortStats);
      const auto [highestOutBytesIncrement, lowestOutBytesIncrement] =
          utility::getHighestAndLowestBytesIncrement(
              beforePortStats, afterPortStats);
      const auto deviationPct = lowestOutBytesIncrement == 0
          ? (highestOutBytesIncrement == 0
                 ? 0.0
                 : std::numeric_limits<double>::infinity())
          : static_cast<double>(
                highestOutBytesIncrement - lowestOutBytesIncrement) /
              lowestOutBytesIncrement * 100.0;
      XLOG(INFO) << egressPortDescription
                 << " out packets before traffic: " << beforeOutPkts
                 << ", after traffic: " << afterOutPkts
                 << ", lowest out bytes increment: " << lowestOutBytesIncrement
                 << ", highest out bytes increment: "
                 << highestOutBytesIncrement << ", deviation: " << deviationPct
                 << "%";
      EXPECT_EVENTUALLY_EQ(afterOutPkts, beforeOutPkts + packetCount);
      EXPECT_EVENTUALLY_TRUE(
          utility::isDeviationWithinThreshold(
              lowestOutBytesIncrement,
              highestOutBytesIncrement,
              kMaxLoadBalanceDeviationPct));
    });
  }

  mutable std::vector<PortID> phyLoopbackPortIds_;
};

TEST_F(AgentAdjFrrRouteTest, routeWithPrimaryAndBackupNhops) {
  auto setup = [this]() { setupRouteWithPrimaryAndBackupNhops(); };

  auto verify = [this]() {
    constexpr int kPacketCount = 10000;
    CHECK_GE(phyLoopbackPortIds_.size(), kNumRequiredPhyLoopbackPorts);
    auto state = getProgrammedState();
    auto primaryPort = phyLoopbackPortIds_.at(0);
    auto injectionPort = phyLoopbackPortIds_.at(kNumRouteNextHops);
    const std::vector<PortID> primaryPorts{primaryPort};
    std::vector<PortID> backupPorts(
        phyLoopbackPortIds_.begin() + 1,
        phyLoopbackPortIds_.begin() + kNumRouteNextHops);
    auto primaryPortState = state->getPorts()->getNode(primaryPort);
    auto injectionPortState = state->getPorts()->getNode(injectionPort);
    XLOG(INFO) << "Injecting traffic through port "
               << injectionPortState->getName() << " (" << injectionPort
               << "); checking primary next-hop port "
               << primaryPortState->getName() << " (" << primaryPort << ")";
    sendTrafficAndVerifyOutPackets(
        injectionPort, primaryPorts, kPacketCount, "Primary");

    bringDownPort(primaryPort);
    sendTrafficAndVerifyOutPackets(
        injectionPort, backupPorts, kPacketCount, "Backup");

    restoreNextHop(primaryPort);

    sendTrafficAndVerifyOutPackets(
        injectionPort,
        primaryPorts,
        kPacketCount,
        "Primary after next-hop re-resolution");
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentAdjFrrRouteTest, sourcePortGetsPruned) {
  auto setup = [this]() { setupRouteWithPrimaryAndBackupNhops(); };

  auto verify = [this]() {
    constexpr int kPacketCount = 10000;
    CHECK_GE(phyLoopbackPortIds_.size(), kNumRequiredPhyLoopbackPorts);
    const auto primaryPort = phyLoopbackPortIds_.at(0);
    const std::vector<PortID> backupPorts(
        phyLoopbackPortIds_.begin() + 1,
        phyLoopbackPortIds_.begin() + kNumRouteNextHops);

    sendTrafficAndVerifyOutPackets(
        primaryPort,
        backupPorts,
        kPacketCount,
        "Backup with primary next-hop ingress");

    bringDownPort(primaryPort);
    const auto backupInjectionPort = backupPorts.front();
    const std::vector<PortID> remainingBackupPorts(
        backupPorts.begin() + 1, backupPorts.end());
    sendTrafficAndVerifyOutPackets(
        backupInjectionPort,
        remainingBackupPorts,
        kPacketCount,
        "Backup with backup next-hop ingress");

    restoreNextHop(primaryPort);
    sendTrafficAndVerifyOutPackets(
        primaryPort,
        backupPorts,
        kPacketCount,
        "Backup after primary next-hop restoration");
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentAdjFrrRouteTest, priAndBackupNextHopFlap) {
  auto setup = [this]() {
    // TODO - start with 0 primaries one vendor lib fixes handling for
    // this
    setupRouteWithPrimaryAndBackupNhops(true /* includePrimaryNextHop */);
  };

  auto verify = [this]() {
    constexpr int kPacketCount = 10000;
    CHECK_GE(phyLoopbackPortIds_.size(), kNumRequiredPhyLoopbackPorts);
    const auto primaryPort = phyLoopbackPortIds_.at(0);
    const auto injectionPort = phyLoopbackPortIds_.at(kNumRouteNextHops);
    const std::vector<PortID> primaryPorts{primaryPort};
    const std::vector<PortID> backupPorts(
        phyLoopbackPortIds_.begin() + 1,
        phyLoopbackPortIds_.begin() + kNumRouteNextHops);
    const auto firstBackupPort = backupPorts.front();
    const std::vector<PortID> remainingBackupPorts(
        backupPorts.begin() + 1, backupPorts.end());

    programRouteWithPrimaryAndBackupNhops(true);
    bringDownPort(primaryPort);
    bringDownPort(firstBackupPort);
    sendTrafficAndVerifyOutPackets(
        injectionPort, remainingBackupPorts, kPacketCount, "Remaining backups");

    restoreNextHop(primaryPort);
    sendTrafficAndVerifyOutPackets(
        injectionPort, primaryPorts, kPacketCount, "Restored primary");

    restoreNextHop(firstBackupPort);
    bringDownPort(primaryPort);
    sendTrafficAndVerifyOutPackets(
        injectionPort, backupPorts, kPacketCount, "All backups");

    restoreNextHop(primaryPort);
  };

  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
