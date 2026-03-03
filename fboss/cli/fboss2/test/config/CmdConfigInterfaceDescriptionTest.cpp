// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/cli/fboss2/test/config/CmdConfigTestBase.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <vector>

#include "fboss/cli/fboss2/commands/config/interface/CmdConfigInterfaceDescription.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/utils/InterfaceList.h"

using namespace ::testing;

namespace facebook::fboss {

class CmdConfigInterfaceDescriptionTestFixture : public CmdConfigTestBase {
 public:
  CmdConfigInterfaceDescriptionTestFixture()
      : CmdConfigTestBase(
            "fboss_description_test_%%%%-%%%%-%%%%-%%%%",
            R"({
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
})") {}

  void SetUp() override {
    CmdConfigTestBase::SetUp();

    setupTestableConfigSession("config interface eth1/1/1 description", "test");
  }
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
