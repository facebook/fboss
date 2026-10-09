// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <configerator/structs/neteng/fboss/bgp/gen-cpp2/bgp_config_types.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

#include "configerator/structs/neteng/bgp_policy/thrift/gen-cpp2/routing_policy_types.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/policy/prefix-list/CmdConfigProtocolBgpPolicyPrefixList.h"
#include "fboss/cli/fboss2/commands/delete/protocol/bgp/policy/prefix-list/CmdDeleteProtocolBgpPolicyPrefixList.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/test/config/CmdConfigTestBase.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

using namespace ::testing;

namespace facebook::fboss {

// Deleting only touches the BGP side of ConfigSession, which seeds from thrift
// schema defaults when neither a staged session nor a system bgpcpp.conf
// exists — so no seed agent config is needed (mirrors
// CmdDeleteBgpPolicyCommunityListTest).
class CmdDeleteBgpPolicyPrefixListTestFixture : public CmdConfigTestBase {
 public:
  CmdDeleteBgpPolicyPrefixListTestFixture()
      : CmdConfigTestBase("bgp_prefix_list_delete_test_%%%%-%%%%-%%%%", "") {}

  void SetUp() override {
    CmdConfigTestBase::SetUp();
    setupTestableConfigSession();
  }

  std::string configure(const std::vector<std::string>& tokens) {
    CmdConfigProtocolBgpPolicyPrefixList cmd;
    HostInfo hostInfo("testhost");
    return cmd.queryClient(hostInfo, BgpPrefixListConfig(tokens));
  }

  std::string del(const std::vector<std::string>& tokens) {
    CmdDeleteProtocolBgpPolicyPrefixList cmd;
    HostInfo hostInfo("testhost");
    return cmd.queryClient(hostInfo, BgpPrefixListRef(tokens));
  }

  const std::vector<bgp::routing_policy::PrefixList>& lists() {
    return *ConfigSession::getInstance()
                .getBgpConfig()
                .policies()
                .ensure()
                .prefix_lists();
  }

  // Seed a routing-policy term whose PREFIX_LIST match names `listName` (the
  // term/match CLIs live in higher PRs), the way bgpd reads the reference:
  // prefix_filters.prefix_list_names.
  void addPolicyTermMatching(
      const std::string& policy,
      int64_t seq,
      const std::string& listName) {
    auto& cfg = ConfigSession::getInstance().getBgpConfig();
    auto& policies = *cfg.policies().ensure().bgp_policy_statements();
    auto it = std::find_if(policies.begin(), policies.end(), [&](auto& p) {
      return *p.name() == policy;
    });
    if (it == policies.end()) {
      policies.emplace_back();
      policies.back().name() = policy;
      it = std::prev(policies.end());
    }
    auto& terms = *it->policy_entries();
    terms.emplace_back();
    terms.back().sequence_number() = seq;
    auto& matches =
        *terms.back().policy_match_entries().ensure().match_entries();
    matches.emplace_back();
    matches.back().type() =
        bgp::bgp_policy::BgpPolicyAtomicMatchType::PREFIX_LIST;
    matches.back().prefix_filters().ensure().prefix_list_names() = {listName};
    ConfigSession::getInstance().saveBgpConfig();
  }

  bool sessionFileExists() {
    return std::filesystem::exists(
        ConfigSession::getInstance().getBgpSessionConfigPath());
  }
};

// ==============================================================================
// BgpPrefixListRef (arg) validation
// ==============================================================================

TEST_F(CmdDeleteBgpPolicyPrefixListTestFixture, argValidation) {
  auto listOnly = BgpPrefixListRef({"PL100"});
  EXPECT_EQ(listOnly.listName(), "PL100");

  // Invalid: empty, empty name, any second token (entries are deleted
  // through the entry subcommand, not here).
  EXPECT_THROW(BgpPrefixListRef({}), std::invalid_argument);
  EXPECT_THROW(BgpPrefixListRef({""}), std::invalid_argument);
  EXPECT_THROW(BgpPrefixListRef({"PL100", "PL200"}), std::invalid_argument);
  EXPECT_THROW(
      BgpPrefixListRef({"PL100", "entry", "10"}), std::invalid_argument);
}

// ==============================================================================
// queryClient: whole-list deletion
// ==============================================================================

TEST_F(CmdDeleteBgpPolicyPrefixListTestFixture, deleteExistingList) {
  configure({"PL100", "description", "drop"});
  configure({"PL200", "description", "keep"});
  ASSERT_EQ(lists().size(), 2);

  auto result = del({"PL100"});
  EXPECT_THAT(result, HasSubstr("Successfully deleted BGP prefix-list PL100"));
  // The other list survives untouched.
  ASSERT_EQ(lists().size(), 1);
  EXPECT_EQ(*lists()[0].name(), "PL200");
  EXPECT_EQ(*lists()[0].description(), "keep");
  EXPECT_TRUE(sessionFileExists());
}

// Delete mirrors add: an absent target is a success with a warning, never an
// error, so a replayed script stays idempotent.
TEST_F(CmdDeleteBgpPolicyPrefixListTestFixture, deleteUnknownListWarns) {
  auto result = del({"NO-SUCH-LIST"});
  EXPECT_THAT(
      result,
      HasSubstr(
          "Warning: BGP prefix-list NO-SUCH-LIST does not exist; nothing to "
          "delete"));
  EXPECT_THAT(result, Not(HasSubstr("Error:")));
  // Nothing changed, so nothing is staged.
  EXPECT_FALSE(sessionFileExists())
      << "session file should not exist after a no-op delete";
}

TEST_F(CmdDeleteBgpPolicyPrefixListTestFixture, deleteTwiceIsIdempotent) {
  configure({"PL100", "description", "delete-me"});
  EXPECT_THAT(del({"PL100"}), HasSubstr("Successfully deleted"));
  EXPECT_TRUE(lists().empty());

  auto again = del({"PL100"});
  EXPECT_THAT(
      again,
      HasSubstr(
          "Warning: BGP prefix-list PL100 does not exist; nothing to delete"));
  EXPECT_THAT(again, Not(HasSubstr("Error:")));
  EXPECT_TRUE(lists().empty());
}

TEST_F(
    CmdDeleteBgpPolicyPrefixListTestFixture,
    deleteUnknownLeavesOthersIntact) {
  configure({"PL100", "description", "one"});
  auto result = del({"PL200"});
  EXPECT_THAT(result, HasSubstr("does not exist"));
  ASSERT_EQ(lists().size(), 1);
  EXPECT_EQ(*lists()[0].name(), "PL100");
}

// ==============================================================================
// Reference guard — a list a routing-policy term still names is not deletable
// ==============================================================================

TEST_F(CmdDeleteBgpPolicyPrefixListTestFixture, deleteReferencedListRejected) {
  configure({"PL100", "description", "in-use"});
  addPolicyTermMatching("RM100", 10, "PL100");

  auto result = del({"PL100"});
  EXPECT_THAT(result, HasSubstr("still referenced"));
  // The refusal names the policy and term so the user can act on it.
  EXPECT_THAT(result, HasSubstr("policy RM100 term 10"));
  EXPECT_THAT(result, HasSubstr("remove those matches first"));
  ASSERT_EQ(lists().size(), 1);
  EXPECT_EQ(*lists()[0].name(), "PL100");
  EXPECT_EQ(*lists()[0].description(), "in-use");
}

TEST_F(
    CmdDeleteBgpPolicyPrefixListTestFixture,
    deleteReferencedListNamesEveryReferrer) {
  configure({"PL100"});
  addPolicyTermMatching("RM100", 10, "PL100");
  addPolicyTermMatching("RM200", 20, "PL100");

  auto result = del({"PL100"});
  EXPECT_THAT(result, HasSubstr("policy RM100 term 10"));
  EXPECT_THAT(result, HasSubstr("policy RM200 term 20"));
  ASSERT_EQ(lists().size(), 1);
}

TEST_F(
    CmdDeleteBgpPolicyPrefixListTestFixture,
    referenceToOtherListDoesNotBlockDelete) {
  configure({"PL100"});
  configure({"PL200"});
  // A term referencing PL100 must not block deleting PL200.
  addPolicyTermMatching("RM100", 10, "PL100");

  auto result = del({"PL200"});
  EXPECT_THAT(result, HasSubstr("Successfully deleted BGP prefix-list PL200"));
  ASSERT_EQ(lists().size(), 1);
  EXPECT_EQ(*lists()[0].name(), "PL100");
}

} // namespace facebook::fboss
