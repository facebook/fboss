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
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <system_error>
#include <vector>

#include "fboss/cli/fboss2/commands/config/interface/CmdConfigInterface.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/session/Git.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"
#include "fboss/cli/fboss2/test/TestableConfigSession.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"
#include "fboss/cli/fboss2/utils/InterfacesConfig.h"
#include "fboss/cli/fboss2/utils/PortMap.h" // NOLINT(misc-include-cleaner)

namespace fs = std::filesystem;

using namespace ::testing;

namespace facebook::fboss {

class CmdConfigInterfaceTestFixture : public CmdHandlerTestBase {
 public:
  void SetUp() override {
    CmdHandlerTestBase::SetUp();

    // Create unique test directories
    auto tempBase = fs::temp_directory_path();
    auto uniquePath = boost::filesystem::unique_path(
        "fboss_interface_test_%%%%-%%%%-%%%%-%%%%");
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
    sessionConfigDir_ = testHomeDir_ / ".fboss2";
    fs::create_directories(systemConfigDir_ / "cli");

    // NOLINTNEXTLINE(concurrency-mt-unsafe,misc-include-cleaner)
    setenv("HOME", testHomeDir_.c_str(), 1);
    // NOLINTNEXTLINE(concurrency-mt-unsafe,misc-include-cleaner)
    setenv("USER", "testuser", 1);

    // Create a test system config file at cli/agent.conf
    fs::path cliConfigPath = systemConfigDir_ / "cli" / "agent.conf";
    createTestConfig(cliConfigPath, R"({
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

    // Create symlink at /etc/coop/agent.conf -> cli/agent.conf
    fs::create_symlink("cli/agent.conf", systemConfigDir_ / "agent.conf");

    // Initialize Git repository and create initial commit
    Git git(systemConfigDir_.string());
    git.init();
    git.commit({cliConfigPath.string()}, "Initial commit");

    // Initialize the ConfigSession singleton for all tests
    fs::create_directories(sessionConfigDir_);
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

// ============================================================================
// InterfacesConfig Validation Tests
// ============================================================================

// Test valid config with port + description
TEST_F(CmdConfigInterfaceTestFixture, interfaceConfigValidPortAndDescription) {
  utils::InterfacesConfig config({"eth1/1/1", "description", "My port"});
  EXPECT_EQ(config.getInterfaces().size(), 1);
  EXPECT_TRUE(config.hasAttributes());
  ASSERT_EQ(config.getAttributes().size(), 1);
  EXPECT_EQ(config.getAttributes()[0].first, "description");
  EXPECT_EQ(config.getAttributes()[0].second, "My port");
}

// Test valid config with port + mtu
TEST_F(CmdConfigInterfaceTestFixture, interfaceConfigValidPortAndMtu) {
  utils::InterfacesConfig config({"eth1/1/1", "mtu", "9000"});
  EXPECT_EQ(config.getInterfaces().size(), 1);
  EXPECT_TRUE(config.hasAttributes());
  ASSERT_EQ(config.getAttributes().size(), 1);
  EXPECT_EQ(config.getAttributes()[0].first, "mtu");
  EXPECT_EQ(config.getAttributes()[0].second, "9000");
}

// Test valid config with port + both attributes
TEST_F(CmdConfigInterfaceTestFixture, interfaceConfigValidPortAndBothAttrs) {
  utils::InterfacesConfig config(
      {"eth1/1/1", "description", "My port", "mtu", "9000"});
  EXPECT_EQ(config.getInterfaces().size(), 1);
  EXPECT_TRUE(config.hasAttributes());
  ASSERT_EQ(config.getAttributes().size(), 2);
  EXPECT_EQ(config.getAttributes()[0].first, "description");
  EXPECT_EQ(config.getAttributes()[0].second, "My port");
  EXPECT_EQ(config.getAttributes()[1].first, "mtu");
  EXPECT_EQ(config.getAttributes()[1].second, "9000");
}

// Test valid config with multiple ports + attributes
TEST_F(CmdConfigInterfaceTestFixture, interfaceConfigMultiplePorts) {
  utils::InterfacesConfig config(
      {"eth1/1/1", "eth1/2/1", "description", "Uplink ports"});
  EXPECT_EQ(config.getInterfaces().size(), 2);
  EXPECT_TRUE(config.hasAttributes());
  ASSERT_EQ(config.getAttributes().size(), 1);
  EXPECT_EQ(config.getAttributes()[0].first, "description");
  EXPECT_EQ(config.getAttributes()[0].second, "Uplink ports");
}

// Test port only (no attributes) - for subcommand pass-through
TEST_F(CmdConfigInterfaceTestFixture, interfaceConfigPortOnly) {
  utils::InterfacesConfig config({"eth1/1/1"});
  EXPECT_EQ(config.getInterfaces().size(), 1);
  EXPECT_FALSE(config.hasAttributes());
  EXPECT_TRUE(config.getAttributes().empty());
}

// Test case-insensitive attribute names
TEST_F(CmdConfigInterfaceTestFixture, interfaceConfigCaseInsensitiveAttrs) {
  utils::InterfacesConfig config(
      {"eth1/1/1", "DESCRIPTION", "Test", "MTU", "9000"});
  EXPECT_TRUE(config.hasAttributes());
  ASSERT_EQ(config.getAttributes().size(), 2);
  // Attributes should be normalized to lowercase
  EXPECT_EQ(config.getAttributes()[0].first, "description");
  EXPECT_EQ(config.getAttributes()[1].first, "mtu");
}

// Test empty input throws
TEST_F(CmdConfigInterfaceTestFixture, interfaceConfigEmptyThrows) {
  EXPECT_THROW(utils::InterfacesConfig({}), std::invalid_argument);
}

// Test first token is attribute (no port name)
TEST_F(CmdConfigInterfaceTestFixture, interfaceConfigNoPortNameThrows) {
  try {
    utils::InterfacesConfig config({"description", "My port"});
    FAIL() << "Expected std::invalid_argument";
  } catch (const std::invalid_argument& e) {
    EXPECT_THAT(e.what(), HasSubstr("No interface name provided"));
    EXPECT_THAT(e.what(), HasSubstr("description"));
  }
}

// Test missing value for attribute
TEST_F(CmdConfigInterfaceTestFixture, interfaceConfigMissingValueThrows) {
  try {
    utils::InterfacesConfig config({"eth1/1/1", "description"});
    FAIL() << "Expected std::invalid_argument";
  } catch (const std::invalid_argument& e) {
    EXPECT_THAT(e.what(), HasSubstr("Missing value for attribute"));
    EXPECT_THAT(e.what(), HasSubstr("description"));
  }
}

// Test value is actually another attribute (forgot value)
TEST_F(CmdConfigInterfaceTestFixture, interfaceConfigValueIsAttributeThrows) {
  try {
    utils::InterfacesConfig config({"eth1/1/1", "description", "mtu"});
    FAIL() << "Expected std::invalid_argument";
  } catch (const std::invalid_argument& e) {
    EXPECT_THAT(e.what(), HasSubstr("Missing value for attribute"));
    EXPECT_THAT(e.what(), HasSubstr("description"));
    EXPECT_THAT(e.what(), HasSubstr("mtu"));
  }
}

// Test unknown attribute name - unknown attribute must appear AFTER a known
// attribute to trigger the error. Otherwise, unknown tokens are treated as
// port names and fail during port resolution.
TEST_F(CmdConfigInterfaceTestFixture, interfaceConfigUnknownAttributeThrows) {
  try {
    // "speed" comes after "description", so it's recognized as an attribute
    utils::InterfacesConfig config(
        {"eth1/1/1", "description", "Test", "speed", "100000"});
    FAIL() << "Expected std::invalid_argument";
  } catch (const std::invalid_argument& e) {
    EXPECT_THAT(e.what(), HasSubstr("Unknown attribute"));
    EXPECT_THAT(e.what(), HasSubstr("speed"));
  }
}

// Test non-existent port throws
TEST_F(CmdConfigInterfaceTestFixture, interfaceConfigNonExistentPortThrows) {
  EXPECT_THROW(
      utils::InterfacesConfig({"eth1/99/1", "description", "Test"}),
      std::invalid_argument);
}

// ============================================================================
// CmdConfigInterface::queryClient Tests
// ============================================================================

// Test setting description on a single interface
TEST_F(CmdConfigInterfaceTestFixture, queryClientSetsDescription) {
  auto cmd = CmdConfigInterface();
  utils::InterfacesConfig config(
      {"eth1/1/1", "description", "New description"});

  auto result = cmd.queryClient(localhost(), config);

  EXPECT_THAT(result, HasSubstr("Successfully configured"));
  EXPECT_THAT(result, HasSubstr("eth1/1/1"));
  EXPECT_THAT(result, HasSubstr("description"));

  // Verify the description was updated
  auto& session = ConfigSession::getInstance();
  auto& agentConfig = session.getAgentConfig();
  auto& switchConfig = *agentConfig.sw();
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

// Test setting MTU on a single interface
TEST_F(CmdConfigInterfaceTestFixture, queryClientSetsMtu) {
  auto cmd = CmdConfigInterface();
  utils::InterfacesConfig config({"eth1/1/1", "mtu", "9000"});

  auto result = cmd.queryClient(localhost(), config);

  EXPECT_THAT(result, HasSubstr("Successfully configured"));
  EXPECT_THAT(result, HasSubstr("eth1/1/1"));
  EXPECT_THAT(result, HasSubstr("mtu=9000"));

  // Verify the MTU was updated
  auto& session = ConfigSession::getInstance();
  auto& agentConfig = session.getAgentConfig();
  auto& switchConfig = *agentConfig.sw();
  auto& intfs = *switchConfig.interfaces();
  for (const auto& intf : intfs) {
    if (*intf.name() == "eth1/1/1") {
      EXPECT_EQ(*intf.mtu(), 9000);
    } else {
      // Other interfaces should be unchanged
      EXPECT_EQ(*intf.mtu(), 1500);
    }
  }
}

// Test setting both description and MTU
TEST_F(CmdConfigInterfaceTestFixture, queryClientSetsBothAttributes) {
  auto cmd = CmdConfigInterface();
  utils::InterfacesConfig config(
      {"eth1/1/1", "description", "Updated port", "mtu", "9000"});

  auto result = cmd.queryClient(localhost(), config);

  EXPECT_THAT(result, HasSubstr("Successfully configured"));
  EXPECT_THAT(result, HasSubstr("eth1/1/1"));
  EXPECT_THAT(result, HasSubstr("description"));
  EXPECT_THAT(result, HasSubstr("mtu=9000"));

  // Verify both were updated
  auto& session = ConfigSession::getInstance();
  auto& agentConfig = session.getAgentConfig();
  auto& switchConfig = *agentConfig.sw();

  auto& ports = *switchConfig.ports();
  for (const auto& port : ports) {
    if (*port.name() == "eth1/1/1") {
      EXPECT_EQ(*port.description(), "Updated port");
    }
  }

  auto& intfs = *switchConfig.interfaces();
  for (const auto& intf : intfs) {
    if (*intf.name() == "eth1/1/1") {
      EXPECT_EQ(*intf.mtu(), 9000);
    }
  }
}

// Test setting attributes on multiple interfaces
TEST_F(CmdConfigInterfaceTestFixture, queryClientMultipleInterfaces) {
  auto cmd = CmdConfigInterface();
  utils::InterfacesConfig config(
      {"eth1/1/1", "eth1/2/1", "description", "Shared description"});

  auto result = cmd.queryClient(localhost(), config);

  EXPECT_THAT(result, HasSubstr("Successfully configured"));
  EXPECT_THAT(result, HasSubstr("eth1/1/1"));
  EXPECT_THAT(result, HasSubstr("eth1/2/1"));

  // Verify both descriptions were updated
  auto& session = ConfigSession::getInstance();
  auto& agentConfig = session.getAgentConfig();
  auto& switchConfig = *agentConfig.sw();
  auto& ports = *switchConfig.ports();
  for (const auto& port : ports) {
    EXPECT_EQ(*port.description(), "Shared description");
  }
}

// Test no attributes throws (pass-through case)
TEST_F(CmdConfigInterfaceTestFixture, queryClientNoAttributesThrows) {
  auto cmd = CmdConfigInterface();
  utils::InterfacesConfig config({"eth1/1/1"});

  EXPECT_THROW(cmd.queryClient(localhost(), config), std::runtime_error);
}

// Test invalid MTU value (non-numeric)
TEST_F(CmdConfigInterfaceTestFixture, queryClientInvalidMtuNonNumeric) {
  auto cmd = CmdConfigInterface();
  utils::InterfacesConfig config({"eth1/1/1", "mtu", "abc"});

  try {
    cmd.queryClient(localhost(), config);
    FAIL() << "Expected std::invalid_argument";
  } catch (const std::invalid_argument& e) {
    EXPECT_THAT(e.what(), HasSubstr("Invalid MTU value"));
    EXPECT_THAT(e.what(), HasSubstr("abc"));
  }
}

// Test MTU out of range (too low)
TEST_F(CmdConfigInterfaceTestFixture, queryClientMtuTooLow) {
  auto cmd = CmdConfigInterface();
  utils::InterfacesConfig config(
      {"eth1/1/1", "mtu", std::to_string(utils::kMtuMin - 1)});

  try {
    cmd.queryClient(localhost(), config);
    FAIL() << "Expected std::invalid_argument";
  } catch (const std::invalid_argument& e) {
    EXPECT_THAT(e.what(), HasSubstr("out of range"));
  }
}

// Test MTU out of range (too high)
TEST_F(CmdConfigInterfaceTestFixture, queryClientMtuTooHigh) {
  auto cmd = CmdConfigInterface();
  utils::InterfacesConfig config(
      {"eth1/1/1", "mtu", std::to_string(utils::kMtuMax + 1)});

  try {
    cmd.queryClient(localhost(), config);
    FAIL() << "Expected std::invalid_argument";
  } catch (const std::invalid_argument& e) {
    EXPECT_THAT(e.what(), HasSubstr("out of range"));
  }
}

// Test MTU boundary values (valid)
TEST_F(CmdConfigInterfaceTestFixture, queryClientMtuBoundaryValid) {
  auto cmd = CmdConfigInterface();

  // Test minimum MTU
  utils::InterfacesConfig configMin(
      {"eth1/1/1", "mtu", std::to_string(utils::kMtuMin)});
  EXPECT_THAT(
      cmd.queryClient(localhost(), configMin),
      HasSubstr("Successfully configured"));

  // Test maximum MTU
  utils::InterfacesConfig configMax(
      {"eth1/1/1", "mtu", std::to_string(utils::kMtuMax)});
  EXPECT_THAT(
      cmd.queryClient(localhost(), configMax),
      HasSubstr("Successfully configured"));
}

// Test printOutput
TEST_F(CmdConfigInterfaceTestFixture, printOutput) {
  auto cmd = CmdConfigInterface();
  std::string successMessage =
      "Successfully configured interface(s) eth1/1/1: description=\"Test\"";

  std::stringstream buffer;
  std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
  cmd.printOutput(successMessage);
  std::cout.rdbuf(old);

  EXPECT_EQ(buffer.str(), successMessage + "\n");
}

} // namespace facebook::fboss
