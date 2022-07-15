// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "fboss/cli/fboss2/CmdGlobalOptions.h"
#include "fboss/cli/fboss2/commands/show/port/CmdShowPort.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"
#include "nettools/common/TestUtils.h"

using namespace ::testing;
namespace facebook::fboss {

/*
 * Set up port test data
 */
cli::ShowPortModel createTestModel() {
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

auto createSingleTermFilter() {
  CmdGlobalOptions::FilterTerm filterTerm = {
      "linkState", std::make_shared<FilterOpEq>(), "Up"};
  CmdGlobalOptions::IntersectionList intersectList = {filterTerm};
  CmdGlobalOptions::UnionList unionList = {intersectList};
  return unionList;
}

auto createIntersectionFilter() {
  CmdGlobalOptions::FilterTerm filterTerm1 = {
      "linkState", std::make_shared<FilterOpEq>(), "Down"};
  CmdGlobalOptions::FilterTerm filterTerm2 = {
      "adminState", std::make_shared<FilterOpEq>(), "Disabled"};
  CmdGlobalOptions::IntersectionList intersectList = {filterTerm1, filterTerm2};
  CmdGlobalOptions::UnionList unionList = {intersectList};
  return unionList;
}

auto createUnionFilter() {
  CmdGlobalOptions::FilterTerm filterTerm1 = {
      "id", std::make_shared<FilterOpGt>(), "7"};
  CmdGlobalOptions::FilterTerm filterTerm2 = {
      "tcvrPresent", std::make_shared<FilterOpEq>(), "Absent"};
  CmdGlobalOptions::IntersectionList intersectList1 = {filterTerm1};
  CmdGlobalOptions::IntersectionList intersectList2 = {filterTerm2};
  CmdGlobalOptions::UnionList unionList = {intersectList1, intersectList2};
  return unionList;
}

auto createComplicatedFilter() {
  CmdGlobalOptions::FilterTerm filterTerm1 = {
      "id", std::make_shared<FilterOpGt>(), "3"};
  CmdGlobalOptions::FilterTerm filterTerm2 = {
      "tcvrPresent", std::make_shared<FilterOpEq>(), "Present"};
  CmdGlobalOptions::FilterTerm filterTerm3 = {
      "adminState", std::make_shared<FilterOpEq>(), "Disabled"};
  CmdGlobalOptions::FilterTerm filterTerm4 = {
      "linkState", std::make_shared<FilterOpEq>(), "Up"};
  CmdGlobalOptions::IntersectionList intersectList1 = {
      filterTerm1, filterTerm2};
  CmdGlobalOptions::IntersectionList intersectList2 = {
      filterTerm3, filterTerm4};
  CmdGlobalOptions::UnionList unionList = {intersectList1, intersectList2};
  return unionList;
}

auto emptyFilteredOutput() {
  CmdGlobalOptions::FilterTerm filterTerm1 = {
      "id", std::make_shared<FilterOpLte>(), "2"};
  CmdGlobalOptions::FilterTerm filterTerm2 = {
      "linkState", std::make_shared<FilterOpEq>(), "Up"};
  CmdGlobalOptions::IntersectionList intersectList = {filterTerm1, filterTerm2};
  CmdGlobalOptions::UnionList unionList = {intersectList};
  return unionList;
}

auto noModificationFilter() {
  // all entires will satisify this filter because linkState can be either Up or
  // Down
  CmdGlobalOptions::FilterTerm filterTerm1 = {
      "linkState", std::make_shared<FilterOpEq>(), "Down"};
  CmdGlobalOptions::FilterTerm filterTerm2 = {
      "linkState", std::make_shared<FilterOpEq>(), "Up"};
  CmdGlobalOptions::IntersectionList intersectList1 = {filterTerm1};
  CmdGlobalOptions::IntersectionList intersectList2 = {filterTerm2};
  CmdGlobalOptions::UnionList unionList = {intersectList1, intersectList2};
  return unionList;
}

class FilterTestFixture : public CmdHandlerTestBase {
 public:
  cli::ShowPortModel model;
  ValidFilterMapType validFilterMap;

  void SetUp() override {
    CmdHandlerTestBase::SetUp();
    model = createTestModel();
    validFilterMap = CmdShowPort().getValidFilters();
  }
};

TEST_F(FilterTestFixture, singleTermFiltering) {
  const auto& singleTermFilter = createSingleTermFilter();
  const auto& filteredResult =
      filterOutput<CmdShowPort>(model, singleTermFilter, validFilterMap);
  // check that all linkState are "Up" in the filtered model.
  for (const auto& portInfo : filteredResult.get_portEntries()) {
    const auto& linkState = folly::to<std::string>(portInfo.get_linkState());
    EXPECT_EQ(linkState, "Up");
  }
}

TEST_F(FilterTestFixture, itersectionFiltering) {
  const auto& intersectionFilter = createIntersectionFilter();
  const auto& filteredResult =
      filterOutput<CmdShowPort>(model, intersectionFilter, validFilterMap);
  // check that all linkState are "Down" AND adminState are "Disabled" in the
  // filtered model.
  for (const auto& portInfo : filteredResult.get_portEntries()) {
    const auto& linkState = folly::to<std::string>(portInfo.get_linkState());
    const auto& adminState = folly::to<std::string>(portInfo.get_adminState());
    EXPECT_EQ(linkState, "Down");
    EXPECT_EQ(adminState, "Disabled");
  }
}

TEST_F(FilterTestFixture, unionFiltering) {
  const auto& unionFilter = createUnionFilter();
  const auto& filteredResult =
      filterOutput<CmdShowPort>(model, unionFilter, validFilterMap);
  // check that all id > 7 OR tcvrPresent is "Absent" in the
  // filtered model.
  for (const auto& portInfo : filteredResult.get_portEntries()) {
    const auto& id = folly::to<int>(portInfo.get_id());
    const auto& tcvr = folly::to<std::string>(portInfo.get_tcvrPresent());
    EXPECT_TRUE((id > 7 || tcvr == "Absent"));
  }
}

TEST_F(FilterTestFixture, complicatedFiltering) {
  const auto& compFilter = createComplicatedFilter();
  const auto& filteredResult =
      filterOutput<CmdShowPort>(model, compFilter, validFilterMap);
  // check that (id > 3 && tcvrPresent == "Present") || (linkState == "Up" &&
  // adminState == "Disabled") in all rows of the filtered model.
  for (const auto& portInfo : filteredResult.get_portEntries()) {
    const auto& id = folly::to<int>(portInfo.get_id());
    const auto& tcvr = folly::to<std::string>(portInfo.get_tcvrPresent());
    const auto& linkState = folly::to<std::string>(portInfo.get_linkState());
    const auto& adminState = folly::to<std::string>(portInfo.get_adminState());
    EXPECT_TRUE(
        (id > 3 && tcvr == "Present") ||
        (adminState == "Disabled" && linkState == "Up"));
  }
}

TEST_F(FilterTestFixture, noEntrySatisfiesFilter) {
  const auto& emptyFilter = emptyFilteredOutput();
  const auto& filteredResult =
      filterOutput<CmdShowPort>(model, emptyFilter, validFilterMap);
  // No entry satisfies (id <= 2 && linkState == "Up")
  EXPECT_EQ(filteredResult.get_portEntries().size(), 0);
}

TEST_F(FilterTestFixture, allEntriesSatisfyFilter) {
  const auto& noModification = noModificationFilter();
  const auto& filteredResult =
      filterOutput<CmdShowPort>(model, noModification, validFilterMap);
  // All entries satisfy (linkState == Up || linkState == Down)
  EXPECT_EQ(filteredResult.get_portEntries().size(), 6);
}
} // namespace facebook::fboss
