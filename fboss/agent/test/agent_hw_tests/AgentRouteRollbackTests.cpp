/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/test/agent_hw_tests/AgentRollbackTests.h"

#include "fboss/agent/FbossHwUpdateError.h"
#include "fboss/agent/SwSwitchRouteUpdateWrapper.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/RouteTestUtils.h"

#include <folly/Conv.h>
#include <folly/IPAddress.h>
#include <folly/IPAddressV4.h>
#include <folly/IPAddressV6.h>

namespace {
constexpr auto kEcmpWidth = 2;
} // namespace

namespace facebook::fboss {

class AgentRouteRollbackTest : public AgentRollbackTest {
 protected:
  std::unique_ptr<utility::EcmpSetupAnyNPorts4> v4EcmpHelper_;
  std::unique_ptr<utility::EcmpSetupAnyNPorts6> v6EcmpHelper_;

  void setup() {
    v4EcmpHelper_ = std::make_unique<utility::EcmpSetupAnyNPorts4>(
        getProgrammedState(), RouterID(0));
    v6EcmpHelper_ = std::make_unique<utility::EcmpSetupAnyNPorts6>(
        getProgrammedState(), RouterID(0));
  }
  // Route prefixes used in tests
  RoutePrefixV4 kV4Prefix1() const {
    return RoutePrefixV4{folly::IPAddressV4("100.1.1.0"), 24};
  }

  RoutePrefixV6 kV6Prefix1() const {
    return RoutePrefixV6{folly::IPAddressV6("100:1:1::"), 64};
  }

  RoutePrefixV4 kV4Prefix2() const {
    return RoutePrefixV4{folly::IPAddressV4("200.1.1.0"), 24};
  }

  RoutePrefixV6 kV6Prefix2() const {
    return RoutePrefixV6{folly::IPAddressV6("200:1:1::"), 64};
  }

  // Network helpers
  template <typename T>
  folly::CIDRNetwork routePrefixToNetwork(const RoutePrefix<T>& route) const {
    return folly::CIDRNetwork(folly::IPAddress(route.network()), route.mask());
  }

  folly::CIDRNetwork kV4Network1() const {
    return routePrefixToNetwork(kV4Prefix1());
  }

  folly::CIDRNetwork kV6Network1() const {
    return routePrefixToNetwork(kV6Prefix1());
  }

  folly::CIDRNetwork kV4Network2() const {
    return routePrefixToNetwork(kV4Prefix2());
  }

  folly::CIDRNetwork kV6Network2() const {
    return routePrefixToNetwork(kV6Prefix2());
  }

  std::vector<folly::CIDRNetwork> ecmpNetworks() const {
    return {kV4Network1(), kV6Network1()};
  }

  std::vector<folly::CIDRNetwork> nonEcmpNetworks() const {
    return {kV4Network2(), kV6Network2()};
  }

  std::vector<folly::CIDRNetwork> allNetworks() const {
    return {kV4Network1(), kV6Network1(), kV4Network2(), kV6Network2()};
  }

  // Neighbor resolution
  void resolveNextHops(int width) {
    applyNewState([this, width](const std::shared_ptr<SwitchState>& in) {
      return v4EcmpHelper_->resolveNextHops(in, width);
    });
    applyNewState([this, width](const std::shared_ptr<SwitchState>& in) {
      return v6EcmpHelper_->resolveNextHops(in, width);
    });
  }

  // Route programming helpers
  void programEcmpRoutes() {
    auto updater = getSw()->getRouteUpdater();
    v4EcmpHelper_->programRoutes(&updater, kEcmpWidth, {kV4Prefix1()});
    v6EcmpHelper_->programRoutes(&updater, kEcmpWidth, {kV6Prefix1()});
  }

  void programNonEcmpRoutes() {
    auto updater = getSw()->getRouteUpdater();
    v4EcmpHelper_->programRoutes(&updater, 1, {kV4Prefix2()});
    v6EcmpHelper_->programRoutes(&updater, 1, {kV6Prefix2()});
  }

  void unprogramEcmpRoutes() {
    auto updater = getSw()->getRouteUpdater();
    v4EcmpHelper_->unprogramRoutes(&updater, {kV4Prefix1()});
    v6EcmpHelper_->unprogramRoutes(&updater, {kV6Prefix1()});
  }

  void unprogramNonEcmpRoutes() {
    auto updater = getSw()->getRouteUpdater();
    v4EcmpHelper_->unprogramRoutes(&updater, {kV4Prefix2()});
    v6EcmpHelper_->unprogramRoutes(&updater, {kV6Prefix2()});
  }

  // Route verification helpers
  bool routeExists(const folly::CIDRNetwork& network) const {
    auto state = getProgrammedState();
    auto rib = state->getFibsInfoMap()->getFibContainer(RouterID(0));
    if (network.first.isV4()) {
      auto fib = rib->getFibV4();
      auto prefix = RoutePrefixV4{network.first.asV4(), network.second};
      return fib->exactMatch(prefix) != nullptr;
    } else {
      auto fib = rib->getFibV6();
      auto prefix = RoutePrefixV6{network.first.asV6(), network.second};
      return fib->exactMatch(prefix) != nullptr;
    }
  }

  void verifyRoutesExist(const std::vector<folly::CIDRNetwork>& networks) {
    for (const auto& network : networks) {
      EXPECT_TRUE(routeExists(network))
          << "Expected route to exist: " << network.first.str() << "/"
          << network.second;
    }
  }

  void verifyRoutesNotExist(const std::vector<folly::CIDRNetwork>& networks) {
    for (const auto& network : networks) {
      EXPECT_FALSE(routeExists(network))
          << "Expected route to NOT exist: " << network.first.str() << "/"
          << network.second;
    }
  }

  void verifyFibState(const std::shared_ptr<SwitchState>& stateBeforeRollback) {
    auto ribBefore =
        stateBeforeRollback->getFibsInfoMap()->getFibContainer(RouterID(0));
    auto ribAfter =
        getProgrammedState()->getFibsInfoMap()->getFibContainer(RouterID(0));
    EXPECT_EQ(ribBefore->getFibV4()->size(), ribAfter->getFibV4()->size());
    EXPECT_EQ(ribBefore->getFibV6()->size(), ribAfter->getFibV6()->size());
  }

  std::optional<StateDeltaApplication> getDeltaApplicationForRollback() {
    StateDeltaApplication deltaApplication;
    deltaApplication.mode() = DeltaApplicationMode::ROLLBACK;
    return deltaApplication;
  }
};

TEST_F(AgentRouteRollbackTest, VerifyRollback) {
  this->setup();
  auto setup = [this]() {
    resolveNextHops(kEcmpWidth);
    // Program ECMP routes
    programEcmpRoutes();
  };

  auto verify = [this]() {
    // Verify routes exist after setup
    verifyRoutesExist(ecmpNetworks());

    // verify rollback after add
    {
      auto stateBeforeRollback = getProgrammedState();

      EXPECT_THROW(
          {
            auto updater = std::make_unique<SwSwitchRouteUpdateWrapper>(
                getSw(), getSw()->getRib(), getDeltaApplicationForRollback());
            v6EcmpHelper_->programRoutes(updater.get(), 1, {kV6Prefix2()});
          },
          FbossHwUpdateError);
      // Verify state was rolled back - original ECMP routes should still exist
      verifyRoutesExist(ecmpNetworks());

      // Verify the non-ECMP routes were NOT added (rolled back)
      verifyRoutesNotExist(nonEcmpNetworks());
      verifyFibState(stateBeforeRollback);
    }
    // verify rollback after remove
    {
      programNonEcmpRoutes();
      auto stateBeforeRollback = getProgrammedState();

      EXPECT_THROW(
          {
            auto updater = std::make_unique<SwSwitchRouteUpdateWrapper>(
                getSw(), getSw()->getRib(), getDeltaApplicationForRollback());
            v6EcmpHelper_->unprogramRoutes(updater.get(), {kV6Prefix2()});
          },
          FbossHwUpdateError);

      // Verify all routes still exist after rollback
      verifyRoutesExist(allNetworks());
      verifyFibState(stateBeforeRollback);
      unprogramNonEcmpRoutes();
    }
    // verify rollback after update
    {
      auto stateBeforeRollback = getProgrammedState();

      EXPECT_THROW(
          {
            auto updater = std::make_unique<SwSwitchRouteUpdateWrapper>(
                getSw(), getSw()->getRib(), getDeltaApplicationForRollback());
            // Reprogram with different ECMP width (use kEcmpWidth + 1)
            v6EcmpHelper_->programRoutes(
                updater.get(), kEcmpWidth + 1, {kV6Prefix1()});
          },
          FbossHwUpdateError);

      verifyRoutesExist(ecmpNetworks());
      verifyFibState(stateBeforeRollback);
    }
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentRouteRollbackTest, VerifyRollbackWithCombinedRouteOperations) {
  this->setup();
  constexpr int kNumRoutes = 100;

  // Helper to generate V6 prefixes
  auto generateV6Prefixes = [](int baseFirstSegment, int count) {
    std::vector<RoutePrefixV6> prefixes;
    prefixes.reserve(count);
    for (int i = 1; i <= count; i++) {
      // Generate prefix like "100:1:1::", "100:1:2::", etc.
      std::string prefixStr =
          folly::to<std::string>(baseFirstSegment, ":1:", i, "::");
      prefixes.push_back(RoutePrefixV6{folly::IPAddressV6(prefixStr), 64});
    }
    return prefixes;
  };

  // Initial 300 routes (3 groups of 100)
  // Group 1: Routes that will be updated during rollback
  auto updatePrefixes = generateV6Prefixes(100, kNumRoutes);
  // Group 2: Routes that will be removed during rollback
  auto removePrefixes = generateV6Prefixes(200, kNumRoutes);
  // Group 3: Routes that remain unchanged
  auto unchangedPrefixes = generateV6Prefixes(300, kNumRoutes);

  // New routes to add during rollback (not in initial 300)
  auto newPrefixes = generateV6Prefixes(400, kNumRoutes);

  // Helper to convert prefixes to networks for verification
  auto prefixesToNetworks = [this](const std::vector<RoutePrefixV6>& prefixes) {
    std::vector<folly::CIDRNetwork> networks;
    networks.reserve(prefixes.size());
    for (const auto& prefix : prefixes) {
      networks.push_back(routePrefixToNetwork(prefix));
    }
    return networks;
  };

  auto setup = [&]() {
    resolveNextHops(kEcmpWidth + 1); // Resolve enough for updated width

    // Program initial 300 routes with kEcmpWidth
    auto updater = getSw()->getRouteUpdater();
    v6EcmpHelper_->programRoutes(&updater, kEcmpWidth, updatePrefixes);
    v6EcmpHelper_->programRoutes(&updater, kEcmpWidth, removePrefixes);
    v6EcmpHelper_->programRoutes(&updater, kEcmpWidth, unchangedPrefixes);
  };

  auto verify = [&]() {
    // Verify all 300 initial routes exist
    verifyRoutesExist(prefixesToNetworks(updatePrefixes));
    verifyRoutesExist(prefixesToNetworks(removePrefixes));
    verifyRoutesExist(prefixesToNetworks(unchangedPrefixes));

    // New routes should not exist yet
    verifyRoutesNotExist(prefixesToNetworks(newPrefixes));

    auto stateBeforeRollback = getProgrammedState();

    // Combined rollback operation - all in single update
    EXPECT_THROW(
        {
          auto updater = std::make_unique<SwSwitchRouteUpdateWrapper>(
              getSw(), getSw()->getRib(), getDeltaApplicationForRollback());

          // Get next hops for building route entries
          const auto& nhops = v6EcmpHelper_->getNextHops();

          // Build next hop set for updates (kEcmpWidth + 1)
          RouteNextHopSet nhopsForUpdate;
          for (int i = 0; i < kEcmpWidth + 1; ++i) {
            nhopsForUpdate.emplace(UnresolvedNextHop(nhops[i].ip, ECMP_WEIGHT));
          }

          // Build next hop set for adds (kEcmpWidth)
          RouteNextHopSet nhopsForAdd;
          for (int i = 0; i < kEcmpWidth; ++i) {
            nhopsForAdd.emplace(UnresolvedNextHop(nhops[i].ip, ECMP_WEIGHT));
          }

          // 1. Update 100 routes to new width (kEcmpWidth + 1)
          for (const auto& prefix : updatePrefixes) {
            updater->addRoute(
                RouterID(0),
                folly::IPAddress(prefix.network()),
                prefix.mask(),
                ClientID::BGPD,
                RouteNextHopEntry(nhopsForUpdate, AdminDistance::EBGP));
          }

          // 2. Remove 100 routes
          for (const auto& prefix : removePrefixes) {
            updater->delRoute(
                RouterID(0),
                folly::IPAddress(prefix.network()),
                prefix.mask(),
                ClientID::BGPD);
          }

          // 3. Add 100 new routes
          for (const auto& prefix : newPrefixes) {
            updater->addRoute(
                RouterID(0),
                folly::IPAddress(prefix.network()),
                prefix.mask(),
                ClientID::BGPD,
                RouteNextHopEntry(nhopsForAdd, AdminDistance::EBGP));
          }

          // Single program() call for all operations
          updater->program();
        },
        FbossHwUpdateError);

    // Verify rollback: all original 300 routes still exist unchanged
    verifyRoutesExist(prefixesToNetworks(updatePrefixes));
    verifyRoutesExist(prefixesToNetworks(removePrefixes));
    verifyRoutesExist(prefixesToNetworks(unchangedPrefixes));

    // New routes should NOT have been added (rolled back)
    verifyRoutesNotExist(prefixesToNetworks(newPrefixes));

    // Verify FIB state matches pre-rollback
    verifyFibState(stateBeforeRollback);
  };

  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
