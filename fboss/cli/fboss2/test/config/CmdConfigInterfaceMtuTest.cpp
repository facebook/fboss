// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/cli/fboss2/test/config/CmdConfigTestBase.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <vector>

#include "fboss/cli/fboss2/commands/config/interface/CmdConfigInterfaceMtu.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"
#include "fboss/cli/fboss2/utils/InterfaceList.h"

using namespace ::testing;

namespace facebook::fboss {

class CmdConfigInterfaceMtuTestFixture : public CmdConfigTestBase {
 public:
  CmdConfigInterfaceMtuTestFixture()
      : CmdConfigTestBase(
            "fboss_mtu_test_%%%%-%%%%-%%%%-%%%%",
            R"({
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
})") {}

  void SetUp() override {
    CmdConfigTestBase::SetUp();

    setupTestableConfigSession("config interface eth1/1/1 mtu", "9000");
  }
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
