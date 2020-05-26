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
  static constexpr uint8_t numNextHops_{8};
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
  void programRouteWithUnresolvedNhops(size_t numNhops) {
    applyNewState(
        ecmpHelper_->setupECMPForwarding(getProgrammedState(), numNhops));
  }
};

TEST_F(HwEcmpTest, L2ResolveOneNhopInEcmp) {
  auto setup = [=]() {
    programRouteWithUnresolvedNhops(numNextHops_);
    resolveNhops(1);
  };
  auto verify = [=]() {
    EXPECT_EQ(
        1,
        utility::getEcmpSizeInHw(
            getHwSwitch(), kDefaultRoutePrefix, kRid, numNextHops_));
  };
  verifyAcrossWarmBoots(setup, verify);
}
} // namespace facebook::fboss
