/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <boost/filesystem.hpp>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <sstream>

#include "fboss/cli/fboss2/commands/config/interface/switchport/access/vlan/CmdConfigInterfaceSwitchportAccessVlan.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"
#include "fboss/cli/fboss2/test/TestableConfigSession.h"
#include "fboss/cli/fboss2/utils/PortMap.h"

namespace fs = std::filesystem;

using namespace ::testing;

namespace facebook::fboss {

class CmdConfigInterfaceSwitchportAccessVlanTestFixture
    : public CmdHandlerTestBase {
 public:
  void SetUp() override {
    CmdHandlerTestBase::SetUp();

    // Create unique test directories
    auto tempBase = fs::temp_directory_path();
    auto uniquePath = boost::filesystem::unique_path(
        "fboss_switchport_test_%%%%-%%%%-%%%%-%%%%");
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

    // Set environment variables
    setenv("HOME", testHomeDir_.c_str(), 1);
    setenv("USER", "testuser", 1);

    // Create a test system config file with ports and vlanPorts
    fs::path initialRevision = testEtcDir_ / "coop" / "cli" / "agent-r1.conf";
    createTestConfig(initialRevision, R"({
  "sw": {
    "ports": [
      {
        "logicalID": 1,
        "name": "eth1/1/1",
        "state": 2,
        "speed": 100000,
        "ingressVlan": 1
      },
      {
        "logicalID": 2,
        "name": "eth1/2/1",
        "state": 2,
        "speed": 100000,
        "ingressVlan": 1
      }
    ],
    "vlanPorts": [
      {
        "vlanID": 1,
        "logicalPort": 1,
        "spanningTreeState": 2,
        "emitTags": false
      },
      {
        "vlanID": 1,
        "logicalPort": 2,
        "spanningTreeState": 2,
        "emitTags": false
      }
    ]
  }
})");

    // Create symlink
    systemConfigPath_ = testEtcDir_ / "coop" / "agent.conf";
    fs::create_symlink(initialRevision, systemConfigPath_);

    // Create session config path
    sessionConfigPath_ = testHomeDir_ / ".fboss2" / "agent.conf";
    cliConfigDir_ = testEtcDir_ / "coop" / "cli";

    // Initialize the ConfigSession singleton for all tests
    auto testSession = std::make_unique<TestableConfigSession>(
        sessionConfigPath_.string(),
        systemConfigPath_.string(),
        cliConfigDir_.string());
    // Set a default command line for tests that call saveConfig()
    testSession->setCommandLine(
        "config interface switchport access vlan eth1/1/1 100");
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
  fs::path sessionConfigPath_;
  fs::path cliConfigDir_;
};

// Tests for VlanIdValue validation

TEST_F(CmdConfigInterfaceSwitchportAccessVlanTestFixture, vlanIdValidMin) {
  VlanIdValue vlanId({"1"});
  EXPECT_EQ(vlanId.getVlanId(), 1);
}

TEST_F(CmdConfigInterfaceSwitchportAccessVlanTestFixture, vlanIdValidMax) {
  VlanIdValue vlanId({"4094"});
  EXPECT_EQ(vlanId.getVlanId(), 4094);
}

TEST_F(CmdConfigInterfaceSwitchportAccessVlanTestFixture, vlanIdValidMid) {
  VlanIdValue vlanId({"100"});
  EXPECT_EQ(vlanId.getVlanId(), 100);
}

TEST_F(CmdConfigInterfaceSwitchportAccessVlanTestFixture, vlanIdZeroInvalid) {
  EXPECT_THROW(VlanIdValue({"0"}), std::invalid_argument);
}

TEST_F(
    CmdConfigInterfaceSwitchportAccessVlanTestFixture,
    vlanIdTooHighInvalid) {
  EXPECT_THROW(VlanIdValue({"4095"}), std::invalid_argument);
}

TEST_F(
    CmdConfigInterfaceSwitchportAccessVlanTestFixture,
    vlanIdNegativeInvalid) {
  EXPECT_THROW(VlanIdValue({"-1"}), std::invalid_argument);
}

TEST_F(
    CmdConfigInterfaceSwitchportAccessVlanTestFixture,
    vlanIdNonNumericInvalid) {
  EXPECT_THROW(VlanIdValue({"abc"}), std::invalid_argument);
}

TEST_F(CmdConfigInterfaceSwitchportAccessVlanTestFixture, vlanIdEmptyInvalid) {
  EXPECT_THROW(VlanIdValue({}), std::invalid_argument);
}

TEST_F(
    CmdConfigInterfaceSwitchportAccessVlanTestFixture,
    vlanIdMultipleValuesInvalid) {
  EXPECT_THROW(VlanIdValue({"100", "200"}), std::invalid_argument);
}

// Test error message format
TEST_F(
    CmdConfigInterfaceSwitchportAccessVlanTestFixture,
    vlanIdOutOfRangeErrorMessage) {
  try {
    VlanIdValue({"9999"});
    FAIL() << "Expected std::invalid_argument";
  } catch (const std::invalid_argument& e) {
    std::string errorMsg = e.what();
    EXPECT_THAT(errorMsg, HasSubstr("VLAN ID must be between 1 and 4094"));
    EXPECT_THAT(errorMsg, HasSubstr("9999"));
  }
}

TEST_F(
    CmdConfigInterfaceSwitchportAccessVlanTestFixture,
    vlanIdNonNumericErrorMessage) {
  try {
    VlanIdValue({"notanumber"});
    FAIL() << "Expected std::invalid_argument";
  } catch (const std::invalid_argument& e) {
    std::string errorMsg = e.what();
    EXPECT_THAT(errorMsg, HasSubstr("Invalid VLAN ID"));
    EXPECT_THAT(errorMsg, HasSubstr("notanumber"));
  }
}

// Tests for queryClient

TEST_F(
    CmdConfigInterfaceSwitchportAccessVlanTestFixture,
    queryClientSetsIngressVlanMultiplePorts) {
  auto cmd = CmdConfigInterfaceSwitchportAccessVlan();
  VlanIdValue vlanId({"2001"});

  // Create InterfaceList from port names
  utils::InterfaceList interfaces({"eth1/1/1", "eth1/2/1"});

  auto result = cmd.queryClient(localhost(), interfaces, vlanId);

  EXPECT_THAT(result, HasSubstr("Successfully set access VLAN"));
  EXPECT_THAT(result, HasSubstr("eth1/1/1"));
  EXPECT_THAT(result, HasSubstr("eth1/2/1"));
  EXPECT_THAT(result, HasSubstr("2001"));

  // Verify the ingressVlan was updated for both ports
  auto& session = ConfigSession::getInstance();
  auto& config = session.getAgentConfig();
  auto& switchConfig = *config.sw();
  auto& ports = *switchConfig.ports();
  for (const auto& port : ports) {
    if (*port.name() == "eth1/1/1" || *port.name() == "eth1/2/1") {
      EXPECT_EQ(*port.ingressVlan(), 2001);
    }
  }

  // Verify the vlanPorts entries were also updated
  auto& vlanPorts = *switchConfig.vlanPorts();
  for (const auto& vlanPort : vlanPorts) {
    if (*vlanPort.logicalPort() == 1 || *vlanPort.logicalPort() == 2) {
      EXPECT_EQ(*vlanPort.vlanID(), 2001);
    }
  }
}

TEST_F(
    CmdConfigInterfaceSwitchportAccessVlanTestFixture,
    queryClientThrowsOnEmptyInterfaceList) {
  auto cmd = CmdConfigInterfaceSwitchportAccessVlan();
  VlanIdValue vlanId({"100"});

  // Empty InterfaceList is valid to construct but queryClient should throw
  utils::InterfaceList emptyInterfaces({});
  EXPECT_THROW(
      cmd.queryClient(localhost(), emptyInterfaces, vlanId),
      std::invalid_argument);
}

} // namespace facebook::fboss
