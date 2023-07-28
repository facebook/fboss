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

#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"
#include "fboss/agent/hw/test/HwTestPtpTcUtils.h"

namespace {
constexpr int kLoopCount = 10;
} // namespace

namespace facebook::fboss {

class HwPtpTcTest : public HwLinkStateDependentTest {
 protected:
  void SetUp() override {
    HwLinkStateDependentTest::SetUp();
  }

  cfg::SwitchConfig initialConfig() const override {
    return utility::onePortPerInterfaceConfig(
        getHwSwitch(),
        masterLogicalPortIds(),
        getAsic()->desiredLoopbackModes());
  }

  void setupHelper() {
    auto newCfg{initialConfig()};
    applyNewConfig(newCfg);
    cfg_ = newCfg;
  }

  void setPtpTc(const bool val) {
    cfg_.switchSettings()->ptpTcEnable() = val;
    applyNewConfig(cfg_);
  }

  cfg::SwitchConfig cfg_;
};

TEST_F(HwPtpTcTest, VerifyPtpTcEnable) {
  const auto& asic = getHwSwitchEnsemble()->getPlatform()->getAsic();
  if (!(asic->isSupported(HwAsic::Feature::PTP_TC) ||
        asic->isSupported(HwAsic::Feature::PTP_TC_PCS))) {
#if defined(GTEST_SKIP)
    GTEST_SKIP();
#endif
    return;
  }

  auto setup = [=]() {
    setupHelper();
    setPtpTc(true);
  };
  auto verify = [&]() { EXPECT_TRUE(utility::getPtpTcEnabled(getHwSwitch())); };
  auto verifyPostWarmboot = [&]() {
    EXPECT_TRUE(utility::getPtpTcEnabled(getHwSwitch()));
  };

  verifyAcrossWarmBoots(
      setup, verify, []() {}, verifyPostWarmboot);
}

TEST_F(HwPtpTcTest, VerifyPtpTcToggle) {
  const auto& asic = getHwSwitchEnsemble()->getPlatform()->getAsic();
  if (!(asic->isSupported(HwAsic::Feature::PTP_TC) ||
        asic->isSupported(HwAsic::Feature::PTP_TC_PCS))) {
#if defined(GTEST_SKIP)
    GTEST_SKIP();
#endif
    return;
  }

  auto setup = [=]() { setupHelper(); };
  auto enabled = false;
  auto verify = [&]() {
    EXPECT_EQ(enabled, utility::getPtpTcEnabled(getHwSwitch()));
  };

  setup();

  for (int i = 0; i < kLoopCount; ++i) {
    enabled = !enabled; // toggle

    setPtpTc(enabled);
    verify();
  }
}

} // namespace facebook::fboss
