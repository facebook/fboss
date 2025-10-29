/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/EcmpResourceManager.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/state/Route.h"
#include "fboss/agent/state/RouteNextHopEntry.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/BaseEcmpResourceManagerTest.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/agent/types.h"

#include <folly/IPAddress.h>
#include <gtest/gtest.h>

namespace facebook::fboss {

class EcmpResourceManagerRibFibTest : public ::testing::Test {
 public:
  static constexpr auto kNumIntfs = 20;
  void SetUp() override {
    FLAGS_enable_ecmp_resource_manager = true;
    FLAGS_ecmp_resource_percentage = 100;
    FLAGS_ars_resource_percentage = 100;
    FLAGS_flowletSwitchingEnable = true;
    FLAGS_dlbResourceCheckEnable = false;
    auto cfg = onePortPerIntfConfig(kNumIntfs);
    handle_ = createTestHandle(&cfg);
    sw_ = handle_->getSw();
    ASSERT_NE(sw_->getEcmpResourceManager(), nullptr);
    // Taken from mock asic
    EXPECT_EQ(sw_->getEcmpResourceManager()->getMaxPrimaryEcmpGroups(), 5);
    // Backup ecmp group type will com from default flowlet confg
    EXPECT_EQ(
        *sw_->getEcmpResourceManager()->getBackupEcmpSwitchingMode(),
        *cfg.flowletSwitchingConfig()->backupSwitchingMode());

    // Assert rib/fib sync after config application
    assertRibFibEquivalence();
    // Add 5 routes with distinct nhops, should not go
    // over DLB limits
    auto newNhops = defaultNhopSets();
    auto newState = sw_->getState()->clone();
    auto fib6 = fib(newState);
    for (auto i = 0; i < numStartRoutes(); ++i) {
      auto newRoute = makeRoute(makePrefix(i), RouteNextHopSet(newNhops[i]));
      fib6->addNode(newRoute);
    }
    updateRoutes(newState);
  }
  void assertRibFibEquivalence() const {
    waitForStateUpdates(sw_);
    for (const auto& [_, route] : std::as_const(*cfib(sw_->getState()))) {
      auto ribRoute =
          sw_->getRib()->longestMatch(route->prefix().network(), RouterID(0));
      ASSERT_NE(ribRoute, nullptr);
      // TODO - check why are the pointers different even though the
      // forwarding info matches. This is true with or w/o consolidator
      EXPECT_EQ(ribRoute->getForwardInfo(), route->getForwardInfo());
    }
  }
  RouteNextHopSet defaultNhops() const {
    return makeNextHops(kNumIntfs);
  }
  int numStartRoutes() const {
    return 5;
  }
  std::vector<RouteNextHopSet> defaultNhopSets() const {
    std::vector<RouteNextHopSet> defaultNhopGroups;
    auto beginNhops = defaultNhops();
    for (auto i = 0; i < numStartRoutes(); ++i) {
      defaultNhopGroups.push_back(beginNhops);
      beginNhops.erase(beginNhops.begin());
      CHECK_GT(beginNhops.size(), 1);
    }
    return defaultNhopGroups;
  }

  std::vector<RouteNextHopSet> nextNhopSets(int numSets = 5) {
    auto nhopsStart = defaultNhopSets().back();
    std::vector<RouteNextHopSet> nhopsTo;
    for (auto i = 0; i < numSets; ++i) {
      nhopsStart.erase(nhopsStart.begin());
      CHECK_GT(nhopsStart.size(), 1);
      nhopsTo.push_back(nhopsStart);
    }
    return nhopsTo;
  }
  void updateRoutes(const std::shared_ptr<SwitchState>& newState) {
    auto updater = sw_->getRouteUpdater();
    StateDelta delta(sw_->getState(), newState);
    processFibsDeltaInHwSwitchOrder(
        delta,
        [&updater](RouterID rid, const auto& oldRoute, const auto& newRoute) {
          updater.addRoute(
              rid,
              newRoute->prefix().network(),
              newRoute->prefix().mask(),
              kClientID,
              newRoute->getForwardInfo());
        },
        [&updater](RouterID rid, const auto& newRoute) {
          updater.addRoute(
              rid,
              newRoute->prefix().network(),
              newRoute->prefix().mask(),
              kClientID,
              newRoute->getForwardInfo());
        },
        [&updater](RouterID rid, const auto& oldRoute) {
          IpPrefix pfx;
          pfx.ip() = network::toBinaryAddress(oldRoute->prefix().network());
          pfx.prefixLength() = oldRoute->prefix().mask();
          updater.delRoute(rid, pfx, kClientID);
        });

    updater.program();
    assertRibFibEquivalence();
  }
  std::unique_ptr<HwTestHandle> handle_;
  SwSwitch* sw_;
};

TEST_F(EcmpResourceManagerRibFibTest, init) {
  assertRibFibEquivalence();
}

TEST_F(EcmpResourceManagerRibFibTest, addRoutesAboveEcmpLimit) {
  // Add new routes pointing to new nhops. ECMP limit is breached.
  auto nhopSets = nextNhopSets();
  auto newState = sw_->getState()->clone();
  auto fib6 = fib(newState);
  auto routesBefore = fib6->size();
  std::set<RouteNextHopSet> nhops;
  std::set<RouteV6::Prefix> overflowPrefixes;
  for (auto i = 0; i < numStartRoutes(); ++i) {
    auto route = makeRoute(makePrefix(routesBefore + i), nhopSets[i]);
    overflowPrefixes.insert(route->prefix());
    nhops.insert(nhopSets[i]);
    fib6->addNode(route);
  }
  updateRoutes(newState);
}

} // namespace facebook::fboss
