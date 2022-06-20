// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <fboss/cli/fboss2/CmdGlobalOptions.h>
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

// Checks for operator validity and term validity are done during parsing
TEST_F(FilterValidatorFixture, invalidOperatorTest) {
  std::string filterInput = "linkState == Up&&id = 2||adminState != Enabled";
  auto cmd = CmdGlobalOptions();
  auto errorCode = CmdGlobalOptions::CliOptionResult::EOK;
  cmd.setFilterInput(filterInput);
  cmd.getFilters(errorCode);
  // op error here because of id = 2. (should be ==)
  EXPECT_EQ(errorCode, CmdGlobalOptions::CliOptionResult::OP_ERROR);
}

TEST_F(FilterValidatorFixture, filterTermTest) {
  std::string filterInput = "linkState && adminState == disabled";
  auto cmd = CmdGlobalOptions();
  auto errorCode = CmdGlobalOptions::CliOptionResult::EOK;
  cmd.setFilterInput(filterInput);
  cmd.getFilters(errorCode);
  // term error here because of the term "linkState". (Terms need to be of the
  // form <key op value>.
  EXPECT_EQ(errorCode, CmdGlobalOptions::CliOptionResult::TERM_ERROR);
}

TEST_F(FilterValidatorFixture, validInputParsing) {
  std::string filterInput =
      "linkState == Down&&adminState != Disabled||id <= 12";
  auto cmd = CmdGlobalOptions();
  auto errorCode = CmdGlobalOptions::CliOptionResult::EOK;
  cmd.setFilterInput(filterInput);
  const auto& parsedFilters = cmd.getFilters(errorCode);
  EXPECT_EQ(errorCode, CmdGlobalOptions::CliOptionResult::EOK);

  // Check that it is correctly parsed as a unionlist of size 2.
  EXPECT_EQ(parsedFilters.size(), 2);

  // Check that first intersection list has 2 terms and second has 1 term.
  const auto& intersectList1 = parsedFilters[0];
  const auto& intersectList2 = parsedFilters[1];
  EXPECT_EQ(intersectList1.size(), 2);
  EXPECT_EQ(intersectList2.size(), 1);

  // Checks for the filter terms
  const auto& filterTerm1 = intersectList1[0];
  const auto& filterTerm2 = intersectList1[1];
  const auto& filterTerm3 = intersectList2[0];

  EXPECT_EQ(std::get<0>(filterTerm1), "linkState");
  EXPECT_EQ(std::get<1>(filterTerm1), CmdGlobalOptions::FilterOp::EQ);
  EXPECT_EQ(std::get<2>(filterTerm1), "Down");

  EXPECT_EQ(std::get<0>(filterTerm2), "adminState");
  EXPECT_EQ(std::get<1>(filterTerm2), CmdGlobalOptions::FilterOp::NEQ);
  EXPECT_EQ(std::get<2>(filterTerm2), "Disabled");

  EXPECT_EQ(std::get<0>(filterTerm3), "id");
  EXPECT_EQ(std::get<1>(filterTerm3), CmdGlobalOptions::FilterOp::LTE);
  EXPECT_EQ(std::get<2>(filterTerm3), "12");
}

} // namespace facebook::fboss
