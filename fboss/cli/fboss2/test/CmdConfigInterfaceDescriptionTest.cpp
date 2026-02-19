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

#include "fboss/cli/fboss2/commands/config/interface/CmdConfigInterfaceDescription.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"
#include "fboss/cli/fboss2/test/TestableConfigSession.h"
#include "fboss/cli/fboss2/utils/InterfaceList.h"
#include "fboss/cli/fboss2/utils/PortMap.h" // NOLINT(misc-include-cleaner)

namespace fs = std::filesystem;

using namespace ::testing;

namespace facebook::fboss {

class CmdConfigInterfaceDescriptionTestFixture : public CmdHandlerTestBase {
 public:
  void SetUp() override {
    CmdHandlerTestBase::SetUp();

    // Create unique test directories
    auto tempBase = fs::temp_directory_path();
    auto uniquePath = boost::filesystem::unique_path(
        "fboss_description_test_%%%%-%%%%-%%%%-%%%%");
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
        "speed": 100000,
        "description": "original description of eth1/1/1"
      },
      {
        "logicalID": 2,
        "name": "eth1/2/1",
        "state": 2,
        "speed": 100000,
        "description": "original description of eth1/2/1"
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
    testSession->setCommandLine("config interface eth1/1/1 description test");
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

// Test setting description on a single existing interface
TEST_F(CmdConfigInterfaceDescriptionTestFixture, singleInterface) {
  auto cmd = CmdConfigInterfaceDescription();
  utils::InterfaceList interfaces({"eth1/1/1"});

  auto result = cmd.queryClient(
      localhost(), interfaces, std::vector<std::string>{"New description"});

  EXPECT_THAT(result, HasSubstr("Successfully set description"));
  EXPECT_THAT(result, HasSubstr("eth1/1/1"));

  // Verify the description was updated
  auto& session = ConfigSession::getInstance();
  auto& config = session.getAgentConfig();
  auto& switchConfig = *config.sw();
  auto& ports = *switchConfig.ports();
  for (const auto& port : ports) {
    if (*port.name() == "eth1/1/1") {
      EXPECT_EQ(*port.description(), "New description");
    } else {
      // Other ports should be unchanged
      EXPECT_EQ(*port.description(), "original description of " + *port.name());
    }
  }
}

// Test setting description on a single non-existent interface
TEST_F(CmdConfigInterfaceDescriptionTestFixture, nonExistentInterface) {
  // Creating InterfaceList with a non-existent port should throw
  EXPECT_THROW(utils::InterfaceList({"eth1/99/1"}), std::invalid_argument);

  // Verify the config was not changed
  auto& session = ConfigSession::getInstance();
  auto& config = session.getAgentConfig();
  auto& switchConfig = *config.sw();
  auto& ports = *switchConfig.ports();
  for (const auto& port : ports) {
    EXPECT_EQ(*port.description(), "original description of " + *port.name());
  }
}

// Test setting description on two valid interfaces at once
TEST_F(CmdConfigInterfaceDescriptionTestFixture, twoValidInterfacesAtOnce) {
  auto cmd = CmdConfigInterfaceDescription();
  utils::InterfaceList interfaces({"eth1/1/1", "eth1/2/1"});

  auto result = cmd.queryClient(
      localhost(), interfaces, std::vector<std::string>{"Shared description"});

  EXPECT_THAT(result, HasSubstr("Successfully set description"));
  EXPECT_THAT(result, HasSubstr("eth1/1/1"));
  EXPECT_THAT(result, HasSubstr("eth1/2/1"));

  // Verify both descriptions were updated
  auto& session = ConfigSession::getInstance();
  auto& config = session.getAgentConfig();
  auto& switchConfig = *config.sw();
  auto& ports = *switchConfig.ports();
  for (const auto& port : ports) {
    if (*port.name() == "eth1/1/1" || *port.name() == "eth1/2/1") {
      EXPECT_EQ(*port.description(), "Shared description");
    }
  }
}

// Test that mixing valid and invalid interfaces fails without changing anything
TEST_F(CmdConfigInterfaceDescriptionTestFixture, mixValidInvalidInterfaces) {
  // Creating InterfaceList with one valid and one invalid port should throw
  // because InterfaceList validates all ports before returning
  EXPECT_THROW(
      utils::InterfaceList({"eth1/1/1", "eth1/99/1"}), std::invalid_argument);

  // Verify no config changes were made
  auto& session = ConfigSession::getInstance();
  auto& config = session.getAgentConfig();
  auto& switchConfig = *config.sw();
  auto& ports = *switchConfig.ports();
  for (const auto& port : ports) {
    EXPECT_EQ(*port.description(), "original description of " + *port.name());
  }
}

// Test that empty interface list throws
TEST_F(CmdConfigInterfaceDescriptionTestFixture, emptyInterfaceList) {
  auto cmd = CmdConfigInterfaceDescription();
  utils::InterfaceList emptyInterfaces({});
  EXPECT_THROW(
      cmd.queryClient(
          localhost(),
          emptyInterfaces,
          std::vector<std::string>{"Some description"}),
      std::invalid_argument);
}

} // namespace facebook::fboss
