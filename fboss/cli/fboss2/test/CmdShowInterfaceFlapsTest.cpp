// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "fboss/cli/fboss2/commands/show/hardware/CmdShowHardware.h"
#include "fboss/cli/fboss2/commands/show/interface/flaps/CmdShowInterfaceFlaps.h"
#include "fboss/cli/fboss2/commands/show/interface/flaps/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"
#include "nettools/common/TestUtils.h"

using namespace ::testing;

namespace facebook::fboss {

/*
 * Set up test data
 */

std::map<std::string, std::int64_t> createQueriedData() {
  std::map<std::string, std::int64_t> fb303_counters;
  fb303_counters["eth1/1/1.link_state.flap.sum.60"] = 0;
  fb303_counters["eth1/1/1.link_state.flap.sum.600"] = 0;
  fb303_counters["eth1/1/1.link_state.flap.sum.3600"] = 0;
  fb303_counters["eth1/1/1.link_state.flap.sum"] = 0;

  fb303_counters["eth2/1/1.link_state.flap.sum.60"] = 1;
  fb303_counters["eth2/1/1.link_state.flap.sum.600"] = 2;
  fb303_counters["eth2/1/1.link_state.flap.sum.3600"] = 3;
  fb303_counters["eth2/1/1.link_state.flap.sum"] = 6;

  fb303_counters["eth3/1/1.link_state.flap.sum.60"] = 1000;
  fb303_counters["eth3/1/1.link_state.flap.sum.600"] = 10000;
  fb303_counters["eth3/1/1.link_state.flap.sum.3600"] = 100000;
  fb303_counters["eth3/1/1.link_state.flap.sum"] = 111000;

  return fb303_counters;
}

std::vector<std::string> createDistinctInterfaceNames() {
  std::vector<std::string> ifNames = {"eth1/1/1", "eth2/1/1", "eth3/1/1"};
  return ifNames;
}

class CmdShowInterfaceFlapsTestFixture : public CmdHandlerTestBase {
 public:
  std::map<std::string, std::int64_t> queriedData;
  std::vector<std::string> ifNames;
  std::vector<std::string> queriedPorts;

  void SetUp() override {
    CmdHandlerTestBase::SetUp();
    queriedData = createQueriedData();
    ifNames = createDistinctInterfaceNames();
  }
};

TEST_F(CmdShowInterfaceFlapsTestFixture, queryClient) {
  setupMockedAgentServer();

  /* This unit test is a special case because the thrift spec for
  getRegexCounters uses "thread = eb".  This requires a pretty ugly mock
  definition and call to work */
  std::map<std::string, std::int64_t> response = queriedData;
  EXPECT_CALL(getMockAgent(), async_eb_getRegexCounters(_, _))
      .WillOnce(Invoke([&response](auto callback, Unused) {
        callback->result(std::move(response));
      }));
  auto results = CmdShowInterfaceFlaps().queryClient(localhost(), queriedPorts);

  /* queryClient returns the output from createModel so this is a bit of a
  duplicate test as the one below but still worth checking */
  auto cmd = CmdShowInterfaceFlaps();
  auto model = cmd.createModel(ifNames, queriedData, queriedPorts);
  EXPECT_THRIFT_EQ(model, results);
}

TEST_F(CmdShowInterfaceFlapsTestFixture, createModel) {
  auto cmd = CmdShowInterfaceFlaps();
  auto model = cmd.createModel(ifNames, queriedData, queriedPorts);
  auto flapsEntries = model.flap_counters().value();

  EXPECT_EQ(flapsEntries.size(), 3);

  EXPECT_EQ(flapsEntries[0].get_interfaceName(), "eth1/1/1");
  EXPECT_EQ(flapsEntries[0].get_oneMinute(), 0);
  EXPECT_EQ(flapsEntries[0].get_tenMinute(), 0);
  EXPECT_EQ(flapsEntries[0].get_oneHour(), 0);
  EXPECT_EQ(flapsEntries[0].get_totalFlaps(), 0);

  EXPECT_EQ(flapsEntries[1].get_interfaceName(), "eth2/1/1");
  EXPECT_EQ(flapsEntries[1].get_oneMinute(), 1);
  EXPECT_EQ(flapsEntries[1].get_tenMinute(), 2);
  EXPECT_EQ(flapsEntries[1].get_oneHour(), 3);
  EXPECT_EQ(flapsEntries[1].get_totalFlaps(), 6);

  EXPECT_EQ(flapsEntries[2].get_interfaceName(), "eth3/1/1");
  EXPECT_EQ(flapsEntries[2].get_oneMinute(), 1000);
  EXPECT_EQ(flapsEntries[2].get_tenMinute(), 10000);
  EXPECT_EQ(flapsEntries[2].get_oneHour(), 100000);
  EXPECT_EQ(flapsEntries[2].get_totalFlaps(), 111000);
}

TEST_F(CmdShowInterfaceFlapsTestFixture, printOutput) {
  auto cmd = CmdShowInterfaceFlaps();
  auto model = cmd.createModel(ifNames, queriedData, queriedPorts);

  std::stringstream ss;
  cmd.printOutput(model, ss);

  std::string output = ss.str();
  std::string expectOutput =
      " Interface Name  1 Min  10 Min  60 Min  Total (since last reboot) \n"
      "------------------------------------------------------------------------\n"
      " eth1/1/1        0      0       0       0                         \n"
      " eth2/1/1        1      2       3       6                         \n"
      " eth3/1/1        1000   10000   100000  111000                    \n\n";

  EXPECT_EQ(output, expectOutput);
}
} // namespace facebook::fboss
