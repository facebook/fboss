// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <folly/IPAddressV4.h>

#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/cli/fboss2/utils/CmdClientUtils.h"

#include "fboss/cli/fboss2/commands/set/port/state/CmdSetPortState.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"

using namespace ::testing;

namespace facebook::fboss {

/*
 * Set up test data
 */
std::map<int32_t, PortInfoThrift> createEntriesSetPortState() {
  std::map<int32_t, PortInfoThrift> portMap;

  PortInfoThrift portEntry1;
  portEntry1.portId() = 1;
  portEntry1.name() = "eth1/5/1";
  portEntry1.adminState() = PortAdminState::ENABLED;

  PortInfoThrift portEntry2;
  portEntry2.portId() = 2;
  portEntry2.name() = "eth1/5/2";
  portEntry2.adminState() = PortAdminState::DISABLED;

  PortInfoThrift portEntry3;
  portEntry3.portId() = 3;
  portEntry3.name() = "eth1/5/3";
  portEntry3.adminState() = PortAdminState::ENABLED;

  portMap[portEntry1.get_portId()] = portEntry1;
  portMap[portEntry2.get_portId()] = portEntry2;
  portMap[portEntry3.get_portId()] = portEntry3;
  return portMap;
}

class CmdSetPortStateTestFixture : public CmdHandlerTestBase {
 public:
  std::map<int32_t, facebook::fboss::PortInfoThrift> portEntries;

  void SetUp() override {
    CmdHandlerTestBase::SetUp();
    portEntries = createEntriesSetPortState();
  }
};

TEST_F(CmdSetPortStateTestFixture, nonexistentPort) {
  std::vector<std::string> queriedPortNames = {"eth1/5/10"};
  EXPECT_THROW(
      getQueriedPortIds(portEntries, queriedPortNames), std::runtime_error);
}

TEST_F(CmdSetPortStateTestFixture, disableOnePort) {
  setupMockedAgentServer();
  std::vector<std::string> queriedPortNames = {"eth1/5/1"};
  // corresponding portIds: {1};
  std::vector<std::string> state = {"disable"};

  EXPECT_CALL(getMockAgent(), getAllPortInfo(_))
      .WillOnce(Invoke([&](auto& entries) { entries = portEntries; }));

  EXPECT_CALL(getMockAgent(), setPortState(_, _))
      .WillOnce(Invoke([&](int32_t, bool) {
        portEntries[1].adminState() = PortAdminState::DISABLED;
      }));

  EXPECT_EQ(portEntries[1].adminState(), PortAdminState::ENABLED);
  auto cmd = CmdSetPortState();
  auto model = cmd.queryClient(localhost(), queriedPortNames, state);
  EXPECT_EQ(portEntries[1].adminState(), PortAdminState::DISABLED);
  EXPECT_EQ(model, "Disabling port eth1/5/1\n");
}

TEST_F(CmdSetPortStateTestFixture, enableOnePort) {
  setupMockedAgentServer();
  std::vector<std::string> queriedPortNames = {"eth1/5/2"};
  // corresponding portIds: {2};
  std::vector<std::string> state = {"enable"};

  EXPECT_CALL(getMockAgent(), getAllPortInfo(_))
      .WillOnce(Invoke([&](auto& entries) { entries = portEntries; }));

  EXPECT_CALL(getMockAgent(), setPortState(_, _))
      .WillOnce(Invoke([&](int32_t, bool) {
        portEntries[2].adminState() = PortAdminState::ENABLED;
      }));

  EXPECT_EQ(portEntries[2].adminState(), PortAdminState::DISABLED);
  auto cmd = CmdSetPortState();
  auto model = cmd.queryClient(localhost(), queriedPortNames, state);
  EXPECT_EQ(portEntries[2].adminState(), PortAdminState::ENABLED);
  EXPECT_EQ(model, "Enabling port eth1/5/2\n");
}

TEST_F(CmdSetPortStateTestFixture, disableTwoPorts) {
  setupMockedAgentServer();
  std::vector<std::string> queriedPortNames = {"eth1/5/1", "eth1/5/3"};
  // corresponding portIds: {1, 3};
  std::vector<std::string> state = {"disable"};

  EXPECT_CALL(getMockAgent(), getAllPortInfo(_))
      .WillOnce(Invoke([&](auto& entries) { entries = portEntries; }));

  EXPECT_CALL(getMockAgent(), setPortState(_, _))
      .WillOnce(Invoke([&](int32_t, bool) {
        portEntries[1].adminState() = PortAdminState::DISABLED;
      }))
      .WillOnce(Invoke([&](int32_t, bool) {
        portEntries[3].adminState() = PortAdminState::DISABLED;
      }));

  auto cmd = CmdSetPortState();
  auto model = cmd.queryClient(localhost(), queriedPortNames, state);
  EXPECT_EQ(portEntries[1].adminState(), PortAdminState::DISABLED);
  EXPECT_EQ(portEntries[3].adminState(), PortAdminState::DISABLED);
  EXPECT_EQ(model, "Disabling port eth1/5/1\nDisabling port eth1/5/3\n");
}

TEST_F(CmdSetPortStateTestFixture, disableDuplicatePorts) {
  setupMockedAgentServer();
  std::vector<std::string> queriedPortNames = {"eth1/5/1", "eth1/5/1"};
  // corresponding portIds: {1};
  std::vector<std::string> state = {"disable"};

  EXPECT_CALL(getMockAgent(), getAllPortInfo(_))
      .WillOnce(Invoke([&](auto& entries) { entries = portEntries; }));

  EXPECT_CALL(getMockAgent(), setPortState(_, _))
      .WillOnce(Invoke([&](int32_t, bool) {
        portEntries[1].adminState() = PortAdminState::DISABLED;
      }));

  auto cmd = CmdSetPortState();
  auto model = cmd.queryClient(localhost(), queriedPortNames, state);
  EXPECT_EQ(portEntries[1].adminState(), PortAdminState::DISABLED);
  EXPECT_EQ(model, "Disabling port eth1/5/1\n");
}

TEST_F(CmdSetPortStateTestFixture, disableEnablePort) {
  // Toggle from enable -> disable -> enable again on one port.
  // Check the result of a sequence of behaviors.
  setupMockedAgentServer();
  std::vector<std::string> queriedPortNames = {"eth1/5/2"};
  // corresponding portIds: {2};
  std::vector<std::string> state;

  EXPECT_CALL(getMockAgent(), getAllPortInfo(_))
      .Times(3)
      .WillRepeatedly(Invoke([&](auto& entries) { entries = portEntries; }));

  EXPECT_CALL(getMockAgent(), setPortState(_, _))
      .WillOnce(Invoke([&](int32_t, bool) {
        portEntries[2].adminState() = PortAdminState::ENABLED;
      }))
      .WillOnce(Invoke([&](int32_t, bool) {
        portEntries[2].adminState() = PortAdminState::DISABLED;
      }))
      .WillOnce(Invoke([&](int32_t, bool) {
        portEntries[2].adminState() = PortAdminState::ENABLED;
      }));

  auto cmd = CmdSetPortState();
  state = {"enable"};
  cmd.queryClient(localhost(), queriedPortNames, state);
  EXPECT_EQ(portEntries[2].adminState(), PortAdminState::ENABLED);

  state = {"disable"};
  cmd.queryClient(localhost(), queriedPortNames, state);
  EXPECT_EQ(portEntries[2].adminState(), PortAdminState::DISABLED);

  state = {"enable"};
  cmd.queryClient(localhost(), queriedPortNames, state);
  EXPECT_EQ(portEntries[2].adminState(), PortAdminState::ENABLED);
}

} // namespace facebook::fboss
