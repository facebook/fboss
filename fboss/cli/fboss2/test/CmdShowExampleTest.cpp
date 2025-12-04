// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "fboss/cli/fboss2/commands/show/example/CmdShowExample.h"
#include "fboss/cli/fboss2/commands/show/example/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"
#include "nettools/common/TestUtils.h"

using namespace ::testing;
namespace facebook::fboss {

/*
 * Set up test data
 */
std::map<int32_t, facebook::fboss::PortInfoThrift> createQueriedData() {
  std::map<int32_t, facebook::fboss::PortInfoThrift> query;

  facebook::fboss::PortInfoThrift data1;
  data1.portId() = 1;
  data1.name() = "eth1/5/1";
  query[folly::copy(data1.portId().value())] = data1;

  fboss::PortInfoThrift data2;
  data2.portId() = 2;
  data2.name() = "eth1/5/2";
  query[folly::copy(data2.portId().value())] = data2;

  return query;
}

/*
 * Setup expected normalized data
 */
cli::ShowExampleModel createNormalizedData() {
  cli::ShowExampleModel model;

  cli::ExampleData data1;
  data1.id() = 1;
  data1.name() = "eth1/5/1";
  model.exampleData()->push_back(data1);

  cli::ExampleData data2;
  data2.id() = 2;
  data2.name() = "eth1/5/2";
  model.exampleData()->push_back(data2);

  return model;
}

class CmdShowExampleTestFixture : public CmdHandlerTestBase {
 public:
  std::map<int32_t, facebook::fboss::PortInfoThrift> queriedData;
  cli::ShowExampleModel normalizedModel;

  void SetUp() override {
    CmdHandlerTestBase::SetUp();
    queriedData = createQueriedData();
    normalizedModel = createNormalizedData();
  }
};

TEST_F(CmdShowExampleTestFixture, queryClient) {
  // CmdHandlerTestBase provides this helper to create a mocked wedge_agent
  // server
  setupMockedAgentServer();
  // We can then mock agent calls that our command will use
  EXPECT_CALL(getMockAgent(), getAllPortInfo(_))
      .WillOnce(Invoke([&](auto& entries) { entries = queriedData; }));

  // Query our mocked server and verify the normalized model returned
  CmdShowExampleTraits::ObjectArgType queriedEntries;
  auto results = CmdShowExample().queryClient(localhost(), queriedEntries);

  EXPECT_THRIFT_EQ(normalizedModel, results);
}

TEST_F(CmdShowExampleTestFixture, printOutput) {
  std::stringstream ss;
  CmdShowExample().printOutput(normalizedModel, ss);

  std::string output = ss.str();
  std::string expectOutput =
      " ID  Name     \n"
      "-----------------\n"
      " 1   eth1/5/1 \n"
      " 2   eth1/5/2 \n"
      "\n";
  EXPECT_EQ(output, expectOutput);
}

} // namespace facebook::fboss
