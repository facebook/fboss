// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <fboss/cli/fboss2/CmdGlobalOptions.h>
#include <fboss/cli/fboss2/utils/FilterOp.h>
#include "fboss/cli/fboss2/commands/show/port/CmdShowPort.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"
#include "nettools/common/TestUtils.h"

using namespace ::testing;
namespace facebook::fboss {

/*
 * Set up port test data
 */
cli::ShowPortModel createTestPortModel() {
  cli::ShowPortModel model;

  cli::PortEntry entry1, entry2, entry3, entry4, entry5, entry6;
  entry1.id() = 1;
  entry1.hwLogicalPortId() = 1;
  entry1.name() = "eth1/5/1";
  entry1.adminState() = "Enabled";
  entry1.linkState() = "Down";
  entry1.speed() = "100G";
  entry1.profileId() = "PROFILE_100G_4_NRZ_CL91_COPPER";
  entry1.tcvrID() = 0;
  entry1.tcvrPresent() = "Present";

  entry2.id() = 2;
  entry2.hwLogicalPortId() = 2;
  entry2.name() = "eth1/5/2";
  entry2.adminState() = "Disabled";
  entry2.linkState() = "Down";
  entry2.speed() = "25G";
  entry2.profileId() = "PROFILE_25G_1_NRZ_CL74_COPPER";
  entry2.tcvrID() = 1;
  entry2.tcvrPresent() = "Present";

  entry3.id() = 3;
  entry3.hwLogicalPortId() = 3;
  entry3.name() = "eth1/5/3";
  entry3.adminState() = "Enabled";
  entry3.linkState() = "Up";
  entry3.speed() = "100G";
  entry3.profileId() = "PROFILE_100G_4_NRZ_CL91_COPPER";
  entry3.tcvrID() = 2;
  entry3.tcvrPresent() = "Absent";

  entry4.id() = 8;
  entry4.hwLogicalPortId() = 8;
  entry4.name() = "fab402/9/1";
  entry4.adminState() = "Enabled";
  entry4.linkState() = "Up";
  entry4.speed() = "100G";
  entry4.profileId() = "PROFILE_100G_4_NRZ_NOFEC_COPPER";
  entry4.tcvrID() = 3;
  entry4.tcvrPresent() = "Absent";

  entry5.id() = 7;
  entry5.hwLogicalPortId() = 7;
  entry5.name() = "eth1/10/2";
  entry5.adminState() = "Enabled";
  entry5.linkState() = "Up";
  entry5.speed() = "100G";
  entry5.profileId() = "PROFILE_100G_4_NRZ_CL91_OPTICAL";
  entry5.tcvrID() = 4;
  entry5.tcvrPresent() = "Present";

  entry6.id() = 9;
  entry6.hwLogicalPortId() = 9;
  entry6.name() = "eth1/4/1";
  entry6.adminState() = "Enabled";
  entry6.linkState() = "Up";
  entry6.speed() = "100G";
  entry6.profileId() = "PROFILE_100G_4_NRZ_CL91_OPTICAL";
  entry6.tcvrID() = 5;
  entry6.tcvrPresent() = "Present";

  model.portEntries() = {entry1, entry2, entry3, entry4, entry5, entry6};
  return model;
}

auto createSumAggregate() {
  CmdGlobalOptions::AggregateOption parsedAggregate;
  parsedAggregate.aggOp = CmdGlobalOptions().getAggregateOp("SUM");
  parsedAggregate.columnName = "tcvrID";
  parsedAggregate.acrossHosts = false;
  return parsedAggregate;
}

auto createMinAggregate() {
  CmdGlobalOptions::AggregateOption parsedAggregate;
  parsedAggregate.aggOp = CmdGlobalOptions().getAggregateOp("MIN");
  parsedAggregate.columnName = "hwLogicalPortId";
  parsedAggregate.acrossHosts = false;
  return parsedAggregate;
}

auto createMaxAggregate() {
  CmdGlobalOptions::AggregateOption parsedAggregate;
  parsedAggregate.aggOp = CmdGlobalOptions().getAggregateOp("MAX");
  parsedAggregate.columnName = "id";
  parsedAggregate.acrossHosts = false;
  return parsedAggregate;
}

auto createCountAggregate() {
  CmdGlobalOptions::AggregateOption parsedAggregate;
  parsedAggregate.aggOp = CmdGlobalOptions().getAggregateOp("COUNT");
  // no port_name column exists in the port thrift.
  parsedAggregate.columnName = "name";
  parsedAggregate.acrossHosts = false;
  return parsedAggregate;
}

auto createFilterInput() {
  CmdGlobalOptions::FilterTerm filterTerm = {
      "linkState", std::make_shared<FilterOpEq>(), "Up"};
  CmdGlobalOptions::IntersectionList intersectList = {filterTerm};
  CmdGlobalOptions::UnionList unionList = {intersectList};
  return unionList;
}

class AggregateFixture : public CmdHandlerTestBase {
 public:
  cli::ShowPortModel model;
  ValidAggMapType validAggMap;

  void SetUp() override {
    CmdHandlerTestBase::SetUp();
    model = createTestPortModel();
    validAggMap = CmdShowPort().getValidAggs();
  }
};

TEST_F(AggregateFixture, sumAggregation) {
  const auto& parsedSumAgg = createSumAggregate();
  const auto& aggResult =
      performAggregation<CmdShowPort>(model, parsedSumAgg, validAggMap);
  EXPECT_EQ(aggResult, 15);
}

TEST_F(AggregateFixture, aggWithFilter) {
  const auto& validFilterMap = CmdShowPort().getValidFilters();
  const auto& filterInput = createFilterInput();
  const auto& filteredResult =
      filterOutput<CmdShowPort>(model, filterInput, validFilterMap);
  const auto& parsedCountAgg = createCountAggregate();
  const auto& aggResult = performAggregation<CmdShowPort>(
      filteredResult, parsedCountAgg, validAggMap);
  EXPECT_EQ(aggResult, 4);
}

TEST_F(AggregateFixture, minAggregation) {
  const auto& parsedSumAgg = createMinAggregate();
  const auto& aggResult =
      performAggregation<CmdShowPort>(model, parsedSumAgg, validAggMap);
  EXPECT_EQ(aggResult, 1);
}
TEST_F(AggregateFixture, MaxAggregation) {
  const auto& parsedSumAgg = createMaxAggregate();
  const auto& aggResult =
      performAggregation<CmdShowPort>(model, parsedSumAgg, validAggMap);
  EXPECT_EQ(aggResult, 9);
}

TEST_F(AggregateFixture, countAggregation) {
  const auto& parsedSumAgg = createCountAggregate();
  const auto& aggResult =
      performAggregation<CmdShowPort>(model, parsedSumAgg, validAggMap);
  EXPECT_EQ(aggResult, 6);
}

} // namespace facebook::fboss
