/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <folly/IPAddress.h>
#include "fboss/agent/test/EcmpSetupHelper.h"

#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"
#include "fboss/agent/hw/test/HwTestEcmpUtils.h"

namespace {
folly::CIDRNetwork kDefaultRoutePrefix{folly::IPAddress("::"), 0};
}
namespace facebook::fboss {

class HwEcmpTest : public HwLinkStateDependentTest {
 public:
 protected:
  const RouterID kRid{0};
  static constexpr auto kNumNextHops{8};
  std::unique_ptr<utility::EcmpSetupAnyNPorts<folly::IPAddressV6>> ecmpHelper_;
  void SetUp() override {
    HwLinkStateDependentTest::SetUp();
    ecmpHelper_ = std::make_unique<utility::EcmpSetupAnyNPorts6>(
        getProgrammedState(), kRid);
  }
  cfg::SwitchConfig initialConfig() const override {
    return utility::onePortPerVlanConfig(
        getHwSwitch(), masterLogicalPortIds(), cfg::PortLoopbackMode::MAC);
  }

  void resolveNhops(int numNhops) {
    applyNewState(ecmpHelper_->resolveNextHops(getProgrammedState(), numNhops));
  }
  void resolveNhops(const std::vector<PortDescriptor>& portDescs) {
    applyNewState(ecmpHelper_->resolveNextHops(
        getProgrammedState(),
        flat_set<PortDescriptor>(portDescs.begin(), portDescs.end())));
  }
  void programRouteWithUnresolvedNhops() {
    applyNewState(
        ecmpHelper_->setupECMPForwarding(getProgrammedState(), kNumNextHops));
  }
  int getEcmpSizeInHw(int sizeInSw = kNumNextHops) const {
    return utility::getEcmpSizeInHw(
        getHwSwitch(), kDefaultRoutePrefix, kRid, sizeInSw);
  }
};

TEST_F(HwEcmpTest, L2ResolveOneNhopInEcmp) {
  auto setup = [=]() {
    programRouteWithUnresolvedNhops();
    resolveNhops(1);
  };
  auto verify = [=]() { EXPECT_EQ(1, getEcmpSizeInHw()); };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwEcmpTest, L2ResolveOneNhopThenLinkDownThenUp) {
  auto setup = [=]() {
    programRouteWithUnresolvedNhops();
    resolveNhops(1);

    EXPECT_EQ(1, getEcmpSizeInHw());

    auto nhop = ecmpHelper_->nhop(0);
    bringDownPort(nhop.portDesc.phyPortID());
  };

  auto verify = [=]() {
    // ECMP shrunk on port down
    EXPECT_EQ(0, getEcmpSizeInHw());
  };

  auto setupPostWarmboot = [=]() {
    auto nhop = ecmpHelper_->nhop(0);
    bringUpPort(nhop.portDesc.phyPortID());
  };

  auto verifyPostWarmboot = [=]() {
    auto nhop = ecmpHelper_->nhop(0);
    // ECMP stays shrunk on port up
    ASSERT_EQ(0, getEcmpSizeInHw());

    // Bring port back down so we can warmboot more than once. This is
    // necessary because verify() and verifyPostWarmboot() assume that the port
    // is down and the nexthop unresolved, which won't be true if we warmboot
    // after bringing the port up in setupPostWarmboot().
    bringDownPort(nhop.portDesc.phyPortID());
  };

  verifyAcrossWarmBoots(setup, verify, setupPostWarmboot, verifyPostWarmboot);
}

} // namespace facebook::fboss
