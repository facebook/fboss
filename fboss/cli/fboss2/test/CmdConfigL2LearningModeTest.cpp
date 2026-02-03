/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <boost/filesystem/operations.hpp>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>

#include "fboss/cli/fboss2/commands/config/l2/learning_mode/CmdConfigL2LearningMode.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/session/Git.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"
#include "fboss/cli/fboss2/test/TestableConfigSession.h"
#include "fboss/cli/fboss2/utils/PortMap.h" // NOLINT(misc-include-cleaner)

namespace fs = std::filesystem;

using namespace ::testing;

namespace facebook::fboss {

class CmdConfigL2LearningModeTestFixture : public CmdHandlerTestBase {
 public:
  void SetUp() override {
    CmdHandlerTestBase::SetUp();

    // Create unique test directories
    auto tempBase = fs::temp_directory_path();
    auto uniquePath = boost::filesystem::unique_path(
        "fboss_l2_learning_test_%%%%-%%%%-%%%%-%%%%");
    testHomeDir_ = tempBase / (uniquePath.string() + "_home");
    testEtcDir_ = tempBase / (uniquePath.string() + "_etc");

    std::error_code ec;
    if (fs::exists(testHomeDir_)) {
      fs::remove_all(testHomeDir_, ec);
    }
    if (fs::exists(testEtcDir_)) {
      fs::remove_all(testEtcDir_, ec);
    }

    // Create test directories
    fs::create_directories(testHomeDir_);
    systemConfigDir_ = testEtcDir_ / "coop";
    fs::create_directories(systemConfigDir_ / "cli");

    // NOLINTNEXTLINE(concurrency-mt-unsafe) - acceptable in unit tests
    setenv("HOME", testHomeDir_.c_str(), 1);
    // NOLINTNEXTLINE(concurrency-mt-unsafe) - acceptable in unit tests
    setenv("USER", "testuser", 1);

    // Create a test system config file at cli/agent.conf
    fs::path cliConfigPath = systemConfigDir_ / "cli" / "agent.conf";
    createTestConfig(cliConfigPath, R"({
  "sw": {
    "switchSettings": {
      "l2LearningMode": 0
    }
  }
})");

    // Create symlink at /etc/coop/agent.conf -> cli/agent.conf
    fs::create_symlink("cli/agent.conf", systemConfigDir_ / "agent.conf");

    // Initialize Git repository and create initial commit
    Git git(systemConfigDir_.string());
    git.init();
    git.commit({cliConfigPath.string()}, "Initial commit");

    // Initialize the ConfigSession singleton for all tests
    sessionConfigDir_ = testHomeDir_ / ".fboss2";
    TestableConfigSession::setInstance(
        std::make_unique<TestableConfigSession>(
            sessionConfigDir_.string(), systemConfigDir_.string()));
  }

  void TearDown() override {
    // Reset the singleton to ensure tests don't interfere with each other
    TestableConfigSession::setInstance(nullptr);
    std::error_code ec;
    if (fs::exists(testHomeDir_)) {
      fs::remove_all(testHomeDir_, ec);
    }
    if (fs::exists(testEtcDir_)) {
      fs::remove_all(testEtcDir_, ec);
    }
    CmdHandlerTestBase::TearDown();
  }

 protected:
  void createTestConfig(const fs::path& path, const std::string& content) {
    std::ofstream file(path);
    file << content;
    file.close();
  }

  fs::path testHomeDir_;
  fs::path testEtcDir_;
  fs::path systemConfigDir_;
  fs::path sessionConfigDir_;
};

// ==============================================================================
// L2LearningModeArg Validation Tests
// ==============================================================================

TEST_F(CmdConfigL2LearningModeTestFixture, learningModeArgValidation) {
  // Test valid modes (lowercase)
  EXPECT_EQ(
      L2LearningModeArg({"hardware"}).getL2LearningMode(),
      cfg::L2LearningMode::HARDWARE);
  EXPECT_EQ(
      L2LearningModeArg({"software"}).getL2LearningMode(),
      cfg::L2LearningMode::SOFTWARE);
  EXPECT_EQ(
      L2LearningModeArg({"disabled"}).getL2LearningMode(),
      cfg::L2LearningMode::DISABLED);

  // Test valid modes (uppercase)
  EXPECT_EQ(
      L2LearningModeArg({"HARDWARE"}).getL2LearningMode(),
      cfg::L2LearningMode::HARDWARE);
  EXPECT_EQ(
      L2LearningModeArg({"SOFTWARE"}).getL2LearningMode(),
      cfg::L2LearningMode::SOFTWARE);
  EXPECT_EQ(
      L2LearningModeArg({"DISABLED"}).getL2LearningMode(),
      cfg::L2LearningMode::DISABLED);

  // Test valid modes (mixed case)
  EXPECT_EQ(
      L2LearningModeArg({"HaRdWaRe"}).getL2LearningMode(),
      cfg::L2LearningMode::HARDWARE);

  // Test invalid cases
  EXPECT_THROW(L2LearningModeArg({}), std::invalid_argument);
  EXPECT_THROW(L2LearningModeArg({"hardware", "extra"}), std::invalid_argument);
  EXPECT_THROW(L2LearningModeArg({"invalid"}), std::invalid_argument);
}

// ==============================================================================
// Command Execution Tests
// ==============================================================================

TEST_F(CmdConfigL2LearningModeTestFixture, setLearningModeToSoftware) {
  CmdConfigL2LearningMode cmd;
  HostInfo hostInfo("testhost");
  L2LearningModeArg modeArg({"software"});

  auto result = cmd.queryClient(hostInfo, modeArg);
  EXPECT_THAT(result, HasSubstr("software"));

  // Verify the config was updated
  auto& config = ConfigSession::getInstance().getAgentConfig();
  auto mode = *config.sw()->switchSettings()->l2LearningMode();
  EXPECT_EQ(mode, cfg::L2LearningMode::SOFTWARE);
}

TEST_F(CmdConfigL2LearningModeTestFixture, setLearningModeToHardware) {
  // First set to software, then back to hardware
  CmdConfigL2LearningMode cmd;
  HostInfo hostInfo("testhost");

  L2LearningModeArg softwareArg({"software"});
  cmd.queryClient(hostInfo, softwareArg);

  L2LearningModeArg hardwareArg({"hardware"});
  auto result = cmd.queryClient(hostInfo, hardwareArg);
  EXPECT_THAT(result, HasSubstr("hardware"));

  // Verify the config was updated
  auto& config = ConfigSession::getInstance().getAgentConfig();
  auto mode = *config.sw()->switchSettings()->l2LearningMode();
  EXPECT_EQ(mode, cfg::L2LearningMode::HARDWARE);
}

TEST_F(CmdConfigL2LearningModeTestFixture, setLearningModeToDisabled) {
  CmdConfigL2LearningMode cmd;
  HostInfo hostInfo("testhost");
  L2LearningModeArg modeArg({"disabled"});

  auto result = cmd.queryClient(hostInfo, modeArg);
  EXPECT_THAT(result, HasSubstr("disabled"));

  // Verify the config was updated
  auto& config = ConfigSession::getInstance().getAgentConfig();
  auto mode = *config.sw()->switchSettings()->l2LearningMode();
  EXPECT_EQ(mode, cfg::L2LearningMode::DISABLED);
}

TEST_F(CmdConfigL2LearningModeTestFixture, setLearningModeAlreadySet) {
  CmdConfigL2LearningMode cmd;
  HostInfo hostInfo("testhost");

  // Default is HARDWARE (mode=0)
  L2LearningModeArg hardwareArg({"hardware"});
  auto result = cmd.queryClient(hostInfo, hardwareArg);
  EXPECT_THAT(result, HasSubstr("already"));
}

} // namespace facebook::fboss
