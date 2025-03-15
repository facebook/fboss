// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/lib/CommonUtils.h"

#include "folly/testing/TestUtil.h"

namespace facebook::fboss {
class AgentVoqSwitchInterruptTest : public AgentHwTest {
 public:
  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    return {production_features::ProductionFeature::VOQ_DNX_INTERRUPTS};
  }

 protected:
  void runCint(const std::string& cintStr) {
    folly::test::TemporaryFile file;
    XLOG(INFO) << " Cint file " << file.path().c_str();
    folly::writeFull(file.fd(), cintStr.c_str(), cintStr.size());
    auto cmd = folly::sformat("cint {}\n", file.path().c_str());
    runCmd(cmd);
  }
  void runCmd(const std::string& cmd) {
    for (auto voqSwitchId : getSw()->getSwitchInfoTable().getSwitchIdsOfType(
             cfg::SwitchType::VOQ)) {
      std::string out;
      getAgentEnsemble()->runDiagCommand(cmd, out, voqSwitchId);
    }
  }
  std::unordered_set<uint16_t> voqIndices() const {
    return getSw()->getSwitchInfoTable().getSwitchIndicesOfType(
        cfg::SwitchType::VOQ);
  }
  std::map<uint16_t, HwAsicErrors> getVoqAsicErrors() {
    auto voqIdxs = voqIndices();
    std::map<uint16_t, HwAsicErrors> asicErrors;
    WITH_RETRIES({
      asicErrors.clear();
      for (const auto& [switchIdx, switchStats] :
           getSw()->getHwSwitchStatsExpensive()) {
        if (voqIdxs.find(switchIdx) != voqIdxs.end()) {
          asicErrors.insert({switchIdx, *switchStats.hwAsicErrors()});
        }
      }
      EXPECT_EVENTUALLY_EQ(asicErrors.size(), voqIdxs.size());
    });
    if (asicErrors.size() != voqIdxs.size()) {
      throw FbossError("Could not get voq asic error stats");
    }

    return asicErrors;
  }
};

TEST_F(AgentVoqSwitchInterruptTest, ireError) {
  auto verify = [=, this]() {
    constexpr auto kIreErrorIncjectorCintStr = R"(
  cint_reset();
  bcm_switch_event_control_t event_ctrl;
  event_ctrl.event_id = 2064;
  event_ctrl.index = 0; /* core ID */
  event_ctrl.action = bcmSwitchEventForce;
  print bcm_switch_event_control_set(0, BCM_SWITCH_EVENT_DEVICE_INTERRUPT, event_ctrl, 0);
  print bcm_switch_event_control_set(0, BCM_SWITCH_EVENT_DEVICE_INTERRUPT, event_ctrl, 1);
  )";
    runCint(kIreErrorIncjectorCintStr);
    WITH_RETRIES({
      auto asicErrors = getVoqAsicErrors();
      for (const auto& [idx, asicError] : asicErrors) {
        auto ireErrors = asicError.ingressReceiveEditorErrors().value_or(0);
        XLOG(INFO) << "Switch index: " << idx << " IRE Errors: " << ireErrors;
        EXPECT_EVENTUALLY_GT(ireErrors, 0);
      }
    });
  };
  verifyAcrossWarmBoots([]() {}, verify);
}

TEST_F(AgentVoqSwitchInterruptTest, itppError) {
  auto verify = [=, this]() {
    std::string out;
    runCmd("s itpp_interrupt_mask_register 0x3f\n");
    runCmd("s itpp_interrupt_register_test 0x0\n");
    runCmd("s itpp_interrupt_register_test 0x2\n");
    runCmd("s itppd_interrupt_mask_register 0x3f\n");
    runCmd("s itppd_interrupt_register_test 0x0\n");
    runCmd("s itppd_interrupt_register_test 0x2\n");
    WITH_RETRIES({
      auto asicErrors = getVoqAsicErrors();
      for (const auto& [idx, asicError] : asicErrors) {
        auto itppErrors = asicError.ingressTransmitPipelineErrors().value_or(0);
        XLOG(INFO) << "Switch index: " << idx << " ITPP Errors: " << itppErrors;
        EXPECT_EVENTUALLY_GT(itppErrors, 0);
      }
    });
  };
  verifyAcrossWarmBoots([]() {}, verify);
}

TEST_F(AgentVoqSwitchInterruptTest, epniError) {
  auto verify = [=, this]() {
    constexpr auto kEpniErrorIncjectorCintStr = R"(
  cint_reset();
  bcm_switch_event_control_t event_ctrl;
  event_ctrl.event_id = 717;  // JR3_INT_EPNI_FIFO_OVERFLOW_INT
  event_ctrl.index = 0; /* core ID */
  event_ctrl.action = bcmSwitchEventForce;
  print bcm_switch_event_control_set(0, BCM_SWITCH_EVENT_DEVICE_INTERRUPT, event_ctrl, 0);
  print bcm_switch_event_control_set(0, BCM_SWITCH_EVENT_DEVICE_INTERRUPT, event_ctrl, 1);
  )";
    runCint(kEpniErrorIncjectorCintStr);
    WITH_RETRIES({
      auto asicErrors = getVoqAsicErrors();
      for (const auto& [idx, asicError] : asicErrors) {
        auto epniErrors =
            asicError.egressPacketNetworkInterfaceErrors().value_or(0);
        XLOG(INFO) << "Switch index: " << idx << " EPNI Errors: " << epniErrors;
        EXPECT_EVENTUALLY_GT(epniErrors, 0);
      }
    });
  };
  verifyAcrossWarmBoots([]() {}, verify);
}

TEST_F(AgentVoqSwitchInterruptTest, alignerError) {
  auto verify = [=, this]() {
    constexpr auto kAlignerErrorIncjectorCintStr = R"(
  cint_reset();
  bcm_switch_event_control_t event_ctrl;
  event_ctrl.event_id = 8;  // JR3_INT_ALIGNER_PKT_SIZE_EOP_MISMATCH_INT
  event_ctrl.index = 0; /* core ID */
  event_ctrl.action = bcmSwitchEventForce;
  print bcm_switch_event_control_set(0, BCM_SWITCH_EVENT_DEVICE_INTERRUPT, event_ctrl, 0);
  print bcm_switch_event_control_set(0, BCM_SWITCH_EVENT_DEVICE_INTERRUPT, event_ctrl, 1);
  )";
    runCint(kAlignerErrorIncjectorCintStr);
    WITH_RETRIES({
      auto asicErrors = getVoqAsicErrors();
      for (const auto& [idx, asicError] : asicErrors) {
        auto alignerErrors = asicError.alignerErrors().value_or(0);
        XLOG(INFO) << "Switch index: " << idx
                   << " Aligner Errors: " << alignerErrors;
        EXPECT_EVENTUALLY_GT(alignerErrors, 0);
      }
    });
  };
  verifyAcrossWarmBoots([]() {}, verify);
}

TEST_F(AgentVoqSwitchInterruptTest, fqpError) {
  auto verify = [=, this]() {
    constexpr auto kFqpErrorIncjectorCintStr = R"(
  cint_reset();
  bcm_switch_event_control_t event_ctrl;
  event_ctrl.event_id = 1294;  // JR3_INT_FQP_ECC_ECC_1B_ERR_INT
  event_ctrl.index = 0; /* core ID */
  event_ctrl.action = bcmSwitchEventForce;
  print bcm_switch_event_control_set(0, BCM_SWITCH_EVENT_DEVICE_INTERRUPT, event_ctrl, 0);
  print bcm_switch_event_control_set(0, BCM_SWITCH_EVENT_DEVICE_INTERRUPT, event_ctrl, 1);
  )";
    runCint(kFqpErrorIncjectorCintStr);
    WITH_RETRIES({
      auto asicErrors = getVoqAsicErrors();
      for (const auto& [idx, asicError] : asicErrors) {
        auto fqpErrors = asicError.forwardingQueueProcessorErrors().value_or(0);
        XLOG(INFO) << "Switch index: " << idx << " FQP Errors: " << fqpErrors;
        EXPECT_EVENTUALLY_GT(fqpErrors, 0);
      }
    });
  };
  verifyAcrossWarmBoots([]() {}, verify);
}

TEST_F(AgentVoqSwitchInterruptTest, allReassemblyContextsTakenError) {
  auto verify = [=, this]() {
    std::string out;
    runCmd(
        "modreg RQP_PKT_REAS_INTERRUPT_REGISTER_TEST PKT_REAS_INTERRUPT_REGISTER_TEST=0x8000\n");
    runCmd(
        "modreg RQP_PKT_REAS_INTERRUPT_REGISTER_TEST PKT_REAS_INTERRUPT_REGISTER_TEST=0x10000\n");
    WITH_RETRIES({
      auto asicErrors = getVoqAsicErrors();
      for (const auto& [idx, asicError] : asicErrors) {
        auto allReassemblyContextsTaken =
            asicError.allReassemblyContextsTaken().value_or(0);
        XLOG(INFO) << " Switch index: " << idx
                   << " All Reassemble contexts taken: "
                   << allReassemblyContextsTaken;
        EXPECT_EVENTUALLY_GT(allReassemblyContextsTaken, 0);
      }
    });
  };
  verifyAcrossWarmBoots([]() {}, verify);
}
} // namespace facebook::fboss
