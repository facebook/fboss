// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "fboss/cli/fboss2/commands/show/route/CmdShowRouteCounters.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"

#include <thrift/lib/cpp2/reflection/testing.h>

using namespace ::testing;

namespace facebook::fboss {

std::map<std::string, HwSwitchCounter> createRouteCounters() {
  HwSwitchCounter counter1;
  counter1.bytes() = 100;
  counter1.packets() = 10;
  HwSwitchCounter counter2;
  counter2.bytes() = 200;
  return {{"counter1", counter1}, {"counter2", counter2}};
}

cli::ShowRouteCountersModel createRouteCountersModel() {
  cli::ShowRouteCountersModel model;
  cli::RouteCounterEntry entry1;
  entry1.counterID() = "counter1";
  entry1.bytes() = 100;
  entry1.packets() = 10;
  model.routeCounters()->push_back(entry1);
  cli::RouteCounterEntry entry2;
  entry2.counterID() = "counter2";
  entry2.bytes() = 200;
  model.routeCounters()->push_back(entry2);
  return model;
}

class CmdShowRouteCountersTestFixture : public CmdHandlerTestBase {};

TEST_F(CmdShowRouteCountersTestFixture, createModel) {
  const auto model = CmdShowRouteCounters().createModel(createRouteCounters());
  EXPECT_THRIFT_EQ(createRouteCountersModel(), model);
}

TEST_F(CmdShowRouteCountersTestFixture, queryClient) {
  setupMockedAgentServer();
  EXPECT_CALL(getMockAgent(), getRouteCounters(_))
      .WillOnce(
          Invoke([](auto& counters) { counters = createRouteCounters(); }));

  const auto model = CmdShowRouteCounters().queryClient(localhost());
  EXPECT_THRIFT_EQ(createRouteCountersModel(), model);
}

TEST_F(CmdShowRouteCountersTestFixture, printOutput) {
  std::stringstream output;
  CmdShowRouteCounters().printOutput(createRouteCountersModel(), output);
  EXPECT_THAT(output.str(), HasSubstr("Counter ID"));
  EXPECT_THAT(output.str(), HasSubstr("counter1"));
  EXPECT_THAT(output.str(), HasSubstr("100"));
  EXPECT_THAT(output.str(), HasSubstr("counter2"));
  EXPECT_THAT(output.str(), HasSubstr("-"));
}

TEST_F(CmdShowRouteCountersTestFixture, wikiDocHooks) {
  EXPECT_FALSE(CmdShowRouteCountersTraits::description().empty());
  EXPECT_FALSE(CmdShowRouteCounters::sampleModel().routeCounters()->empty());
}

} // namespace facebook::fboss
