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
};

} // namespace facebook::fboss
