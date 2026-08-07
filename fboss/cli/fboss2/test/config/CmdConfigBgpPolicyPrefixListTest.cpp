// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <configerator/structs/neteng/fboss/bgp/gen-cpp2/bgp_config_types.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

#include "configerator/structs/neteng/bgp_policy/thrift/gen-cpp2/routing_policy_types.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/policy/prefix-list/CmdConfigProtocolBgpPolicyPrefixList.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/test/config/CmdConfigTestBase.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

using namespace ::testing;
using facebook::bgp::routing_policy::BooleanOperator;
using facebook::bgp::routing_policy::ComparisonOperator;
using facebook::bgp::routing_policy::MatchValueLogicOperator;

namespace facebook::fboss {

// The prefix-list dispatcher only touches the BGP side of ConfigSession,
// which seeds from thrift schema defaults when neither a staged session nor a
// system bgpcpp.conf exists — so no seed agent config is needed (mirrors
// CmdConfigBgpPolicyCommunityListTest).
class CmdConfigBgpPolicyPrefixListTestFixture : public CmdConfigTestBase {
 public:
  CmdConfigBgpPolicyPrefixListTestFixture()
      : CmdConfigTestBase("bgp_prefix_list_test_%%%%-%%%%-%%%%", "") {}

  void SetUp() override {
    CmdConfigTestBase::SetUp();
    setupTestableConfigSession();
  }

  std::string run(const std::vector<std::string>& tokens) {
    CmdConfigProtocolBgpPolicyPrefixList cmd;
    HostInfo hostInfo("testhost");
    return cmd.queryClient(hostInfo, BgpPrefixListConfig(tokens));
  }

  const std::vector<bgp::routing_policy::PrefixList>& lists() {
    return *ConfigSession::getInstance()
                .getBgpConfig()
                .policies()
                .ensure()
                .prefix_lists();
  }

  // Entry `idx` of list `listIdx`.
  const bgp::routing_policy::PrefixListEntry& entry(
      size_t listIdx,
      size_t idx) {
    return lists()[listIdx].prefixes()->at(idx);
  }

  bool sessionFileExists() {
    return std::filesystem::exists(
        ConfigSession::getInstance().getBgpSessionConfigPath());
  }
};

// ==============================================================================
// BgpPrefixListConfig (arg) validation
// ==============================================================================

TEST_F(CmdConfigBgpPolicyPrefixListTestFixture, argValidation) {
  // Bare create list.
  auto bareList = BgpPrefixListConfig({"PL100"});
  EXPECT_EQ(bareList.listName(), "PL100");
  EXPECT_FALSE(bareList.hasEntry());
  EXPECT_TRUE(bareList.attr().empty());

  // List-level attribute.
  auto listAttr = BgpPrefixListConfig({"PL100", "description", "my", "list"});
  EXPECT_FALSE(listAttr.hasEntry());
  EXPECT_EQ(listAttr.attr(), "description");
  EXPECT_EQ(listAttr.values(), std::vector<std::string>({"my", "list"}));

  // Bare create entry.
  auto bareEntry = BgpPrefixListConfig({"PL100", "entry", "10"});
  EXPECT_TRUE(bareEntry.hasEntry());
  EXPECT_EQ(bareEntry.seqNum(), 10);
  EXPECT_TRUE(bareEntry.attr().empty());

  // Entry-level attribute.
  auto entryAttr = BgpPrefixListConfig(
      {"PL100", "entry", "10", "base-prefix", "10.0.0.0/8"});
  EXPECT_TRUE(entryAttr.hasEntry());
  EXPECT_EQ(entryAttr.seqNum(), 10);
  EXPECT_EQ(entryAttr.attr(), "base-prefix");
  EXPECT_EQ(entryAttr.values(), std::vector<std::string>({"10.0.0.0/8"}));

  // Invalid: empty, empty name, entry keyword without a seq-num, non-integer
  // and negative seq-nums, unknown list/entry attributes.
  EXPECT_THROW(BgpPrefixListConfig({}), std::invalid_argument);
  EXPECT_THROW(BgpPrefixListConfig({""}), std::invalid_argument);
  EXPECT_THROW(BgpPrefixListConfig({"PL100", "entry"}), std::invalid_argument);
  EXPECT_THROW(
      BgpPrefixListConfig({"PL100", "entry", "ten"}), std::invalid_argument);
  EXPECT_THROW(
      BgpPrefixListConfig({"PL100", "entry", "-1"}), std::invalid_argument);
  EXPECT_THROW(
      BgpPrefixListConfig({"PL100", "no-such-attr", "1"}),
      std::invalid_argument);
  // base-prefix is an entry attribute, not a list attribute.
  EXPECT_THROW(
      BgpPrefixListConfig({"PL100", "base-prefix", "10.0.0.0/8"}),
      std::invalid_argument);
  // ip-version is a list attribute, not an entry attribute.
  EXPECT_THROW(
      BgpPrefixListConfig({"PL100", "entry", "10", "ip-version", "v4"}),
      std::invalid_argument);
  // description IS valid at both levels, so this must NOT throw.
  EXPECT_NO_THROW(
      BgpPrefixListConfig({"PL100", "entry", "10", "description", "x"}));
}

// ==============================================================================
// List-level handlers
// ==============================================================================

TEST_F(CmdConfigBgpPolicyPrefixListTestFixture, bareCreateList) {
  auto result = run({"PL100"});
  EXPECT_THAT(result, HasSubstr("Successfully created BGP prefix-list PL100"));
  ASSERT_EQ(lists().size(), 1);
  EXPECT_EQ(*lists()[0].name(), "PL100");
  EXPECT_TRUE(lists()[0].prefixes()->empty());
  EXPECT_TRUE(sessionFileExists());

  run({"PL200"});
  ASSERT_EQ(lists().size(), 2);
  EXPECT_EQ(*lists()[1].name(), "PL200");
}

TEST_F(CmdConfigBgpPolicyPrefixListTestFixture, setListDescription) {
  auto result = run({"PL100", "description", "spine", "prefixes"});
  EXPECT_THAT(result, HasSubstr("Successfully set description"));
  ASSERT_EQ(lists().size(), 1);
  // Multi-token description is re-joined.
  EXPECT_EQ(*lists()[0].description(), "spine prefixes");
}

TEST_F(CmdConfigBgpPolicyPrefixListTestFixture, setBooleanOperator) {
  auto result = run({"PL100", "boolean-operator", "AND"});
  EXPECT_THAT(result, HasSubstr("Successfully set boolean-operator"));
  EXPECT_EQ(*lists()[0].boolean_operator(), BooleanOperator::AND);

  run({"PL100", "boolean-operator", "NOT"});
  EXPECT_EQ(*lists()[0].boolean_operator(), BooleanOperator::NOT);
}

TEST_F(CmdConfigBgpPolicyPrefixListTestFixture, booleanOperatorDefaultIsOr) {
  run({"PL100"});
  EXPECT_EQ(*lists()[0].boolean_operator(), BooleanOperator::OR);
}

TEST_F(CmdConfigBgpPolicyPrefixListTestFixture, setCompareOperator) {
  auto result = run({"PL100", "compare-operator", "GE"});
  EXPECT_THAT(result, HasSubstr("Successfully set compare-operator"));
  ASSERT_TRUE(lists()[0].compare_operator().has_value());
  EXPECT_EQ(*lists()[0].compare_operator(), ComparisonOperator::GE);

  // RG is only documented for the entry-level prefix-len-range.
  auto rejected = run({"PL100", "compare-operator", "RG"});
  EXPECT_THAT(
      rejected,
      HasSubstr(
          "Invalid compare-operator value 'RG'; expected EQ|GE|LE|NE|GT|LT"));
  EXPECT_EQ(*lists()[0].compare_operator(), ComparisonOperator::GE);
}

TEST_F(CmdConfigBgpPolicyPrefixListTestFixture, setIpVersion) {
  auto result = run({"PL100", "ip-version", "v4"});
  EXPECT_THAT(result, HasSubstr("Successfully set ip-version"));
  ASSERT_TRUE(lists()[0].version().has_value());
  EXPECT_EQ(*lists()[0].version(), 4);

  run({"PL100", "ip-version", "v6"});
  EXPECT_EQ(*lists()[0].version(), 6);
}

TEST_F(CmdConfigBgpPolicyPrefixListTestFixture, namedListsAreDistinct) {
  run({"PL100", "description", "one"});
  run({"PL200", "description", "two"});
  ASSERT_EQ(lists().size(), 2);
  // Re-referencing an existing list by name updates it, not appends.
  run({"PL100", "description", "one-updated"});
  ASSERT_EQ(lists().size(), 2);
  EXPECT_EQ(*lists()[0].description(), "one-updated");
}

// ==============================================================================
// Entry-level handlers
// ==============================================================================

TEST_F(CmdConfigBgpPolicyPrefixListTestFixture, bareCreateEntry) {
  auto result = run({"PL100", "entry", "10"});
  EXPECT_THAT(
      result, HasSubstr("Successfully created BGP prefix-list PL100 entry 10"));
  ASSERT_EQ(lists().size(), 1);
  ASSERT_EQ(lists()[0].prefixes()->size(), 1);
  ASSERT_TRUE(entry(0, 0).seq_num().has_value());
  EXPECT_EQ(*entry(0, 0).seq_num(), 10);
}

TEST_F(CmdConfigBgpPolicyPrefixListTestFixture, setEntryAttributes) {
  run({"PL100", "entry", "10", "base-prefix", "10.0.0.0/8"});
  run({"PL100", "entry", "10", "description", "spine", "block"});
  run({"PL100", "entry", "10", "match-logic", "NOT_EQUAL"});
  run({"PL100", "entry", "10", "max-allowed-subnet-count", "64"});
  run({"PL100", "entry", "10", "regex", "^10\\..*"});

  ASSERT_EQ(lists().size(), 1);
  ASSERT_EQ(lists()[0].prefixes()->size(), 1);
  const auto& e = entry(0, 0);
  EXPECT_EQ(*e.seq_num(), 10);
  EXPECT_EQ(*e.base_prefix(), "10.0.0.0/8");
  EXPECT_EQ(*e.description(), "spine block");
  EXPECT_EQ(*e.match_logic(), MatchValueLogicOperator::NOT_EQUAL);
  EXPECT_EQ(*e.max_allowed_golden_prefix_subnet_count(), 64);
  EXPECT_EQ(*e.regex(), "^10\\..*");
}

TEST_F(CmdConfigBgpPolicyPrefixListTestFixture, setEntryPrefixLenRange) {
  run({"PL100", "entry", "10", "prefix-len-range", "compare-operator", "RG"});
  run({"PL100", "entry", "10", "prefix-len-range", "value", "24"});

  // Both sub-attributes land on the single prefix_len_ranges[0] element.
  ASSERT_EQ(entry(0, 0).prefix_len_ranges()->size(), 1);
  const auto& range = entry(0, 0).prefix_len_ranges()->front();
  EXPECT_EQ(*range.compare_operator(), ComparisonOperator::RG);
  EXPECT_EQ(*range.value(), 24);
}

TEST_F(CmdConfigBgpPolicyPrefixListTestFixture, entryCommunitiesAccumulate) {
  auto result = run({"PL100", "entry", "10", "communities", "65000:100"});
  EXPECT_THAT(result, HasSubstr("Successfully added 65000:100 to communities"));
  run({"PL100", "entry", "10", "communities", "65000:200"});

  ASSERT_TRUE(entry(0, 0).communities().has_value());
  EXPECT_THAT(
      *entry(0, 0).communities(),
      UnorderedElementsAre("65000:100", "65000:200"));

  // Re-adding an existing member reports it without duplicating.
  auto repeated = run({"PL100", "entry", "10", "communities", "65000:100"});
  EXPECT_THAT(repeated, HasSubstr("communities already contains 65000:100"));
  EXPECT_EQ(entry(0, 0).communities()->size(), 2);
}

TEST_F(CmdConfigBgpPolicyPrefixListTestFixture, entriesAccumulateAndAreKeyed) {
  run({"PL100", "entry", "10", "base-prefix", "10.0.0.0/8"});
  run({"PL100", "entry", "20", "base-prefix", "192.168.0.0/16"});
  ASSERT_EQ(lists().size(), 1);
  ASSERT_EQ(lists()[0].prefixes()->size(), 2);

  // Re-referencing an existing entry by seq-num updates it, not appends.
  run({"PL100", "entry", "10", "match-logic", "EQUAL"});
  EXPECT_EQ(lists()[0].prefixes()->size(), 2);
  EXPECT_EQ(*entry(0, 0).base_prefix(), "10.0.0.0/8");
  EXPECT_EQ(*entry(0, 0).match_logic(), MatchValueLogicOperator::EQUAL);
}

TEST_F(CmdConfigBgpPolicyPrefixListTestFixture, reReferenceReportsExisting) {
  run({"PL100"});
  // A second bare reference must not claim to have created it again.
  EXPECT_THAT(run({"PL100"}), HasSubstr("already exists"));
  ASSERT_EQ(lists().size(), 1);

  run({"PL100", "entry", "10"});
  EXPECT_THAT(
      run({"PL100", "entry", "10"}), HasSubstr("entry 10 already exists"));
  EXPECT_EQ(lists()[0].prefixes()->size(), 1);
}

// ==============================================================================
// Reject paths — the error is surfaced and nothing is persisted
// ==============================================================================

TEST_F(
    CmdConfigBgpPolicyPrefixListTestFixture,
    invalidBooleanOperatorRejected) {
  auto result = run({"PL100", "boolean-operator", "XOR"});
  EXPECT_THAT(
      result,
      HasSubstr("Invalid boolean-operator value 'XOR'; expected AND|OR|NOT"));
  // The rejected value must not leave a phantom list behind.
  EXPECT_TRUE(lists().empty());
  EXPECT_FALSE(sessionFileExists())
      << "session file should not exist after rejected input";
}

TEST_F(CmdConfigBgpPolicyPrefixListTestFixture, invalidIpVersionRejected) {
  auto result = run({"PL100", "ip-version", "v5"});
  EXPECT_THAT(
      result, HasSubstr("Invalid ip-version value 'v5'; expected v4|v6"));
  EXPECT_TRUE(lists().empty());
  EXPECT_FALSE(sessionFileExists())
      << "session file should not exist after rejected input";
}

TEST_F(CmdConfigBgpPolicyPrefixListTestFixture, invalidBasePrefixRejected) {
  // A base-prefix without an explicit /len is rejected, as is a non-address.
  auto noLen = run({"PL100", "entry", "10", "base-prefix", "10.0.0.0"});
  EXPECT_THAT(
      noLen,
      HasSubstr("Invalid base-prefix value '10.0.0.0'; expected <prefix/len>"));
  auto garbage = run({"PL100", "entry", "10", "base-prefix", "not-a-prefix"});
  EXPECT_THAT(garbage, HasSubstr("Invalid base-prefix value"));
  // folly rejects the rest: an out-of-range mask and a second slash.
  auto badMask = run({"PL100", "entry", "10", "base-prefix", "10.0.0.0/99"});
  EXPECT_THAT(badMask, HasSubstr("Invalid base-prefix value"));
  auto twoSlash = run({"PL100", "entry", "10", "base-prefix", "10.0.0.0/8/8"});
  EXPECT_THAT(twoSlash, HasSubstr("Invalid base-prefix value"));
  EXPECT_TRUE(lists().empty());
  EXPECT_FALSE(sessionFileExists())
      << "session file should not exist after rejected input";
}

TEST_F(CmdConfigBgpPolicyPrefixListTestFixture, basePrefixAcceptsV6) {
  auto result = run({"PL100", "entry", "10", "base-prefix", "2001:db8::/32"});
  EXPECT_THAT(result, HasSubstr("Successfully set base-prefix"));
  // Stored as typed, not normalized.
  EXPECT_EQ(*entry(0, 0).base_prefix(), "2001:db8::/32");
}

TEST_F(CmdConfigBgpPolicyPrefixListTestFixture, invalidPrefixLenRangeRejected) {
  // Unknown sub-attribute.
  auto badSub = run({"PL100", "entry", "10", "prefix-len-range", "min", "8"});
  EXPECT_THAT(
      badSub,
      HasSubstr(
          "Error: prefix-len-range requires <compare-operator|value> <value>"));
  // Out-of-range length.
  auto badLen =
      run({"PL100", "entry", "10", "prefix-len-range", "value", "129"});
  EXPECT_THAT(
      badLen,
      HasSubstr("Invalid prefix-len-range value value '129'; expected 0-128"));
  EXPECT_TRUE(lists().empty());
  EXPECT_FALSE(sessionFileExists())
      << "session file should not exist after rejected input";
}

TEST_F(
    CmdConfigBgpPolicyPrefixListTestFixture,
    rejectedPrefixLenRangeKeepsNoPhantomRange) {
  // Land the entry first, then reject a range value on it: the entry survives
  // but no phantom prefix_len_ranges element may appear.
  run({"PL100", "entry", "10", "base-prefix", "10.0.0.0/8"});
  auto result =
      run({"PL100", "entry", "10", "prefix-len-range", "value", "300"});
  EXPECT_THAT(result, HasSubstr("Invalid"));
  ASSERT_EQ(lists()[0].prefixes()->size(), 1);
  EXPECT_TRUE(entry(0, 0).prefix_len_ranges()->empty());
}

TEST_F(
    CmdConfigBgpPolicyPrefixListTestFixture,
    rejectedEntryOnExistingListKeepsList) {
  // Create the list first, then reject an entry value on it.
  run({"PL100", "description", "keep-me"});
  ASSERT_EQ(lists().size(), 1);

  auto result = run({"PL100", "entry", "10", "match-logic", "MAYBE"});
  EXPECT_THAT(result, HasSubstr("Invalid"));
  // The pre-existing list survives; only the phantom entry is rolled back.
  ASSERT_EQ(lists().size(), 1);
  EXPECT_TRUE(lists()[0].prefixes()->empty());
  EXPECT_EQ(*lists()[0].description(), "keep-me");
}

TEST_F(
    CmdConfigBgpPolicyPrefixListTestFixture,
    rejectedEntryKeepsExistingEntries) {
  run({"PL100", "entry", "10", "base-prefix", "10.0.0.0/8"});
  ASSERT_EQ(lists()[0].prefixes()->size(), 1);

  auto result = run({"PL100", "entry", "20", "match-logic", "MAYBE"});
  EXPECT_THAT(result, HasSubstr("Invalid"));
  // Only the phantom entry is rolled back; the existing one survives.
  ASSERT_EQ(lists()[0].prefixes()->size(), 1);
  EXPECT_EQ(*entry(0, 0).seq_num(), 10);
}

} // namespace facebook::fboss
