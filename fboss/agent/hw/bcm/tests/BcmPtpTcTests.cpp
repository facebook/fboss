/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <folly/IPAddressV6.h>
#include <folly/logging/xlog.h>
#include "fboss/agent/hw/bcm/BcmLogBuffer.h"
#include "fboss/agent/hw/bcm/BcmPtpTcMgr.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/tests/BcmLinkStateDependentTests.h"
#include "fboss/agent/hw/bcm/tests/BcmSwitchEnsemble.h"
#include "fboss/agent/hw/bcm/tests/BcmTestStatUtils.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwTestOlympicUtils.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/packet/PktUtil.h"
#include "fboss/agent/test/EcmpSetupHelper.h"

namespace {
constexpr int kLoopCount = 10;
} // namespace

namespace facebook::fboss {

class BcmPtpTcTest : public BcmLinkStateDependentTests {
 protected:
  void SetUp() override {
    BcmLinkStateDependentTests::SetUp();
  }

  cfg::SwitchConfig initialConfig() const override {
    std::vector<PortID> ports = {
        masterLogicalPortIds()[0],
        masterLogicalPortIds()[1],
    };
    auto config = utility::onePortPerVlanConfig(
        getHwSwitch(), std::move(ports), cfg::PortLoopbackMode::MAC, true);
    return config;
  }

  void setupHelper() {
    auto newCfg{initialConfig()};
    applyNewConfig(newCfg);
    cfg_ = newCfg;
  }

  void setPtpTc(const bool val) {
    cfg_.switchSettings_ref()->ptpTcEnable_ref() = val;
    applyNewConfig(cfg_);
  }

  cfg::SwitchConfig cfg_;
};

TEST_F(BcmPtpTcTest, VerifyPtpTcEnable) {
  if (!BcmPtpTcMgr::isPtpTcSupported(getHwSwitch())) {
    return;
  }

  auto setup = [=]() {
    setupHelper();
    setPtpTc(true);
  };
  auto verify = [&]() {
    EXPECT_TRUE(getHwSwitch()->getBcmPtpTcMgr()->isPtpTcEnabled());
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(BcmPtpTcTest, VerifyPtpTcToggle) {
  if (!BcmPtpTcMgr::isPtpTcSupported(getHwSwitch())) {
    return;
  }

  auto setup = [=]() { setupHelper(); };

  auto enabled = false;
  auto verify = [&]() {
    EXPECT_EQ(enabled, getHwSwitch()->getBcmPtpTcMgr()->isPtpTcEnabled());
  };

  setup();
  verify();

  for (int i = 0; i < kLoopCount; ++i) {
    enabled = !enabled; // toggle

    setPtpTc(enabled);
    verify();
  }
}

} // namespace facebook::fboss
