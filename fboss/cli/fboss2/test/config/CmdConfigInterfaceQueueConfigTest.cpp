// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/cli/fboss2/test/config/CmdConfigTestBase.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <stdexcept>
#include <string>

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/cli/fboss2/commands/config/interface/CmdConfigInterfaceQueueConfig.h"
#include "fboss/cli/fboss2/commands/config/qos/PortQueueConfigUtils.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/utils/InterfaceList.h"

using namespace ::testing;

namespace facebook::fboss {

// eth1/1/1 starts bound to a named queue config, eth1/2/1 starts unbound, so
// both the "clear an existing binding" and "already default" paths are
// reachable from one seed.
static const std::string kSeedConfig = R"({
  "sw": {
    "ports": [
      {
        "logicalID": 1,
        "name": "eth1/1/1",
        "state": 2,
        "speed": 100000,
        "ingressVlan": 1,
        "portQueueConfigName": "rsw_queues"
      },
      {
        "logicalID": 2,
        "name": "eth1/2/1",
        "state": 2,
        "speed": 100000,
        "ingressVlan": 1
      }
    ],
    "portQueueConfigs": {
      "rsw_queues": [
        {"id": 0, "streamType": 1, "weight": 1, "scheduling": 5}
      ]
    }
  }
})";

class CmdConfigInterfaceQueueConfigTestFixture : public CmdConfigTestBase {
 public:
  CmdConfigInterfaceQueueConfigTestFixture()
      : CmdConfigTestBase("fboss_ifqc_test_%%%%-%%%%-%%%%-%%%%", kSeedConfig) {}

 protected:
  void SetUp() override {
    CmdConfigTestBase::SetUp();
    setupTestableConfigSession(
        "config interface eth1/1/1 queue-config", "rsw_queues");
  }

  static const cfg::Port* findPort(const std::string& name) {
    const auto& ports =
        *ConfigSession::getInstance().getAgentConfig().sw()->ports();
    for (const auto& port : ports) {
      if (*port.name() == name) {
        return &port;
      }
    }
    return nullptr;
  }
};

TEST_F(CmdConfigInterfaceQueueConfigTestFixture, bindsNamedQueueConfig) {
  auto cmd = CmdConfigInterfaceQueueConfig();
  utils::InterfaceList interfaces({"eth1/2/1"});
  auto result = cmd.queryClient(
      localhost(), interfaces, utils::QueueConfigName({"rsw_queues"}));

  EXPECT_THAT(result, HasSubstr("Successfully set queue-config"));
  EXPECT_THAT(result, HasSubstr("rsw_queues"));

  const auto* port = findPort("eth1/2/1");
  ASSERT_NE(port, nullptr);
  ASSERT_TRUE(port->portQueueConfigName().has_value());
  EXPECT_EQ(*port->portQueueConfigName(), "rsw_queues");
}

// `default` is not a portQueueConfigs entry; per Port::portQueueConfigName's
// contract an unset field already resolves to defaultPortQueues, so selecting
// it clears the override rather than writing an unresolvable name.
TEST_F(CmdConfigInterfaceQueueConfigTestFixture, defaultClearsExistingBinding) {
  ASSERT_TRUE(findPort("eth1/1/1")->portQueueConfigName().has_value());

  auto cmd = CmdConfigInterfaceQueueConfig();
  utils::InterfaceList interfaces({"eth1/1/1"});
  auto result = cmd.queryClient(
      localhost(),
      interfaces,
      utils::QueueConfigName({utils::kDefaultQueueConfigName}));

  EXPECT_THAT(result, HasSubstr("default queue config"));

  const auto* port = findPort("eth1/1/1");
  ASSERT_NE(port, nullptr);
  EXPECT_FALSE(port->portQueueConfigName().has_value());
}

TEST_F(CmdConfigInterfaceQueueConfigTestFixture, defaultOnUnboundPortIsNoOp) {
  auto cmd = CmdConfigInterfaceQueueConfig();
  utils::InterfaceList interfaces({"eth1/2/1"});
  EXPECT_NO_THROW(cmd.queryClient(
      localhost(),
      interfaces,
      utils::QueueConfigName({utils::kDefaultQueueConfigName})));

  EXPECT_FALSE(findPort("eth1/2/1")->portQueueConfigName().has_value());
}

// A named config must exist before it can be bound; `default` bypasses this
// check because it never has a portQueueConfigs entry to find.
TEST_F(CmdConfigInterfaceQueueConfigTestFixture, unknownNameThrows) {
  auto cmd = CmdConfigInterfaceQueueConfig();
  utils::InterfaceList interfaces({"eth1/1/1"});
  try {
    cmd.queryClient(
        localhost(), interfaces, utils::QueueConfigName({"no_such_config"}));
    FAIL() << "expected std::invalid_argument";
  } catch (const std::invalid_argument& e) {
    EXPECT_THAT(e.what(), HasSubstr("no_such_config"));
    EXPECT_THAT(e.what(), HasSubstr("does not exist"));
  }
}

TEST_F(CmdConfigInterfaceQueueConfigTestFixture, emptyInterfaceListThrows) {
  auto cmd = CmdConfigInterfaceQueueConfig();
  utils::InterfaceList emptyInterfaces({});
  EXPECT_THROW(
      cmd.queryClient(
          localhost(), emptyInterfaces, utils::QueueConfigName({"rsw_queues"})),
      std::invalid_argument);
}

} // namespace facebook::fboss
