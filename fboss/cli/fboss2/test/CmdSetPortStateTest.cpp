// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <folly/IPAddressV4.h>

#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"

#include "fboss/cli/fboss2/CmdLocalOptions.h"
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

  portMap[folly::copy(portEntry1.portId().value())] = portEntry1;
  portMap[folly::copy(portEntry2.portId().value())] = portEntry2;
  portMap[folly::copy(portEntry3.portId().value())] = portEntry3;
  return portMap;
}

class CmdSetPortStateTestFixture : public CmdHandlerTestBase {
 public:
  std::map<int32_t, facebook::fboss::PortInfoThrift> portEntries;

  void SetUp() override {
    CmdHandlerTestBase::SetUp();
    portEntries = createEntriesSetPortState();
    // The unit-test environment is not a TTY, so the SHT confirmation prompt
    // would refuse the call. Bypass it for the happy-path tests; the
    // dedicated `noTtyAndNoYesFlag_throws` test below clears this flag to
    // exercise the refusal path.
    CmdLocalOptions::getInstance()->setLocalOption(
        kSetPortStateCommandName, kSetPortStateYesFlag, "true");
  }

  void TearDown() override {
    // Singleton survives across tests — reset the flag.
    CmdLocalOptions::getInstance()->setLocalOption(
        kSetPortStateCommandName, kSetPortStateYesFlag, "false");
    CmdHandlerTestBase::TearDown();
  }
};

TEST_F(CmdSetPortStateTestFixture, nonexistentPort) {
  std::vector<std::string> queriedPortNames = {"eth1/5/10"};
  std::vector<facebook::fboss::AggregatePortThrift> aggregatePorts;
  EXPECT_THROW(
      getQueriedPortIds(portEntries, queriedPortNames, aggregatePorts),
      std::runtime_error);
}

TEST_F(CmdSetPortStateTestFixture, disableOnePort) {
  setupMockedAgentServer();
  std::vector<std::string> queriedPortNames = {"eth1/5/1"};
  // corresponding portIds: {1};
  std::vector<std::string> state = {"disable"};

  EXPECT_CALL(getMockAgent(), getAllPortInfo(_))
      .WillOnce(Invoke([&](auto& entries) { entries = portEntries; }));
  EXPECT_CALL(getMockAgent(), getAggregatePortTable(_))
      .WillOnce(Invoke([&](auto& entries) {
        entries = std::vector<AggregatePortThrift>();
      }));

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
  EXPECT_CALL(getMockAgent(), getAggregatePortTable(_))
      .WillOnce(Invoke([&](auto& entries) {
        entries = std::vector<AggregatePortThrift>();
      }));

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
  EXPECT_CALL(getMockAgent(), getAggregatePortTable(_))
      .WillOnce(Invoke([&](auto& entries) {
        entries = std::vector<AggregatePortThrift>();
      }));

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
  EXPECT_CALL(getMockAgent(), getAggregatePortTable(_))
      .WillOnce(Invoke([&](auto& entries) {
        entries = std::vector<AggregatePortThrift>();
      }));

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
  EXPECT_CALL(getMockAgent(), getAggregatePortTable(_))
      .Times(3)
      .WillRepeatedly(Invoke([&](auto& entries) {
        entries = std::vector<AggregatePortThrift>();
      }));

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

TEST_F(CmdSetPortStateTestFixture, noTtyAndNoYesFlag_throws) {
  // Clear the bypass set up by SetUp() so we exercise the SHT refusal path.
  CmdLocalOptions::getInstance()->setLocalOption(
      kSetPortStateCommandName, kSetPortStateYesFlag, "false");

  // No mock expectations: the Thrift agent must NOT be hit.
  EXPECT_CALL(getMockAgent(), getAllPortInfo(_)).Times(0);
  EXPECT_CALL(getMockAgent(), getAggregatePortTable(_)).Times(0);
  EXPECT_CALL(getMockAgent(), setPortState(_, _)).Times(0);

  std::vector<std::string> queriedPortNames = {"eth1/5/1"};
  std::vector<std::string> state = {"disable"};

  auto cmd = CmdSetPortState();
  EXPECT_THROW(
      cmd.queryClient(localhost(), queriedPortNames, state),
      std::invalid_argument);
}

/*
 * Helper function to create aggregate port entries for testing
 */
std::vector<AggregatePortThrift> createAggregatePortEntriesForSetPortState() {
  std::vector<AggregatePortThrift> aggregatePorts;

  // Create Port-Channel1 with members eth1/5/1 and eth1/5/2
  AggregatePortThrift aggPort1;
  aggPort1.name() = "Port-Channel1";
  aggPort1.description() = "Test aggregate port 1";
  aggPort1.key() = 1001;
  aggPort1.minimumLinkCount() = 1;

  AggregatePortMemberThrift member1;
  member1.memberPortID() = 1;
  member1.isForwarding() = true;
  member1.rate() = LacpPortRateThrift::FAST;
  aggPort1.memberPorts()->push_back(member1);

  AggregatePortMemberThrift member2;
  member2.memberPortID() = 2;
  member2.isForwarding() = true;
  member2.rate() = LacpPortRateThrift::FAST;
  aggPort1.memberPorts()->push_back(member2);

  aggregatePorts.push_back(aggPort1);

  // Create Port-Channel2 with member eth1/5/3
  AggregatePortThrift aggPort2;
  aggPort2.name() = "Port-Channel2";
  aggPort2.description() = "Test aggregate port 2";
  aggPort2.key() = 1002;
  aggPort2.minimumLinkCount() = 1;

  AggregatePortMemberThrift member3;
  member3.memberPortID() = 3;
  member3.isForwarding() = true;
  member3.rate() = LacpPortRateThrift::FAST;
  aggPort2.memberPorts()->push_back(member3);

  aggregatePorts.push_back(aggPort2);

  return aggregatePorts;
}

TEST_F(CmdSetPortStateTestFixture, disableAggregatePort) {
  setupMockedAgentServer();
  std::vector<std::string> queriedPortNames = {"Port-Channel1"};
  // Port-Channel1 has members eth1/5/1 (id=1) and eth1/5/2 (id=2)
  std::vector<std::string> state = {"disable"};

  auto aggregatePorts = createAggregatePortEntriesForSetPortState();

  EXPECT_CALL(getMockAgent(), getAllPortInfo(_))
      .WillOnce(Invoke([&](auto& entries) { entries = portEntries; }));
  EXPECT_CALL(getMockAgent(), getAggregatePortTable(_))
      .WillOnce(Invoke([&](auto& entries) { entries = aggregatePorts; }));

  EXPECT_CALL(getMockAgent(), setPortState(_, _))
      .WillOnce(Invoke([&](int32_t, bool) {
        portEntries[1].adminState() = PortAdminState::DISABLED;
      }))
      .WillOnce(Invoke([&](int32_t, bool) {
        portEntries[2].adminState() = PortAdminState::DISABLED;
      }));

  EXPECT_EQ(portEntries[1].adminState(), PortAdminState::ENABLED);
  EXPECT_EQ(portEntries[2].adminState(), PortAdminState::DISABLED);

  auto cmd = CmdSetPortState();
  auto model = cmd.queryClient(localhost(), queriedPortNames, state);

  EXPECT_EQ(portEntries[1].adminState(), PortAdminState::DISABLED);
  EXPECT_EQ(portEntries[2].adminState(), PortAdminState::DISABLED);
  // Both member ports should be disabled
  EXPECT_EQ(model, "Disabling port eth1/5/1\nDisabling port eth1/5/2\n");
}

TEST_F(CmdSetPortStateTestFixture, enableAggregatePort) {
  setupMockedAgentServer();
  std::vector<std::string> queriedPortNames = {"Port-Channel2"};
  // Port-Channel2 has member eth1/5/3 (id=3)
  std::vector<std::string> state = {"enable"};

  auto aggregatePorts = createAggregatePortEntriesForSetPortState();

  EXPECT_CALL(getMockAgent(), getAllPortInfo(_))
      .WillOnce(Invoke([&](auto& entries) { entries = portEntries; }));
  EXPECT_CALL(getMockAgent(), getAggregatePortTable(_))
      .WillOnce(Invoke([&](auto& entries) { entries = aggregatePorts; }));

  EXPECT_CALL(getMockAgent(), setPortState(_, _))
      .WillOnce(Invoke([&](int32_t, bool) {
        portEntries[3].adminState() = PortAdminState::ENABLED;
      }));

  EXPECT_EQ(portEntries[3].adminState(), PortAdminState::ENABLED);

  auto cmd = CmdSetPortState();
  auto model = cmd.queryClient(localhost(), queriedPortNames, state);

  EXPECT_EQ(portEntries[3].adminState(), PortAdminState::ENABLED);
  EXPECT_EQ(model, "Enabling port eth1/5/3\n");
}

TEST_F(CmdSetPortStateTestFixture, mixPortAndAggregatePort) {
  setupMockedAgentServer();
  // Mix individual port and aggregate port
  std::vector<std::string> queriedPortNames = {"eth1/5/3", "Port-Channel1"};
  // eth1/5/3 (id=3) + Port-Channel1 members eth1/5/1 (id=1) and eth1/5/2 (id=2)
  std::vector<std::string> state = {"disable"};

  auto aggregatePorts = createAggregatePortEntriesForSetPortState();

  EXPECT_CALL(getMockAgent(), getAllPortInfo(_))
      .WillOnce(Invoke([&](auto& entries) { entries = portEntries; }));
  EXPECT_CALL(getMockAgent(), getAggregatePortTable(_))
      .WillOnce(Invoke([&](auto& entries) { entries = aggregatePorts; }));

  EXPECT_CALL(getMockAgent(), setPortState(_, _))
      .WillOnce(Invoke([&](int32_t, bool) {
        portEntries[3].adminState() = PortAdminState::DISABLED;
      }))
      .WillOnce(Invoke([&](int32_t, bool) {
        portEntries[1].adminState() = PortAdminState::DISABLED;
      }))
      .WillOnce(Invoke([&](int32_t, bool) {
        portEntries[2].adminState() = PortAdminState::DISABLED;
      }));

  auto cmd = CmdSetPortState();
  auto model = cmd.queryClient(localhost(), queriedPortNames, state);

  EXPECT_EQ(portEntries[1].adminState(), PortAdminState::DISABLED);
  EXPECT_EQ(portEntries[2].adminState(), PortAdminState::DISABLED);
  EXPECT_EQ(portEntries[3].adminState(), PortAdminState::DISABLED);
}

TEST_F(CmdSetPortStateTestFixture, nonexistentAggregatePort) {
  std::vector<std::string> queriedPortNames = {"Port-Channel99"};
  auto aggregatePorts = createAggregatePortEntriesForSetPortState();
  EXPECT_THROW(
      getQueriedPortIds(portEntries, queriedPortNames, aggregatePorts),
      std::runtime_error);
}

} // namespace facebook::fboss
