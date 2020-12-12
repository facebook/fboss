/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/hw_test/SaiRollbackTest.h"

#include "fboss/agent/hw/test/HwTestEcmpUtils.h"
#include "fboss/agent/hw/test/HwTestRouteUtils.h"
#include "fboss/agent/state/RouteTypes.h"
#include "fboss/agent/test/EcmpSetupHelper.h"

#include <folly/IPAddress.h>
#include <folly/IPAddressV4.h>
#include <folly/IPAddressV6.h>

namespace {
constexpr auto kEcmpWidth = 2;
}

namespace facebook::fboss {
class SaiRouteRollbackTest : public SaiRollbackTest {
 private:
  RoutePrefixV4 kV4Prefix1() const {
    return RoutePrefixV4{folly::IPAddressV4("100.0.0.0"), 24};
  }
  RoutePrefixV6 kV6Prefix1() const {
    return RoutePrefixV6{folly::IPAddressV6("100::"), 64};
  }
  RoutePrefixV4 kV4Prefix2() const {
    return RoutePrefixV4{folly::IPAddressV4("200.0.0.0"), 24};
  }
  RoutePrefixV6 kV6Prefix2() const {
    return RoutePrefixV6{folly::IPAddressV6("200::"), 64};
  }
  template <typename T>
  folly::CIDRNetwork routePrefixToNetwork(const RoutePrefix<T>& route) const {
    return folly::CIDRNetwork(folly::IPAddress(route.network), route.mask);
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
  bool isEcmp(const folly::CIDRNetwork& nw) const {
    auto ecmpNws = ecmpNetworks();
    return std::find(ecmpNws.begin(), ecmpNws.end(), nw) != ecmpNws.end();
  }

  void resolveNextHops(int width) {
    applyNewState(v4EcmpHelper_->resolveNextHops(getProgrammedState(), width));
    applyNewState(v6EcmpHelper_->resolveNextHops(getProgrammedState(), width));
  }
  std::shared_ptr<SwitchState> ecmpRoutesState(
      const std::shared_ptr<SwitchState>& in) {
    auto newState =
        v4EcmpHelper_->setupECMPForwarding(in, kEcmpWidth, {kV4Prefix1()});
    return v6EcmpHelper_->setupECMPForwarding(
        newState, kEcmpWidth, {kV6Prefix1()});
  }
  std::shared_ptr<SwitchState> nonEcmpRoutesState(
      const std::shared_ptr<SwitchState>& in) {
    auto newState = v4EcmpHelper_->setupECMPForwarding(in, 1, {kV4Prefix2()});
    return v6EcmpHelper_->setupECMPForwarding(newState, 1, {kV6Prefix2()});
  }

  void setupEcmpRoutes() {
    applyNewState(ecmpRoutesState(getProgrammedState()));
  }

  void setupNonEcmpRoutes() {
    applyNewState(nonEcmpRoutesState(getProgrammedState()));
  }

  void SetUp() override {
    SaiRollbackTest::SetUp();
    v4EcmpHelper_ = std::make_unique<utility::EcmpSetupAnyNPorts4>(
        getProgrammedState(), RouterID(0));
    v6EcmpHelper_ = std::make_unique<utility::EcmpSetupAnyNPorts6>(
        getProgrammedState(), RouterID(0));
  }

 protected:
  void runTest(bool rollbackEcmp, bool rollbackNonEcmp, int numIters) {
    auto verify = [=]() {
      resolveNextHops(kEcmpWidth);
      const RouterID kRouterID(0);
      for (auto i = 0; i < numIters; ++i) {
        // Cache rollback states
        auto noRouteState = getProgrammedState();
        auto ecmpRoutesOnlyState = ecmpRoutesState(getProgrammedState());
        auto nonEcmpRoutesOnlyState = nonEcmpRoutesState(getProgrammedState());
        std::vector<folly::CIDRNetwork> routesPresent, routesRemoved;
        // Program Routes
        setupEcmpRoutes();
        setupNonEcmpRoutes();
        // Rollback
        // TODO verify routess post rollback
        if (rollbackEcmp && rollbackNonEcmp) {
          rollback(noRouteState);
          routesRemoved = allNetworks();
        } else if (rollbackEcmp) {
          rollback(nonEcmpRoutesOnlyState);
          routesRemoved = ecmpNetworks();
          routesPresent = nonEcmpNetworks();
        } else if (rollbackNonEcmp) {
          rollback(ecmpRoutesOnlyState);
          routesRemoved = nonEcmpNetworks();
          routesPresent = ecmpNetworks();
        }
        // Verify routes in HW
        for (auto& route : routesRemoved) {
          EXPECT_THROW(
              utility::getEcmpMembersInHw(
                  getHwSwitch(), route, kRouterID, kEcmpWidth),
              FbossError);
        }
        for (auto& route : routesPresent) {
          if (isEcmp(route)) {
            EXPECT_EQ(
                utility::getEcmpMembersInHw(
                    getHwSwitch(), route, kRouterID, kEcmpWidth)
                    .size(),
                kEcmpWidth);
          } else {
            EXPECT_FALSE(
                utility::isHwRouteMultiPath(getHwSwitch(), kRouterID, route));
          }
        }
      }
    };
    verifyAcrossWarmBoots([]() {}, verify);
  }

 private:
  std::unique_ptr<utility::EcmpSetupAnyNPorts4> v4EcmpHelper_;
  std::unique_ptr<utility::EcmpSetupAnyNPorts6> v6EcmpHelper_;
};

TEST_F(SaiRouteRollbackTest, rollbackAll) {
  runTest(true, true, 1);
}

TEST_F(SaiRouteRollbackTest, rollbackNonEcmp) {
  runTest(false, true, 1);
}

TEST_F(SaiRouteRollbackTest, rollbackEcmp) {
  runTest(true, false, 1);
}

TEST_F(SaiRouteRollbackTest, rollbackManyTimes) {
  runTest(true, true, 10);
}
} // namespace facebook::fboss
