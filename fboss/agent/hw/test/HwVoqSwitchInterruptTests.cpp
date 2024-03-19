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

 protected:
  void runCint(const std::string cintStr) {
    folly::test::TemporaryFile file;
    XLOG(INFO) << " Cint file " << file.path().c_str();
    folly::writeFull(file.fd(), cintStr.c_str(), cintStr.size());
    std::string out;
    getHwSwitchEnsemble()->runDiagCommand(
        folly::sformat("cint {}\n", file.path().c_str()), out);
    getHwSwitchEnsemble()->runDiagCommand("quit\n", out);
  }
};

TEST_F(HwVoqSwitchInterruptTest, alignerError) {
  auto verify = [=, this]() {
    constexpr auto kAlignerErrorIncjectorCintStr = R"(
  cint_reset();
  bcm_switch_event_control_t event_ctrl;
  event_ctrl.event_id = 8;  // JR3_INT_ALIGNER_PKT_SIZE_EOP_MISMATCH_INT
  event_ctrl.index = 0; /* core ID */
  event_ctrl.action = bcmSwitchEventForce;
  print bcm_switch_event_control_set(0, BCM_SWITCH_EVENT_DEVICE_INTERRUPT, event_ctrl, 1);
  )";
    runCint(kAlignerErrorIncjectorCintStr);
    WITH_RETRIES({
      getHwSwitch()->updateStats();
      fb303::ThreadCachedServiceData::get()->publishStats();
      auto alignerErrors = getHwSwitch()
                               ->getSwitchStats()
                               ->getHwAsicErrors()
                               .alignerErrors()
                               .value_or(0);
      XLOG(INFO) << " Aligner Errors: " << alignerErrors;
      EXPECT_EVENTUALLY_GT(alignerErrors, 0);
      EXPECT_EVENTUALLY_GT(
          getHwSwitch()->getSwitchStats()->getAlignerErrors(), 0);
    });
  };
  verifyAcrossWarmBoots([]() {}, verify);
}

TEST_F(HwVoqSwitchInterruptTest, fqpError) {
  auto verify = [=, this]() {
    constexpr auto kFqpErrorIncjectorCintStr = R"(
  cint_reset();
  bcm_switch_event_control_t event_ctrl;
  event_ctrl.event_id = 1294;  // JR3_INT_FQP_ECC_ECC_1B_ERR_INT
  event_ctrl.index = 0; /* core ID */
  event_ctrl.action = bcmSwitchEventForce;
  print bcm_switch_event_control_set(0, BCM_SWITCH_EVENT_DEVICE_INTERRUPT, event_ctrl, 1);
  )";
    runCint(kFqpErrorIncjectorCintStr);
    WITH_RETRIES({
      getHwSwitch()->updateStats();
      fb303::ThreadCachedServiceData::get()->publishStats();
      auto fqpErrors = getHwSwitch()
                           ->getSwitchStats()
                           ->getHwAsicErrors()
                           .forwardingQueueProcessorErrors()
                           .value_or(0);
      XLOG(INFO) << " FQP Errors: " << fqpErrors;
      EXPECT_EVENTUALLY_GT(fqpErrors, 0);
      EXPECT_EVENTUALLY_GT(
          getHwSwitch()->getSwitchStats()->getForwardingQueueProcessorErrors(),
          0);
    });
  };
  verifyAcrossWarmBoots([]() {}, verify);
}

TEST_F(HwVoqSwitchInterruptTest, allReassemblyContextsTakenError) {
  auto verify = [=, this]() {
    std::string out;
    getHwSwitchEnsemble()->runDiagCommand(
        "modreg RQP_PKT_REAS_INTERRUPT_REGISTER_TEST PKT_REAS_INTERRUPT_REGISTER_TEST=0x8000\n",
        out);
    getHwSwitchEnsemble()->runDiagCommand(
        "modreg RQP_PKT_REAS_INTERRUPT_REGISTER_TEST PKT_REAS_INTERRUPT_REGISTER_TEST=0x10000\n",
        out);
    getHwSwitchEnsemble()->runDiagCommand("quit\n", out);
    WITH_RETRIES({
      getHwSwitch()->updateStats();
      fb303::ThreadCachedServiceData::get()->publishStats();
      auto allReassemblyContextsTaken = getHwSwitch()
                                            ->getSwitchStats()
                                            ->getHwAsicErrors()
                                            .allReassemblyContextsTaken()
                                            .value_or(0);
      XLOG(INFO) << " All Reassemble contexts taken: "
                 << allReassemblyContextsTaken;
      EXPECT_EVENTUALLY_GE(allReassemblyContextsTaken, 1);
      EXPECT_EVENTUALLY_GE(
          getHwSwitch()->getSwitchStats()->getAllReassemblyContextsTakenError(),
          1);
    });
  };
  verifyAcrossWarmBoots([]() {}, verify);
}

} // namespace facebook::fboss
