// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <fboss/cli/fboss2/CmdGlobalOptions.h>
#include "fboss/cli/fboss2/commands/show/port/CmdShowPort.h"
#include "fboss/cli/fboss2/gen-cpp2/cli_types.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"

using namespace ::testing;
namespace facebook::fboss {
CmdGlobalOptions::AggregateOption createInvalidKeyInput() {
  CmdGlobalOptions::AggregateOption parsedAggregate;
  parsedAggregate.aggOp = CmdGlobalOptions().getAggregateOp("SUM");
  // no port_name column exists in the port thrift.
  parsedAggregate.columnName = "port_name";
  parsedAggregate.acrossHosts = true;
  return parsedAggregate;
}

CmdGlobalOptions::AggregateOption createValidInput() {
  CmdGlobalOptions::AggregateOption parsedAggregate;
  parsedAggregate.aggOp = CmdGlobalOptions().getAggregateOp("COUNT");
  parsedAggregate.columnName = "name";
  parsedAggregate.acrossHosts = false;
  return parsedAggregate;
}

class AggregateValidationFixture : public CmdHandlerTestBase {
 public:
  void SetUp() override {
    CmdHandlerTestBase::SetUp();
  }
};

TEST_F(AggregateValidationFixture, keyValidation) {
  const auto& invalidKeyInput = createInvalidKeyInput();
  auto errCode = CmdGlobalOptions::getInstance()->isValidAggregate(
      CmdShowPort().getValidAggs(), invalidKeyInput);
  EXPECT_EQ(errCode, cli::CliOptionResult::KEY_ERROR);
}

TEST_F(AggregateValidationFixture, incompatibeOperation) {
  // sum operator is not supported for string columns.
  std::string aggregateInput = "SUM(name)";
  auto cmd = CmdGlobalOptions();
  auto errorCode = cli::CliOptionResult::EOK;
  cmd.setAggregateInput(aggregateInput);
  const auto& parsedAggs = cmd.parseAggregate(errorCode);

  // parsing should work correctly for this, but validation should fail.
  EXPECT_EQ(errorCode, cli::CliOptionResult::EOK);
  EXPECT_TRUE(parsedAggs.has_value());

  errorCode = CmdGlobalOptions::getInstance()->isValidAggregate(
      CmdShowPort().getValidAggs(), parsedAggs);
  EXPECT_EQ(errorCode, cli::CliOptionResult::OP_ERROR);
}

TEST_F(AggregateValidationFixture, validAggregate) {
  const auto& validInput = createValidInput();
  auto errCode = CmdGlobalOptions::getInstance()->isValidAggregate(
      CmdShowPort().getValidAggs(), validInput);
  EXPECT_EQ(errCode, cli::CliOptionResult::EOK);
}
} // namespace facebook::fboss
