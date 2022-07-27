// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <folly/IPAddressV4.h>
#include "fboss/cli/fboss2/commands/show/agent/CmdShowAgentSsl.h"
#include "fboss/cli/fboss2/commands/show/agent/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"

using namespace ::testing;

namespace facebook::fboss {

class CmdShowAgentSslTestFixture : public CmdHandlerTestBase {
 public:
  void SetUp() override {
    CmdHandlerTestBase::SetUp();
  }
};

TEST_F(CmdShowAgentSslTestFixture, queryClientAndPrintOutput) {
  setupMockedAgentServer();
  EXPECT_CALL(getMockAgent(), getSSLPolicy())
      .WillOnce(Return(SSLType::DISABLED))
      .WillOnce(Return(SSLType::PERMITTED))
      .WillOnce(Return(SSLType::REQUIRED));

  auto cmd = CmdShowAgentSsl();
  std::stringstream ss;

  auto model = cmd.queryClient(localhost());
  EXPECT_EQ(
      "DISABLED (allow plaintext clients only)", model.get_AgentSslStatus());

  cmd.printOutput(model, ss);
  EXPECT_EQ(
      "Secure Thrift server SSL config: DISABLED (allow plaintext clients only)\n",
      ss.str());

  model = cmd.queryClient(localhost());
  EXPECT_EQ(
      "PERMITTED (allow plaintext & encrypted clients)",
      model.get_AgentSslStatus());

  ss.str("");
  cmd.printOutput(model, ss);
  EXPECT_EQ(
      "Secure Thrift server SSL config: PERMITTED (allow plaintext & encrypted clients)\n",
      ss.str());

  model = cmd.queryClient(localhost());
  EXPECT_EQ(
      "REQUIRED (allow encrypted clients only)", model.get_AgentSslStatus());

  ss.str("");
  cmd.printOutput(model, ss);
  EXPECT_EQ(
      "Secure Thrift server SSL config: REQUIRED (allow encrypted clients only)\n",
      ss.str());
}

} // namespace facebook::fboss
