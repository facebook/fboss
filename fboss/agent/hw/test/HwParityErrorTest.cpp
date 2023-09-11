// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"

#include "fboss/agent/hw/HwSwitchFb303Stats.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwTestTamUtils.h"

#include <fb303/ServiceData.h>
#include <fb303/ThreadCachedServiceData.h>

namespace facebook::fboss {

class HwParityErrorTest : public HwLinkStateDependentTest {
 protected:
  cfg::SwitchConfig initialConfig() const override {
    return utility::onePortPerInterfaceConfig(
        getHwSwitch(), {masterLogicalPortIds()[0]});
  }

  HwSwitchEnsemble::Features featuresDesired() const override {
    return {HwSwitchEnsemble::LINKSCAN, HwSwitchEnsemble::TAM_NOTIFY};
  }

  bool skipTest() {
    return !getPlatform()->getAsic()->isSupported(
        HwAsic::Feature::TELEMETRY_AND_MONITORING);
  }

  void generateParityError() {
    auto asic = getPlatform()->getAsic()->getAsicType();
    switch (asic) {
      case cfg::AsicType::ASIC_TYPE_FAKE:
      case cfg::AsicType::ASIC_TYPE_MOCK:
      case cfg::AsicType::ASIC_TYPE_ELBERT_8DD:
      case cfg::AsicType::ASIC_TYPE_SANDIA_PHY:
      case cfg::AsicType::ASIC_TYPE_JERICHO2:
      case cfg::AsicType::ASIC_TYPE_JERICHO3:
      case cfg::AsicType::ASIC_TYPE_RAMON:
      case cfg::AsicType::ASIC_TYPE_RAMON3:
        XLOG(FATAL) << "Unsupported HwAsic";
        break;
      case cfg::AsicType::ASIC_TYPE_EBRO:
      case cfg::AsicType::ASIC_TYPE_GARONNE:
      case cfg::AsicType::ASIC_TYPE_YUBA:
      case cfg::AsicType::ASIC_TYPE_TRIDENT2:
      case cfg::AsicType::ASIC_TYPE_TOMAHAWK:
      case cfg::AsicType::ASIC_TYPE_TOMAHAWK3:
      case cfg::AsicType::ASIC_TYPE_TOMAHAWK4:
      case cfg::AsicType::ASIC_TYPE_TOMAHAWK5:
        utility::triggerParityError(getHwSwitchEnsemble());
    }
  }

  int64_t getCorrectedParityErrorCount() const {
    // Aggregate TL stats. In HwTests we don't start a TL aggregation thread.
    fb303::ThreadCachedServiceData::get()->publishStats();
    auto asicErrors = getHwSwitch()->getSwitchStats()->getHwAsicErrors();
    return *asicErrors.correctedParityErrors();
  }
};

TEST_F(HwParityErrorTest, verifyParityError) {
  auto setup = [=]() { applyNewConfig(initialConfig()); };
  auto verify = [=]() {
    EXPECT_EQ(getCorrectedParityErrorCount(), 0);
    generateParityError();
    auto retries = 3;
    while (retries-- && getCorrectedParityErrorCount() == 0) {
      std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    EXPECT_GT(getCorrectedParityErrorCount(), 0);
  };
  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
