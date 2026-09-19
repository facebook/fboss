// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/if/gen-cpp2/AgentHwTestCtrl.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/utils/ConfigUtils.h"

#include <folly/logging/xlog.h>
#include <thrift/lib/cpp/util/EnumUtils.h>

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

  // The INIT frame action to build into the initial config. An LLR config
  // change on an administratively enabled port is rejected (Broadcom
  // CS00012478409), so each accepted action is covered by its own fixture
  // rather than by re-applying config within a single test.
  virtual cfg::LlrFrameAction initFrameAction() const {
    return cfg::LlrFrameAction::BEST_EFFORT;
  }

  void verifyLlrProgrammed();

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
    // LLR port at another speed is added. replayCountMax, flushFrameAction and
    // ctlosTargetSpacing keep their thrift defaults (2, BLOCK, 2048);
    // initFrameAction comes from the fixture.
    llr.initFrameAction() = initFrameAction();
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

// The initial config reaching hardware without throwing is the "SAI profile
// create/bind accepted this action" assertion. The SDK rejects INIT=DISCARD and
// any non-BLOCK FLUSH at profile-create, so FLUSH stays at its BLOCK default.
void AgentHwLlrTest::verifyLlrProgrammed() {
  auto setup = []() {};
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
          initFrameAction());
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
      // Broadcom LLR stat extensions, fetched in their own read. Present
      // together or not at all, so a partial set here means the extension read
      // returned NOT_SUPPORTED on some but not all ids.
      EXPECT_TRUE(stats.llrTxEligiblePkts_().has_value());
      EXPECT_TRUE(stats.llrTxIneligiblePkts_().has_value());
      EXPECT_TRUE(stats.llrRxEligiblePkts_().has_value());
      EXPECT_TRUE(stats.llrRxIneligiblePkts_().has_value());
      EXPECT_TRUE(stats.llrTxNackReplayEvent_().has_value());
      EXPECT_TRUE(stats.llrTxTimerReplayEvent_().has_value());
      EXPECT_TRUE(stats.llrTxError_().has_value());

      // The LLR TX/RX state machine status (UE Spec 1.0.2 sections 5.1.5 and
      // 5.1.7), read from hardware for every LLR-bound port regardless of link
      // state. The read leaves both fields unset on failure, so has_value() is
      // the assertion that the SDK actually served the attribute.
      ASSERT_TRUE(stats.llrTxStatus_().has_value());
      ASSERT_TRUE(stats.llrRxStatus_().has_value());
      // Logged, not asserted. LLR state is not reproducible in this ensemble:
      // cold boot leaves every port OFF with txInit 0, while warm boot
      // transmits INITs on some ports and varies run to run in how many reach
      // ADVANCE, including none. The same code reaches ADVANCE on a cold booted
      // switch with a real link, so this is a property of the loopback rig.
      // txInit is the TX_LLR_INIT_OS count and shows whether an INIT was
      // transmitted at all (CS00012475411).
      XLOG(DBG2) << "Port " << portId << " LLR status: tx="
                 << apache::thrift::util::enumNameSafe(*stats.llrTxStatus_())
                 << " rx="
                 << apache::thrift::util::enumNameSafe(*stats.llrRxStatus_())
                 << " txInit=" << *stats.llrTxInitCtlOs_()
                 << " rxInit=" << *stats.llrRxInitCtlOs_();

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
      EXPECT_EQ(*llrInfo.initFrameAction(), initFrameAction());
      EXPECT_EQ(*llrInfo.flushFrameAction(), cfg::LlrFrameAction::BLOCK);
    }
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentHwLlrTest, verifyLlrConfig) {
  verifyLlrProgrammed();
}

// INIT=BLOCK is the case beyond the BEST_EFFORT default.
class AgentHwLlrBlockInitTest : public AgentHwLlrTest {
 protected:
  cfg::LlrFrameAction initFrameAction() const override {
    return cfg::LlrFrameAction::BLOCK;
  }
};

TEST_F(AgentHwLlrBlockInitTest, verifyLlrConfig) {
  verifyLlrProgrammed();
}

} // namespace facebook::fboss
