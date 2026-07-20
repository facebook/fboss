/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/test/config/CmdConfigTestBase.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <algorithm>
#include <stdexcept>
#include <string>

#include "fboss/cli/fboss2/commands/config/interface/switchport/trunk/allowed/vlan/CmdConfigInterfaceSwitchportTrunkAllowedVlan.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/utils/InterfaceList.h"

using namespace ::testing;

namespace facebook::fboss {

class CmdConfigInterfaceSwitchportTrunkAllowedVlanTestFixture
    : public CmdConfigTestBase {
 public:
  CmdConfigInterfaceSwitchportTrunkAllowedVlanTestFixture()
      : CmdConfigTestBase(
            "fboss_trunk_vlan_test_%%%%-%%%%-%%%%-%%%%",
            R"({
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
    "vlanPorts": [],
    "interfaces": [
      {
        "intfID": 300,
        "vlanID": 300,
        "routerID": 0,
        "type": 1,
        "mtu": 9412,
        "name": "vlan300"
      }
    ]
  }
})") {}

  void SetUp() override {
    CmdConfigTestBase::SetUp();

    setupTestableConfigSession(
        "config interface switchport trunk allowed vlan eth1/1/1", "add 100");
  }

 protected:
  // Verify that exactly one VlanPort entry exists for (vlanId, logicalPort)
  // and that it is tagged (emitTags=true) with a forwarding spanning tree
  // state, as required for trunk ports.
  static void expectVlanPort(
      const cfg::SwitchConfig& swConfig,
      int32_t vlanId,
      int32_t logicalPort) {
    auto matches = [vlanId, logicalPort](const auto& vp) {
      return *vp.vlanID() == vlanId && *vp.logicalPort() == logicalPort;
    };
    const auto& vlanPorts = *swConfig.vlanPorts();
    EXPECT_EQ(std::count_if(vlanPorts.cbegin(), vlanPorts.cend(), matches), 1)
        << "Expected exactly one VlanPort entry for VLAN " << vlanId
        << " on logical port " << logicalPort;
    auto vpItr = std::find_if(vlanPorts.cbegin(), vlanPorts.cend(), matches);
    ASSERT_NE(vpItr, vlanPorts.cend())
        << "VlanPort entry not found for VLAN " << vlanId << " on logical port "
        << logicalPort;
    EXPECT_TRUE(*vpItr->emitTags()); // Should be tagged for trunk
    EXPECT_EQ(*vpItr->spanningTreeState(), cfg::SpanningTreeState::FORWARDING);
  }
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
  EXPECT_EQ(action.getVlanIds()[0], 100);
  EXPECT_EQ(action.getVlanIds()[1], 200);
  EXPECT_EQ(action.getVlanIds()[2], 300);
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
  EXPECT_EQ(action.getVlanIds()[0], 100);
  EXPECT_EQ(action.getVlanIds()[1], 200);
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

TEST_F(
    CmdConfigInterfaceSwitchportTrunkAllowedVlanTestFixture,
    trunkVlanActionVlanIdNonNumericErrorMessage) {
  try {
    auto unused = TrunkVlanAction({"add", "10-20"});
    (void)unused;
    FAIL() << "Expected std::invalid_argument";
  } catch (const std::invalid_argument& e) {
    std::string errorMsg = e.what();
    EXPECT_THAT(errorMsg, HasSubstr("Invalid VLAN ID: '10-20'"));
    EXPECT_THAT(
        errorMsg, HasSubstr("range notation such as 10-20 is not supported"));
  }
}

// Tests for queryClient - Add VLANs

TEST_F(
    CmdConfigInterfaceSwitchportTrunkAllowedVlanTestFixture,
    queryClientAddVlanToSinglePort) {
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

  EXPECT_EQ(switchConfig.vlanPorts()->size(), 1);
  expectVlanPort(switchConfig, 100, 1);

  // VLAN changes require an agent warmboot, so the save must record it
  EXPECT_EQ(
      session.getRequiredAction(cli::ServiceType::AGENT),
      cli::ConfigActionLevel::AGENT_WARMBOOT);
}

TEST_F(
    CmdConfigInterfaceSwitchportTrunkAllowedVlanTestFixture,
    queryClientAddMultipleVlansToMultiplePorts) {
  auto cmd = CmdConfigInterfaceSwitchportTrunkAllowedVlan();
  TrunkVlanAction vlanAction({"add", "100,200"});

  utils::InterfaceList interfaces({"eth1/1/1", "eth1/2/1"});

  auto result = cmd.queryClient(localhost(), interfaces, vlanAction);

  EXPECT_THAT(result, HasSubstr("Successfully"));
  EXPECT_THAT(result, HasSubstr("4")); // 2 ports x 2 VLANs = 4 associations

  // Verify exactly the expected (vlanID, logicalPort) entries were created
  auto& session = ConfigSession::getInstance();
  auto& config = session.getAgentConfig();
  auto& switchConfig = *config.sw();

  EXPECT_EQ(switchConfig.vlanPorts()->size(), 4);
  expectVlanPort(switchConfig, 100, 1);
  expectVlanPort(switchConfig, 100, 2);
  expectVlanPort(switchConfig, 200, 1);
  expectVlanPort(switchConfig, 200, 2);
}

TEST_F(
    CmdConfigInterfaceSwitchportTrunkAllowedVlanTestFixture,
    queryClientAddVlanAlreadyExists) {
  auto cmd = CmdConfigInterfaceSwitchportTrunkAllowedVlan();
  TrunkVlanAction vlanAction({"add", "100"});

  utils::InterfaceList interfaces({"eth1/1/1"});

  // Add the VLAN first time
  cmd.queryClient(localhost(), interfaces, vlanAction);

  // Add the same VLAN again
  auto result = cmd.queryClient(localhost(), interfaces, vlanAction);

  EXPECT_THAT(result, HasSubstr("No changes made"));
  EXPECT_THAT(result, HasSubstr("already"));

  // Verify no duplicate entry was created
  auto& config = ConfigSession::getInstance().getAgentConfig();
  auto& switchConfig = *config.sw();
  EXPECT_EQ(switchConfig.vlanPorts()->size(), 1);
  expectVlanPort(switchConfig, 100, 1);
}

TEST_F(
    CmdConfigInterfaceSwitchportTrunkAllowedVlanTestFixture,
    queryClientAddNonExistentVlanThrowsError) {
  auto cmd = CmdConfigInterfaceSwitchportTrunkAllowedVlan();
  TrunkVlanAction vlanAction({"add", "999"}); // VLAN 999 doesn't exist

  utils::InterfaceList interfaces({"eth1/1/1"});

  // Should throw an error because VLAN 999 doesn't exist
  EXPECT_THROW(
      cmd.queryClient(localhost(), interfaces, vlanAction),
      std::invalid_argument);

  // Verify no VlanPort entry was created
  auto& config = ConfigSession::getInstance().getAgentConfig();
  EXPECT_TRUE(config.sw()->vlanPorts()->empty());
}

TEST_F(
    CmdConfigInterfaceSwitchportTrunkAllowedVlanTestFixture,
    queryClientThrowsWhenInterfaceDoesNotResolveToPort) {
  auto cmd = CmdConfigInterfaceSwitchportTrunkAllowedVlan();
  TrunkVlanAction vlanAction({"add", "100"});

  // "vlan300" resolves to an interface but not to a port, so the command
  // must fail without applying a partial change.
  utils::InterfaceList interfaces({"eth1/1/1", "vlan300"});

  try {
    cmd.queryClient(localhost(), interfaces, vlanAction);
    FAIL() << "Expected std::invalid_argument";
  } catch (const std::invalid_argument& e) {
    std::string errorMsg = e.what();
    EXPECT_THAT(errorMsg, HasSubstr("vlan300"));
    EXPECT_THAT(errorMsg, HasSubstr("do not resolve to a port"));
    EXPECT_THAT(errorMsg, HasSubstr("no changes made"));
  }

  // Verify no VlanPort entry was created for the resolvable port either
  auto& config = ConfigSession::getInstance().getAgentConfig();
  EXPECT_TRUE(config.sw()->vlanPorts()->empty());
}

// Tests for queryClient - Remove VLANs

TEST_F(
    CmdConfigInterfaceSwitchportTrunkAllowedVlanTestFixture,
    queryClientRemoveVlan) {
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
  expectVlanPort(switchConfig, 100, 1);

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
  auto cmd = CmdConfigInterfaceSwitchportTrunkAllowedVlan();
  TrunkVlanAction vlanAction({"remove", "100"});

  utils::InterfaceList interfaces({"eth1/1/1"});

  auto result = cmd.queryClient(localhost(), interfaces, vlanAction);

  EXPECT_THAT(result, HasSubstr("No changes made"));

  // A no-op must skip the save, so the required action stays at the
  // default (HITLESS) instead of being escalated to AGENT_WARMBOOT.
  auto& session = ConfigSession::getInstance();
  EXPECT_EQ(
      session.getRequiredAction(cli::ServiceType::AGENT),
      cli::ConfigActionLevel::HITLESS);
}

TEST_F(
    CmdConfigInterfaceSwitchportTrunkAllowedVlanTestFixture,
    queryClientThrowsOnEmptyInterfaceList) {
  auto cmd = CmdConfigInterfaceSwitchportTrunkAllowedVlan();
  TrunkVlanAction vlanAction({"add", "100"});
  // queryClient throws when given an empty interface list
  EXPECT_THROW(
      cmd.queryClient(localhost(), utils::InterfaceList({}), vlanAction),
      std::invalid_argument);
}

} // namespace facebook::fboss
