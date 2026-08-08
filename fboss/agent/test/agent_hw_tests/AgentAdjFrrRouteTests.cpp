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

namespace facebook::fboss {

class AgentAdjFrrRouteTest : public AgentHwTest {
 protected:
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
    return config;
  }

  void setCmdLineFlagOverrides() const override {
    AgentHwTest::setCmdLineFlagOverrides();
    FLAGS_flowletSwitchingEnable = true;
  }

  mutable std::vector<PortID> phyLoopbackPortIds_;
};

TEST_F(AgentAdjFrrRouteTest, routeWithPrimaryAndBackupNhops) {
  auto setup = [this]() {
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

    auto makeNextHop = [this, &ecmpHelper](size_t index, NextHopRole role) {
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
        makeNextHop(0, NextHopRole::PRIMARY),
        makeNextHop(1, NextHopRole::BACKUP),
        makeNextHop(2, NextHopRole::BACKUP),
        makeNextHop(3, NextHopRole::BACKUP),
        makeNextHop(4, NextHopRole::BACKUP),
    };
    auto routeUpdater = getSw()->getRouteUpdater();
    routeUpdater.addRoute(
        RouterID(0),
        folly::IPAddressV6("2001::"),
        16,
        ClientID::BGPD,
        RouteNextHopEntry(nextHops, AdminDistance::EBGP));
    routeUpdater.program();
  };

  auto verify = [this]() {
    constexpr int kPacketCount = 10000;
    CHECK_GE(phyLoopbackPortIds_.size(), kNumRequiredPhyLoopbackPorts);
    auto state = getProgrammedState();
    auto primaryPort = phyLoopbackPortIds_.at(0);
    auto injectionPort = phyLoopbackPortIds_.at(kNumRouteNextHops);
    std::vector<PortID> backupPorts(
        phyLoopbackPortIds_.begin() + 1,
        phyLoopbackPortIds_.begin() + kNumRouteNextHops);
    auto primaryPortState = state->getPorts()->getNode(primaryPort);
    auto injectionPortState = state->getPorts()->getNode(injectionPort);
    XLOG(INFO) << "Injecting traffic through port "
               << injectionPortState->getName() << " (" << injectionPort
               << "); checking primary next-hop port "
               << primaryPortState->getName() << " (" << primaryPort << ")";
    auto beforeOutPkts = *getLatestPortStats(primaryPort).outUnicastPkts__ref();

    auto pumpTestTraffic = [this, injectionPort](int packetCount) {
      utility::pumpTraffic(
          true,
          utility::getAllocatePktFn(getAgentEnsemble()),
          utility::getSendPktFunc(getAgentEnsemble()),
          getMacForFirstInterfaceWithPortsForTesting(getProgrammedState()),
          getVlanIDForTx(),
          injectionPort,
          255,
          packetCount);
    };
    pumpTestTraffic(kPacketCount);

    WITH_RETRIES({
      auto afterOutPkts =
          *getLatestPortStats(primaryPort).outUnicastPkts__ref();
      XLOG(INFO) << "Primary out packets before traffic: " << beforeOutPkts
                 << ", after traffic: " << afterOutPkts;
      EXPECT_EVENTUALLY_EQ(afterOutPkts, beforeOutPkts + kPacketCount);
    });

    auto getBackupOutPkts = [this, &backupPorts]() {
      uint64_t outPkts{0};
      for (auto port : backupPorts) {
        outPkts += *getLatestPortStats(port).outUnicastPkts__ref();
      }
      return outPkts;
    };
    bringDownPort(primaryPort);
    auto beforeBackupOutPkts = getBackupOutPkts();
    pumpTestTraffic(kPacketCount);

    WITH_RETRIES({
      auto afterBackupOutPkts = getBackupOutPkts();
      XLOG(INFO) << "Backup out packets before traffic: " << beforeBackupOutPkts
                 << ", after traffic: " << afterBackupOutPkts;
      EXPECT_EVENTUALLY_EQ(
          afterBackupOutPkts, beforeBackupOutPkts + kPacketCount);
    });
    bringUpPort(primaryPort);
  };

  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
