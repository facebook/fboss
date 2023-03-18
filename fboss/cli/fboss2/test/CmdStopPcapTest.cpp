// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/cli/fboss2/utils/CmdClientUtils.h"

#include "fboss/cli/fboss2/commands/stop/pcap/CmdStopPcap.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"
#include "nettools/common/TestUtils.h"

using namespace ::testing;

namespace facebook::fboss {

std::string createExpectedStopPcapOutput() {
  return "Stopping packet capture \"packet_capture\"";
}

class CmdStopPcapTestFixture : public CmdHandlerTestBase {
 public:
  std::string expectedStopPcapOutput;

  void SetUp() override {
    CmdHandlerTestBase::SetUp();
    expectedStopPcapOutput = createExpectedStopPcapOutput();
  }
};

TEST_F(CmdStopPcapTestFixture, queryClient) {
  setupMockedAgentServer();
  EXPECT_CALL(getMockAgent(), stopPktCapture(_))
      .WillOnce(Invoke([&](std::unique_ptr<std::string>) {}));

  auto cmd = CmdStopPcap();
  auto stopPcapOutput = cmd.queryClient(localhost());
  EXPECT_THRIFT_EQ(stopPcapOutput, expectedStopPcapOutput);
}

TEST_F(CmdStopPcapTestFixture, printOutput) {
  std::stringstream ss;
  CmdStopPcap().printOutput(expectedStopPcapOutput, ss);

  std::string output = ss.str();
  std::string expectedOutput = "Stopping packet capture \"packet_capture\"\n";

  EXPECT_EQ(output, expectedOutput);
}

} // namespace facebook::fboss
