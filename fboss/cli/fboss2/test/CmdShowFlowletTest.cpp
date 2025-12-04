// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <folly/IPAddressV4.h>

#include <fboss/agent/if/gen-cpp2/common_types.h>
#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"

#include "fboss/cli/fboss2/commands/show/flowlet/CmdShowFlowlet.h"
#include "fboss/cli/fboss2/commands/show/flowlet/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"
#include "nettools/common/TestUtils.h"

using namespace ::testing;

namespace facebook::fboss {

/*
 * Set up test data
 */
std::vector<EcmpDetails> createEcmpEntries() {
  std::vector<EcmpDetails> ecmpEntries;
  EcmpDetails ecmpEntry1, ecmpEntry2;

  // ecmpEntry1
  ecmpEntry1.ecmpId() = 200000;
  ecmpEntry1.flowletEnabled() = true;
  ecmpEntry1.flowletInterval() = 512;
  ecmpEntry1.flowletTableSize() = 2048;
  ecmpEntries.emplace_back(ecmpEntry1);

  // ecmpEntry2
  ecmpEntry2.ecmpId() = 200001;
  ecmpEntry2.flowletEnabled() = true;
  ecmpEntry2.flowletInterval() = 256;
  ecmpEntry2.flowletTableSize() = 1024;
  ecmpEntries.emplace_back(ecmpEntry2);
  return ecmpEntries;
}

cli::ShowEcmpEntryModel createEcmpEntryModel() {
  cli::ShowEcmpEntryModel model;
  cli::EcmpEntry entry1, entry2;

  // entry1
  entry1.ecmpId() = 200000;
  entry1.flowletEnabled() = true;
  entry1.flowletInterval() = 512;
  entry1.flowletTableSize() = 2048;

  // entry2
  entry2.ecmpId() = 200001;
  entry2.flowletEnabled() = true;
  entry2.flowletInterval() = 256;
  entry2.flowletTableSize() = 1024;

  model.ecmpEntries() = {entry1, entry2};
  return model;
}

class CmdShowFlowletTestFixture : public CmdHandlerTestBase {
 public:
  std::vector<facebook::fboss::EcmpDetails> ecmpEntries;
  cli::ShowEcmpEntryModel normalizedModel;

  void SetUp() override {
    CmdHandlerTestBase::SetUp();
    ecmpEntries = createEcmpEntries();
    normalizedModel = createEcmpEntryModel();
  }
};

TEST_F(CmdShowFlowletTestFixture, queryClient) {
  setupMockedAgentServer();
  EXPECT_CALL(getMockAgent(), getAllEcmpDetails(_))
      .WillOnce(Invoke([&](auto& entries) { entries = ecmpEntries; }));

  auto cmd = CmdShowFlowlet();
  auto model = cmd.queryClient(localhost());

  EXPECT_THRIFT_EQ(normalizedModel, model);
}

TEST_F(CmdShowFlowletTestFixture, printOutput) {
  std::stringstream ss;
  CmdShowFlowlet().printOutput(normalizedModel, ss);

  std::string output = ss.str();
  std::string expectOutput = R"(  ECMP ID: 200000
    Flowlet Enabled: true
    Flowlet Interval: 512
    Flowlet Table Size: 2048
  ECMP ID: 200001
    Flowlet Enabled: true
    Flowlet Interval: 256
    Flowlet Table Size: 1024
)";
  EXPECT_EQ(output, expectOutput);
}

} // namespace facebook::fboss
