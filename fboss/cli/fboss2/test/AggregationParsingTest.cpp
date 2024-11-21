// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <fboss/cli/fboss2/CmdGlobalOptions.h>
#include "fboss/cli/fboss2/commands/show/port/CmdShowPort.h"
#include "fboss/cli/fboss2/gen-cpp2/cli_types.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"

using namespace ::testing;
namespace facebook::fboss {
class AggregateParsingFixture : public CmdHandlerTestBase {
 public:
  void SetUp() override {
    CmdHandlerTestBase::SetUp();
  }
};

TEST_F(AggregateParsingFixture, validInputParsing) {
  std::string aggregateInput = "MIN(tcvrID)";
  auto cmd = CmdGlobalOptions();
  auto errorCode = cli::CliOptionResult::EOK;
  cmd.setAggregateInput(aggregateInput);
  const auto& parsedAggs = cmd.parseAggregate(errorCode);
  EXPECT_EQ(errorCode, cli::CliOptionResult::EOK);

  EXPECT_EQ(parsedAggs->aggOp, facebook::fboss::AggregateOpEnum::MIN);
  EXPECT_EQ(parsedAggs->columnName, "tcvrID");
  EXPECT_FALSE(parsedAggs->acrossHosts);
}

TEST_F(AggregateParsingFixture, noOperatorPassed) {
  std::string aggregateInput = "(id)";
  auto cmd = CmdGlobalOptions();
  auto errorCode = cli::CliOptionResult::EOK;
  cmd.setAggregateInput(aggregateInput);
  const auto& parsedAggs = cmd.parseAggregate(errorCode);
  EXPECT_EQ(errorCode, cli::CliOptionResult::AGG_ARGUMENT_ERROR);
  EXPECT_FALSE(parsedAggs.has_value());
}

TEST_F(AggregateParsingFixture, noColumnPassed) {
  std::string aggregateInput = "MAX()";
  auto cmd = CmdGlobalOptions();
  auto errorCode = cli::CliOptionResult::EOK;
  cmd.setAggregateInput(aggregateInput);
  const auto& parsedAggs = cmd.parseAggregate(errorCode);
  EXPECT_EQ(errorCode, cli::CliOptionResult::AGG_ARGUMENT_ERROR);
  EXPECT_FALSE(parsedAggs.has_value());
}

TEST_F(AggregateParsingFixture, emptyInput) {
  std::string aggregateInput = "";
  auto cmd = CmdGlobalOptions();
  auto errorCode = cli::CliOptionResult::EOK;
  cmd.setAggregateInput(aggregateInput);
  const auto& parsedAggs = cmd.parseAggregate(errorCode);
  EXPECT_EQ(errorCode, cli::CliOptionResult::EOK);
  EXPECT_FALSE(parsedAggs.has_value());
}

TEST_F(AggregateParsingFixture, illFormedInput_1) {
  std::string aggregateInput = "SUM(id";
  auto cmd = CmdGlobalOptions();
  auto errorCode = cli::CliOptionResult::EOK;
  cmd.setAggregateInput(aggregateInput);
  const auto& parsedAggs = cmd.parseAggregate(errorCode);
  EXPECT_EQ(errorCode, cli::CliOptionResult::AGG_ARGUMENT_ERROR);
  EXPECT_FALSE(parsedAggs.has_value());
}

TEST_F(AggregateParsingFixture, illFormedInput_2) {
  std::string aggregateInput = "AVG id";
  auto cmd = CmdGlobalOptions();
  auto errorCode = cli::CliOptionResult::EOK;
  cmd.setAggregateInput(aggregateInput);
  const auto& parsedAggs = cmd.parseAggregate(errorCode);
  EXPECT_EQ(errorCode, cli::CliOptionResult::AGG_ARGUMENT_ERROR);
  EXPECT_FALSE(parsedAggs.has_value());
}

TEST_F(AggregateParsingFixture, illFormedInput_3) {
  std::string aggregateInput = "COUNT name)";
  auto cmd = CmdGlobalOptions();
  auto errorCode = cli::CliOptionResult::EOK;
  cmd.setAggregateInput(aggregateInput);
  const auto& parsedAggs = cmd.parseAggregate(errorCode);
  EXPECT_EQ(errorCode, cli::CliOptionResult::AGG_ARGUMENT_ERROR);
  EXPECT_FALSE(parsedAggs.has_value());
}

TEST_F(AggregateParsingFixture, parsingAcrossDeviceAggregate) {
  bool acrossDevices = true;
  std::string aggregateInput = "COUNT(tcvrID)";
  auto cmd = CmdGlobalOptions();
  auto errorCode = cli::CliOptionResult::EOK;
  cmd.setAggregateInput(aggregateInput);
  cmd.aggregateAcrossDevices(acrossDevices);
  const auto& parsedAggs = cmd.parseAggregate(errorCode);
  EXPECT_EQ(errorCode, cli::CliOptionResult::EOK);
  EXPECT_TRUE(parsedAggs.has_value());
  EXPECT_EQ(parsedAggs->aggOp, facebook::fboss::AggregateOpEnum::COUNT);
  EXPECT_EQ(parsedAggs->columnName, "tcvrID");
  EXPECT_TRUE(parsedAggs->acrossHosts);
}

} // namespace facebook::fboss
