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
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/test/config/CmdConfigTestBase.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

using namespace ::testing;
using facebook::bgp::routing_policy::MatchValueLogicOperator;

namespace facebook::fboss {

// The as-path-list dispatcher only touches the BGP side of ConfigSession,
// which seeds from thrift schema defaults when neither a staged session nor a
// system bgpcpp.conf exists — so no seed agent config is needed (mirrors
// CmdConfigBgpPeerGroupTest).
class CmdConfigBgpPolicyAsPathListTestFixture : public CmdConfigTestBase {
 public:
  CmdConfigBgpPolicyAsPathListTestFixture()
      : CmdConfigTestBase("bgp_aspath_list_test_%%%%-%%%%-%%%%", "") {}

  void SetUp() override {
    CmdConfigTestBase::SetUp();
    setupTestableConfigSession();
  }

  std::string run(const std::vector<std::string>& tokens) {
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

  bool sessionFileExists() {
    return std::filesystem::exists(
        ConfigSession::getInstance().getBgpSessionConfigPath());
  }
};

// ==============================================================================
// BgpAsPathListConfig (arg) validation
// ==============================================================================

TEST_F(CmdConfigBgpPolicyAsPathListTestFixture, argValidation) {
  // Bare create list.
  auto bareList = BgpAsPathListConfig({"AS100"});
  EXPECT_EQ(bareList.listName(), "AS100");
  EXPECT_FALSE(bareList.hasEntry());
  EXPECT_TRUE(bareList.attr().empty());

  // List-level attribute.
  auto listAttr = BgpAsPathListConfig({"AS100", "description", "my", "list"});
  EXPECT_FALSE(listAttr.hasEntry());
  EXPECT_EQ(listAttr.attr(), "description");
  EXPECT_EQ(listAttr.values(), std::vector<std::string>({"my", "list"}));

  // Bare create entry.
  auto bareEntry = BgpAsPathListConfig({"AS100", "entry", "10"});
  EXPECT_TRUE(bareEntry.hasEntry());
  EXPECT_EQ(bareEntry.seqNum(), 10);
  EXPECT_TRUE(bareEntry.attr().empty());

  // Entry-level attribute.
  auto entryAttr =
      BgpAsPathListConfig({"AS100", "entry", "10", "asn-regexp", "^65000_"});
  EXPECT_TRUE(entryAttr.hasEntry());
  EXPECT_EQ(entryAttr.seqNum(), 10);
  EXPECT_EQ(entryAttr.attr(), "asn-regexp");
  EXPECT_EQ(entryAttr.values(), std::vector<std::string>({"^65000_"}));

  // Invalid: empty, empty name, bad seq-num, unknown list/entry attributes,
  // entry keyword without a seq-num.
  EXPECT_THROW(BgpAsPathListConfig({}), std::invalid_argument);
  EXPECT_THROW(BgpAsPathListConfig({""}), std::invalid_argument);
  EXPECT_THROW(
      BgpAsPathListConfig({"AS100", "entry", "not-a-num"}),
      std::invalid_argument);
  EXPECT_THROW(BgpAsPathListConfig({"AS100", "entry"}), std::invalid_argument);
  EXPECT_THROW(
      BgpAsPathListConfig({"AS100", "no-such-attr", "1"}),
      std::invalid_argument);
  // asn-regexp is an entry attribute, not a list attribute.
  EXPECT_THROW(
      BgpAsPathListConfig({"AS100", "asn-regexp", "^65000_"}),
      std::invalid_argument);
  // description is a list attribute, not valid at the entry level? It IS valid
  // at both levels, so this must NOT throw.
  EXPECT_NO_THROW(
      BgpAsPathListConfig({"AS100", "entry", "10", "description", "x"}));
}

// ==============================================================================
// List-level handlers
// ==============================================================================

TEST_F(CmdConfigBgpPolicyAsPathListTestFixture, bareCreateList) {
  auto result = run({"AS100"});
  EXPECT_THAT(result, HasSubstr("Successfully created BGP as-path-list AS100"));
  ASSERT_EQ(lists().size(), 1);
  EXPECT_EQ(*lists()[0].name(), "AS100");
  EXPECT_TRUE(lists()[0].as_path_list()->empty());
  EXPECT_TRUE(sessionFileExists());

  run({"AS200"});
  ASSERT_EQ(lists().size(), 2);
  EXPECT_EQ(*lists()[1].name(), "AS200");
}

TEST_F(CmdConfigBgpPolicyAsPathListTestFixture, setListDescription) {
  auto result = run({"AS100", "description", "spine", "as-paths"});
  EXPECT_THAT(result, HasSubstr("Successfully set description"));
  ASSERT_EQ(lists().size(), 1);
  // Multi-token description is re-joined.
  EXPECT_EQ(*lists()[0].description(), "spine as-paths");
}

TEST_F(CmdConfigBgpPolicyAsPathListTestFixture, namedListsAreDistinct) {
  run({"AS100", "description", "one"});
  run({"AS200", "description", "two"});
  ASSERT_EQ(lists().size(), 2);
  // Re-referencing an existing list by name updates it, not appends.
  run({"AS100", "description", "one-updated"});
  ASSERT_EQ(lists().size(), 2);
  EXPECT_EQ(*lists()[0].description(), "one-updated");
}

// ==============================================================================
// Entry-level handlers
// ==============================================================================

TEST_F(CmdConfigBgpPolicyAsPathListTestFixture, bareCreateEntry) {
  auto result = run({"AS100", "entry", "10"});
  EXPECT_THAT(
      result,
      HasSubstr("Successfully created BGP as-path-list AS100 entry 10"));
  ASSERT_EQ(lists().size(), 1);
  ASSERT_EQ(lists()[0].as_path_list()->size(), 1);
  EXPECT_EQ(*lists()[0].as_path_list()->at(0).sequence_number(), 10);
}

TEST_F(CmdConfigBgpPolicyAsPathListTestFixture, setEntryAttributes) {
  run({"AS100", "entry", "10", "asn-regexp", "^65000_"});
  run({"AS100", "entry", "10", "description", "match", "65000"});
  run({"AS100", "entry", "10", "match-logic", "NOT_EQUAL"});

  ASSERT_EQ(lists().size(), 1);
  ASSERT_EQ(lists()[0].as_path_list()->size(), 1);
  const auto& entry = lists()[0].as_path_list()->at(0);
  EXPECT_EQ(*entry.sequence_number(), 10);
  EXPECT_EQ(entry.as_path()->as_path_ref()->asn_regexp().value(), "^65000_");
  EXPECT_EQ(*entry.description(), "match 65000");
  EXPECT_EQ(*entry.match_logic_type(), MatchValueLogicOperator::NOT_EQUAL);
}

TEST_F(CmdConfigBgpPolicyAsPathListTestFixture, entriesAccumulateAndAreKeyed) {
  run({"AS100", "entry", "10", "asn-regexp", "^65000_"});
  run({"AS100", "entry", "20", "asn-regexp", "^65001_"});
  ASSERT_EQ(lists().size(), 1);
  ASSERT_EQ(lists()[0].as_path_list()->size(), 2);

  // Re-referencing an existing entry by seq-num updates it, not appends.
  run({"AS100", "entry", "10", "match-logic", "EQUAL"});
  EXPECT_EQ(lists()[0].as_path_list()->size(), 2);
}

TEST_F(CmdConfigBgpPolicyAsPathListTestFixture, asnRegexpAllowsSpaces) {
  // AS-path regexes separate ASNs with spaces; the pattern must survive as a
  // single re-joined value.
  run({"AS100", "entry", "10", "asn-regexp", "^65000", "65001$"});
  const auto& entry = lists()[0].as_path_list()->at(0);
  EXPECT_EQ(
      entry.as_path()->as_path_ref()->asn_regexp().value(), "^65000 65001$");
}

TEST_F(CmdConfigBgpPolicyAsPathListTestFixture, reReferenceReportsExisting) {
  run({"AS100"});
  // A second bare reference must not claim to have created it again.
  EXPECT_THAT(run({"AS100"}), HasSubstr("already exists"));
  ASSERT_EQ(lists().size(), 1);

  run({"AS100", "entry", "10"});
  EXPECT_THAT(
      run({"AS100", "entry", "10"}), HasSubstr("entry 10 already exists"));
  EXPECT_EQ(lists()[0].as_path_list()->size(), 1);
}

TEST_F(CmdConfigBgpPolicyAsPathListTestFixture, matchLogicDefaultIsEqual) {
  run({"AS100", "entry", "10"});
  const auto& entry = lists()[0].as_path_list()->at(0);
  EXPECT_EQ(*entry.match_logic_type(), MatchValueLogicOperator::EQUAL);
}

// ==============================================================================
// Reject paths — the error is surfaced and nothing is persisted
// ==============================================================================

TEST_F(CmdConfigBgpPolicyAsPathListTestFixture, invalidMatchLogicRejected) {
  auto result = run({"AS100", "entry", "10", "match-logic", "MAYBE"});
  EXPECT_THAT(result, HasSubstr("Invalid"));
  // The rejected value must not leave a phantom list/entry behind.
  EXPECT_TRUE(lists().empty());
  EXPECT_FALSE(sessionFileExists())
      << "session file should not exist after rejected input";
}

TEST_F(
    CmdConfigBgpPolicyAsPathListTestFixture,
    rejectedEntryOnExistingListKeepsList) {
  // Create the list first, then reject an entry value on it.
  run({"AS100", "description", "keep-me"});
  ASSERT_EQ(lists().size(), 1);

  auto result = run({"AS100", "entry", "10", "match-logic", "MAYBE"});
  EXPECT_THAT(result, HasSubstr("Invalid"));
  // The pre-existing list survives; only the phantom entry is rolled back.
  ASSERT_EQ(lists().size(), 1);
  EXPECT_TRUE(lists()[0].as_path_list()->empty());
  EXPECT_EQ(*lists()[0].description(), "keep-me");
}

} // namespace facebook::fboss
