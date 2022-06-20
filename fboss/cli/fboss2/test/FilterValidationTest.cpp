// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "fboss/cli/fboss2/commands/show/port/CmdShowPort.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"
#include "nettools/common/TestUtils.h"

using namespace ::testing;
namespace facebook::fboss {
CmdGlobalOptions::UnionList createInvalidKeyInput() {
  CmdGlobalOptions::FilterTerm filterTerm1 = {
      "x", CmdGlobalOptions::FilterOp::GT, "22"};
  CmdGlobalOptions::FilterTerm filterTerm2 = {
      "linkState", CmdGlobalOptions::FilterOp::EQ, "Up"};
  CmdGlobalOptions::FilterTerm filterTerm3 = {
      "adminState", CmdGlobalOptions::FilterOp::NEQ, "Enabled"};
  CmdGlobalOptions::IntersectionList intersectList1 = {filterTerm1};
  CmdGlobalOptions::IntersectionList intersectList2 = {
      filterTerm2, filterTerm3};
  CmdGlobalOptions::UnionList filterUnion = {intersectList1, intersectList2};
  return filterUnion;
}

CmdGlobalOptions::UnionList createInvalidValueDataTypeInput() {
  CmdGlobalOptions::FilterTerm filterTerm1 = {
      "linkState", CmdGlobalOptions::FilterOp::EQ, "Down"};
  CmdGlobalOptions::FilterTerm filterTerm2 = {
      "id", CmdGlobalOptions::FilterOp::EQ, "id_12"};
  CmdGlobalOptions::FilterTerm filterTerm3 = {
      "adminState", CmdGlobalOptions::FilterOp::NEQ, "Enabled"};
  CmdGlobalOptions::IntersectionList intersectList1 = {
      filterTerm1, filterTerm3};
  CmdGlobalOptions::IntersectionList intersectList2 = {filterTerm2};
  CmdGlobalOptions::UnionList filterUnion = {intersectList1, intersectList2};
  return filterUnion;
}

CmdGlobalOptions::UnionList createInvalidValueInput() {
  CmdGlobalOptions::FilterTerm filterTerm1 = {
      "linkState", CmdGlobalOptions::FilterOp::EQ, "good"};
  CmdGlobalOptions::FilterTerm filterTerm2 = {
      "id", CmdGlobalOptions::FilterOp::EQ, "12"};
  CmdGlobalOptions::FilterTerm filterTerm3 = {
      "adminState", CmdGlobalOptions::FilterOp::NEQ, "Enabled"};
  CmdGlobalOptions::IntersectionList intersectList1 = {
      filterTerm1, filterTerm3};
  CmdGlobalOptions::IntersectionList intersectList2 = {filterTerm2};
  CmdGlobalOptions::UnionList filterUnion = {intersectList1, intersectList2};
  return filterUnion;
}

CmdGlobalOptions::UnionList createValidInput() {
  CmdGlobalOptions::FilterTerm filterTerm1 = {
      "linkState", CmdGlobalOptions::FilterOp::EQ, "Up"};
  CmdGlobalOptions::FilterTerm filterTerm2 = {
      "id", CmdGlobalOptions::FilterOp::EQ, "12"};
  CmdGlobalOptions::FilterTerm filterTerm3 = {
      "adminState", CmdGlobalOptions::FilterOp::NEQ, "Disabled"};
  CmdGlobalOptions::IntersectionList intersectList1 = {filterTerm1};
  CmdGlobalOptions::IntersectionList intersectList2 = {filterTerm2};
  CmdGlobalOptions::IntersectionList intersectList3 = {filterTerm3};
  CmdGlobalOptions::UnionList filterUnion = {
      intersectList1, intersectList2, intersectList3};
  return filterUnion;
}

class FilterValidatorFixture : public CmdHandlerTestBase {
 public:
  void SetUp() override {
    CmdHandlerTestBase::SetUp();
  }
};

TEST_F(FilterValidatorFixture, keyValidation) {
  auto invalidKeyInput = createInvalidKeyInput();
  auto errCode = CmdGlobalOptions::getInstance()->isValid(
      CmdShowPort().getValidFilters(), invalidKeyInput);
  EXPECT_EQ(errCode, CmdGlobalOptions::CliOptionResult::KEY_ERROR);
}

TEST_F(FilterValidatorFixture, valueDataTypeValidation) {
  auto invalidDataTypeInput = createInvalidValueDataTypeInput();
  auto errCode = CmdGlobalOptions::getInstance()->isValid(
      CmdShowPort().getValidFilters(), invalidDataTypeInput);
  EXPECT_EQ(errCode, CmdGlobalOptions::CliOptionResult::TYPE_ERROR);
}

TEST_F(FilterValidatorFixture, valueValidation) {
  auto invalidValueInput = createInvalidValueInput();
  auto errCode = CmdGlobalOptions::getInstance()->isValid(
      CmdShowPort().getValidFilters(), invalidValueInput);

  EXPECT_EQ(errCode, CmdGlobalOptions::CliOptionResult::VALUE_ERROR);
}

TEST_F(FilterValidatorFixture, testOk) {
  auto validInput = createValidInput();
  auto errCode = CmdGlobalOptions::getInstance()->isValid(
      CmdShowPort().getValidFilters(), validInput);

  EXPECT_EQ(errCode, CmdGlobalOptions::CliOptionResult::EOK);
}
} // namespace facebook::fboss
