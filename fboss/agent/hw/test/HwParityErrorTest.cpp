// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"

#include "fboss/agent/hw/HwSwitchStats.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwTestTamUtils.h"

#include <fb303/ServiceData.h>
#include <fb303/ThreadCachedServiceData.h>

namespace facebook::fboss {

class HwParityErrorTest : public HwLinkStateDependentTest {
 protected:
  cfg::SwitchConfig initialConfig() const override {
    return utility::onePortPerVlanConfig(
        getHwSwitch(), {masterLogicalPortIds()[0]});
  }

  HwSwitchEnsemble::Features featuresDesired() const override {
    return {HwSwitchEnsemble::LINKSCAN, HwSwitchEnsemble::TAM_NOTIFY};
  }

  bool skipTest() {
    return !getPlatform()->getAsic()->isSupported(
        HwAsic::Feature::TELEMETRY_AND_MONITORING);
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

  void generateTajoParityError() {
    std::string out;
    auto ensemble = getHwSwitchEnsemble();
    ensemble->runDiagCommand("\n", out);
    ensemble->runDiagCommand("from cli import sai_cli\n", out);
    ensemble->runDiagCommand("saidev = sai_cli.sai_device()\n", out);
    ensemble->runDiagCommand("saidev.inject_ecc_error()\n", out);
    ensemble->runDiagCommand("quit\n", out);
    std::ignore = out;
  }

  void generateParityError() {
    auto asic = getPlatform()->getAsic()->getAsicType();
    switch (asic) {
      case HwAsic::AsicType::ASIC_TYPE_FAKE:
      case HwAsic::AsicType::ASIC_TYPE_MOCK:
      case HwAsic::AsicType::ASIC_TYPE_ELBERT_8DD:
        XLOG(FATAL) << "Unsupported HwAsic";
        break;
      case HwAsic::AsicType::ASIC_TYPE_TAJO:
      case HwAsic::AsicType::ASIC_TYPE_TRIDENT2:
      case HwAsic::AsicType::ASIC_TYPE_TOMAHAWK:
      case HwAsic::AsicType::ASIC_TYPE_TOMAHAWK3:
      case HwAsic::AsicType::ASIC_TYPE_TOMAHAWK4:
        utility::triggerParityError(getHwSwitchEnsemble());
    }
  }

  int64_t getParityErrorCount() const {
    // Aggregate TL stats. In HwTests we don't start a TL aggregation thread.
    fb303::ThreadCachedServiceData::get()->publishStats();
    return fb303::fbData->getCounter(
        getAsic()->getVendor() + ".parity.corr.sum.60");
  }
  void verifyParityError() {
    auto stats = getHwSwitchEnsemble()->getHwSwitch()->getSwitchStats();
    EXPECT_GT(getParityErrorCount(), 0);
  }
};

TEST_F(HwParityErrorTest, verifyParityError) {
  auto setup = [=]() { applyNewConfig(initialConfig()); };
  auto verify = [=]() {
    auto ensemble = getHwSwitchEnsemble();
    auto stats = ensemble->getHwSwitch()->getSwitchStats();
    EXPECT_EQ(stats->getCorrParityErrorCount(), 0);
    generateParityError();
    auto retries = 3;
    while (retries-- && getParityErrorCount() == 0) {
      std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    verifyParityError();
  };
  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
