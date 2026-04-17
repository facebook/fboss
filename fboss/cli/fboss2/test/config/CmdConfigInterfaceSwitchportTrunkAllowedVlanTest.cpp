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

#include "fboss/cli/fboss2/commands/config/interface/switchport/trunk/allowed/vlan/CmdConfigInterfaceSwitchportTrunkAllowedVlan.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/session/Git.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"
#include "fboss/cli/fboss2/test/TestableConfigSession.h"
#include "fboss/cli/fboss2/utils/InterfaceList.h"
#include "fboss/cli/fboss2/utils/PortMap.h" // NOLINT(misc-include-cleaner)

namespace fs = std::filesystem;

using namespace ::testing;

namespace facebook::fboss {

class CmdConfigInterfaceSwitchportTrunkAllowedVlanTestFixture
    : public CmdHandlerTestBase {
 public:
  void SetUp() override {
    CmdHandlerTestBase::SetUp();

    // Create unique test directories
    auto tempBase = fs::temp_directory_path();
    auto uniquePath = boost::filesystem::unique_path(
        "fboss_trunk_vlan_test_%%%%-%%%%-%%%%-%%%%");
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
    // System config dir is the Git repository root
    systemConfigDir_ = testEtcDir_ / "coop";
    sessionConfigDir_ = testHomeDir_ / ".fboss2";
    fs::create_directories(systemConfigDir_ / "cli");

    // NOLINTNEXTLINE(concurrency-mt-unsafe) - acceptable in unit tests
    setenv("HOME", testHomeDir_.c_str(), 1);
    // NOLINTNEXTLINE(concurrency-mt-unsafe) - acceptable in unit tests
    setenv("USER", "testuser", 1);

    // Initialize Git repository
    Git git(systemConfigDir_.string());
    git.init();

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
    "vlans": [
      {"id": 100, "name": "vlan100"},
      {"id": 200, "name": "vlan200"},
      {"id": 300, "name": "vlan300"}
    ],
    "vlanPorts": []
  }
})");

    // Create symlink at agent.conf -> cli/agent.conf
    fs::create_symlink("cli/agent.conf", systemConfigDir_ / "agent.conf");

    // Create initial commit
    git.commit({"cli/agent.conf"}, "Initial commit");
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

// Tests for TrunkVlanAction validation - Add action

TEST_F(
    CmdConfigInterfaceSwitchportTrunkAllowedVlanTestFixture,
    trunkVlanActionAddSingleVlan) {
  TrunkVlanAction action({"add", "100"});
  EXPECT_TRUE(action.isAdd());
  EXPECT_FALSE(action.isRemove());
  EXPECT_EQ(action.getVlanIds().size(), 1);
  EXPECT_EQ(action.getVlanIds()[0], 100);
}

TEST_F(
    CmdConfigInterfaceSwitchportTrunkAllowedVlanTestFixture,
    trunkVlanActionAddMultipleVlans) {
  TrunkVlanAction action({"add", "100,200,300"});
  EXPECT_TRUE(action.isAdd());
  EXPECT_EQ(action.getVlanIds().size(), 3);
  EXPECT_EQ(action.getVlanIds()[0], 100);
  EXPECT_EQ(action.getVlanIds()[1], 200);
  EXPECT_EQ(action.getVlanIds()[2], 300);
}

TEST_F(
    CmdConfigInterfaceSwitchportTrunkAllowedVlanTestFixture,
    trunkVlanActionAddMultipleArgs) {
  TrunkVlanAction action({"add", "100", "200", "300"});
  EXPECT_TRUE(action.isAdd());
  EXPECT_EQ(action.getVlanIds().size(), 3);
}

TEST_F(
    CmdConfigInterfaceSwitchportTrunkAllowedVlanTestFixture,
    trunkVlanActionAddCaseInsensitive) {
  TrunkVlanAction action({"ADD", "100"});
  EXPECT_TRUE(action.isAdd());
}

// Tests for TrunkVlanAction validation - Remove action

TEST_F(
    CmdConfigInterfaceSwitchportTrunkAllowedVlanTestFixture,
    trunkVlanActionRemoveSingleVlan) {
  TrunkVlanAction action({"remove", "100"});
  EXPECT_TRUE(action.isRemove());
  EXPECT_FALSE(action.isAdd());
  EXPECT_EQ(action.getVlanIds().size(), 1);
  EXPECT_EQ(action.getVlanIds()[0], 100);
}

TEST_F(
    CmdConfigInterfaceSwitchportTrunkAllowedVlanTestFixture,
    trunkVlanActionRemoveMultipleVlans) {
  TrunkVlanAction action({"remove", "100,200"});
  EXPECT_TRUE(action.isRemove());
  EXPECT_EQ(action.getVlanIds().size(), 2);
}

TEST_F(
    CmdConfigInterfaceSwitchportTrunkAllowedVlanTestFixture,
    trunkVlanActionRemoveCaseInsensitive) {
  TrunkVlanAction action({"REMOVE", "100"});
  EXPECT_TRUE(action.isRemove());
}

// Tests for TrunkVlanAction validation - Invalid inputs

TEST_F(
    CmdConfigInterfaceSwitchportTrunkAllowedVlanTestFixture,
    trunkVlanActionEmptyInvalid) {
  EXPECT_THROW(TrunkVlanAction({}), std::invalid_argument);
}

TEST_F(
    CmdConfigInterfaceSwitchportTrunkAllowedVlanTestFixture,
    trunkVlanActionNoVlanIdsInvalid) {
  EXPECT_THROW(TrunkVlanAction({"add"}), std::invalid_argument);
}

TEST_F(
    CmdConfigInterfaceSwitchportTrunkAllowedVlanTestFixture,
    trunkVlanActionInvalidActionInvalid) {
  EXPECT_THROW(TrunkVlanAction({"invalid", "100"}), std::invalid_argument);
}

TEST_F(
    CmdConfigInterfaceSwitchportTrunkAllowedVlanTestFixture,
    trunkVlanActionVlanIdZeroInvalid) {
  EXPECT_THROW(TrunkVlanAction({"add", "0"}), std::invalid_argument);
}

TEST_F(
    CmdConfigInterfaceSwitchportTrunkAllowedVlanTestFixture,
    trunkVlanActionVlanIdTooHighInvalid) {
  EXPECT_THROW(TrunkVlanAction({"add", "4095"}), std::invalid_argument);
}

TEST_F(
    CmdConfigInterfaceSwitchportTrunkAllowedVlanTestFixture,
    trunkVlanActionVlanIdNegativeInvalid) {
  EXPECT_THROW(TrunkVlanAction({"add", "-1"}), std::invalid_argument);
}

TEST_F(
    CmdConfigInterfaceSwitchportTrunkAllowedVlanTestFixture,
    trunkVlanActionVlanIdNonNumericInvalid) {
  EXPECT_THROW(TrunkVlanAction({"add", "abc"}), std::invalid_argument);
}

// Test error message format
TEST_F(
    CmdConfigInterfaceSwitchportTrunkAllowedVlanTestFixture,
    trunkVlanActionInvalidActionErrorMessage) {
  try {
    auto unused = TrunkVlanAction({"badaction", "100"});
    (void)unused;
    FAIL() << "Expected std::invalid_argument";
  } catch (const std::invalid_argument& e) {
    std::string errorMsg = e.what();
    EXPECT_THAT(errorMsg, HasSubstr("add"));
    EXPECT_THAT(errorMsg, HasSubstr("remove"));
  }
}

TEST_F(
    CmdConfigInterfaceSwitchportTrunkAllowedVlanTestFixture,
    trunkVlanActionVlanIdOutOfRangeErrorMessage) {
  try {
    auto unused = TrunkVlanAction({"add", "9999"});
    (void)unused;
    FAIL() << "Expected std::invalid_argument";
  } catch (const std::invalid_argument& e) {
    std::string errorMsg = e.what();
    EXPECT_THAT(errorMsg, HasSubstr("VLAN ID must be between 1 and 4094"));
    EXPECT_THAT(errorMsg, HasSubstr("9999"));
  }
}

// Tests for queryClient - Add VLANs

TEST_F(
    CmdConfigInterfaceSwitchportTrunkAllowedVlanTestFixture,
    queryClientAddVlanToSinglePort) {
  fs::create_directories(sessionConfigDir_);

  TestableConfigSession::setInstance(
      std::make_unique<TestableConfigSession>(
          sessionConfigDir_.string(), systemConfigDir_.string()));

  auto cmd = CmdConfigInterfaceSwitchportTrunkAllowedVlan();
  TrunkVlanAction vlanAction({"add", "100"});

  utils::InterfaceList interfaces({"eth1/1/1"});

  auto result = cmd.queryClient(localhost(), interfaces, vlanAction);

  EXPECT_THAT(result, HasSubstr("Successfully"));
  EXPECT_THAT(result, HasSubstr("added"));
  EXPECT_THAT(result, HasSubstr("eth1/1/1"));
  EXPECT_THAT(result, HasSubstr("100"));

  // Verify the VlanPort entry was created
  auto& session = ConfigSession::getInstance();
  auto& config = session.getAgentConfig();
  auto& switchConfig = *config.sw();
  auto& vlanPorts = *switchConfig.vlanPorts();

  bool found = false;
  for (const auto& vp : vlanPorts) {
    if (*vp.vlanID() == 100 && *vp.logicalPort() == 1) {
      found = true;
      EXPECT_TRUE(*vp.emitTags()); // Should be tagged for trunk
      EXPECT_EQ(*vp.spanningTreeState(), cfg::SpanningTreeState::FORWARDING);
    }
  }
  EXPECT_TRUE(found) << "VlanPort entry not found";
}

TEST_F(
    CmdConfigInterfaceSwitchportTrunkAllowedVlanTestFixture,
    queryClientAddMultipleVlansToMultiplePorts) {
  fs::create_directories(sessionConfigDir_);

  TestableConfigSession::setInstance(
      std::make_unique<TestableConfigSession>(
          sessionConfigDir_.string(), systemConfigDir_.string()));

  auto cmd = CmdConfigInterfaceSwitchportTrunkAllowedVlan();
  TrunkVlanAction vlanAction({"add", "100,200"});

  utils::InterfaceList interfaces({"eth1/1/1", "eth1/2/1"});

  auto result = cmd.queryClient(localhost(), interfaces, vlanAction);

  EXPECT_THAT(result, HasSubstr("Successfully"));
  EXPECT_THAT(result, HasSubstr("4")); // 2 ports x 2 VLANs = 4 associations

  // Verify the VlanPort entries were created
  auto& session = ConfigSession::getInstance();
  auto& config = session.getAgentConfig();
  auto& switchConfig = *config.sw();
  auto& vlanPorts = *switchConfig.vlanPorts();

  EXPECT_EQ(vlanPorts.size(), 4);
}

TEST_F(
    CmdConfigInterfaceSwitchportTrunkAllowedVlanTestFixture,
    queryClientAddVlanAlreadyExists) {
  fs::create_directories(sessionConfigDir_);

  TestableConfigSession::setInstance(
      std::make_unique<TestableConfigSession>(
          sessionConfigDir_.string(), systemConfigDir_.string()));

  auto cmd = CmdConfigInterfaceSwitchportTrunkAllowedVlan();
  TrunkVlanAction vlanAction({"add", "100"});

  utils::InterfaceList interfaces({"eth1/1/1"});

  // Add the VLAN first time
  cmd.queryClient(localhost(), interfaces, vlanAction);

  // Add the same VLAN again
  auto result = cmd.queryClient(localhost(), interfaces, vlanAction);

  EXPECT_THAT(result, HasSubstr("No changes made"));
  EXPECT_THAT(result, HasSubstr("already"));
}

TEST_F(
    CmdConfigInterfaceSwitchportTrunkAllowedVlanTestFixture,
    queryClientAddNonExistentVlanThrowsError) {
  fs::create_directories(sessionConfigDir_);

  TestableConfigSession::setInstance(
      std::make_unique<TestableConfigSession>(
          sessionConfigDir_.string(), systemConfigDir_.string()));

  auto cmd = CmdConfigInterfaceSwitchportTrunkAllowedVlan();
  TrunkVlanAction vlanAction({"add", "999"}); // VLAN 999 doesn't exist

  utils::InterfaceList interfaces({"eth1/1/1"});

  // Should throw an error because VLAN 999 doesn't exist
  EXPECT_THROW(
      cmd.queryClient(localhost(), interfaces, vlanAction),
      std::invalid_argument);
}

// Tests for queryClient - Remove VLANs

TEST_F(
    CmdConfigInterfaceSwitchportTrunkAllowedVlanTestFixture,
    queryClientRemoveVlan) {
  fs::create_directories(sessionConfigDir_);

  TestableConfigSession::setInstance(
      std::make_unique<TestableConfigSession>(
          sessionConfigDir_.string(), systemConfigDir_.string()));

  auto cmd = CmdConfigInterfaceSwitchportTrunkAllowedVlan();
  utils::InterfaceList interfaces({"eth1/1/1"});

  // First add a VLAN (VLAN 100 already exists in test config)
  TrunkVlanAction addAction({"add", "100"});
  cmd.queryClient(localhost(), interfaces, addAction);

  // Verify it was added
  auto& session = ConfigSession::getInstance();
  auto& config = session.getAgentConfig();
  auto& switchConfig = *config.sw();
  EXPECT_EQ(switchConfig.vlanPorts()->size(), 1);

  // Count VLANs before removal
  size_t vlanCountBefore = switchConfig.vlans()->size();

  // Now remove it
  TrunkVlanAction removeAction({"remove", "100"});
  auto result = cmd.queryClient(localhost(), interfaces, removeAction);

  EXPECT_THAT(result, HasSubstr("Successfully"));
  EXPECT_THAT(result, HasSubstr("removed"));
  EXPECT_THAT(result, HasSubstr("100"));

  // Verify VlanPort was removed
  EXPECT_EQ(switchConfig.vlanPorts()->size(), 0);

  // Verify VLAN still exists (we don't auto-delete VLANs)
  EXPECT_EQ(switchConfig.vlans()->size(), vlanCountBefore);
  auto vitr = std::find_if(
      switchConfig.vlans()->cbegin(),
      switchConfig.vlans()->cend(),
      [](const auto& vlan) { return *vlan.id() == 100; });
  EXPECT_NE(vitr, switchConfig.vlans()->cend());
}

TEST_F(
    CmdConfigInterfaceSwitchportTrunkAllowedVlanTestFixture,
    queryClientRemoveNonExistentVlan) {
  fs::create_directories(sessionConfigDir_);

  TestableConfigSession::setInstance(
      std::make_unique<TestableConfigSession>(
          sessionConfigDir_.string(), systemConfigDir_.string()));

  auto cmd = CmdConfigInterfaceSwitchportTrunkAllowedVlan();
  TrunkVlanAction vlanAction({"remove", "100"});

  utils::InterfaceList interfaces({"eth1/1/1"});

  auto result = cmd.queryClient(localhost(), interfaces, vlanAction);

  EXPECT_THAT(result, HasSubstr("No changes made"));
}

TEST_F(
    CmdConfigInterfaceSwitchportTrunkAllowedVlanTestFixture,
    queryClientThrowsOnEmptyInterfaceList) {
  fs::create_directories(sessionConfigDir_);

  TestableConfigSession::setInstance(
      std::make_unique<TestableConfigSession>(
          sessionConfigDir_.string(), systemConfigDir_.string()));

  auto cmd = CmdConfigInterfaceSwitchportTrunkAllowedVlan();
  TrunkVlanAction vlanAction({"add", "100"});
  // queryClient throws when given an empty interface list
  EXPECT_THROW(
      cmd.queryClient(localhost(), utils::InterfaceList({}), vlanAction),
      std::invalid_argument);
}

} // namespace facebook::fboss
