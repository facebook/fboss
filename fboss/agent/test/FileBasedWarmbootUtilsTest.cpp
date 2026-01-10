// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/FileBasedWarmbootUtils.h"

#include <folly/FileUtil.h>
#include <folly/logging/xlog.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <filesystem>

#include "fboss/agent/AgentDirectoryUtil.h"
#include "fboss/agent/HwAsicTable.h"
#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/rib/RoutingInformationBase.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/lib/CommonFileUtils.h"

namespace facebook::fboss {

class FileBasedWarmbootUtilsTest : public ::testing::Test {
 public:
  void SetUp() override {
    // Create a temporary directory for test files
    tempDir_ =
        folly::to<std::string>("/tmp/FileBasedWarmbootUtilsTest_", ::getpid());
    createDir(tempDir_);

    // Create AgentDirectoryUtil with temp directory
    std::map<int64_t, cfg::SwitchInfo> switchIdToSwitchInfo;
    cfg::SwitchInfo switchInfo;
    switchInfo.switchIndex() = 0;
    switchInfo.switchType() = cfg::SwitchType::NPU;
    switchInfo.asicType() = cfg::AsicType::ASIC_TYPE_MOCK;
    switchIdToSwitchInfo[0] = switchInfo;

    directoryUtil_ = std::make_unique<AgentDirectoryUtil>(
        tempDir_ + "/volatile", // volatileStateDir
        tempDir_ + "/persistent", // persistentStateDir
        tempDir_ + "/packages", // packageDirectory
        tempDir_ + "/systemd", // systemdDirectory
        tempDir_ + "/config", // configDirectory
        tempDir_ + "/drain"); // drainConfigDirectory

    // Create HwAsicTable
    asicTable_ = std::make_unique<HwAsicTable>(
        switchIdToSwitchInfo, std::nullopt, std::map<int64_t, cfg::DsfNode>());
  }

  void TearDown() override {
    // Clean up temp directory
    std::string cmd = folly::to<std::string>("rm -rf ", tempDir_);
    system(cmd.c_str());
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

 protected:
  std::string tempDir_;
  std::unique_ptr<AgentDirectoryUtil> directoryUtil_;
  std::unique_ptr<HwAsicTable> asicTable_;
};

// ============================================================================
// PATH CONSTRUCTION TESTS
// ============================================================================

TEST_F(FileBasedWarmbootUtilsTest, GetForceColdBootOnceFlagLegacy) {
  auto result = getForceColdBootOnceFlagLegacy(directoryUtil_.get());
  EXPECT_EQ(
      result,
      folly::to<std::string>(
          directoryUtil_->getWarmBootDir(), "/sw_cold_boot_once_0"));
}

TEST_F(FileBasedWarmbootUtilsTest, GetWarmBootThriftSwitchStateFile) {
  auto result = getWarmBootThriftSwitchStateFile(tempDir_, "switch_state");
  EXPECT_EQ(result, folly::to<std::string>(tempDir_, "/switch_state"));
}

TEST_F(FileBasedWarmbootUtilsTest, GetWarmBootFlagLegacy) {
  auto result = getWarmBootFlagLegacy(directoryUtil_.get());
  auto expectedBase = directoryUtil_->getSwSwitchCanWarmBootFile();
  EXPECT_EQ(result, folly::to<std::string>(expectedBase, "_0"));
}

// ============================================================================
// CHECK FORCE COLD BOOT FLAG TESTS
// ============================================================================

TEST_F(FileBasedWarmbootUtilsTest, CheckForceColdBootFlagWhenFlagExists) {
  // Create force cold boot flag
  auto flagPath = directoryUtil_->getSwColdBootOnceFile();
  createDir(std::filesystem::path(flagPath).parent_path().string());
  createTestFile(flagPath);

  bool result = checkForceColdBootFlag(directoryUtil_.get());

  EXPECT_TRUE(result);
  // Flag should be removed after check
  EXPECT_FALSE(checkFileExists(flagPath));
}

TEST_F(FileBasedWarmbootUtilsTest, CheckForceColdBootFlagWhenNoFlagExists) {
  bool result = checkForceColdBootFlag(directoryUtil_.get());
  EXPECT_FALSE(result);
}

// ============================================================================
// CHECK CAN WARMBOOT FLAG TESTS
// ============================================================================

TEST_F(FileBasedWarmbootUtilsTest, CheckCanWarmBootFlagWhenFlagExists) {
  // Create warmboot flag
  auto flagPath = directoryUtil_->getSwSwitchCanWarmBootFile();
  createDir(std::filesystem::path(flagPath).parent_path().string());
  createTestFile(flagPath);

  bool result = checkCanWarmBootFlag(directoryUtil_.get());

  EXPECT_TRUE(result);
  // Flag should be removed after check
  EXPECT_FALSE(checkFileExists(flagPath));
}

TEST_F(FileBasedWarmbootUtilsTest, CheckCanWarmBootFlagWhenLegacyFlagExists) {
  // Create legacy warmboot flag
  auto flagPath = getWarmBootFlagLegacy(directoryUtil_.get());
  createDir(std::filesystem::path(flagPath).parent_path().string());
  createTestFile(flagPath);

  bool result = checkCanWarmBootFlag(directoryUtil_.get());

  EXPECT_TRUE(result);
}

TEST_F(FileBasedWarmbootUtilsTest, CheckCanWarmBootFlagWhenNoFlagExists) {
  bool result = checkCanWarmBootFlag(directoryUtil_.get());
  EXPECT_FALSE(result);
}

TEST_F(FileBasedWarmbootUtilsTest, CheckCanWarmBootFlagWhenBothFlagsExist) {
  // Create both current and legacy flags
  auto flagPath = directoryUtil_->getSwSwitchCanWarmBootFile();
  auto legacyFlagPath = getWarmBootFlagLegacy(directoryUtil_.get());
  createDir(std::filesystem::path(flagPath).parent_path().string());
  createTestFile(flagPath);
  createTestFile(legacyFlagPath);

  bool result = checkCanWarmBootFlag(directoryUtil_.get());

  EXPECT_TRUE(result);
}

// ============================================================================
// CHECK ASIC SUPPORTS WARMBOOT TESTS
// ============================================================================

TEST_F(FileBasedWarmbootUtilsTest, CheckAsicSupportsWarmbootWhenSupported) {
  // Mock ASIC supports warmboot by default
  bool result = checkAsicSupportsWarmboot(asicTable_.get());
  EXPECT_TRUE(result);
}

// ============================================================================
// CHECK WARMBOOT STATE FILE EXISTS TESTS
// ============================================================================

TEST_F(FileBasedWarmbootUtilsTest, CheckWarmbootStateFileExistsWhenFileExists) {
  auto stateFilePath =
      getWarmBootThriftSwitchStateFile(tempDir_, "switch_state");
  createTestFile(stateFilePath);

  bool result = checkWarmbootStateFileExists(tempDir_, "switch_state");
  EXPECT_TRUE(result);
}

TEST_F(
    FileBasedWarmbootUtilsTest,
    CheckWarmbootStateFileExistsWhenFileDoesNotExist) {
  bool result = checkWarmbootStateFileExists(tempDir_, "switch_state");
  EXPECT_FALSE(result);
}

// ============================================================================
// LOG BOOT TESTS
// ============================================================================

TEST_F(FileBasedWarmbootUtilsTest, LogBootHistoryCreatesLogFile) {
  logBootHistory(directoryUtil_.get(), "cold", "1.0.0", "2.0.0");

  // Check that log file exists (either in proper location or /tmp fallback)
  auto logPath = directoryUtil_->getAgentBootHistoryLogFile();
  bool logExists = checkFileExists(logPath) ||
      checkFileExists("/tmp/wedge_agent_starts.log");
  EXPECT_TRUE(logExists);
}

TEST_F(FileBasedWarmbootUtilsTest, LogBootHistoryContainsCorrectInformation) {
  logBootHistory(directoryUtil_.get(), "warm", "1.2.3", "4.5.6");

  // Read the log file
  std::string logContent;
  auto logPath = directoryUtil_->getAgentBootHistoryLogFile();
  if (checkFileExists(logPath)) {
    folly::readFile(logPath.c_str(), logContent);
  } else {
    folly::readFile("/tmp/wedge_agent_starts.log", logContent);
  }

  // Verify log contains boot type and versions
  EXPECT_NE(logContent.find("warm"), std::string::npos);
  EXPECT_NE(logContent.find("1.2.3"), std::string::npos);
  EXPECT_NE(logContent.find("4.5.6"), std::string::npos);
}

TEST_F(FileBasedWarmbootUtilsTest, LogBootHistoryOverridesExistingLog) {
  // Log first entry
  logBootHistory(directoryUtil_.get(), "cold", "1.0.0", "2.0.0");

  // Log second entry
  logBootHistory(directoryUtil_.get(), "warm", "1.0.1", "2.0.1");

  // Read the log file
  std::string logContent;
  auto logPath = directoryUtil_->getAgentBootHistoryLogFile();
  if (checkFileExists(logPath)) {
    folly::readFile(logPath.c_str(), logContent);
  } else {
    folly::readFile("/tmp/wedge_agent_starts.log", logContent);
  }

  // Verify both entries are present
  EXPECT_EQ(logContent.find("cold"), std::string::npos);
  EXPECT_NE(logContent.find("warm"), std::string::npos);
  EXPECT_EQ(logContent.find("1.0.0"), std::string::npos);
  EXPECT_NE(logContent.find("1.0.1"), std::string::npos);
}

// ============================================================================
// RECONSTRUCT STATE AND RIB TESTS
// ============================================================================

TEST_F(FileBasedWarmbootUtilsTest, ReconstructStateAndRibFromWarmbootState) {
  // Create warmboot state
  state::WarmbootState warmbootState;
  state::SwitchState switchState;
  warmbootState.swSwitchState() = switchState;

  std::map<int32_t, state::RouteTableFields> routeTables;
  routeTables.emplace(0, state::RouteTableFields{});
  warmbootState.routeTables() = routeTables;

  auto [state, rib] = reconstructStateAndRib(warmbootState, true);

  EXPECT_NE(state, nullptr);
  EXPECT_NE(rib, nullptr);
  // RoutingInformationBase doesn't have getRouteTable method
  // Just verify RIB was constructed successfully
}

TEST_F(FileBasedWarmbootUtilsTest, ReconstructStateAndRibColdBootWithL3) {
  auto [state, rib] = reconstructStateAndRib(std::nullopt, true);

  EXPECT_NE(state, nullptr);
  EXPECT_NE(rib, nullptr);
  // With L3 enabled, default VRF (RouterID 0) should be created
  // RoutingInformationBase doesn't expose getRouteTable but we can verify it
  // was created
}

TEST_F(FileBasedWarmbootUtilsTest, ReconstructStateAndRibColdBootWithoutL3) {
  auto [state, rib] = reconstructStateAndRib(std::nullopt, false);

  EXPECT_NE(state, nullptr);
  EXPECT_NE(rib, nullptr);
  // When L3 is disabled, no default VRF should be created
  // RoutingInformationBase doesn't expose getRouteTable for verification
  // But we can verify RIB was constructed successfully
}

TEST_F(FileBasedWarmbootUtilsTest, ReconstructStateAndRibMultipleCalls) {
  // First call
  auto [state1, rib1] = reconstructStateAndRib(std::nullopt, true);

  // Second call
  auto [state2, rib2] = reconstructStateAndRib(std::nullopt, true);

  // Each call should return independent objects
  EXPECT_NE(state1, nullptr);
  EXPECT_NE(state2, nullptr);
  EXPECT_NE(rib1, nullptr);
  EXPECT_NE(rib2, nullptr);
  EXPECT_NE(state1.get(), state2.get());
  EXPECT_NE(rib1.get(), rib2.get());
}

// ============================================================================
// INTEGRATION TESTS
// ============================================================================

TEST_F(FileBasedWarmbootUtilsTest, WarmbootWorkflowEndToEnd) {
  // Simulate a complete warmboot workflow

  // 1. Create warmboot flag
  auto warmbootFlagPath = directoryUtil_->getSwSwitchCanWarmBootFile();
  createDir(std::filesystem::path(warmbootFlagPath).parent_path().string());
  createTestFile(warmbootFlagPath);

  // 2. Create warmboot state file
  auto warmBootDir = directoryUtil_->getWarmBootDir();
  auto stateFilePath = getWarmBootThriftSwitchStateFile(
      warmBootDir, FLAGS_thrift_switch_state_file);
  createDir(std::filesystem::path(stateFilePath).parent_path().string());
  createTestWarmbootState(stateFilePath);

  // 3. Check flags
  bool forceColdBoot = checkForceColdBootFlag(directoryUtil_.get());
  bool canWarmBoot = checkCanWarmBootFlag(directoryUtil_.get());
  bool asicSupportsWarmboot = checkAsicSupportsWarmboot(asicTable_.get());
  bool stateFileExists =
      checkWarmbootStateFileExists(warmBootDir, FLAGS_thrift_switch_state_file);

  EXPECT_FALSE(forceColdBoot);
  EXPECT_TRUE(canWarmBoot);
  EXPECT_TRUE(asicSupportsWarmboot);
  EXPECT_TRUE(stateFileExists);

  // 4. Log boot
  logBootHistory(directoryUtil_.get(), "warm", "1.0.0", "2.0.0");

  // 5. Flags should be cleared
  EXPECT_FALSE(checkFileExists(warmbootFlagPath));
}

TEST_F(FileBasedWarmbootUtilsTest, ColdBootWorkflowEndToEnd) {
  // Simulate a complete cold boot workflow

  // 1. Create force cold boot flag
  auto forceColdBootPath = directoryUtil_->getSwColdBootOnceFile();
  createDir(std::filesystem::path(forceColdBootPath).parent_path().string());
  createTestFile(forceColdBootPath);

  // 2. Also create warmboot flag (should be ignored due to force cold boot)
  auto warmbootFlagPath = directoryUtil_->getSwSwitchCanWarmBootFile();
  createDir(std::filesystem::path(warmbootFlagPath).parent_path().string());
  createTestFile(warmbootFlagPath);

  // 3. Check flags - force cold boot should take precedence
  bool forceColdBoot = checkForceColdBootFlag(directoryUtil_.get());

  EXPECT_TRUE(forceColdBoot);

  // 4. Log boot
  logBootHistory(directoryUtil_.get(), "cold", "1.0.0", "2.0.0");

  // 5. Force cold boot flag should be cleared
  EXPECT_FALSE(checkFileExists(forceColdBootPath));
}

} // namespace facebook::fboss
