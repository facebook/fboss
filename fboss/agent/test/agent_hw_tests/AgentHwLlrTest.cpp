// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/if/gen-cpp2/AgentHwTestCtrl.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/utils/ConfigUtils.h"

namespace facebook::fboss {

// AgentHwTest for UEC Link Layer Retry (UE Spec 1.0.2 section 5.1). LLR is a
// Tomahawk Ultra feature today; getProductionFeaturesVerified() gates these
// tests to LLR-capable ASICs through the test runner.
class AgentHwLlrTest : public AgentHwTest {
 public:
  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {ProductionFeature::LINK_LAYER_RETRANSMISSION};
  }

 protected:
  static constexpr auto kLlrConfigName = "llr_default";

  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto cfg = AgentHwTest::initialConfig(ensemble);
    // Only attach LLR config on ASICs that support it; otherwise
    // ApplyThriftConfig rejects the config (loud rejection).
    if (ensemble.getSw()->getHwAsicTable()->isFeatureSupportedOnAllAsic(
            HwAsic::Feature::LINK_LAYER_RETRANSMISSION)) {
      addLlrConfig(cfg, ensemble);
    }
    return cfg;
  }

  void addLlrConfig(cfg::SwitchConfig& cfg, const AgentEnsemble& ensemble)
      const {
    cfg::LlrConfig llr;
    // 400G TU1 UEC LLR profile mirroring the production COOP values, sized from
    // the Broadcom BCM78920 LLR Reach Calculator (checked in at
    // fboss/agent/facebook/wiki/BCM78920_LLR_ReachCalculator_v5.1.html). At
    // 400G (MTU 9000, ACK 2048): External Budget = 2708ns usable buffer - 132
    // PHY
    // - 2x180 MTU - 2x41 ACK - 8 CtlOS - 5.1 header - 5.1 AM ~= 2116ns; x 50
    // B/ns ~= 105800 B. outstandingBytesMax is the outstanding (un-acked)
    // window; the full 135400 B usable buffer leaves no room for those reserved
    // overheads and hangs the profile program, so External Budget is the HW
    // ceiling; 105800 is bind-validated on TU1 (cold+warm).
    // outstandingFramesMax = ceil(105800 / 64B min frame) = 1654 so
    // bytes stays the binding limit. LLR is TU1-400G only today; revisit if an
    // LLR port at another speed is added. replayCountMax, frame actions and
    // ctlosTargetSpacing keep their thrift defaults (2, init BEST_EFFORT, flush
    // BLOCK, ctlos 2048).
    llr.outstandingFramesMax() = 1654;
    llr.outstandingBytesMax() = 105800;
    llr.replayTimerMax() = 5000; // ns
    cfg.llrConfigs() = {{kLlrConfigName, llr}};
    for (const auto& portId : ensemble.masterLogicalInterfacePortIds()) {
      auto portCfg = utility::findCfgPort(cfg, portId);
      portCfg->llrConfigName() = kLlrConfigName;
    }
  }
};

// Verify LLR config -- including each accepted INIT frame action -- applies to
// ports, the per-port LLR counters are collected, and all of it survives a warm
// boot. initFrameAction is swept BEST_EFFORT then BLOCK (BLOCK last, so the
// "beyond BEST_EFFORT" case is the one carried across the warm boot and read
// back); each applyNewConfig that reaches hardware without throwing is the "SAI
// profile create/bind accepted this action" assertion. The SDK rejects
// INIT=DISCARD and any non-BLOCK FLUSH at profile-create, so FLUSH stays at its
// BLOCK default.
TEST_F(AgentHwLlrTest, verifyLlrConfig) {
  const std::vector<cfg::LlrFrameAction> kInitActions = {
      cfg::LlrFrameAction::BEST_EFFORT, cfg::LlrFrameAction::BLOCK};
  auto setup = [&]() {
    for (auto action : kInitActions) {
      auto cfg = initialConfig(*getAgentEnsemble());
      (*cfg.llrConfigs())[kLlrConfigName].initFrameAction() = action;
      applyNewConfig(cfg);
    }
  };
  auto verify = [&]() {
    auto state = getProgrammedState();
    auto portStats = getLatestPortStats(masterLogicalInterfacePortIds());
    for (const auto& portId : masterLogicalInterfacePortIds()) {
      auto port = state->getPorts()->getNodeIf(portId);
      ASSERT_NE(port, nullptr);
      ASSERT_TRUE(port->getLlrConfigName().has_value());
      EXPECT_EQ(*port->getLlrConfigName(), kLlrConfigName);
      ASSERT_TRUE(port->getLlrConfig().has_value());
      // replayCountMax defaults to 2 (thrift default / Meta sim
      // recommendation).
      EXPECT_EQ(port->getLlrConfig().value()->getReplayCountMax(), 2);
      EXPECT_EQ(
          port->getLlrConfig().value()->getInitFrameAction(),
          kInitActions.back());
      EXPECT_EQ(
          port->getLlrConfig().value()->getFlushFrameAction(),
          cfg::LlrFrameAction::BLOCK);

      // Every TU1-supported per-port LLR counter is collected into HwPortStats.
      // Counters read 0 without induced traffic, but each must be present once
      // LLR is bound. The 4 stats with no SDK backing on Tomahawk Ultra 1
      // (RX_BAD, TX_DISCARD, TX_POISONED, RX_POISONED) are neither fetched nor
      // asserted (Broadcom CS00012472055).
      const auto& stats = portStats.at(portId);
      EXPECT_TRUE(stats.llrTxOk_().has_value());
      EXPECT_TRUE(stats.llrTxReplay_().has_value());
      EXPECT_TRUE(stats.llrRxOk_().has_value());
      EXPECT_TRUE(stats.llrRxMissingSeq_().has_value());
      EXPECT_TRUE(stats.llrRxDuplicateSeq_().has_value());
      EXPECT_TRUE(stats.llrRxAckNackSeqError_().has_value());
      EXPECT_TRUE(stats.llrRxReplay_().has_value());
      // Additional Table 5-13 CtlOS and expected-sequence counters.
      EXPECT_TRUE(stats.llrTxInitCtlOs_().has_value());
      EXPECT_TRUE(stats.llrTxInitEchoCtlOs_().has_value());
      EXPECT_TRUE(stats.llrTxAckCtlOs_().has_value());
      EXPECT_TRUE(stats.llrTxNackCtlOs_().has_value());
      EXPECT_TRUE(stats.llrRxInitCtlOs_().has_value());
      EXPECT_TRUE(stats.llrRxInitEchoCtlOs_().has_value());
      EXPECT_TRUE(stats.llrRxAckCtlOs_().has_value());
      EXPECT_TRUE(stats.llrRxNackCtlOs_().has_value());
      EXPECT_TRUE(stats.llrRxExpectedSeqGood_().has_value());
      EXPECT_TRUE(stats.llrRxExpectedSeqPoisoned_().has_value());
      EXPECT_TRUE(stats.llrRxExpectedSeqBad_().has_value());

      // Read the LLR binding back from hardware (via the HwAgent process, so
      // this works in both mono and multi-switch). This confirms the SAI
      // profile object still exists, is bound to the port, and carries the
      // last-applied frame actions after the warm boot -- not just that the
      // SwitchState intent survived.
      auto switchId =
          getAgentEnsemble()->scopeResolver().scope(portId).switchId();
      auto client = getAgentEnsemble()->getHwAgentTestClient(switchId);
      utility::PortLlrInfo llrInfo;
      client->sync_getPortLlrInfo(llrInfo, portId);
      EXPECT_TRUE(*llrInfo.hasProfile());
      EXPECT_NE(*llrInfo.profileId(), 0);
      EXPECT_EQ(*llrInfo.initFrameAction(), kInitActions.back());
      EXPECT_EQ(*llrInfo.flushFrameAction(), cfg::LlrFrameAction::BLOCK);
    }
  };
  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
