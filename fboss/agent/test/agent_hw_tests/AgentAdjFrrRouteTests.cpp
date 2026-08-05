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
#include "fboss/agent/test/utils/LoadBalancerTestUtils.h"

namespace facebook::fboss {

class AgentAdjFrrRouteTest : public AgentHwTest {
 protected:
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
};

TEST_F(AgentAdjFrrRouteTest, routeWithPrimaryAndBackupNhops) {
  auto setup = [this]() {
    utility::EcmpSetupAnyNPorts<folly::IPAddressV6> ecmpHelper(
        getProgrammedState(), getSw()->needL2EntryForNeighbor());
    applyNewState([&](const std::shared_ptr<SwitchState>& state) {
      return ecmpHelper.resolveNextHops(state, 5);
    });

    auto makeNextHop = [&ecmpHelper](int index, NextHopRole role) {
      return UnresolvedNextHop(
          ecmpHelper.ip(index),
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
        folly::IPAddressV6("2401:db00::"),
        64,
        ClientID::BGPD,
        RouteNextHopEntry(nextHops, AdminDistance::EBGP));
    routeUpdater.program();
  };

  verifyAcrossWarmBoots(setup, []() {});
}

} // namespace facebook::fboss
