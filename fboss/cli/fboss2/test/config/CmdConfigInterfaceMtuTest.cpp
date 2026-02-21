// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <boost/filesystem/operations.hpp>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <system_error>
#include <vector>

#include "fboss/cli/fboss2/commands/config/interface/CmdConfigInterfaceMtu.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"
#include "fboss/cli/fboss2/test/TestableConfigSession.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"
#include "fboss/cli/fboss2/utils/InterfaceList.h"
#include "fboss/cli/fboss2/utils/PortMap.h" // NOLINT(misc-include-cleaner)

namespace fs = std::filesystem;

using namespace ::testing;

namespace facebook::fboss {

class CmdConfigInterfaceMtuTestFixture : public CmdHandlerTestBase {
 public:
  void SetUp() override {
    CmdHandlerTestBase::SetUp();

    // Create unique test directories
    auto tempBase = fs::temp_directory_path();
    auto uniquePath =
        boost::filesystem::unique_path("fboss_mtu_test_%%%%-%%%%-%%%%-%%%%");
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
    fs::create_directories(testEtcDir_ / "coop");
    fs::create_directories(testEtcDir_ / "coop" / "cli");

    // NOLINTNEXTLINE(concurrency-mt-unsafe,misc-include-cleaner)
    setenv("HOME", testHomeDir_.c_str(), 1);
    // NOLINTNEXTLINE(concurrency-mt-unsafe,misc-include-cleaner)
    setenv("USER", "testuser", 1);

    // Create a test system config file as agent-r1.conf in the cli directory
    fs::path initialRevision = testEtcDir_ / "coop" / "cli" / "agent-r1.conf";
    createTestConfig(initialRevision, R"({
  "sw": {
    "ports": [
      {
        "logicalID": 1,
        "name": "eth1/1/1",
        "state": 2,
        "speed": 100000
      },
      {
        "logicalID": 2,
        "name": "eth1/2/1",
        "state": 2,
        "speed": 100000
      }
    ],
    "vlanPorts": [
      {
        "vlanID": 1,
        "logicalPort": 1
      },
      {
        "vlanID": 2,
        "logicalPort": 2
      }
    ],
    "interfaces": [
      {
        "intfID": 1,
        "routerID": 0,
        "vlanID": 1,
        "name": "eth1/1/1",
        "mtu": 1500
      },
      {
        "intfID": 2,
        "routerID": 0,
        "vlanID": 2,
        "name": "eth1/2/1",
        "mtu": 1500
      }
    ]
  }
})");

    // Create symlink at agent.conf pointing to agent-r1.conf
    systemConfigPath_ = testEtcDir_ / "coop" / "agent.conf";
    fs::create_symlink(initialRevision, systemConfigPath_);

    // Initialize the ConfigSession singleton for all tests
    fs::path sessionConfig = testHomeDir_ / ".fboss2" / "agent.conf";
    auto testSession = std::make_unique<TestableConfigSession>(
        sessionConfig.string(),
        systemConfigPath_.string(),
        (testEtcDir_ / "coop" / "cli").string());
    // Set a default command line for tests that call saveConfig()
    testSession->setCommandLine("config interface eth1/1/1 mtu 9000");
    TestableConfigSession::setInstance(std::move(testSession));
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
  fs::path systemConfigPath_;
};

// Test setting MTU on a single existing interface
TEST_F(CmdConfigInterfaceMtuTestFixture, singleInterface) {
  auto cmd = CmdConfigInterfaceMtu();
  utils::InterfaceList interfaces({"eth1/1/1"});
  MtuValue mtuValue({"1500"});

  auto result = cmd.queryClient(localhost(), interfaces, mtuValue);

  EXPECT_THAT(result, HasSubstr("Successfully set MTU"));
  EXPECT_THAT(result, HasSubstr("eth1/1/1"));

  // Verify the MTU was updated
  auto& session = ConfigSession::getInstance();
  auto& config = session.getAgentConfig();
  auto& switchConfig = *config.sw();
  auto& intfs = *switchConfig.interfaces();
  for (const auto& intf : intfs) {
    if (*intf.name() == "eth1/1/1") {
      EXPECT_EQ(*intf.mtu(), 1500);
    }
  }
}

// Test setting MTU on two valid interfaces at once
TEST_F(CmdConfigInterfaceMtuTestFixture, twoValidInterfacesAtOnce) {
  auto cmd = CmdConfigInterfaceMtu();
  utils::InterfaceList interfaces({"eth1/1/1", "eth1/2/1"});
  MtuValue mtuValue({"9000"});

  auto result = cmd.queryClient(localhost(), interfaces, mtuValue);

  EXPECT_THAT(result, HasSubstr("Successfully set MTU"));
  EXPECT_THAT(result, HasSubstr("eth1/1/1"));
  EXPECT_THAT(result, HasSubstr("eth1/2/1"));

  // Verify both MTUs were updated
  auto& session = ConfigSession::getInstance();
  auto& config = session.getAgentConfig();
  auto& switchConfig = *config.sw();
  auto& intfs = *switchConfig.interfaces();
  for (const auto& intf : intfs) {
    if (*intf.name() == "eth1/1/1" || *intf.name() == "eth1/2/1") {
      EXPECT_EQ(*intf.mtu(), 9000);
    }
  }
}

// Test MTU boundary & edge cases validation
TEST_F(CmdConfigInterfaceMtuTestFixture, mtuBoundaryValidation) {
  auto cmd = CmdConfigInterfaceMtu();
  utils::InterfaceList interfaces({"eth1/1/1"});

  // Valid: kMtuMin and kMtuMax should succeed
  EXPECT_THAT(
      cmd.queryClient(
          localhost(), interfaces, MtuValue({std::to_string(utils::kMtuMin)})),
      HasSubstr("Successfully set MTU"));
  EXPECT_THAT(
      cmd.queryClient(
          localhost(), interfaces, MtuValue({std::to_string(utils::kMtuMax)})),
      HasSubstr("Successfully set MTU"));

  // Invalid: kMtuMin - 1 and kMtuMax + 1 should throw
  EXPECT_THROW(
      MtuValue({std::to_string(utils::kMtuMin - 1)}), std::invalid_argument);
  EXPECT_THROW(
      MtuValue({std::to_string(utils::kMtuMax + 1)}), std::invalid_argument);

  // Test that non-numeric MTU value throws
  EXPECT_THROW(MtuValue({"abc"}), std::invalid_argument);

  // Test that empty MTU value throws
  EXPECT_THROW(MtuValue({}), std::invalid_argument);
}

} // namespace facebook::fboss
