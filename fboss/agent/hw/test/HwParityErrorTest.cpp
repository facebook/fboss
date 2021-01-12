// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"

#include "fboss/agent/hw/HwSwitchStats.h"
#include "fboss/agent/hw/test/ConfigFactory.h"

namespace facebook::fboss {

class HwParityErrorTest : public HwLinkStateDependentTest {
 protected:
  cfg::SwitchConfig initialConfig() const override {
    return utility::onePortPerVlanConfig(getHwSwitch(), masterLogicalPortIds());
  }

  HwSwitchEnsemble::Features featuresDesired() const override {
    return {HwSwitchEnsemble::LINKSCAN, HwSwitchEnsemble::TAM_NOTIFY};
  }

  bool skipTest() {
    auto asic = getPlatform()->getAsic()->getAsicType();
    switch (asic) {
      case HwAsic::AsicType::ASIC_TYPE_FAKE:
      case HwAsic::AsicType::ASIC_TYPE_MOCK:
      case HwAsic::AsicType::ASIC_TYPE_TAJO:
        break;
      case HwAsic::AsicType::ASIC_TYPE_TRIDENT2:
      case HwAsic::AsicType::ASIC_TYPE_TOMAHAWK:
      case HwAsic::AsicType::ASIC_TYPE_TOMAHAWK3:
      case HwAsic::AsicType::ASIC_TYPE_TOMAHAWK4:
        return false;
    }
    return true;
  }

  void generateBcmParityError() {
    std::string out;
    auto asic = getPlatform()->getAsic()->getAsicType();
    auto ensemble = getHwSwitchEnsemble();
    ensemble->runDiagCommand("\n", out);
    if (asic == HwAsic::AsicType::ASIC_TYPE_TOMAHAWK4) {
      ensemble->runDiagCommand("ser inject pt=L2_ENTRY_SINGLEm\n", out);
      ensemble->runDiagCommand("ser LOG\n", out);
    } else {
      ensemble->runDiagCommand(
          "ser INJECT memory=L2_ENTRY index=10 pipe=pipe_x\n", out);
      ensemble->runDiagCommand("d chg L2_ENTRY 10 1\n", out);
    }
    ensemble->runDiagCommand("quit\n", out);
    std::ignore = out;
  }

  void generateParityError() {
    auto asic = getPlatform()->getAsic()->getAsicType();
    switch (asic) {
      case HwAsic::AsicType::ASIC_TYPE_FAKE:
      case HwAsic::AsicType::ASIC_TYPE_MOCK:
      case HwAsic::AsicType::ASIC_TYPE_TAJO:
        XLOG(FATAL) << "Unsupported HwAsic";
        break;
      case HwAsic::AsicType::ASIC_TYPE_TRIDENT2:
      case HwAsic::AsicType::ASIC_TYPE_TOMAHAWK:
      case HwAsic::AsicType::ASIC_TYPE_TOMAHAWK3:
      case HwAsic::AsicType::ASIC_TYPE_TOMAHAWK4:
        generateBcmParityError();
    }
  }

  void verifyParityError() {
    auto stats = getHwSwitchEnsemble()->getHwSwitch()->getSwitchStats();
    EXPECT_GT(stats->getCorrParityErrorCount(), 0);
  }
};

TEST_F(HwParityErrorTest, verifyParityError) {
  if (skipTest()) {
// profile is not supported.
#if defined(GTEST_SKIP)
    GTEST_SKIP();
#endif
    return;
  }
  auto setup = [=]() { applyNewConfig(initialConfig()); };
  auto verify = [=]() {
    auto ensemble = getHwSwitchEnsemble();
    auto stats = ensemble->getHwSwitch()->getSwitchStats();
    EXPECT_EQ(stats->getCorrParityErrorCount(), 0);
    generateParityError();
    auto retries = 3;
    stats = ensemble->getHwSwitch()->getSwitchStats();
    while (retries-- && stats->getCorrParityErrorCount() == 0) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      stats = ensemble->getHwSwitch()->getSwitchStats();
    }
    verifyParityError();
  };
  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
