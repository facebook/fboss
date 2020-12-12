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
 protected:
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
  void setupRoutes() {
    applyNewState(
        v4EcmpHelper_->resolveNextHops(getProgrammedState(), kEcmpWidth));
    applyNewState(
        v6EcmpHelper_->resolveNextHops(getProgrammedState(), kEcmpWidth));
    // ECMP routes
    applyNewState(v4EcmpHelper_->setupECMPForwarding(
        getProgrammedState(), kEcmpWidth, {kV4Prefix1()}));
    applyNewState(v6EcmpHelper_->setupECMPForwarding(
        getProgrammedState(), kEcmpWidth, {kV6Prefix1()}));
    // Non ECMP routes
    applyNewState(v4EcmpHelper_->setupECMPForwarding(
        getProgrammedState(), 1, {kV4Prefix2()}));
    applyNewState(v6EcmpHelper_->setupECMPForwarding(
        getProgrammedState(), 1, {kV6Prefix2()}));
  }
  void SetUp() override {
    SaiRollbackTest::SetUp();
    v4EcmpHelper_ = std::make_unique<utility::EcmpSetupAnyNPorts4>(
        getProgrammedState(), RouterID(0));
    v6EcmpHelper_ = std::make_unique<utility::EcmpSetupAnyNPorts6>(
        getProgrammedState(), RouterID(0));
  }
  std::unique_ptr<utility::EcmpSetupAnyNPorts4> v4EcmpHelper_;
  std::unique_ptr<utility::EcmpSetupAnyNPorts6> v6EcmpHelper_;
};

TEST_F(SaiRouteRollbackTest, rollbackAll) {
  auto verify = [this]() {
    auto noRouteState = getProgrammedState();
    setupRoutes();
    rollback(noRouteState);
  };
  verifyAcrossWarmBoots([]() {}, verify);
}

} // namespace facebook::fboss
