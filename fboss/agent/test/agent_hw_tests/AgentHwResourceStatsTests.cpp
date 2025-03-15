/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/FbossHwUpdateError.h"
#include "fboss/agent/hw/HwResourceStatsPublisher.h"
#include "fboss/agent/state/Route.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/utils/AclTestUtils.h"
#include "fboss/agent/test/utils/AsicUtils.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/QueuePerHostTestUtils.h"

#include <folly/IPAddressV4.h>
#include <folly/IPAddressV6.h>
#include <folly/logging/xlog.h>

#include <tuple>

using namespace facebook::fb303;

namespace facebook::fboss {

class AgentHwResourceStatsTest : public AgentHwTest {
 protected:
  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    return {production_features::ProductionFeature::L3_FORWARDING};
  }

  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto cfg = AgentHwTest::initialConfig(ensemble);
    if (FLAGS_enable_acl_table_group) {
      utility::setupDefaultAclTableGroups(cfg);
    }
    return cfg;
  }

  using Prefix6 = typename Route<folly::IPAddressV6>::Prefix;
  using Prefix4 = typename Route<folly::IPAddressV4>::Prefix;
  Prefix6 kPrefix6() const {
    return Prefix6{folly::IPAddressV6("100::"), 64};
  }
  Prefix4 kPrefix4() const {
    return Prefix4{folly::IPAddressV4("100.100.100.0"), 24};
  }
};

TEST_F(AgentHwResourceStatsTest, l3Stats) {
  auto setup = [] {};
  auto verify = [this] {
    auto newCfg = this->initialConfig(*getAgentEnsemble());
    this->applyNewConfig(newCfg);
    // Trigger a stats collection
    this->getLatestPortStats(this->masterLogicalPortIds());
    auto constexpr kEcmpWidth = 2;
    utility::EcmpSetupAnyNPorts4 ecmp4(this->getProgrammedState());
    utility::EcmpSetupAnyNPorts6 ecmp6(this->getProgrammedState());

    // lamda to get stats from fbData for multi switch
    auto getStatsFn = [this]() {
      auto switchId = getAgentEnsemble()
                          ->getSw()
                          ->getScopeResolver()
                          ->scope(getAgentEnsemble()->masterLogicalPortIds())
                          .switchId();
      multiswitch::HwSwitchStats hwSwitchStats =
          getSw()->getHwSwitchStatsExpensive(switchId);
      HwResourceStats& hwResourceStats = *hwSwitchStats.hwResourceStats();
      EXPECT_EQ(0, hwResourceStats.hw_table_stats_stale().value());
      return std::make_tuple(
          hwResourceStats.lpm_ipv6_free().value(),
          hwResourceStats.lpm_ipv4_free().value(),
          hwResourceStats.l3_ecmp_groups_free().value(),
          hwResourceStats.l3_ipv4_nexthops_free().value(),
          hwResourceStats.l3_ipv6_nexthops_free().value(),
          hwResourceStats.l3_ipv4_host_free().value(),
          hwResourceStats.l3_ipv6_host_free().value());
    };

    auto
        [v6RouteFreeBefore,
         v4RouteFreeBefore,
         ecmpFreeBefore,
         v4NextHopsFreeBefore,
         v6NextHopsFreeBefore,
         v4HostFreeBefore,
         v6HostFreeBefore] = getStatsFn();

    auto wrapper = getSw()->getRouteUpdater();

    ecmp4.programRoutes(&wrapper, kEcmpWidth, {this->kPrefix4()});
    ecmp6.programRoutes(&wrapper, kEcmpWidth, {this->kPrefix6()});
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      auto newState = ecmp4.resolveNextHops(in, kEcmpWidth);
      return newState;
    });
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      auto newState = ecmp6.resolveNextHops(in, kEcmpWidth);
      return newState;
    });
    // Trigger a stats collection
    WITH_RETRIES({
      this->getLatestPortStats(this->masterLogicalPortIds());
      auto
          [v6RouteFreeAfter,
           v4RouteFreeAfter,
           ecmpFreeAfter,
           v4NextHopsFreeAfter,
           v6NextHopsFreeAfter,
           v4HostFreeAfter,
           v6HostFreeAfter] = getStatsFn();

      // Less than since v4, v6 routes, nhops, neighbors compete for the
      // same resources so free counts would not necessarily decrement by 1
      EXPECT_EVENTUALLY_LT(v6RouteFreeAfter, v6RouteFreeBefore);
      EXPECT_EVENTUALLY_LT(v4RouteFreeAfter, v4RouteFreeBefore);
      EXPECT_EVENTUALLY_LT(v6NextHopsFreeAfter, v6NextHopsFreeBefore);
      EXPECT_EVENTUALLY_LT(v4NextHopsFreeAfter, v4NextHopsFreeBefore);

      auto switchId =
          getSw()->getScopeResolver()->scope(masterLogicalPortIds()).switchId();
      auto asic = getSw()->getHwAsicTable()->getHwAsic(switchId);

      if (asic->isSupported(HwAsic::Feature::HOSTTABLE)) {
        EXPECT_EVENTUALLY_LT(v6HostFreeAfter, v6HostFreeBefore);
        EXPECT_EVENTUALLY_LT(v4HostFreeAfter, v4HostFreeBefore);
      }
      EXPECT_EVENTUALLY_EQ(ecmpFreeAfter, ecmpFreeBefore - 2);
    });

    // Unresolve so we can rerun verify for many (warmboot) iterations
    ecmp4.unprogramRoutes(&wrapper, {this->kPrefix4()});
    ecmp6.unprogramRoutes(&wrapper, {this->kPrefix6()});
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      auto newState = ecmp4.unresolveNextHops(in, kEcmpWidth);
      return newState;
    });
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      auto newState = ecmp6.unresolveNextHops(in, kEcmpWidth);
      return newState;
    });
  };
  this->verifyAcrossWarmBoots(setup, verify);
};

TEST_F(AgentHwResourceStatsTest, aclStats) {
  auto setup = [] {};
  auto verify = [this] {
    // Trigger a stats collection
    this->getLatestPortStats(this->masterLogicalPortIds());

    // lamda to get stats from fbData for multi switch
    auto getStatsFn = [this]() {
      auto switchId = getAgentEnsemble()
                          ->getSw()
                          ->getScopeResolver()
                          ->scope(getAgentEnsemble()->masterLogicalPortIds())
                          .switchId();
      multiswitch::HwSwitchStats hwSwitchStats =
          getSw()->getHwSwitchStatsExpensive(switchId);
      HwResourceStats& hwResourceStats = *hwSwitchStats.hwResourceStats();
      EXPECT_EQ(0, hwResourceStats.hw_table_stats_stale().value());
      return std::make_tuple(
          hwResourceStats.acl_entries_free().value(),
          hwResourceStats.acl_counters_free().value());
    };

    // Add Acl Table before querying stats so the stats are retrieved
    auto newCfg = this->initialConfig(*getAgentEnsemble());
    this->applyNewConfig(newCfg);
    // Trigger a stats collection
    this->getLatestPortStats(this->masterLogicalPortIds());

    auto l3Asics = getAgentEnsemble()->getL3Asics();
    auto asic = utility::checkSameAndGetAsic(l3Asics);
    auto [aclEntriesFreeBefore, aclCountersFreeBefore] = getStatsFn();
    // getSw()->isRunModeMultiSwitch() ? getMultiStatsFn() : getMonoStatsFn();
    auto acl = utility::addAcl(&newCfg, "acl0");
    acl->dscp() = 0x10;
    if (asic->isSupported(HwAsic::Feature::ACL_ENTRY_ETHER_TYPE)) {
      acl->etherType() = cfg::EtherType::IPv6;
    }
    utility::addAclStat(&newCfg, "acl0", "stat0");
    this->applyNewConfig(newCfg);
    // Trigger a stats collection
    this->getLatestPortStats(this->masterLogicalPortIds());

    WITH_RETRIES({
      auto [aclEntriesFreeAfter, aclCountersFreeAfter] = getStatsFn();
      EXPECT_EVENTUALLY_EQ(aclEntriesFreeAfter, aclEntriesFreeBefore - 1);
      // More than one h/w resource gets consumed on configuring
      // a counter
      EXPECT_EVENTUALLY_LT(aclCountersFreeAfter, aclCountersFreeBefore);
    });
    this->applyNewConfig(this->initialConfig(*getAgentEnsemble()));
  };
  this->verifyAcrossWarmBoots(setup, verify);
}
} // namespace facebook::fboss
