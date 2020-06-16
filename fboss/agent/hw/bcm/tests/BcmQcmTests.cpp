/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/bcm/BcmLogBuffer.h"
#include "fboss/agent/hw/bcm/BcmPlatform.h"
#include "fboss/agent/hw/bcm/BcmQcmManager.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/tests/BcmTest.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/hw/test/ConfigFactory.h"

namespace {
const std::string kCollctorSrcIp = "10.0.0.2/32";
const std::string kCollctorDstIp = "11.0.0.2/32";
int kCollectorUDPSrcPort = 20000;
} // namespace
namespace facebook::fboss {
class BcmQcmTest : public BcmTest {
 protected:
  void SetUp() override {
    FLAGS_load_qcm_fw = true;
    BcmTest::SetUp();
  }

  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::oneL3IntfConfig(
        getHwSwitch(), masterLogicalPortIds()[0], cfg::PortLoopbackMode::MAC);
    return cfg;
  }

  void setupHelper() {
    auto newCfg{initialConfig()};
    auto qcmCfg = cfg::QcmConfig();
    // some dummy address
    qcmCfg.collectorDstIp = kCollctorDstIp;
    qcmCfg.collectorSrcIp = kCollctorSrcIp;
    qcmCfg.collectorSrcPort = kCollectorUDPSrcPort;
    newCfg.qcmConfig_ref() = qcmCfg;
    newCfg.switchSettings.qcmEnable = true;
    applyNewConfig(newCfg);
  }

  bool skipTest() {
#if BCM_VER_SUPPORT_QCM
    return !getPlatform()->getAsic()->isSupported(HwAsic::Feature::QCM);
#else
    XLOG(WARN) << "Test not supported for BCM SDK version other than 6.5.16";
    return true;
#endif
  }
};

TEST_F(BcmQcmTest, VerifyQcmFirmwareInit) {
  if (skipTest()) {
    // This test only applies to ceratin ASIC e.g. TH
    // and to specific sdk versions
    return;
  }

  // intent of this test is to load the QcmFirmware during
  // initialization (using flags above in SetUp) and ensure it goes through fine
  // Verify if the status of firmware is good
  auto setup = [=]() { setupHelper(); };

  auto verify = [this]() {
    BcmLogBuffer logBuffer;
    getHwSwitch()->printDiagCmd("mcsstatus Quick=true");
    std::string mcsStatusStr =
        logBuffer.getBuffer()->moveToFbString().toStdString();
    XLOG(INFO) << "MCStatus:" << mcsStatusStr;
    EXPECT_TRUE(mcsStatusStr.find("uC 0 status: OK") != std::string::npos);
  };
  // TODO: rohitpuri
  // Enable warmboot once QCM supports it CS00010434741
  // verifyAcrossWarmBoots(setup, verify);
  setup();
  verify();
}
} // namespace facebook::fboss
