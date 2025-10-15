// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <folly/IPAddressV4.h>

#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"

#include "fboss/cli/fboss2/commands/show/interface/counters/CmdShowInterfaceCounters.h"
#include "fboss/cli/fboss2/commands/show/interface/counters/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"
#include "nettools/common/TestUtils.h"

using namespace ::testing;

namespace facebook::fboss {

/*
 * Set up test data
 */
std::map<int32_t, PortInfoThrift> createInterfaceCountersEntries() {
  std::map<int32_t, PortInfoThrift> portMap;

  PortInfoThrift portEntry1;
  portEntry1.portId() = 1;
  portEntry1.name() = "eth1/1/1";

  PortCounters inputCounter;
  PortCounters outputCounter;

  inputCounter.bytes() = 0;
  inputCounter.ucastPkts() = 0;
  inputCounter.multicastPkts() = 0;
  inputCounter.broadcastPkts() = 0;

  outputCounter.bytes() = 0;
  outputCounter.ucastPkts() = 0;
  outputCounter.multicastPkts() = 0;
  outputCounter.broadcastPkts() = 0;

  portEntry1.input() = inputCounter;
  portEntry1.output() = outputCounter;

  PortInfoThrift portEntry2;
  portEntry2.portId() = 2;
  portEntry2.name() = "eth2/1/1";

  PortCounters inputCounter2;
  PortCounters outputCounter2;

  inputCounter2.bytes() = 100;
  inputCounter2.ucastPkts() = 100;
  inputCounter2.multicastPkts() = 100;
  inputCounter2.broadcastPkts() = 100;

  outputCounter2.bytes() = 100;
  outputCounter2.ucastPkts() = 100;
  outputCounter2.multicastPkts() = 100;
  outputCounter2.broadcastPkts() = 100;

  portEntry2.input() = inputCounter2;
  portEntry2.output() = outputCounter2;

  portMap[folly::copy(portEntry1.portId().value())] = portEntry1;
  portMap[folly::copy(portEntry2.portId().value())] = portEntry2;

  return portMap;
}

class CmdShowInterfaceCountersTestFixture : public CmdHandlerTestBase {
 public:
  std::map<int32_t, facebook::fboss::PortInfoThrift> portEntries;
  std::vector<std::string> queriedEntries;
  void SetUp() override {
    CmdHandlerTestBase::SetUp();
    portEntries = createInterfaceCountersEntries();
  }
};

TEST_F(CmdShowInterfaceCountersTestFixture, queryClient) {
  setupMockedAgentServer();
  EXPECT_CALL(getMockAgent(), getAllPortInfo(_))
      .WillOnce(Invoke(
          [&](auto& countersEntries) { countersEntries = portEntries; }));

  auto cmd = CmdShowInterfaceCounters();
  auto result = cmd.queryClient(localhost(), queriedEntries);
  auto model = cmd.createModel(portEntries, queriedEntries);

  EXPECT_THRIFT_EQ(model, result);
}

TEST_F(CmdShowInterfaceCountersTestFixture, createModel) {
  auto cmd = CmdShowInterfaceCounters();
  auto model = cmd.createModel(portEntries, queriedEntries);
  auto intCounters = model.int_counters().value();

  EXPECT_EQ(intCounters.size(), 2);
}

TEST_F(CmdShowInterfaceCountersTestFixture, printOutput) {
  auto cmd = CmdShowInterfaceCounters();
  auto model = cmd.createModel(portEntries, queriedEntries);

  std::stringstream ss;
  cmd.printOutput(model, ss);

  std::string output = ss.str();
  std::string expectOutput =
      " Interface Name  Bytes(in)  Unicast Pkts(in)  Multicast Pkts(in)  Broadcast Pkts(in)  Bytes(out)  Unicast Pkts(out)  Multicast Pkts(out)  Broadcast Pkts(out) \n"
      "------------------------------------------------------------------------------------------------------------------------------------------------------------------------\n"
      " eth1/1/1        0          0                 0                   0                   0           0                  0                    0                   \n"
      " eth2/1/1        100        100               100                 100                 100         100                100                  100                 \n\n";
  EXPECT_EQ(output, expectOutput);
}
} // namespace facebook::fboss
