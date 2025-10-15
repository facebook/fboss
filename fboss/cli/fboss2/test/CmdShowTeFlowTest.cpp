// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <folly/IPAddressV4.h>

#include <fboss/agent/if/gen-cpp2/common_types.h>
#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"

#include "fboss/cli/fboss2/commands/show/teflow/CmdShowTeFlow.h"
#include "fboss/cli/fboss2/commands/show/teflow/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"
#include "nettools/common/TestUtils.h"

using namespace ::testing;

namespace facebook::fboss {

/*
 * Set up test data
 */
std::vector<TeFlowDetails> createTeFlowEntries() {
  std::vector<TeFlowDetails> teFlowEntries;
  TeFlowDetails teFlowEntry1, teFlowEntry2;

  // flowEntry1
  IpPrefix ipPrefix1;
  ipPrefix1.ip() =
      facebook::network::toBinaryAddress(folly::IPAddressV6("2401:db00::"));
  ipPrefix1.prefixLength() = 32;

  std::vector<NextHopThrift> nextHops1;
  NextHopThrift nextHop1;
  nextHop1.address() = facebook::network::toBinaryAddress(
      folly::IPAddressV6("2401:db00:e32f:8fc::2"));
  nextHop1.weight() = 1;
  nextHops1.emplace_back(nextHop1);

  teFlowEntry1.flow()->srcPort() = 100;
  teFlowEntry1.flow()->dstPrefix() = ipPrefix1;
  teFlowEntry1.enabled() = true;
  teFlowEntry1.nexthops() = nextHops1;
  teFlowEntry1.resolvedNexthops() = nextHops1;
  teFlowEntry1.counterID() = "counter1";

  teFlowEntries.emplace_back(teFlowEntry1);

  // flowEntry2
  IpPrefix ipPrefix2;
  ipPrefix2.ip() =
      facebook::network::toBinaryAddress(folly::IPAddressV6("2401:db01::"));
  ipPrefix2.prefixLength() = 32;

  std::vector<NextHopThrift> nextHops2;
  NextHopThrift nextHop2_1;
  nextHop2_1.address() = facebook::network::toBinaryAddress(
      folly::IPAddressV6("2401:db00:e32f:8fc::2"));
  nextHop2_1.weight() = 1;
  nextHops2.emplace_back(nextHop2_1);
  NextHopThrift nextHop2_2;
  nextHop2_2.address() = facebook::network::toBinaryAddress(
      folly::IPAddressV6("2401:db00:e32f:8fc::3"));
  nextHop2_2.weight() = 1;
  nextHops2.emplace_back(nextHop2_2);

  teFlowEntry2.flow()->srcPort() = 100;
  teFlowEntry2.flow()->dstPrefix() = ipPrefix2;
  teFlowEntry2.enabled() = true;
  teFlowEntry2.nexthops() = nextHops2;
  teFlowEntry2.resolvedNexthops() = nextHops2;
  teFlowEntry2.counterID() = "counter2";

  teFlowEntries.emplace_back(teFlowEntry2);
  return teFlowEntries;
}

std::map<int32_t, PortInfoThrift> createTeFlowTestPortEntries() {
  std::map<int32_t, PortInfoThrift> portMap;

  PortInfoThrift portEntry1;
  portEntry1.portId() = 100;
  portEntry1.name() = "eth1/5/1";
  portEntry1.adminState() = PortAdminState::ENABLED;
  portEntry1.operState() = PortOperState::DOWN;
  portEntry1.speedMbps() = 100000;
  portEntry1.profileID() = "PROFILE_100G_4_NRZ_CL91_COPPER";
  TransceiverIdxThrift tcvr1;
  tcvr1.transceiverId() = 0;
  portEntry1.transceiverIdx() = tcvr1;

  portMap[folly::copy(portEntry1.portId().value())] = portEntry1;
  return portMap;
}

cli::ShowTeFlowEntryModel createTeFlowEntryModel() {
  cli::ShowTeFlowEntryModel model;
  cli::TeFlowEntry entry1, entry2;

  // entry1
  entry1.dstIp() = "2401:db00::";
  entry1.dstIpPrefixLength() = 32;
  entry1.srcPort() = 100;
  entry1.srcPortName() = "eth1/5/1";
  entry1.enabled() = true;

  cli::NextHopInfo nextHopInfo1_1;
  nextHopInfo1_1.addr() = "2401:db00:e32f:8fc::2";
  nextHopInfo1_1.weight() = 1;

  entry1.nextHops()->emplace_back(nextHopInfo1_1);
  entry1.resolvedNextHops()->emplace_back(nextHopInfo1_1);
  entry1.counterID() = "counter1";

  // entry2
  entry2.dstIp() = "2401:db01::";
  entry2.dstIpPrefixLength() = 32;
  entry2.srcPort() = 100;
  entry2.srcPortName() = "eth1/5/1";
  entry2.enabled() = true;

  cli::NextHopInfo nextHopInfo2_1;
  nextHopInfo2_1.addr() = "2401:db00:e32f:8fc::2";
  nextHopInfo2_1.weight() = 1;
  cli::NextHopInfo nextHopInfo2_2;
  nextHopInfo2_2.addr() = "2401:db00:e32f:8fc::3";
  nextHopInfo2_2.weight() = 1;

  entry2.nextHops()->emplace_back(nextHopInfo2_1);
  entry2.nextHops()->emplace_back(nextHopInfo2_2);
  entry2.resolvedNextHops()->emplace_back(nextHopInfo2_1);
  entry2.resolvedNextHops()->emplace_back(nextHopInfo2_2);
  entry2.counterID() = "counter2";
  model.flowEntries() = {entry1, entry2};
  return model;
}

class CmdShowTeFlowTestFixture : public CmdHandlerTestBase {
 public:
  std::vector<facebook::fboss::TeFlowDetails> teFlowEntries;
  std::map<int32_t, facebook::fboss::PortInfoThrift> portEntries;
  cli::ShowTeFlowEntryModel normalizedModel;

  void SetUp() override {
    CmdHandlerTestBase::SetUp();
    teFlowEntries = createTeFlowEntries();
    portEntries = createTeFlowTestPortEntries();
    normalizedModel = createTeFlowEntryModel();
  }
};

TEST_F(CmdShowTeFlowTestFixture, queryClient) {
  setupMockedAgentServer();
  EXPECT_CALL(getMockAgent(), getTeFlowTableDetails(_))
      .WillOnce(Invoke([&](auto& entries) { entries = teFlowEntries; }));
  EXPECT_CALL(getMockAgent(), getAllPortInfo(_))
      .WillOnce(Invoke([&](auto& entries) { entries = portEntries; }));

  auto cmd = CmdShowTeFlow();
  CmdShowTeFlowTraits::ObjectArgType queriedEntries;
  auto model = cmd.queryClient(localhost(), queriedEntries);

  EXPECT_THRIFT_EQ(normalizedModel, model);
}

TEST_F(CmdShowTeFlowTestFixture, printOutput) {
  std::stringstream ss;
  CmdShowTeFlow().printOutput(normalizedModel, ss);

  std::string output = ss.str();
  std::string expectOutput = R"(
Flow key: dst prefix 2401:db00::/32, src port eth1/5/1
Match Action:
  Counter ID: counter1
  Redirect to Nexthops:
    2401:db00:e32f:8fc::2 weight 1
State:
  Enabled: true
  Resolved Nexthops:
    2401:db00:e32f:8fc::2 weight 1

Flow key: dst prefix 2401:db01::/32, src port eth1/5/1
Match Action:
  Counter ID: counter2
  Redirect to Nexthops:
    2401:db00:e32f:8fc::2 weight 1
    2401:db00:e32f:8fc::3 weight 1
State:
  Enabled: true
  Resolved Nexthops:
    2401:db00:e32f:8fc::2 weight 1
    2401:db00:e32f:8fc::3 weight 1
)";
  EXPECT_EQ(output, expectOutput);
}

} // namespace facebook::fboss
