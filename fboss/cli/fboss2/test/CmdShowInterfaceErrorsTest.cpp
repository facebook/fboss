// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <folly/IPAddressV4.h>

#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/cli/fboss2/utils/CmdClientUtils.h"

#include "fboss/cli/fboss2/commands/show/interface/errors/CmdShowInterfaceErrors.h"
#include "fboss/cli/fboss2/commands/show/interface/errors/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"
#include "nettools/common/TestUtils.h"

using namespace ::testing;

namespace facebook::fboss {

/*
 * Set up test data
 */
std::map<int32_t, PortInfoThrift> createInterfaceErrorsEntries() {
  std::map<int32_t, PortInfoThrift> portMap;

  PortInfoThrift portEntry1;
  portEntry1.portId_ref() = 1;
  portEntry1.name_ref() = "eth1/1/1";

  PortCounters inputCounter;
  PortCounters outputCounter;
  PortErrors inputErrors;
  PortErrors outputErrors;
  inputErrors.errors_ref() = 0;
  inputErrors.discards_ref() = 0;
  outputErrors.errors_ref() = 0;
  outputErrors.discards_ref() = 0;

  inputCounter.errors_ref() = inputErrors;
  outputCounter.errors_ref() = outputErrors;

  portEntry1.input_ref() = inputCounter;
  portEntry1.output_ref() = outputCounter;

  PortInfoThrift portEntry2;
  portEntry2.portId_ref() = 2;
  portEntry2.name_ref() = "eth2/1/1";

  PortCounters inputCounter2;
  PortCounters outputCounter2;
  PortErrors inputErrors2;
  PortErrors outputErrors2;
  inputErrors2.errors_ref() = 100;
  inputErrors2.discards_ref() = 0;
  outputErrors2.errors_ref() = 100;
  outputErrors2.discards_ref() = 0;

  inputCounter2.errors_ref() = inputErrors2;
  outputCounter2.errors_ref() = outputErrors2;

  portEntry2.input_ref() = inputCounter2;
  portEntry2.output_ref() = outputCounter2;

  PortInfoThrift portEntry3;
  portEntry3.portId_ref() = 3;
  portEntry3.name_ref() = "eth3/1/1";

  PortCounters inputCounter3;
  PortCounters outputCounter3;
  PortErrors inputErrors3;
  PortErrors outputErrors3;
  inputErrors3.errors_ref() = 0;
  inputErrors3.discards_ref() = 100;
  outputErrors3.errors_ref() = 0;
  outputErrors3.discards_ref() = 100;

  inputCounter3.errors_ref() = inputErrors3;
  outputCounter3.errors_ref() = outputErrors3;

  portEntry3.input_ref() = inputCounter3;
  portEntry3.output_ref() = outputCounter3;

  portMap[portEntry1.get_portId()] = portEntry1;
  portMap[portEntry2.get_portId()] = portEntry2;
  portMap[portEntry3.get_portId()] = portEntry3;

  return portMap;
}

class CmdShowInterfaceErrorsTestFixture : public CmdHandlerTestBase {
 public:
  std::map<int32_t, facebook::fboss::PortInfoThrift> portEntries;
  CmdShowInterfaceErrorsTraits::ObjectArgType queriedEntries;
  void SetUp() override {
    CmdHandlerTestBase::SetUp();
    portEntries = createInterfaceErrorsEntries();
  }
};

TEST_F(CmdShowInterfaceErrorsTestFixture, queryClient) {
  setupMockedAgentServer();
  EXPECT_CALL(getMockAgent(), getAllPortInfo(_))
      .WillOnce(Invoke(
          [&](auto& countersEntries) { countersEntries = portEntries; }));

  auto cmd = CmdShowInterfaceErrors();
  auto result = cmd.queryClient(localhost(), queriedEntries);
  auto model = cmd.createModel(portEntries, queriedEntries);

  EXPECT_THRIFT_EQ(result, model);
}

TEST_F(CmdShowInterfaceErrorsTestFixture, createModel) {
  auto cmd = CmdShowInterfaceErrors();
  auto model = cmd.createModel(portEntries, queriedEntries);
  auto errorCounters = model.get_error_counters();

  EXPECT_EQ(errorCounters.size(), 3);

  EXPECT_EQ(errorCounters[0].get_interfaceName(), "eth1/1/1");
  EXPECT_EQ(errorCounters[0].get_inputErrors(), 0);
  EXPECT_EQ(errorCounters[0].get_inputDiscards(), 0);
  EXPECT_EQ(errorCounters[0].get_outputErrors(), 0);
  EXPECT_EQ(errorCounters[0].get_outputDiscards(), 0);

  EXPECT_EQ(errorCounters[1].get_interfaceName(), "eth2/1/1");
  EXPECT_EQ(errorCounters[1].get_inputErrors(), 100);
  EXPECT_EQ(errorCounters[1].get_inputDiscards(), 0);
  EXPECT_EQ(errorCounters[1].get_outputErrors(), 100);
  EXPECT_EQ(errorCounters[1].get_outputDiscards(), 0);

  EXPECT_EQ(errorCounters[2].get_interfaceName(), "eth3/1/1");
  EXPECT_EQ(errorCounters[2].get_inputErrors(), 0);
  EXPECT_EQ(errorCounters[2].get_inputDiscards(), 100);
  EXPECT_EQ(errorCounters[2].get_outputErrors(), 0);
  EXPECT_EQ(errorCounters[2].get_outputDiscards(), 100);
}

TEST_F(CmdShowInterfaceErrorsTestFixture, printOutput) {
  auto cmd = CmdShowInterfaceErrors();
  auto model = cmd.createModel(portEntries, queriedEntries);

  std::stringstream ss;
  cmd.printOutput(model, ss);

  std::string output = ss.str();
  std::string expectOutput =
      " Interface Name  Input Errors  Input Discards  Output Errors  Output Discards \n"
      "------------------------------------------------------------------------------------\n"
      " eth1/1/1        0             0               0              0               \n"
      " eth2/1/1        100           0               100            0               \n"
      " eth3/1/1        0             100             0              100             \n\n";
  EXPECT_EQ(output, expectOutput);
}

} // namespace facebook::fboss
