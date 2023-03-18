// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/cli/fboss2/utils/CmdClientUtils.h"

#include "fboss/cli/fboss2/commands/start/pcap/CmdStartPcap.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"
#include "nettools/common/TestUtils.h"

using namespace ::testing;

namespace facebook::fboss {

std::string createExpectedStartPcapOutput() {
  return "Starting both Tx and Rx packet capture \"packet_capture\"";
}

class CmdStartPcapTestFixture : public CmdHandlerTestBase {
 public:
  std::string expectedStartPcapOutput;

  void SetUp() override {
    CmdHandlerTestBase::SetUp();
    expectedStartPcapOutput = createExpectedStartPcapOutput();
  }
};

TEST_F(CmdStartPcapTestFixture, queryClient) {
  setupMockedAgentServer();
  EXPECT_CALL(getMockAgent(), startPktCapture(_))
      .WillOnce(Invoke([&](std::unique_ptr<CaptureInfo>) {}));

  auto cmd = CmdStartPcap();
  auto startPcapOutput = cmd.queryClient(localhost());
  EXPECT_THRIFT_EQ(startPcapOutput, expectedStartPcapOutput);
}

TEST_F(CmdStartPcapTestFixture, printOutput) {
  std::stringstream ss;
  CmdStartPcap().printOutput(expectedStartPcapOutput, ss);

  std::string output = ss.str();
  std::string expectedOutput =
      "Starting both Tx and Rx packet capture \"packet_capture\"\n";

  EXPECT_EQ(output, expectedOutput);
}

} // namespace facebook::fboss
