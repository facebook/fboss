// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <configerator/structs/neteng/fboss/bgp/gen-cpp2/bgp_config_types.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

#include "configerator/structs/neteng/bgp_policy/thrift/gen-cpp2/bgp_policy_types.h"
#include "configerator/structs/neteng/bgp_policy/thrift/gen-cpp2/routing_policy_types.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/policy/as-path-list/CmdConfigProtocolBgpPolicyAsPathList.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/policy/as-path-list/entry/CmdConfigProtocolBgpPolicyAsPathListEntry.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/test/config/CmdConfigTestBase.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

using namespace ::testing;
using facebook::bgp::routing_policy::MatchValueLogicOperator;

namespace facebook::fboss {

// The entry dispatcher only touches the BGP side of ConfigSession, which seeds
// from thrift schema defaults when neither a staged session nor a system
// bgpcpp.conf exists — so no seed agent config is needed (mirrors
// CmdConfigBgpPolicyAsPathListTest).
class CmdConfigBgpPolicyAsPathListEntryTestFixture : public CmdConfigTestBase {
 public:
  CmdConfigBgpPolicyAsPathListEntryTestFixture()
      : CmdConfigTestBase("bgp_aspath_list_entry_test_%%%%-%%%%-%%%%", "") {}

  void SetUp() override {
    CmdConfigTestBase::SetUp();
    setupTestableConfigSession();
  }

  // Invoke the entry handler the way the framework does: the parent's parsed
  // args plus the entry's own tokens.
  std::string runEntry(
      const std::vector<std::string>& listTokens,
      const std::vector<std::string>& entryTokens) {
    CmdConfigProtocolBgpPolicyAsPathListEntry cmd;
    HostInfo hostInfo("testhost");
    return cmd.queryClient(
        hostInfo,
        BgpAsPathListConfig(listTokens),
        BgpAsPathListEntryConfig(entryTokens));
  }

  std::string runList(const std::vector<std::string>& tokens) {
    CmdConfigProtocolBgpPolicyAsPathList cmd;
    HostInfo hostInfo("testhost");
    return cmd.queryClient(hostInfo, BgpAsPathListConfig(tokens));
  }

  const std::vector<bgp::bgp_policy::AsPathList>& lists() {
    return *ConfigSession::getInstance()
                .getBgpConfig()
                .policies()
                .ensure()
                .aspath_lists();
  }

  const std::vector<bgp::bgp_policy::AsPathListEntry>& entries(size_t listIdx) {
    return *lists()[listIdx].as_path_list();
  }

  bool sessionFileExists() {
    return std::filesystem::exists(
        ConfigSession::getInstance().getBgpSessionConfigPath());
  }
};

// ==============================================================================
// BgpAsPathListEntryConfig (arg) validation
// ==============================================================================

TEST_F(CmdConfigBgpPolicyAsPathListEntryTestFixture, argValidation) {
  // Bare create.
  auto bare = BgpAsPathListEntryConfig({"10"});
  EXPECT_EQ(bare.seqNum(), 10);
  EXPECT_TRUE(bare.attr().empty());

  // Attribute with values.
  auto attr = BgpAsPathListEntryConfig({"10", "asn-regexp", "^65000_"});
  EXPECT_EQ(attr.seqNum(), 10);
  EXPECT_EQ(attr.attr(), "asn-regexp");
  EXPECT_EQ(attr.values(), std::vector<std::string>({"^65000_"}));

  // Invalid: empty, non-numeric or negative seq-num, unknown attribute.
  EXPECT_THROW(BgpAsPathListEntryConfig({}), std::invalid_argument);
  EXPECT_THROW(BgpAsPathListEntryConfig({"not-a-num"}), std::invalid_argument);
  EXPECT_THROW(BgpAsPathListEntryConfig({"-1"}), std::invalid_argument);
  EXPECT_THROW(
      BgpAsPathListEntryConfig({"10", "no-such-attr", "1"}),
      std::invalid_argument);
}

// ==============================================================================
// Entry-level handlers
// ==============================================================================

TEST_F(CmdConfigBgpPolicyAsPathListEntryTestFixture, bareCreateEntry) {
  auto result = runEntry({"AS100"}, {"10"});
  EXPECT_THAT(
      result,
      HasSubstr("Successfully created BGP as-path-list AS100 entry 10"));
  ASSERT_EQ(lists().size(), 1);
  ASSERT_EQ(entries(0).size(), 1);
  EXPECT_EQ(*entries(0)[0].sequence_number(), 10);
}

TEST_F(CmdConfigBgpPolicyAsPathListEntryTestFixture, setEntryAttributes) {
  runEntry({"AS100"}, {"10", "asn-regexp", "^65000_"});
  runEntry({"AS100"}, {"10", "description", "match", "65000"});
  runEntry({"AS100"}, {"10", "match-logic", "NOT_EQUAL"});

  ASSERT_EQ(lists().size(), 1);
  ASSERT_EQ(entries(0).size(), 1);
  const auto& entry = entries(0)[0];
  EXPECT_EQ(*entry.sequence_number(), 10);
  EXPECT_EQ(entry.as_path()->as_path_ref()->asn_regexp().value(), "^65000_");
  EXPECT_EQ(*entry.description(), "match 65000");
  EXPECT_EQ(*entry.match_logic_type(), MatchValueLogicOperator::NOT_EQUAL);
}

TEST_F(
    CmdConfigBgpPolicyAsPathListEntryTestFixture,
    entriesAccumulateAndAreKeyed) {
  runEntry({"AS100"}, {"10", "asn-regexp", "^65000_"});
  runEntry({"AS100"}, {"20", "asn-regexp", "^65001_"});
  ASSERT_EQ(lists().size(), 1);
  ASSERT_EQ(entries(0).size(), 2);

  // Re-referencing an existing entry by seq-num updates it, not appends.
  runEntry({"AS100"}, {"10", "match-logic", "EQUAL"});
  EXPECT_EQ(entries(0).size(), 2);
}

TEST_F(CmdConfigBgpPolicyAsPathListEntryTestFixture, asnRegexpAllowsSpaces) {
  // AS-path regexes separate ASNs with spaces; the pattern must survive as a
  // single re-joined value.
  runEntry({"AS100"}, {"10", "asn-regexp", "^65000", "65001$"});
  EXPECT_EQ(
      entries(0)[0].as_path()->as_path_ref()->asn_regexp().value(),
      "^65000 65001$");
}

TEST_F(
    CmdConfigBgpPolicyAsPathListEntryTestFixture,
    reReferenceReportsExisting) {
  runEntry({"AS100"}, {"10"});
  EXPECT_THAT(
      runEntry({"AS100"}, {"10"}), HasSubstr("entry 10 already exists"));
  EXPECT_EQ(entries(0).size(), 1);
}

TEST_F(CmdConfigBgpPolicyAsPathListEntryTestFixture, matchLogicDefaultIsEqual) {
  runEntry({"AS100"}, {"10"});
  EXPECT_EQ(*entries(0)[0].match_logic_type(), MatchValueLogicOperator::EQUAL);
}

// ==============================================================================
// Reject paths — the error is surfaced and nothing is persisted
// ==============================================================================

TEST_F(
    CmdConfigBgpPolicyAsPathListEntryTestFixture,
    invalidMatchLogicRejected) {
  auto result = runEntry({"AS100"}, {"10", "match-logic", "MAYBE"});
  // The full message matters: the accepted-values suffix once printed garbage
  // (a string_view capture dangling over a fmt::format temporary).
  EXPECT_THAT(
      result,
      HasSubstr("Invalid match-logic value 'MAYBE'; expected EQUAL|NOT_EQUAL"));
  // The rejected value must not leave a phantom list/entry behind.
  EXPECT_TRUE(lists().empty());
  EXPECT_FALSE(sessionFileExists())
      << "session file should not exist after rejected input";
}

TEST_F(
    CmdConfigBgpPolicyAsPathListEntryTestFixture,
    rejectedEntryOnExistingListKeepsList) {
  // Create the list first, then reject an entry value on it.
  runList({"AS100", "description", "keep-me"});
  ASSERT_EQ(lists().size(), 1);

  auto result = runEntry({"AS100"}, {"10", "match-logic", "MAYBE"});
  EXPECT_THAT(result, HasSubstr("Invalid"));
  // The pre-existing list survives; only the phantom entry is rolled back.
  ASSERT_EQ(lists().size(), 1);
  EXPECT_TRUE(entries(0).empty());
  EXPECT_EQ(*lists()[0].description(), "keep-me");
}

// A list-level attribute alongside an entry must be rejected: only the leaf
// command runs, so the list attribute would be silently dropped.
TEST_F(
    CmdConfigBgpPolicyAsPathListEntryTestFixture,
    listAttributeMixedWithEntryRejected) {
  auto result = runEntry({"AS100", "description", "mixed"}, {"10"});
  EXPECT_THAT(result, HasSubstr("separate commands"));
  EXPECT_TRUE(lists().empty());
  EXPECT_FALSE(sessionFileExists());
}

} // namespace facebook::fboss
