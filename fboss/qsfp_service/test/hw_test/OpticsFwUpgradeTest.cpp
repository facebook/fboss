// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>

#include <folly/logging/xlog.h>
#include "fboss/qsfp_service/test/hw_test/HwPortUtils.h"
#include "fboss/qsfp_service/test/hw_test/HwTest.h"

using namespace ::testing;

namespace facebook::fboss {

class OpticsFwUpgradeTest : public HwTest {
 public:
  OpticsFwUpgradeTest(bool setupOverrideTcvrToPortAndProfile = true)
      : HwTest(setupOverrideTcvrToPortAndProfile) {}

  void SetUp() override {
    // Enable the firmware_upgrade_supported flag for these tests. Without this,
    // firmware upgrade functionality will be skipped
    gflags::SetCommandLineOptionWithMode(
        "firmware_upgrade_supported", "1", gflags::SET_FLAGS_DEFAULT);
    // Set max concurrent evb fw upgrade to 8 for cold boot init before the test
    // starts This ensures that all the transceivers have a chance to upgrade to
    // the latest version of the firmware from the config before the actual test
    // starts later
    gflags::SetCommandLineOptionWithMode(
        "max_concurrent_evb_fw_upgrade", "8", gflags::SET_FLAGS_DEFAULT);
    HwTest::SetUp();
    // Revert the max_concurrent_evb_fw_upgrade back to 1 which is the default
    gflags::SetCommandLineOptionWithMode(
        "max_concurrent_evb_fw_upgrade", "1", gflags::SET_FLAGS_DEFAULT);
  }

  void printDebugInfo(
      TransceiverID tcvrID,
      bool upgradeExpected,
      long upgradeSinceMs,
      const TcvrStats& tcvrStats) const {
    XLOG(INFO) << "Transceiver " << tcvrID
               << " :upgradeExpected = " << upgradeExpected
               << " :lastFwUpgradeStartTime = "
               << *tcvrStats.lastFwUpgradeStartTime()
               << ", lastFwUpgradeEndTime = "
               << *tcvrStats.lastFwUpgradeEndTime()
               << " :upgradeSince = " << upgradeSinceMs;
  }

  // Helper function that verifies if an upgrade was done or not
  // Returns true if upgrade was done and was expected to be done, false
  // otherwise
  // Helper function that verifies if an upgrade was done or not
  // Returns true if upgrade was done and was expected to be done, false
  // otherwise
  bool verifyUpgrade(
      bool upgradeExpected,
      long upgradeSinceTsSec,
      std::vector<int32_t> tcvrs) {
    if (tcvrs.empty()) {
      tcvrs = utility::legacyTransceiverIds(
          utility::getCabledPortTranceivers(getHwQsfpEnsemble()));
    }
    bool result = true;
    for (auto tcvrID : tcvrs) {
      auto tcvrInfo =
          getHwQsfpEnsemble()->getWedgeManager()->getTransceiverInfo(
              TransceiverID(tcvrID));
      auto& tcvrStats = *tcvrInfo.tcvrStats();
      if (upgradeExpected &&
          (*tcvrStats.lastFwUpgradeStartTime() <= upgradeSinceTsSec ||
           *tcvrStats.lastFwUpgradeEndTime() <=
               *tcvrStats.lastFwUpgradeStartTime())) {
        printDebugInfo(
            TransceiverID(tcvrID),
            upgradeExpected,
            upgradeSinceTsSec,
            tcvrStats);
        result = false;
      } else if (
          !upgradeExpected &&
          (*tcvrStats.lastFwUpgradeStartTime() > upgradeSinceTsSec ||
           *tcvrStats.lastFwUpgradeEndTime() > upgradeSinceTsSec)) {
        printDebugInfo(
            TransceiverID(tcvrID),
            upgradeExpected,
            upgradeSinceTsSec,
            tcvrStats);
        result = false;
      }
    }
    return result;
  }

  // Helper function that sets the port status of all transceivers to the given
  // status. It does that by setting setOverrideAgentPortStatusForTesting to
  // true and then refreshing the state machines so that the state can be
  // transitioned to ACTIVE or INACTIVE states.
  void setPortStatus(bool status) {
    auto wedgeMgr = getHwQsfpEnsemble()->getWedgeManager();
    wedgeMgr->setOverrideAgentPortStatusForTesting(status, true /* enabled */);
    wedgeMgr->refreshStateMachines();

    auto cabledTransceivers = utility::legacyTransceiverIds(
        utility::getCabledPortTranceivers(getHwQsfpEnsemble()));
    for (auto id : cabledTransceivers) {
      auto curState = wedgeMgr->getCurrentState(TransceiverID(id));
      if (status) {
        EXPECT_EQ(curState, TransceiverStateMachineState::ACTIVE)
            << "Transceiver:" << id
            << " Actual: " << apache::thrift::util::enumNameSafe(curState)
            << ", Expected: ACTIVE";
      } else {
        // When forcing link down in tests, we also end up triggering
        // remediation since initial_remediate_interval is set to 0 in HwTest
        // SetUp. Since not every module can be remediated, the expected states
        // when forcing link down are either INACTIVE or XPHY_PORTS_PROGRAMMED
        // In addition, when firmware upgrade happens, the state machine
        // transitions from INACTIVE to UPGRADING to NOT_PRESENT and then
        // eventually to IPHY_PORTS_PROGRAMMED state. Therefore expect that too
        // when forcing link down
        EXPECT_TRUE(
            curState == TransceiverStateMachineState::IPHY_PORTS_PROGRAMMED ||
            curState == TransceiverStateMachineState::XPHY_PORTS_PROGRAMMED ||
            curState == TransceiverStateMachineState::INACTIVE)
            << "Transceiver:" << id
            << " Actual: " << apache::thrift::util::enumNameSafe(curState)
            << ", Expected: IPHY_PORTS_PROGRAMMED or XPHY_PORTS_PROGRAMMED or INACTIVE";
      }
    }
  }
};

TEST_F(OpticsFwUpgradeTest, noUpgradeForSameVersion) {
  // In this test, firmware versions in qsfp config is not changed. Hence, the
  // firmware upgrade shouldn't be triggered under any circumstances After
  // Coldboot init, verify there were no upgrades done during cold boot After
  // Warmboot init, verify there were no upgrades done during warm boot Force
  // links to go down and verify there are no upgrades done

  auto verify = [&]() {
    CHECK(verifyUpgrade(
        false /* upgradeExpected */, 0 /* upgradeSinceTsSec */, {} /* tcvrs */))
        << "No upgrades expected during cold/warm boot";
    // Force link up
    setPortStatus(true);
    CHECK(verifyUpgrade(
        false /* upgradeExpected */, 0 /* upgradeSinceTsSec */, {} /* tcvrs */))
        << "No upgrades expected on port up";
    // Force link down
    setPortStatus(false);
    CHECK(verifyUpgrade(
        false /* upgradeExpected */, 0 /* upgradeSinceTsSec */, {} /* tcvrs */))
        << "No upgrades expected on port down";
  };
  verifyAcrossWarmBoots([]() {}, verify);
}
} // namespace facebook::fboss
