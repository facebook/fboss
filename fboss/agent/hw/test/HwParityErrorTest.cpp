// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"

#include "fboss/agent/hw/HwSwitchFb303Stats.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwTestTamUtils.h"
#include "fboss/lib/CommonUtils.h"

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

  void generateParityError() {
    utility::triggerParityError(getHwSwitchEnsemble());
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
    WITH_RETRIES({ EXPECT_EVENTUALLY_GT(getCorrectedParityErrorCount(), 0); });
  };
  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
