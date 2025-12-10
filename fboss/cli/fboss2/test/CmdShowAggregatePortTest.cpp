// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <folly/IPAddressV4.h>

#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"

#include "fboss/cli/fboss2/commands/show/aggregateport/CmdShowAggregatePort.h"
#include "fboss/cli/fboss2/commands/show/aggregateport/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"
#include "nettools/common/TestUtils.h"

using namespace ::testing;

namespace facebook::fboss {

/*
 * Set up test data
 */
std::vector<AggregatePortThrift> createAggregatePortEntries() {
  std::vector<AggregatePortThrift> aggregatePorts;
  AggregatePortThrift aggregatePortEntry1, aggregatePortEntry2;
  aggregatePortEntry1.name() = "Port-Channel1";
  aggregatePortEntry1.description() = "Port Channel 1";
  aggregatePortEntry1.minimumLinkCount() = 2;
  aggregatePortEntry1.minimumLinkCountToUp() = 2;
  AggregatePortMemberThrift member1, member2;
  member1.memberPortID() = 1;
  member1.isForwarding() = true;
  member1.rate() = LacpPortRateThrift::FAST;
  aggregatePortEntry1.memberPorts()->emplace_back(member1);
  member2.memberPortID() = 2;
  member2.isForwarding() = true;
  member2.rate() = LacpPortRateThrift::FAST;
  aggregatePortEntry1.memberPorts()->emplace_back(member2);

  aggregatePortEntry2.name() = "Port-Channel2";
  aggregatePortEntry2.description() = "Port Channel 2";
  aggregatePortEntry2.minimumLinkCount() = 2;
  AggregatePortMemberThrift member3, member4;
  member3.memberPortID() = 3;
  member3.isForwarding() = true;
  member3.rate() = LacpPortRateThrift::FAST;
  aggregatePortEntry2.memberPorts()->emplace_back(member3);
  member4.memberPortID() = 4;
  member4.isForwarding() = true;
  member4.rate() = LacpPortRateThrift::FAST;
  aggregatePortEntry2.memberPorts()->emplace_back(member4);

  aggregatePorts.emplace_back(aggregatePortEntry1);
  aggregatePorts.emplace_back(aggregatePortEntry2);
  return aggregatePorts;
}

std::map<int32_t, PortInfoThrift> createTestPortEntries() {
  std::map<int32_t, PortInfoThrift> portMap;

  PortInfoThrift portEntry1;
  portEntry1.portId() = 1;
  portEntry1.name() = "eth1/5/1";
  portEntry1.adminState() = PortAdminState::ENABLED;
  portEntry1.operState() = PortOperState::DOWN;
  portEntry1.speedMbps() = 100000;
  portEntry1.profileID() = "PROFILE_100G_4_NRZ_CL91_COPPER";
  TransceiverIdxThrift tcvr1;
  tcvr1.transceiverId() = 0;
  portEntry1.transceiverIdx() = tcvr1;

  PortInfoThrift portEntry2;
  portEntry2.portId() = 2;
  portEntry2.name() = "eth1/5/2";
  portEntry2.adminState() = PortAdminState::DISABLED;
  portEntry2.operState() = PortOperState::DOWN;
  portEntry2.speedMbps() = 25000;
  portEntry2.profileID() = "PROFILE_25G_1_NRZ_CL74_COPPER";
  TransceiverIdxThrift tcvr2;
  tcvr2.transceiverId() = 1;
  portEntry2.transceiverIdx() = tcvr2;

  PortInfoThrift portEntry3;
  portEntry3.portId() = 3;
  portEntry3.name() = "eth1/5/3";
  portEntry3.adminState() = PortAdminState::ENABLED;
  portEntry3.operState() = PortOperState::UP;
  portEntry3.speedMbps() = 100000;
  portEntry3.profileID() = "PROFILE_100G_4_NRZ_CL91_COPPER";
  TransceiverIdxThrift tcvr3;
  tcvr3.transceiverId() = 2;
  portEntry3.transceiverIdx() = tcvr3;

  PortInfoThrift portEntry4;
  portEntry4.portId() = 4;
  portEntry4.name() = "eth1/5/4";
  portEntry4.adminState() = PortAdminState::ENABLED;
  portEntry4.operState() = PortOperState::UP;
  portEntry4.speedMbps() = 100000;
  portEntry4.profileID() = "PROFILE_100G_4_NRZ_CL91_COPPER";
  TransceiverIdxThrift tcvr4;
  tcvr4.transceiverId() = 2;
  portEntry4.transceiverIdx() = tcvr3;

  portMap[folly::copy(portEntry1.portId().value())] = portEntry1;
  portMap[folly::copy(portEntry2.portId().value())] = portEntry2;
  portMap[folly::copy(portEntry3.portId().value())] = portEntry3;
  portMap[folly::copy(portEntry4.portId().value())] = portEntry4;
  return portMap;
}

cli::ShowAggregatePortModel createAggregatePortModel() {
  cli::ShowAggregatePortModel model;

  cli::AggregatePortEntry entry1, entry2;
  entry1.name() = "Port-Channel1";
  entry1.description() = "Port Channel 1";
  entry1.activeMembers() = 2;
  entry1.configuredMembers() = 2;
  entry1.minMembers() = 2;
  entry1.minMembersToUp() = 2;
  cli::AggregateMemberPortEntry member1, member2;
  member1.name() = "eth1/5/1";
  member1.id() = 1;
  member1.isUp() = true;
  member1.lacpRate() = "Fast";
  entry1.members()->emplace_back(member1);
  member2.name() = "eth1/5/2";
  member2.id() = 2;
  member2.isUp() = true;
  member2.lacpRate() = "Fast";
  entry1.members()->emplace_back(member2);

  entry2.name() = "Port-Channel2";
  entry2.description() = "Port Channel 2";
  entry2.activeMembers() = 2;
  entry2.configuredMembers() = 2;
  entry2.minMembers() = 2;
  cli::AggregateMemberPortEntry member3, member4;
  member3.name() = "eth1/5/3";
  member3.id() = 3;
  member3.isUp() = true;
  member3.lacpRate() = "Fast";
  entry2.members()->emplace_back(member3);
  member4.name() = "eth1/5/4";
  member4.id() = 4;
  member4.isUp() = true;
  member4.lacpRate() = "Fast";
  entry2.members()->emplace_back(member4);
  model.aggregatePortEntries() = {entry1, entry2};
  return model;
}

class CmdShowAggregatePortTestFixture : public CmdHandlerTestBase {
 public:
  std::vector<facebook::fboss::AggregatePortThrift> aggregatePortEntries;
  std::map<int32_t, facebook::fboss::PortInfoThrift> portEntries;
  cli::ShowAggregatePortModel normalizedModel;

  void SetUp() override {
    CmdHandlerTestBase::SetUp();
    aggregatePortEntries = createAggregatePortEntries();
    portEntries = createTestPortEntries();
    normalizedModel = createAggregatePortModel();
  }
};

TEST_F(CmdShowAggregatePortTestFixture, queryClient) {
  setupMockedAgentServer();
  EXPECT_CALL(getMockAgent(), getAggregatePortTable(_))
      .WillOnce(Invoke([&](auto& entries) { entries = aggregatePortEntries; }));
  EXPECT_CALL(getMockAgent(), getAllPortInfo(_))
      .WillOnce(Invoke([&](auto& entries) { entries = portEntries; }));

  auto cmd = CmdShowAggregatePort();
  CmdShowAggregatePortTraits::ObjectArgType queriedEntries;
  auto model = cmd.queryClient(localhost(), queriedEntries);

  EXPECT_THRIFT_EQ(normalizedModel, model);
}

TEST_F(CmdShowAggregatePortTestFixture, printOutput) {
  std::stringstream ss;
  CmdShowAggregatePort().printOutput(normalizedModel, ss);

  std::string output = ss.str();
  std::string expectOutput =
      "\nPort name: Port-Channel1"
      "\nDescription: Port Channel 1"
      "\nActive members/Configured members/Min members: 2/2/[2, 2]"
      "\n\t Member:   eth1/5/1, id:   1, Up:  True, Rate: Fast"
      "\n\t Member:   eth1/5/2, id:   2, Up:  True, Rate: Fast"
      "\n"
      "\nPort name: Port-Channel2"
      "\nDescription: Port Channel 2"
      "\nActive members/Configured members/Min members: 2/2/2"
      "\n\t Member:   eth1/5/3, id:   3, Up:  True, Rate: Fast"
      "\n\t Member:   eth1/5/4, id:   4, Up:  True, Rate: Fast\n";
  EXPECT_EQ(output, expectOutput);
}

} // namespace facebook::fboss
