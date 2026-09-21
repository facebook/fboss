// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <fboss/cli/fboss2/CmdGlobalOptions.h>
#include <fboss/cli/fboss2/commands/show/aggregateport/CmdShowAggregatePort.h>
#include <fboss/cli/fboss2/commands/show/ndp/CmdShowNdp.h>
#include <fboss/cli/fboss2/utils/FilterOp.h>
#include "fboss/cli/fboss2/commands/show/arp/CmdShowArp.h"
#include "fboss/cli/fboss2/commands/show/port/CmdShowPort.h"
#include "fboss/cli/fboss2/gen-cpp2/cli_types.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"

using namespace ::testing;
namespace facebook::fboss {
CmdGlobalOptions::UnionList createInvalidKeyInput() {
  CmdGlobalOptions::FilterTerm filterTerm1 = {
      "x", std::make_shared<FilterOpGt>(), "22"};
  CmdGlobalOptions::FilterTerm filterTerm2 = {
      "linkState", std::make_shared<FilterOpEq>(), "Up"};
  CmdGlobalOptions::FilterTerm filterTerm3 = {
      "adminState", std::make_shared<FilterOpNeq>(), "Enabled"};
  CmdGlobalOptions::IntersectionList intersectList1 = {filterTerm1};
  CmdGlobalOptions::IntersectionList intersectList2 = {
      filterTerm2, filterTerm3};
  CmdGlobalOptions::UnionList filterUnion = {intersectList1, intersectList2};
  return filterUnion;
}

CmdGlobalOptions::UnionList createInvalidValueDataTypeInput() {
  CmdGlobalOptions::FilterTerm filterTerm1 = {
      "linkState", std::make_shared<FilterOpEq>(), "Down"};
  CmdGlobalOptions::FilterTerm filterTerm2 = {
      "id", std::make_shared<FilterOpEq>(), "id_12"};
  CmdGlobalOptions::FilterTerm filterTerm3 = {
      "adminState", std::make_shared<FilterOpNeq>(), "Enabled"};
  CmdGlobalOptions::IntersectionList intersectList1 = {
      filterTerm1, filterTerm3};
  CmdGlobalOptions::IntersectionList intersectList2 = {filterTerm2};
  CmdGlobalOptions::UnionList filterUnion = {intersectList1, intersectList2};
  return filterUnion;
}

CmdGlobalOptions::UnionList createInvalidValueInput() {
  CmdGlobalOptions::FilterTerm filterTerm1 = {
      "linkState", std::make_shared<FilterOpEq>(), "good"};
  CmdGlobalOptions::FilterTerm filterTerm2 = {
      "id", std::make_shared<FilterOpEq>(), "12"};
  CmdGlobalOptions::FilterTerm filterTerm3 = {
      "adminState", std::make_shared<FilterOpNeq>(), "Enabled"};
  CmdGlobalOptions::IntersectionList intersectList1 = {
      filterTerm1, filterTerm3};
  CmdGlobalOptions::IntersectionList intersectList2 = {filterTerm2};
  CmdGlobalOptions::UnionList filterUnion = {intersectList1, intersectList2};
  return filterUnion;
}

CmdGlobalOptions::UnionList createValidInput() {
  CmdGlobalOptions::FilterTerm filterTerm1 = {
      "linkState", std::make_shared<FilterOpEq>(), "Up"};
  CmdGlobalOptions::FilterTerm filterTerm2 = {
      "id", std::make_shared<FilterOpEq>(), "12"};
  CmdGlobalOptions::FilterTerm filterTerm3 = {
      "adminState", std::make_shared<FilterOpNeq>(), "Disabled"};
  CmdGlobalOptions::IntersectionList intersectList1 = {filterTerm1};
  CmdGlobalOptions::IntersectionList intersectList2 = {filterTerm2};
  CmdGlobalOptions::IntersectionList intersectList3 = {filterTerm3};
  CmdGlobalOptions::UnionList filterUnion = {
      intersectList1, intersectList2, intersectList3};
  return filterUnion;
}

// checking if thrift reflections are being properly generated from the thrift
// struct. This is done to check if more fields from thrift struct are being
// correctly validated.
CmdGlobalOptions::UnionList createValidInputOtherFields() {
  CmdGlobalOptions::FilterTerm filterTerm1 = {
      "linkState", std::make_shared<FilterOpEq>(), "Up"};
  CmdGlobalOptions::FilterTerm filterTerm2 = {
      "id", std::make_shared<FilterOpEq>(), "12"};
  CmdGlobalOptions::FilterTerm filterTerm3 = {
      "adminState", std::make_shared<FilterOpNeq>(), "Disabled"};
  CmdGlobalOptions::FilterTerm filterTerm4 = {
      "tcvrID", std::make_shared<FilterOpNeq>(), "20"};
  CmdGlobalOptions::FilterTerm filterTerm5 = {
      "tcvrPresent", std::make_shared<FilterOpNeq>(), "yes"};
  CmdGlobalOptions::FilterTerm filterTerm6 = {
      "speed", std::make_shared<FilterOpNeq>(), "100mbps"};

  CmdGlobalOptions::IntersectionList intersectList1 = {
      filterTerm1, filterTerm4};
  CmdGlobalOptions::IntersectionList intersectList2 = {filterTerm2};
  CmdGlobalOptions::IntersectionList intersectList3 = {
      filterTerm3, filterTerm5};
  CmdGlobalOptions::IntersectionList intersectList4 = {filterTerm6};
  CmdGlobalOptions::UnionList filterUnion = {
      intersectList1, intersectList2, intersectList3, intersectList4};
  return filterUnion;
}

CmdGlobalOptions::UnionList createInvalidValueOtherFields() {
  CmdGlobalOptions::FilterTerm filterTerm1 = {
      "linkState", std::make_shared<FilterOpEq>(), "Up"};
  CmdGlobalOptions::FilterTerm filterTerm2 = {
      "id", std::make_shared<FilterOpEq>(), "12"};
  CmdGlobalOptions::FilterTerm filterTerm3 = {
      "adminState", std::make_shared<FilterOpNeq>(), "Disabled"};
  CmdGlobalOptions::FilterTerm filterTerm4 = {
      "tcvrID", std::make_shared<FilterOpNeq>(), "my_id_12"};
  CmdGlobalOptions::FilterTerm filterTerm5 = {
      "tcvrPresent", std::make_shared<FilterOpNeq>(), "yes"};
  CmdGlobalOptions::FilterTerm filterTerm6 = {
      "speed", std::make_shared<FilterOpNeq>(), "100mbps"};

  CmdGlobalOptions::IntersectionList intersectList1 = {
      filterTerm1, filterTerm2, filterTerm3};
  CmdGlobalOptions::IntersectionList intersectList2 = {
      filterTerm4, filterTerm5};
  CmdGlobalOptions::IntersectionList intersectList3 = {filterTerm6};
  CmdGlobalOptions::UnionList filterUnion = {
      intersectList1, intersectList2, intersectList3};
  return filterUnion;
}

CmdGlobalOptions::UnionList createNdpValidInput() {
  CmdGlobalOptions::FilterTerm filterTerm1 = {
      "ip", std::make_shared<FilterOpEq>(), "10.128.40.16"};
  CmdGlobalOptions::FilterTerm filterTerm2 = {
      "state", std::make_shared<FilterOpNeq>(), "REACHABLE"};
  CmdGlobalOptions::FilterTerm filterTerm3 = {
      "ttl", std::make_shared<FilterOpGt>(), "5"};
  CmdGlobalOptions::FilterTerm filterTerm4 = {
      "classID", std::make_shared<FilterOpEq>(), "8"};
  CmdGlobalOptions::IntersectionList intersectList1 = {filterTerm1};
  CmdGlobalOptions::IntersectionList intersectList2 = {
      filterTerm2, filterTerm3};
  return {intersectList1, intersectList2};
}

CmdGlobalOptions::UnionList createNdpInvalidKey() {
  CmdGlobalOptions::FilterTerm filterTerm1 = {
      "ip", std::make_shared<FilterOpEq>(), "10.128.40.16"};
  CmdGlobalOptions::FilterTerm filterTerm2 = {
      "state", std::make_shared<FilterOpNeq>(), "REACHABLE"};
  // There is no field for status in NDP. Hence, invalid key
  CmdGlobalOptions::FilterTerm filterTerm3 = {
      "status", std::make_shared<FilterOpNeq>(), "unknown"};

  CmdGlobalOptions::IntersectionList intersectList1 = {
      filterTerm1, filterTerm2, filterTerm3};
  CmdGlobalOptions::UnionList filterUnion = {intersectList1};
  return filterUnion;
}

CmdGlobalOptions::UnionList createArpValidInput() {
  CmdGlobalOptions::FilterTerm filterTerm1 = {
      "ip", std::make_shared<FilterOpEq>(), "10.128.40.16"};
  CmdGlobalOptions::FilterTerm filterTerm2 = {
      "state", std::make_shared<FilterOpNeq>(), "REACHABLE"};
  CmdGlobalOptions::FilterTerm filterTerm3 = {
      "ttl", std::make_shared<FilterOpGt>(), "5"};
  CmdGlobalOptions::FilterTerm filterTerm4 = {
      "classID", std::make_shared<FilterOpEq>(), "8"};

  CmdGlobalOptions::IntersectionList intersectList1 = {
      filterTerm1, filterTerm2, filterTerm3};
  CmdGlobalOptions::IntersectionList intersectList2 = {filterTerm4};
  return {intersectList1, intersectList2};
}

CmdGlobalOptions::UnionList createArpInvalidValue() {
  // state in ARP can only be "REACHABLE" or "UNREACHABLE"
  CmdGlobalOptions::FilterTerm filterTerm1 = {
      "state", std::make_shared<FilterOpNeq>(), "good"};
  CmdGlobalOptions::UnionList filterUnion = {{filterTerm1}};
  return filterUnion;
}

CmdGlobalOptions::UnionList createAggPortInvalidType() {
  CmdGlobalOptions::FilterTerm filterTerm1 = {
      "activeMembers", std::make_shared<FilterOpEq>(), "5"};
  CmdGlobalOptions::FilterTerm filterTerm2 = {
      "configuredMembers", std::make_shared<FilterOpNeq>(), "3"};
  // minMembers accepts integers
  CmdGlobalOptions::FilterTerm filterTerm3 = {
      "minMembers", std::make_shared<FilterOpNeq>(), "unknown"};
  CmdGlobalOptions::FilterTerm filterTerm4 = {
      "name", std::make_shared<FilterOpNeq>(), "my_aggTest"};

  CmdGlobalOptions::IntersectionList intersectList1 = {
      filterTerm1, filterTerm2};
  CmdGlobalOptions::IntersectionList intersectList2 = {
      filterTerm3, filterTerm4};
  CmdGlobalOptions::UnionList filterUnion = {intersectList1, intersectList2};
  return filterUnion;
}

CmdGlobalOptions::UnionList createAggPortInvalidFilterable() {
  // members is not filterable because it's a list. We only support primitive
  // types and strings for filtering
  CmdGlobalOptions::FilterTerm filterTerm1 = {
      "members", std::make_shared<FilterOpEq>(), "some_member"};
  return {{filterTerm1}};
}

class FilterValidatorFixture : public CmdHandlerTestBase {
 public:
  void SetUp() override {
    CmdHandlerTestBase::SetUp();
  }

  template <typename T>
  bool isSameOperator(std::shared_ptr<FilterOp> basePtr) {
    return dynamic_cast<T*>(basePtr.get()) != nullptr;
  }
};

TEST_F(FilterValidatorFixture, keyValidation) {
  auto invalidKeyInput = createInvalidKeyInput();
  auto errCode = CmdGlobalOptions::getInstance()->isValid(
      CmdShowPort().getValidFilters(), invalidKeyInput);
  EXPECT_EQ(errCode, cli::CliOptionResult::KEY_ERROR);
}

TEST_F(FilterValidatorFixture, valueDataTypeValidation) {
  auto invalidDataTypeInput = createInvalidValueDataTypeInput();
  auto errCode = CmdGlobalOptions::getInstance()->isValid(
      CmdShowPort().getValidFilters(), invalidDataTypeInput);
  EXPECT_EQ(errCode, cli::CliOptionResult::TYPE_ERROR);
}

TEST_F(FilterValidatorFixture, valueValidation) {
  auto invalidValueInput = createInvalidValueInput();
  auto errCode = CmdGlobalOptions::getInstance()->isValid(
      CmdShowPort().getValidFilters(), invalidValueInput);

  EXPECT_EQ(errCode, cli::CliOptionResult::VALUE_ERROR);
}

TEST_F(FilterValidatorFixture, testOk) {
  auto validInput = createValidInput();
  auto errCode = CmdGlobalOptions::getInstance()->isValid(
      CmdShowPort().getValidFilters(), validInput);

  EXPECT_EQ(errCode, cli::CliOptionResult::EOK);
}

// Checks for operator validity and term validity are done during parsing
TEST_F(FilterValidatorFixture, invalidOperatorTest) {
  std::string filterInput = "linkState == Up&&id = 2||adminState != Enabled";
  auto cmd = CmdGlobalOptions();
  auto errorCode = cli::CliOptionResult::EOK;
  cmd.setFilterInput(filterInput);
  cmd.getFilters(errorCode);
  // op error here because of id = 2. (should be ==)
  EXPECT_EQ(errorCode, cli::CliOptionResult::OP_ERROR);
}

TEST_F(FilterValidatorFixture, filterTermTest) {
  std::string filterInput = "linkState && adminState == disabled";
  auto cmd = CmdGlobalOptions();
  auto errorCode = cli::CliOptionResult::EOK;
  cmd.setFilterInput(filterInput);
  cmd.getFilters(errorCode);
  // term error here because of the term "linkState". (Terms need to be of the
  // form <key op value>.
  EXPECT_EQ(errorCode, cli::CliOptionResult::TERM_ERROR);
}

TEST_F(FilterValidatorFixture, validInputParsing) {
  std::string filterInput =
      "linkState == Down&&adminState != Disabled||id <= 12";
  auto cmd = CmdGlobalOptions();
  auto errorCode = cli::CliOptionResult::EOK;
  cmd.setFilterInput(filterInput);
  const auto& parsedFilters = cmd.getFilters(errorCode);
  EXPECT_EQ(errorCode, cli::CliOptionResult::EOK);

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
  EXPECT_TRUE(isSameOperator<FilterOpEq>(std::get<1>(filterTerm1)));
  EXPECT_EQ(std::get<2>(filterTerm1), "Down");

  EXPECT_EQ(std::get<0>(filterTerm2), "adminState");
  EXPECT_TRUE(isSameOperator<FilterOpNeq>(std::get<1>(filterTerm2)));
  EXPECT_EQ(std::get<2>(filterTerm2), "Disabled");

  EXPECT_EQ(std::get<0>(filterTerm3), "id");
  EXPECT_TRUE(isSameOperator<FilterOpLte>(std::get<1>(filterTerm3)));
  EXPECT_EQ(std::get<2>(filterTerm3), "12");
}

TEST_F(FilterValidatorFixture, validRelaxedParsing) {
  // verify that parser correctly parses input that has spaces between the
  // logical operator and the FilterTerm
  std::string filterInput =
      "linkState == Down && adminState != Disabled||id <= 12 || tcvrID > 5";
  auto cmd = CmdGlobalOptions();
  auto errorCode = cli::CliOptionResult::EOK;
  cmd.setFilterInput(filterInput);
  const auto& parsedFilters = cmd.getFilters(errorCode);
  EXPECT_EQ(errorCode, cli::CliOptionResult::EOK);

  // Check that it is correctly parsed as a unionlist of size 3.
  EXPECT_EQ(parsedFilters.size(), 3);

  // Check that first intersection list has 2 terms, second has 1 term and the
  // third has 1 term.
  const auto& intersectList1 = parsedFilters[0];
  const auto& intersectList2 = parsedFilters[1];
  const auto& intersectList3 = parsedFilters[2];
  EXPECT_EQ(intersectList1.size(), 2);
  EXPECT_EQ(intersectList2.size(), 1);
  EXPECT_EQ(intersectList3.size(), 1);

  // Checks for the filter terms
  const auto& filterTerm1 = intersectList1[0];
  const auto& filterTerm2 = intersectList1[1];
  const auto& filterTerm3 = intersectList2[0];
  const auto& filterTerm4 = intersectList3[0];

  EXPECT_EQ(std::get<0>(filterTerm1), "linkState");
  EXPECT_TRUE(isSameOperator<FilterOpEq>(std::get<1>(filterTerm1)));
  EXPECT_EQ(std::get<2>(filterTerm1), "Down");

  EXPECT_EQ(std::get<0>(filterTerm2), "adminState");
  EXPECT_TRUE(isSameOperator<FilterOpNeq>(std::get<1>(filterTerm2)));
  EXPECT_EQ(std::get<2>(filterTerm2), "Disabled");

  EXPECT_EQ(std::get<0>(filterTerm3), "id");
  EXPECT_TRUE(isSameOperator<FilterOpLte>(std::get<1>(filterTerm3)));
  EXPECT_EQ(std::get<2>(filterTerm3), "12");

  EXPECT_EQ(std::get<0>(filterTerm4), "tcvrID");
  EXPECT_TRUE(isSameOperator<FilterOpGt>(std::get<1>(filterTerm4)));
  EXPECT_EQ(std::get<2>(filterTerm4), "5");
}

TEST_F(FilterValidatorFixture, filterValidationTestPortValid) {
  auto otherFieldsValidInput = createValidInputOtherFields();
  auto errCode = CmdGlobalOptions::getInstance()->isValid(
      CmdShowPort().getValidFilters(), otherFieldsValidInput);

  EXPECT_EQ(errCode, cli::CliOptionResult::EOK);
}

TEST_F(FilterValidatorFixture, filterValidationTestNdpValid) {
  auto ndpValidInput = createNdpValidInput();
  auto errCode = CmdGlobalOptions::getInstance()->isValid(
      CmdShowNdp().getValidFilters(), ndpValidInput);

  EXPECT_EQ(errCode, cli::CliOptionResult::EOK);
}

TEST_F(FilterValidatorFixture, filterValidationTestArpValid) {
  auto arpValidInput = createArpValidInput();
  auto errCode = CmdGlobalOptions::getInstance()->isValid(
      CmdShowArp().getValidFilters(), arpValidInput);

  EXPECT_EQ(errCode, cli::CliOptionResult::EOK);
}

TEST_F(FilterValidatorFixture, filterValidationTestPortInvalid) {
  auto otherFieldsInvalidValueTypeInput = createInvalidValueOtherFields();
  auto errCode = CmdGlobalOptions::getInstance()->isValid(
      CmdShowPort().getValidFilters(), otherFieldsInvalidValueTypeInput);

  EXPECT_EQ(errCode, cli::CliOptionResult::TYPE_ERROR);
}

TEST_F(FilterValidatorFixture, filterValidationTestNdpInvalid) {
  auto ndpInvalidInput = createNdpInvalidKey();
  auto errCode = CmdGlobalOptions::getInstance()->isValid(
      CmdShowNdp().getValidFilters(), ndpInvalidInput);

  EXPECT_EQ(errCode, cli::CliOptionResult::KEY_ERROR);
}

TEST_F(FilterValidatorFixture, filterValidationTestArpInvalid) {
  auto arpInvalidInput = createArpInvalidValue();
  auto errCode = CmdGlobalOptions::getInstance()->isValid(
      CmdShowArp().getValidFilters(), arpInvalidInput);

  EXPECT_EQ(errCode, cli::CliOptionResult::VALUE_ERROR);
}

TEST_F(FilterValidatorFixture, filterValidationTestAggPort) {
  auto aggPortInvalidInput = createAggPortInvalidType();
  auto errCode = CmdGlobalOptions::getInstance()->isValid(
      CmdShowAggregatePort().getValidFilters(), aggPortInvalidInput);

  EXPECT_EQ(errCode, cli::CliOptionResult::TYPE_ERROR);
}

TEST_F(FilterValidatorFixture, filterableCheck) {
  auto aggPortInvalidFilterable = createAggPortInvalidFilterable();
  auto errCode = CmdGlobalOptions::getInstance()->isValid(
      CmdShowAggregatePort().getValidFilters(), aggPortInvalidFilterable);

  EXPECT_EQ(errCode, cli::CliOptionResult::KEY_ERROR);
}

} // namespace facebook::fboss
