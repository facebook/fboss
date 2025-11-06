// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/lib/CommonUtils.h"

#include "folly/testing/TestUtil.h"

namespace facebook::fboss {
class AgentVoqSwitchInterruptTest : public AgentHwTest {
 public:
  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {ProductionFeature::VOQ_DNX_INTERRUPTS};
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

class AgentVoqSwitchVendorSwitchInterruptTest
    : public AgentVoqSwitchInterruptTest {
 public:
  void setCmdLineFlagOverrides() const override {
    AgentHwTest::setCmdLineFlagOverrides();
    FLAGS_ignore_asic_hard_reset_notification = true;
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

TEST_F(AgentVoqSwitchInterruptTest, validateInterruptMaskedEventCallback) {
  auto verify = [=, this]() {
    constexpr auto kInterruptMaskedEventCintStr = R"(
      cint_reset();
      bcm_switch_event_control_t type;
      type.event_id = 2128; // JR3_INT_MACT_LARGE_EM_LARGE_EM_EVENT_FIFO_HIGH_THRESHOLD_REACHED
      type.index = 0;
      type.action = bcmSwitchEventMask;
      bcm_switch_event_control_set(0, BCM_SWITCH_EVENT_DEVICE_INTERRUPT, type, 0);
    )";
    runCint(kInterruptMaskedEventCintStr);
    WITH_RETRIES({
      for (const auto& [switchIdx, switchStats] :
           getSw()->getHwSwitchStatsExpensive()) {
        auto intrMaskedEvents =
            switchStats.fb303GlobalStats()->interrupt_masked_events();
        ASSERT_EVENTUALLY_TRUE(intrMaskedEvents.has_value());
        XLOG(INFO) << "Switch index: " << switchIdx
                   << " Interrupt masked events: " << *intrMaskedEvents;
        EXPECT_EVENTUALLY_GT(*intrMaskedEvents, 0);
      }
    });
  };
  verifyAcrossWarmBoots([]() {}, verify);
}

TEST_F(AgentVoqSwitchInterruptTest, softResetErrors) {
  auto verify = [=, this]() {
    std::string out;
    runCmd("der soft nofabric=1\n");
    runCmd("quit\n");
    WITH_RETRIES({
      auto asicErrors = getVoqAsicErrors();
      for (const auto& [idx, asicError] : asicErrors) {
        auto asicSoftResetErrors = asicError.asicSoftResetErrors().value_or(0);
        XLOG(INFO) << " Switch index: " << idx
                   << " Asic soft reset errors: " << asicSoftResetErrors;
        EXPECT_EVENTUALLY_GT(asicSoftResetErrors, 0);
      }
    });
  };
  verifyAcrossWarmBoots([]() {}, verify);
}

TEST_F(AgentVoqSwitchVendorSwitchInterruptTest, blockLevelErrorInterruptsTest) {
  auto verify = [=, this]() {
    constexpr auto kTriggerBlockWiseInterruptsCintStr = R"(
  cint_reset();
  int count = 11;
  int event_ids[count] = {2321, 521, 792, 728, 1087, 935, 2659, 1079, 2230, 1952, 2438};
  int i;
  for (i=0; i < count; i++) {
    bcm_switch_event_control_t event_ctrl;
    event_ctrl.event_id = event_ids[i];
    event_ctrl.index = 0; /* core id */
    event_ctrl.action = bcmSwitchEventForce;
    bcm_switch_event_control_set(0, BCM_SWITCH_EVENT_DEVICE_INTERRUPT, event_ctrl, 1);
  }
  sal_usleep(100000);
  for (i=0; i < count; i++) {
    bcm_switch_event_control_t event_ctrl;
    event_ctrl.event_id = event_ids[i];
    event_ctrl.index = 0; /* core id */
    event_ctrl.action = bcmSwitchEventForce;
    bcm_switch_event_control_set(0, BCM_SWITCH_EVENT_DEVICE_INTERRUPT, event_ctrl, 0);
  }
)";
    bool hardResetNotifSeen = false;
    WITH_RETRIES({
      runCint(kTriggerBlockWiseInterruptsCintStr);
      auto asicErrors = getVoqAsicErrors();
      for (const auto& [idx, asicError] : asicErrors) {
        XLOG(INFO) << "Errors for switch id " << idx;
        auto dramErrors = asicError.dramErrors().value_or(0);
        XLOG(INFO) << "DRAM errors: " << dramErrors;
        EXPECT_EVENTUALLY_GT(dramErrors, 0);
        auto egressTmErrors = asicError.egressTmErrors().value_or(0);
        XLOG(INFO) << "Egress TM errors: " << egressTmErrors;
        EXPECT_EVENTUALLY_GT(egressTmErrors, 0);
        auto fabricLinkErrors = asicError.fabricLinkErrors().value_or(0);
        XLOG(INFO) << "Fabric Link errors: " << fabricLinkErrors;
        EXPECT_EVENTUALLY_GT(fabricLinkErrors, 0);
        auto fabricRxErrors = asicError.fabricRxErrors().value_or(0);
        XLOG(INFO) << "Fabric RX errors: " << fabricRxErrors;
        EXPECT_EVENTUALLY_GT(fabricRxErrors, 0);
        auto fabricTxErrors = asicError.fabricTxErrors().value_or(0);
        XLOG(INFO) << "Fabric TX errors: " << fabricTxErrors;
        EXPECT_EVENTUALLY_GT(fabricTxErrors, 0);
        auto ingressTmErrors = asicError.ingressTmErrors().value_or(0);
        XLOG(INFO) << "Ingress TM errors: " << ingressTmErrors;
        EXPECT_EVENTUALLY_GT(ingressTmErrors, 0);
        auto ingressPpErrors = asicError.ingressPpErrors().value_or(0);
        XLOG(INFO) << "Ingress PP errors: " << ingressPpErrors;
        EXPECT_EVENTUALLY_GT(ingressPpErrors, 0);
        auto egressPpErrors = asicError.egressPpErrors().value_or(0);
        XLOG(INFO) << "Egress PP errors: " << egressPpErrors;
        EXPECT_EVENTUALLY_GT(egressPpErrors, 0);
        auto counterAndMeterErrors =
            asicError.counterAndMeterErrors().value_or(0);
        XLOG(INFO) << "Counter and Meter errors: " << counterAndMeterErrors;
        EXPECT_EVENTUALLY_GT(counterAndMeterErrors, 0);
        auto fabricTopologyErrors =
            asicError.fabricTopologyErrors().value_or(0);
        XLOG(INFO) << "Fabric Topology errors: " << fabricTopologyErrors;
        EXPECT_EVENTUALLY_GT(fabricTopologyErrors, 0);
        auto networkInterfaceErrors =
            asicError.networkInterfaceErrors().value_or(0);
        XLOG(INFO) << "Network Interface errors: " << networkInterfaceErrors;
        EXPECT_EVENTUALLY_GT(networkInterfaceErrors, 0);
      }
    });
    // Make sure that hard reset notification callback is working as expected!
    // Hard reset notification callback should result from the interrupt ID
    // 1952 used in this test.
    for (const auto& [switchId, asic] : getAsics()) {
      auto switchIndex =
          getSw()->getSwitchInfoTable().getSwitchIndexFromSwitchId(switchId);
      int hardResetNotificationReceived =
          *getSw()
               ->getHwSwitchStatsExpensive()[switchIndex]
               .hardResetStats()
               ->hard_reset_notification_received();
      if (hardResetNotificationReceived) {
        hardResetNotifSeen = true;
      }
    }
    EXPECT_TRUE(hardResetNotifSeen);
  };
  verifyAcrossWarmBoots([]() {}, verify);
}
} // namespace facebook::fboss
