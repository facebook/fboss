// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/single/MonolithicHwSwitchHandler.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/lib/CommonUtils.h"

#include "folly/experimental/TestUtil.h"

namespace facebook::fboss {
class AgentVoqSwitchInterruptTest : public AgentHwTest {
 public:
  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    return {production_features::ProductionFeature::VOQ_DNX_INTERRUPTS};
  }

 protected:
  void runCint(const std::string cintStr) {
    folly::test::TemporaryFile file;
    XLOG(INFO) << " Cint file " << file.path().c_str();
    folly::writeFull(file.fd(), cintStr.c_str(), cintStr.size());
    for (auto voqSwitchId : getSw()->getSwitchInfoTable().getSwitchIdsOfType(
             cfg::SwitchType::VOQ)) {
      std::string out;
      getAgentEnsemble()->runDiagCommand(
          folly::sformat("cint {}\n", file.path().c_str()), out, voqSwitchId);
    }
  }
  std::unordered_set<uint16_t> voqIndices() const {
    return getSw()->getSwitchInfoTable().getSwitchIndicesOfType(
        cfg::SwitchType::VOQ);
  }
  std::map<uint16_t, HwAsicErrors> getVoqAsicErrors() {
    std::map<uint16_t, HwAsicErrors> asicErrors;
    auto voqIdxs = voqIndices();
    for (const auto& [switchIdx, switchStats] :
         getSw()->getHwSwitchStatsExpensive()) {
      if (voqIdxs.find(switchIdx) != voqIdxs.end()) {
        asicErrors.insert({switchIdx, *switchStats.hwAsicErrors()});
      }
    }
    return asicErrors;
  };
};

TEST_F(AgentVoqSwitchInterruptTest, ireError) {
  auto verify = [=, this]() {
    constexpr auto kIreErrorIncjectorCintStr = R"(
  cint_reset();
  bcm_switch_event_control_t event_ctrl;
  event_ctrl.event_id = 2034;
  event_ctrl.index = 0; /* core ID */
  event_ctrl.action = bcmSwitchEventForce;
  print bcm_switch_event_control_set(0, BCM_SWITCH_EVENT_DEVICE_INTERRUPT, event_ctrl, 1);
  )";
    runCint(kIreErrorIncjectorCintStr);
    WITH_RETRIES({
      auto asicErrors = getVoqAsicErrors();
      EXPECT_EVENTUALLY_EQ(asicErrors.size(), voqIndices().size());
      for (const auto& [idx, asicError] : asicErrors) {
        auto ireErrors = asicError.ingressReceiveEditorErrors().value_or(0);
        XLOG(INFO) << "Switch index: " << idx << " IRE Errors: " << ireErrors;
        EXPECT_EVENTUALLY_GT(ireErrors, 0);
      }
    });
  };
  verifyAcrossWarmBoots([]() {}, verify);
}
} // namespace facebook::fboss
