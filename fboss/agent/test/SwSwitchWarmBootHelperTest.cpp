// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/SwSwitchWarmBootHelper.h"

#include <folly/FileUtil.h>
#include <folly/logging/xlog.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <filesystem>

#include "fboss/agent/AgentDirectoryUtil.h"
#include "fboss/agent/FileBasedWarmbootUtils.h"
#include "fboss/agent/HwAsicTable.h"
#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/lib/CommonFileUtils.h"

namespace facebook::fboss {

class SwSwitchWarmBootHelperTest : public ::testing::Test {
 public:
  void SetUp() override {
    // Save original flag values
    savedCanWarmBoot_ = FLAGS_can_warm_boot;
    savedThriftSwitchStateFile_ = FLAGS_thrift_switch_state_file;
    savedRecoverFromHwSwitch_ = FLAGS_recover_from_hw_switch;

    // Create a temporary directory for test files
    tempDir_ = folly::to<std::string>(
        "/tmp/SwSwitchWarmBootHelperTest_", ::getpid(), "_", testCounter_++);
    createDir(tempDir_);

    // Create AgentDirectoryUtil with temp directory
    directoryUtil_ = std::make_unique<AgentDirectoryUtil>(
        tempDir_ + "/volatile",
        tempDir_ + "/persistent",
        tempDir_ + "/packages",
        tempDir_ + "/systemd",
        tempDir_ + "/config",
        tempDir_ + "/drain");

    // Create the warm boot directory
    createDir(directoryUtil_->getWarmBootDir());

    // Create HwAsicTable with single NPU switch
    std::map<int64_t, cfg::SwitchInfo> switchIdToSwitchInfo;
    cfg::SwitchInfo switchInfo;
    switchInfo.switchIndex() = 0;
    switchInfo.switchType() = cfg::SwitchType::NPU;
    switchInfo.asicType() = cfg::AsicType::ASIC_TYPE_MOCK;
    switchIdToSwitchInfo[0] = switchInfo;

    asicTable_ = std::make_unique<HwAsicTable>(
        switchIdToSwitchInfo, std::nullopt, std::map<int64_t, cfg::DsfNode>());

    // Create mock thrift client table
    testThriftClientTable_ =
        std::make_unique<HwSwitchThriftClientTableForTesting>(
            0, switchIdToSwitchInfo);

    // Reset flags to defaults
    FLAGS_can_warm_boot = true;
    FLAGS_thrift_switch_state_file = "thrift_switch_state";
    FLAGS_recover_from_hw_switch = false;
  }

  void TearDown() override {
    // Restore original flag values
    FLAGS_can_warm_boot = savedCanWarmBoot_;
    FLAGS_thrift_switch_state_file = savedThriftSwitchStateFile_;
    FLAGS_recover_from_hw_switch = savedRecoverFromHwSwitch_;

    // Clean up temp directory
    std::filesystem::remove_all(tempDir_);
  }

  void createTestFile(const std::string& path) {
    folly::writeFileAtomic(path, "test");
  }

  void createTestWarmbootState(const std::string& path) {
    state::WarmbootState warmbootState;
    state::SwitchState switchState;
    warmbootState.swSwitchState() = switchState;

    std::map<int32_t, state::RouteTableFields> routeTables;
    routeTables.emplace(0, state::RouteTableFields{});
    warmbootState.routeTables() = routeTables;

    auto serialized =
        apache::thrift::BinarySerializer::serialize<std::string>(warmbootState);
    folly::writeFileAtomic(path, serialized);
  }

  void setupForWarmBootFromFile() {
    // Create warmboot flag
    auto warmbootFlagPath = directoryUtil_->getSwSwitchCanWarmBootFile();
    createDir(std::filesystem::path(warmbootFlagPath).parent_path().string());
    createTestFile(warmbootFlagPath);

    // Create warmboot state file
    auto stateFilePath = folly::to<std::string>(
        directoryUtil_->getWarmBootDir(), "/", FLAGS_thrift_switch_state_file);
    createTestWarmbootState(stateFilePath);
  }

  void setupForWarmBootFromThrift() {
    FLAGS_recover_from_hw_switch = true;
    testThriftClientTable_->setRunState(SwitchRunState::CONFIGURED);
    state::SwitchState mockState;
    testThriftClientTable_->setProgrammedState(mockState);
  }

 protected:
  std::string tempDir_;
  std::unique_ptr<AgentDirectoryUtil> directoryUtil_;
  std::unique_ptr<HwAsicTable> asicTable_;
  std::unique_ptr<HwSwitchThriftClientTableForTesting> testThriftClientTable_;

  bool savedCanWarmBoot_{false};
  std::string savedThriftSwitchStateFile_;
  bool savedRecoverFromHwSwitch_{false};
  static int testCounter_;
};

int SwSwitchWarmBootHelperTest::testCounter_ = 0;

// ============================================================================
// canWarmBootFromFile TESTS - via canWarmBoot() interface
// ============================================================================

TEST_F(
    SwSwitchWarmBootHelperTest,
    CanWarmBootFromFileSuccessWhenAllConditionsMet) {
  setupForWarmBootFromFile();

  SwSwitchWarmBootHelper helper(directoryUtil_.get(), asicTable_.get());
  // isRunModeMultiSwitch = false, so thrift path won't be tried
  bool result = helper.canWarmBoot(false, nullptr);

  EXPECT_TRUE(result);
}

TEST_F(SwSwitchWarmBootHelperTest, CanWarmBootFromFileFailsWhenForceColdBoot) {
  setupForWarmBootFromFile();

  // Create force cold boot flag
  auto forceColdBootPath = directoryUtil_->getSwColdBootOnceFile();
  createTestFile(forceColdBootPath);

  SwSwitchWarmBootHelper helper(directoryUtil_.get(), asicTable_.get());
  bool result = helper.canWarmBoot(false, nullptr);

  EXPECT_FALSE(result);
}

TEST_F(
    SwSwitchWarmBootHelperTest,
    CanWarmBootFromFileFailsWhenCanWarmBootFlagNotSet) {
  // Setup state file but NOT warmboot flag
  auto stateFilePath = folly::to<std::string>(
      directoryUtil_->getWarmBootDir(), "/", FLAGS_thrift_switch_state_file);
  createTestWarmbootState(stateFilePath);

  SwSwitchWarmBootHelper helper(directoryUtil_.get(), asicTable_.get());
  bool result = helper.canWarmBoot(false, nullptr);

  EXPECT_FALSE(result);
}

TEST_F(
    SwSwitchWarmBootHelperTest,
    CanWarmBootFromFileFailsWhenFlagCanWarmBootDisabled) {
  setupForWarmBootFromFile();
  FLAGS_can_warm_boot = false;

  SwSwitchWarmBootHelper helper(directoryUtil_.get(), asicTable_.get());
  bool result = helper.canWarmBoot(false, nullptr);

  EXPECT_FALSE(result);
}

TEST_F(
    SwSwitchWarmBootHelperTest,
    CanWarmBootFromFileFailsWhenStateFileNotExists) {
  // Setup warmboot flag but NOT state file
  auto warmbootFlagPath = directoryUtil_->getSwSwitchCanWarmBootFile();
  createDir(std::filesystem::path(warmbootFlagPath).parent_path().string());
  createTestFile(warmbootFlagPath);

  SwSwitchWarmBootHelper helper(directoryUtil_.get(), asicTable_.get());

  // This should crash with CHECK failure since state file doesn't exist
  // when other conditions are met
  EXPECT_DEATH(helper.canWarmBoot(false, nullptr), "");
}

TEST_F(
    SwSwitchWarmBootHelperTest,
    CanWarmBootFromFileWithCustomThriftStateFile) {
  FLAGS_thrift_switch_state_file = "custom_state_file";
  setupForWarmBootFromFile();

  SwSwitchWarmBootHelper helper(directoryUtil_.get(), asicTable_.get());
  bool result = helper.canWarmBoot(false, nullptr);

  EXPECT_TRUE(result);
}

// ============================================================================
// canWarmBootFromThrift TESTS - via canWarmBoot() interface
// ============================================================================

TEST_F(
    SwSwitchWarmBootHelperTest,
    CanWarmBootFromThriftSuccessWhenHwSwitchConfigured) {
  setupForWarmBootFromThrift();

  SwSwitchWarmBootHelper helper(directoryUtil_.get(), asicTable_.get());
  // isRunModeMultiSwitch = true to trigger thrift path
  bool result = helper.canWarmBoot(true, testThriftClientTable_.get());

  EXPECT_TRUE(result);
}

TEST_F(
    SwSwitchWarmBootHelperTest,
    CanWarmBootFromThriftFailsWhenNotMultiSwitchMode) {
  setupForWarmBootFromThrift();

  SwSwitchWarmBootHelper helper(directoryUtil_.get(), asicTable_.get());
  // isRunModeMultiSwitch = false, should not try thrift path
  bool result = helper.canWarmBoot(false, testThriftClientTable_.get());

  EXPECT_FALSE(result);
}

TEST_F(
    SwSwitchWarmBootHelperTest,
    CanWarmBootFromThriftFailsWhenHwSwitchNotConfigured) {
  FLAGS_recover_from_hw_switch = true;
  testThriftClientTable_->setRunState(SwitchRunState::INITIALIZED);

  SwSwitchWarmBootHelper helper(directoryUtil_.get(), asicTable_.get());
  bool result = helper.canWarmBoot(true, testThriftClientTable_.get());

  EXPECT_FALSE(result);
}

TEST_F(
    SwSwitchWarmBootHelperTest,
    CanWarmBootFromThriftFailsWhenHwSwitchExiting) {
  FLAGS_recover_from_hw_switch = true;
  testThriftClientTable_->setRunState(SwitchRunState::EXITING);

  SwSwitchWarmBootHelper helper(directoryUtil_.get(), asicTable_.get());
  bool result = helper.canWarmBoot(true, testThriftClientTable_.get());

  EXPECT_FALSE(result);
}

TEST_F(
    SwSwitchWarmBootHelperTest,
    CanWarmBootFromThriftFailsWhenGetRunStateThrows) {
  FLAGS_recover_from_hw_switch = true;
  testThriftClientTable_->setRunState(SwitchRunState::CONFIGURED);
  testThriftClientTable_->setShouldThrowOnGetRunState(true);

  SwSwitchWarmBootHelper helper(directoryUtil_.get(), asicTable_.get());
  bool result = helper.canWarmBoot(true, testThriftClientTable_.get());

  EXPECT_FALSE(result);
}

TEST_F(
    SwSwitchWarmBootHelperTest,
    CanWarmBootFromThriftFailsWhenGetProgrammedStateThrows) {
  FLAGS_recover_from_hw_switch = true;
  testThriftClientTable_->setRunState(SwitchRunState::CONFIGURED);
  testThriftClientTable_->setShouldThrowOnGetProgrammedState(true);

  SwSwitchWarmBootHelper helper(directoryUtil_.get(), asicTable_.get());
  bool result = helper.canWarmBoot(true, testThriftClientTable_.get());

  EXPECT_FALSE(result);
}

TEST_F(
    SwSwitchWarmBootHelperTest,
    CanWarmBootFromThriftFailsWhenFlagRecoverFromHwSwitchDisabled) {
  FLAGS_recover_from_hw_switch = false;
  testThriftClientTable_->setRunState(SwitchRunState::CONFIGURED);

  SwSwitchWarmBootHelper helper(directoryUtil_.get(), asicTable_.get());
  bool result = helper.canWarmBoot(true, testThriftClientTable_.get());

  EXPECT_FALSE(result);
}

TEST_F(
    SwSwitchWarmBootHelperTest,
    CanWarmBootFromThriftWithMultipleSwitchesFails) {
  FLAGS_recover_from_hw_switch = true;
  testThriftClientTable_->setRunState(SwitchRunState::CONFIGURED);

  // Create multi-switch asic table
  std::map<int64_t, cfg::SwitchInfo> switchIdToSwitchInfo;
  for (int i = 0; i < 2; i++) {
    cfg::SwitchInfo switchInfo;
    switchInfo.switchIndex() = i;
    switchInfo.switchType() = cfg::SwitchType::NPU;
    switchInfo.asicType() = cfg::AsicType::ASIC_TYPE_MOCK;
    switchIdToSwitchInfo[i] = switchInfo;
  }
  auto multiSwitchAsicTable = std::make_unique<HwAsicTable>(
      switchIdToSwitchInfo, std::nullopt, std::map<int64_t, cfg::DsfNode>());

  SwSwitchWarmBootHelper helper(
      directoryUtil_.get(), multiSwitchAsicTable.get());
  bool result = helper.canWarmBoot(true, testThriftClientTable_.get());

  // Warmboot from multiple HW Agents is not supported yet.
  // TODO(zecheng): Update this UT when multi HW Agent is supported.
  EXPECT_FALSE(result);
}

// ============================================================================
// PRIORITY TESTS - File vs Thrift
// ============================================================================

TEST_F(
    SwSwitchWarmBootHelperTest,
    CanWarmBootPrefersFileOverThriftWhenBothAvailable) {
  // Setup both file-based and thrift-based warmboot
  setupForWarmBootFromFile();
  setupForWarmBootFromThrift();

  SwSwitchWarmBootHelper helper(directoryUtil_.get(), asicTable_.get());

  // Even in multi-switch mode, file-based warmboot takes precedence
  bool result = helper.canWarmBoot(true, testThriftClientTable_.get());

  EXPECT_TRUE(result);
  // Verify file-based warmboot was used, not thrift-based
  EXPECT_FALSE(helper.isWarmBootFromHwSwitch());
}

TEST_F(
    SwSwitchWarmBootHelperTest,
    CanWarmBootFallsBackToThriftWhenFileNotAvailable) {
  // Only setup thrift-based warmboot (no file-based warmboot)
  setupForWarmBootFromThrift();

  SwSwitchWarmBootHelper helper(directoryUtil_.get(), asicTable_.get());
  bool result = helper.canWarmBoot(true, testThriftClientTable_.get());

  EXPECT_TRUE(result);
  // Verify thrift-based warmboot was used
  EXPECT_TRUE(helper.isWarmBootFromHwSwitch());
}

// ============================================================================
// FORCE COLD BOOT TESTS
// ============================================================================

TEST_F(SwSwitchWarmBootHelperTest, ForceColdBootOverridesFileWarmBoot) {
  setupForWarmBootFromFile();

  // Create force cold boot flag
  auto forceColdBootPath = directoryUtil_->getSwColdBootOnceFile();
  createTestFile(forceColdBootPath);

  SwSwitchWarmBootHelper helper(directoryUtil_.get(), asicTable_.get());
  bool result = helper.canWarmBoot(false, nullptr);

  EXPECT_FALSE(result);
}

TEST_F(SwSwitchWarmBootHelperTest, ForceColdBootOverridesThriftWarmBoot) {
  setupForWarmBootFromThrift();

  // Create force cold boot flag
  auto forceColdBootPath = directoryUtil_->getSwColdBootOnceFile();
  createTestFile(forceColdBootPath);

  SwSwitchWarmBootHelper helper(directoryUtil_.get(), asicTable_.get());
  bool result = helper.canWarmBoot(true, testThriftClientTable_.get());

  EXPECT_FALSE(result);
}

TEST_F(SwSwitchWarmBootHelperTest, LegacyForceColdBootFlagAlsoWorks) {
  setupForWarmBootFromFile();

  // Create legacy force cold boot flag
  auto legacyForceColdBootPath =
      getForceColdBootOnceFlagLegacy(directoryUtil_.get());
  createTestFile(legacyForceColdBootPath);

  SwSwitchWarmBootHelper helper(directoryUtil_.get(), asicTable_.get());
  bool result = helper.canWarmBoot(false, nullptr);

  EXPECT_FALSE(result);
}

// ============================================================================
// FLAG OVERRIDE TESTS
// ============================================================================

TEST_F(SwSwitchWarmBootHelperTest, CanWarmBootFlagFalseOverridesEverything) {
  setupForWarmBootFromFile();
  setupForWarmBootFromThrift();
  FLAGS_can_warm_boot = false;

  SwSwitchWarmBootHelper helper(directoryUtil_.get(), asicTable_.get());
  bool result = helper.canWarmBoot(true, testThriftClientTable_.get());

  EXPECT_FALSE(result);
}

// ============================================================================
// WARMBOOT DIR TESTS
// ============================================================================

TEST_F(SwSwitchWarmBootHelperTest, WarmBootDirReturnedCorrectly) {
  SwSwitchWarmBootHelper helper(directoryUtil_.get(), asicTable_.get());

  EXPECT_EQ(helper.warmBootDir(), directoryUtil_->getWarmBootDir());
}

TEST_F(SwSwitchWarmBootHelperTest, WarmBootThriftSwitchStateFileCorrect) {
  SwSwitchWarmBootHelper helper(directoryUtil_.get(), asicTable_.get());

  auto expectedPath = folly::to<std::string>(
      directoryUtil_->getWarmBootDir(), "/", FLAGS_thrift_switch_state_file);
  EXPECT_EQ(helper.warmBootThriftSwitchStateFile(), expectedPath);
}

// ============================================================================
// STORE AND GET WARMBOOT STATE TESTS
// ============================================================================

TEST_F(SwSwitchWarmBootHelperTest, StoreAndGetWarmBootState) {
  SwSwitchWarmBootHelper helper(directoryUtil_.get(), asicTable_.get());

  // Create a warmboot state
  state::WarmbootState warmbootState;
  state::SwitchState switchState;
  warmbootState.swSwitchState() = switchState;

  std::map<int32_t, state::RouteTableFields> routeTables;
  routeTables.emplace(0, state::RouteTableFields{});
  warmbootState.routeTables() = routeTables;

  // Store it
  helper.storeWarmBootState(warmbootState);

  // Get it back
  auto retrievedState = helper.getWarmBootState();
  EXPECT_EQ(retrievedState.routeTables()->size(), 1);
}

TEST_F(SwSwitchWarmBootHelperTest, StoreWarmBootStateSetsCanWarmBootFlag) {
  SwSwitchWarmBootHelper helper(directoryUtil_.get(), asicTable_.get());

  state::WarmbootState warmbootState;
  helper.storeWarmBootState(warmbootState);

  // Check that warmboot flag was created
  auto warmbootFlagPath = directoryUtil_->getSwSwitchCanWarmBootFile();
  EXPECT_TRUE(checkFileExists(warmbootFlagPath));
}

// ============================================================================
// MULTIPLE CONSECUTIVE CANWARMBOOT CALLS TESTS
// ============================================================================

TEST_F(SwSwitchWarmBootHelperTest, WarmBootFlagConsumedAfterConstruction) {
  setupForWarmBootFromFile();

  // First helper consumes the warmboot flags during construction
  SwSwitchWarmBootHelper helper1(directoryUtil_.get(), asicTable_.get());
  bool result1 = helper1.canWarmBoot(false, nullptr);
  EXPECT_TRUE(result1);

  // Second helper sees no flags — they were consumed by the first
  SwSwitchWarmBootHelper helper2(directoryUtil_.get(), asicTable_.get());
  bool result2 = helper2.canWarmBoot(false, nullptr);
  EXPECT_FALSE(result2);
}

TEST_F(SwSwitchWarmBootHelperTest, ConsecutiveCanWarmBootCallsFromThrift) {
  setupForWarmBootFromThrift();

  SwSwitchWarmBootHelper helper(directoryUtil_.get(), asicTable_.get());

  // Multiple calls should return consistent results
  bool result1 = helper.canWarmBoot(true, testThriftClientTable_.get());
  bool result2 = helper.canWarmBoot(true, testThriftClientTable_.get());

  EXPECT_TRUE(result1);
  EXPECT_TRUE(result2);
}

// ============================================================================
// ASIC WARMBOOT SUPPORT TESTS
// ============================================================================

TEST_F(
    SwSwitchWarmBootHelperTest,
    CanWarmBootFailsWhenAsicDoesNotSupportWarmboot) {
  // Create asic table with ASIC type that doesn't support warmboot
  std::map<int64_t, cfg::SwitchInfo> switchIdToSwitchInfo;
  cfg::SwitchInfo switchInfo;
  switchInfo.switchIndex() = 0;
  switchInfo.switchType() = cfg::SwitchType::NPU;
  switchInfo.asicType() = cfg::AsicType::ASIC_TYPE_FAKE_NO_WARMBOOT;
  switchIdToSwitchInfo[0] = switchInfo;

  auto fakeAsicTable = std::make_unique<HwAsicTable>(
      switchIdToSwitchInfo, std::nullopt, std::map<int64_t, cfg::DsfNode>());

  setupForWarmBootFromFile();

  SwSwitchWarmBootHelper helper(directoryUtil_.get(), fakeAsicTable.get());
  bool result = helper.canWarmBoot(false, testThriftClientTable_.get());

  EXPECT_FALSE(result);
}

} // namespace facebook::fboss
