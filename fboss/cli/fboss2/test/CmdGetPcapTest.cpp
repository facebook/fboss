// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "fboss/cli/fboss2/commands/get/pcap/CmdGetPcap.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"
#include "nettools/common/TestUtils.h"

using namespace ::testing;

namespace facebook::fboss {

std::string createExpectedGetPcapOutput() {
  return "Getting a copy of the packet capture \"packet_capture.pcap\" from test.host...";
}

class CmdGetPcapTestFixture : public CmdHandlerTestBase {
 public:
  std::string expectedGetPcapOutput;

  void SetUp() override {
    CmdHandlerTestBase::SetUp();
    expectedGetPcapOutput = createExpectedGetPcapOutput();
  }
};

TEST_F(CmdGetPcapTestFixture, queryClient) {
  auto cmd = CmdGetPcap();
  auto getPcapOutput = cmd.queryClient(localhost());
  EXPECT_THRIFT_EQ(getPcapOutput, expectedGetPcapOutput);
}

TEST_F(CmdGetPcapTestFixture, printOutput) {
  std::stringstream ss;
  CmdGetPcap().printOutput(expectedGetPcapOutput, ss);

  std::string output = ss.str();
  std::string expectedOutput =
      "Getting a copy of the packet capture \"packet_capture.pcap\" from test.host...\n";

  EXPECT_EQ(output, expectedOutput);
}

} // namespace facebook::fboss
