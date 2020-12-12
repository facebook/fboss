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
  RoutePrefixV4 kV4Prefix1() {
    return RoutePrefixV4{folly::IPAddressV4("1.0.0.0"), 24};
  }
  RoutePrefixV6 kV6Prefix1() {
    return RoutePrefixV6{folly::IPAddressV6("1::"), 64};
  }
  RoutePrefixV4 kV4Prefix2() {
    return RoutePrefixV4{folly::IPAddressV4("2.0.0.0"), 24};
  }
  RoutePrefixV6 kV6Prefix2() {
    return RoutePrefixV6{folly::IPAddressV6("2::"), 64};
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
    // ECMP routes
    applyNewState(ecmpRoutesState(getProgrammedState()));
  }

  void setupNonEcmpRoutes() {
    // Non ECMP routes
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
      for (auto i = 0; i < numIters; ++i) {
        // Cache rollback states
        auto noRouteState = getProgrammedState();
        auto ecmpRoutesOnlyState = ecmpRoutesState(getProgrammedState());
        auto nonEcmpRoutesOnlyState = nonEcmpRoutesState(getProgrammedState());
        // Program Routes
        setupEcmpRoutes();
        setupNonEcmpRoutes();
        // Rollback
        // TODO verify routess post rollback
        if (rollbackEcmp && rollbackNonEcmp) {
          rollback(noRouteState);
        } else if (rollbackEcmp) {
          rollback(nonEcmpRoutesOnlyState);
        } else if (rollbackNonEcmp) {
          rollback(ecmpRoutesOnlyState);
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
