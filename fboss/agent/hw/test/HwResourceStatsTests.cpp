/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/HwResourceStatsPublisher.h"
#include "fboss/agent/hw/gen-cpp2/hardware_stats_types.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"
#include "fboss/agent/hw/test/HwTestAclUtils.h"
#include "fboss/agent/state/Route.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/utils/QueuePerHostTestUtils.h"

#include <fb303/ServiceData.h>

#include <folly/IPAddressV4.h>
#include <folly/IPAddressV6.h>
#include <folly/logging/xlog.h>

#include <tuple>

using namespace facebook::fb303;

namespace facebook::fboss {

class HwResourceStatsTest : public HwLinkStateDependentTest {
 protected:
  void SetUp() override {
    HwTest::SetUp();
  }

  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::onePortPerInterfaceConfig(
        getHwSwitch(), masterLogicalPortIds());
    if (FLAGS_enable_acl_table_group) {
      utility::addAclTableGroup(
          &cfg, cfg::AclStage::INGRESS, utility::getAclTableGroupName());
      utility::addDefaultAclTable(cfg);
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

TEST_F(HwResourceStatsTest, l3Stats) {
  if (!this->isSupported(HwAsic::Feature::RESOURCE_USAGE_STATS)) {
#if defined(GTEST_SKIP)
    GTEST_SKIP();
#endif
    return;
  }
  auto setup = [] {};
  auto verify = [this] {
    auto newCfg = this->initialConfig();
    this->applyNewConfig(newCfg);
    // Trigger a stats collection
    this->getHwSwitchEnsemble()->getLatestPortStats(
        this->masterLogicalPortIds());
    auto constexpr kEcmpWidth = 2;
    utility::EcmpSetupAnyNPorts4 ecmp4(this->getProgrammedState());
    utility::EcmpSetupAnyNPorts6 ecmp6(this->getProgrammedState());
    auto getStatsFn = [] {
      EXPECT_EQ(0, fbData->getCounter(kHwTableStatsStale));
      return std::make_tuple(
          fbData->getCounter(kLpmIpv6Free),
          fbData->getCounter(kLpmIpv4Free),
          fbData->getCounter(kL3EcmpGroupsFree),
          fbData->getCounter(kL3Ipv4NextHopsFree),
          fbData->getCounter(kL3Ipv6NextHopsFree),
          fbData->getCounter(kL3Ipv4HostFree),
          fbData->getCounter(kL3Ipv6HostFree));
    };
    auto
        [v6RouteFreeBefore,
         v4RouteFreeBefore,
         ecmpFreeBefore,
         v4NextHopsFreeBefore,
         v6NextHopsFreeBefore,
         v4HostFreeBefore,
         v6HostFreeBefore] = getStatsFn();
    ecmp4.programRoutes(
        this->getRouteUpdater(), kEcmpWidth, {this->kPrefix4()});
    ecmp6.programRoutes(
        this->getRouteUpdater(), kEcmpWidth, {this->kPrefix6()});
    this->applyNewState(
        ecmp4.resolveNextHops(this->getProgrammedState(), kEcmpWidth));
    this->applyNewState(
        ecmp6.resolveNextHops(this->getProgrammedState(), kEcmpWidth));
    // Trigger a stats collection
    this->getHwSwitchEnsemble()->getLatestPortStats(
        this->masterLogicalPortIds());
    auto
        [v6RouteFreeAfter,
         v4RouteFreeAfter,
         ecmpFreeAfter,
         v4NextHopsFreeAfter,
         v6NextHopsFreeAfter,
         v4HostFreeAfter,
         v6HostFreeAfter] = getStatsFn();
    // Less than since v4, v6 routes, nhops, neighbors compete for the same
    // resources so free counts would not necessarily decrement by 1
    EXPECT_LT(v6RouteFreeAfter, v6RouteFreeBefore);
    EXPECT_LT(v4RouteFreeAfter, v4RouteFreeBefore);
    EXPECT_LT(v6NextHopsFreeAfter, v6NextHopsFreeBefore);
    EXPECT_LT(v4NextHopsFreeAfter, v4NextHopsFreeBefore);
    if (this->getHwSwitch()->getPlatform()->getAsic()->isSupported(
            HwAsic::Feature::HOSTTABLE)) {
      EXPECT_LT(v6HostFreeAfter, v6HostFreeBefore);
      EXPECT_LT(v4HostFreeAfter, v4HostFreeBefore);
    }
    EXPECT_EQ(ecmpFreeAfter, ecmpFreeBefore - 2);
    // Unresolve so we can rerun verify for many (warmboot) iterations
    ecmp4.unprogramRoutes(this->getRouteUpdater(), {this->kPrefix4()});
    ecmp6.unprogramRoutes(this->getRouteUpdater(), {this->kPrefix6()});
    this->applyNewState(
        ecmp4.unresolveNextHops(this->getProgrammedState(), kEcmpWidth));
    this->applyNewState(
        ecmp6.unresolveNextHops(this->getProgrammedState(), kEcmpWidth));
  };
  this->verifyAcrossWarmBoots(setup, verify);
};

TEST_F(HwResourceStatsTest, aclStats) {
  if (!this->isSupported(HwAsic::Feature::RESOURCE_USAGE_STATS)) {
#if defined(GTEST_SKIP)
    GTEST_SKIP();
#endif
    return;
  }
  auto setup = [] {};
  auto verify = [this] {
    // Trigger a stats collection
    this->getHwSwitchEnsemble()->getLatestPortStats(
        this->masterLogicalPortIds());
    auto getStatsFn = [] {
      EXPECT_EQ(0, fbData->getCounter(kHwTableStatsStale));
      return std::make_pair(
          fbData->getCounter(kAclEntriesFree),
          fbData->getCounter(kAclCountersFree));
    };
    // Add Acl Table before querying stats so the stats are retrieved
    auto newCfg = this->initialConfig();
    this->applyNewConfig(newCfg);
    // Trigger a stats collection
    this->getHwSwitchEnsemble()->getLatestPortStats(
        this->masterLogicalPortIds());
    auto [aclEntriesFreeBefore, aclCountersFreeBefore] = getStatsFn();
    auto acl = utility::addAcl(&newCfg, "acl0");
    acl->dscp() = 0x10;
    utility::addAclStat(&newCfg, "acl0", "stat0");
    this->applyNewConfig(newCfg);
    // Trigger a stats collection
    this->getHwSwitchEnsemble()->getLatestPortStats(
        this->masterLogicalPortIds());
    auto [aclEntriesFreeAfter, aclCountersFreeAfter] = getStatsFn();
    EXPECT_EQ(aclEntriesFreeAfter, aclEntriesFreeBefore - 1);
    // More than one h/w resource gets consumed on configuring
    // a counter
    EXPECT_LT(aclCountersFreeAfter, aclCountersFreeBefore);
    this->applyNewConfig(this->initialConfig());
  };
  this->verifyAcrossWarmBoots(setup, verify);
}
} // namespace facebook::fboss
