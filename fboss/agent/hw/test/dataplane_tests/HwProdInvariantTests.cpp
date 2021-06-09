/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"

#include "fboss/agent/hw/test/HwTestProdConfigUtils.h"
#include "fboss/agent/hw/test/gen-cpp2/validated_shell_commands_constants.h"

#include "fboss/agent/hw/test/dataplane_tests/HwProdInvariantHelper.h"

#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwTestPfcUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwTestQosUtils.h"

#include "fboss/agent/state/Port.h"

namespace facebook::fboss {
/*
 * Test to verify that standard invariants hold. DO NOT change the name
 * of this test - its used in testing prod<->trunk warmboots on diff
 */
class HwProdInvariantsTest : public HwLinkStateDependentTest {
 protected:
  cfg::SwitchConfig initialConfig() const override {
    auto cfg =
        utility::onePortPerVlanConfig(getHwSwitch(), masterLogicalPortIds());
    utility::addProdFeaturesToConfig(cfg, getHwSwitch());
    return cfg;
  }

  void SetUp() override {
    HwLinkStateDependentTest::SetUp();
    prodInvariants_ = std::make_unique<HwProdInvariantHelper>(
        getHwSwitchEnsemble(), initialConfig());
    prodInvariants_->setupEcmp();
  }
  void verifyInvariants() {
    prodInvariants_->verifyInvariants();
  }
  HwSwitchEnsemble::Features featuresDesired() const override {
    return HwProdInvariantHelper::featuresDesired();
  }

 private:
  std::unique_ptr<HwProdInvariantHelper> prodInvariants_;
};

TEST_F(HwProdInvariantsTest, verifyInvariants) {
  verifyAcrossWarmBoots([]() {}, [this]() { verifyInvariants(); });
}

class HwProdInvariantsMmuLosslessTest : public HwLinkStateDependentTest {
 protected:
  cfg::SwitchConfig initialConfig() const override {
    auto cfg =
        utility::onePortPerVlanConfig(getHwSwitch(), masterLogicalPortIds());
    utility::addProdFeaturesToConfig(
        cfg, getHwSwitch(), FLAGS_mmu_lossless_mode, masterLogicalPortIds());
    return cfg;
  }

  void SetUp() override {
    FLAGS_mmu_lossless_mode = true;
    HwLinkStateDependentTest::SetUp();
    prodInvariants_ = std::make_unique<HwProdInvariantHelper>(
        getHwSwitchEnsemble(), initialConfig());
    // explicitly creating with interfaceMac as nexthop, causes
    // routed pkts to be not dropped when looped to ingress
    prodInvariants_->setupEcmpWithNextHopMac(dstMac());
  }

  HwSwitchEnsemble::Features featuresDesired() const override {
    return HwProdInvariantHelper::featuresDesired();
  }

  void verifyInvariants() {
    prodInvariants_->verifyInvariants();
    prodInvariants_->verifyNoDiscards();
  }

  void verifyNoDiscards() {
    prodInvariants_->verifyNoDiscards();
  }

  MacAddress dstMac() const {
    auto vlanId = utility::firstVlanID(initialConfig());
    return utility::getInterfaceMac(getProgrammedState(), vlanId);
  }

  void sendTrafficInLoop() {
    prodInvariants_->disableTtl();
    // send traffic from port which is not in ecmp
    prodInvariants_->sendTraffic();
    // since ports in ecmp width start from 0, just ensure atleast first port
    // has attained the line rate
    getHwSwitchEnsemble()->waitForLineRateOnPort(
        getHwSwitchEnsemble()->masterLogicalPortIds()[0]);
  }

 private:
  std::unique_ptr<HwProdInvariantHelper> prodInvariants_;
};

// validate that running there are no discards during line rate run
// of traffic while doing warm boot
TEST_F(HwProdInvariantsMmuLosslessTest, ValidateMmuLosslessMode) {
  if (!getPlatform()->getAsic()->isSupported(HwAsic::Feature::PFC)) {
    return;
  }
  verifyAcrossWarmBoots([]() {}, [this]() { verifyInvariants(); });
}

TEST_F(HwProdInvariantsMmuLosslessTest, ValidateWarmBootNoDiscards) {
  if (!getPlatform()->getAsic()->isSupported(HwAsic::Feature::PFC)) {
    return;
  }
  auto setup = [=]() {
    // run the loop, so traffic continues running
    sendTrafficInLoop();
  };

  auto verify = [=]() { verifyNoDiscards(); };
  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
