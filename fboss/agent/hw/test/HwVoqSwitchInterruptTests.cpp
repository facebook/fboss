// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <fb303/ServiceData.h>
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/hw/HwSwitchFb303Stats.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"
#include "fboss/lib/CommonUtils.h"
#include "folly/experimental/TestUtil.h"

namespace facebook::fboss {
class HwVoqSwitchInterruptTest : public HwLinkStateDependentTest {
 public:
  HwSwitchEnsemble::Features featuresDesired() const override {
    return {HwSwitchEnsemble::LINKSCAN, HwSwitchEnsemble::TAM_NOTIFY};
  }
  cfg::SwitchConfig initialConfig() const override {
    return utility::onePortPerInterfaceConfig(
        getHwSwitch(),
        masterLogicalPortIds(),
        getAsic()->desiredLoopbackModes(),
        true /*interfaceHasSubnet*/);
  }
};

TEST_F(HwVoqSwitchInterruptTest, ireError) {
  auto verify = [=, this]() {
    constexpr auto kIreErrorIncjectorCintStr = R"(
  cint_reset();
  bcm_switch_event_control_t event_ctrl;
  event_ctrl.event_id = 2034;
  event_ctrl.index = 0; /* core ID */
  event_ctrl.action = bcmSwitchEventForce;
  print bcm_switch_event_control_set(0, BCM_SWITCH_EVENT_DEVICE_INTERRUPT, event_ctrl, 1);
  )";
    folly::test::TemporaryFile file;
    XLOG(INFO) << " Cint file " << file.path().c_str();
    folly::writeFull(
        file.fd(),
        kIreErrorIncjectorCintStr,
        std::strlen(kIreErrorIncjectorCintStr));
    std::string out;
    getHwSwitchEnsemble()->runDiagCommand(
        folly::sformat("cint {}\n", file.path().c_str()), out);
    getHwSwitchEnsemble()->runDiagCommand("quit\n", out);
    WITH_RETRIES({
      getHwSwitch()->updateStats();
      fb303::ThreadCachedServiceData::get()->publishStats();
      auto ireErrors = getHwSwitch()
                           ->getSwitchStats()
                           ->getHwAsicErrors()
                           .ingressReceiveEditorErrors()
                           .value_or(0);
      XLOG(INFO) << " IRE Errors: " << ireErrors;
      EXPECT_EVENTUALLY_GT(ireErrors, 0);
      EXPECT_EVENTUALLY_GT(getHwSwitch()->getSwitchStats()->getIreErrors(), 0);
    });
  };
  verifyAcrossWarmBoots([]() {}, verify);
}
} // namespace facebook::fboss
