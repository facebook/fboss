// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <configerator/structs/neteng/fboss/bgp/gen-cpp2/bgp_config_types.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

#include "configerator/structs/neteng/bgp_policy/thrift/gen-cpp2/bgp_policy_types.h"
#include "configerator/structs/neteng/bgp_policy/thrift/gen-cpp2/routing_policy_types.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/policy/community-list/CmdConfigProtocolBgpPolicyCommunityList.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/test/config/CmdConfigTestBase.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

using namespace ::testing;
using facebook::bgp::routing_policy::BooleanOperator;

namespace facebook::fboss {

// The community-list dispatcher only touches the BGP side of ConfigSession,
// which seeds from thrift schema defaults when neither a staged session nor a
// system bgpcpp.conf exists — so no seed agent config is needed (mirrors
// CmdConfigBgpPolicyAsPathListTest).
class CmdConfigBgpPolicyCommunityListTestFixture : public CmdConfigTestBase {
 public:
  CmdConfigBgpPolicyCommunityListTestFixture()
      : CmdConfigTestBase("bgp_community_list_test_%%%%-%%%%-%%%%", "") {}

  void SetUp() override {
    CmdConfigTestBase::SetUp();
    setupTestableConfigSession();
  }

  std::string run(const std::vector<std::string>& tokens) {
    CmdConfigProtocolBgpPolicyCommunityList cmd;
    HostInfo hostInfo("testhost");
    return cmd.queryClient(hostInfo, BgpCommunityListConfig(tokens));
  }

  const std::vector<bgp::bgp_policy::CommunityList>& lists() {
    return *ConfigSession::getInstance()
                .getBgpConfig()
                .policies()
                .ensure()
                .community_lists();
  }

  // The inline Community of member `idx` of list `listIdx`.
  const bgp::bgp_policy::Community& member(size_t listIdx, size_t idx) {
    return *lists()[listIdx].members()->at(idx).community_ref();
  }

  bool sessionFileExists() {
    return std::filesystem::exists(
        ConfigSession::getInstance().getBgpSessionConfigPath());
  }
};

// ==============================================================================
// BgpCommunityListConfig (arg) validation
// ==============================================================================

TEST_F(CmdConfigBgpPolicyCommunityListTestFixture, argValidation) {
  // Bare create list.
  auto bareList = BgpCommunityListConfig({"CL100"});
  EXPECT_EQ(bareList.listName(), "CL100");
  EXPECT_FALSE(bareList.hasCommunity());
  EXPECT_TRUE(bareList.attr().empty());

  // List-level attribute.
  auto listAttr =
      BgpCommunityListConfig({"CL100", "description", "my", "list"});
  EXPECT_FALSE(listAttr.hasCommunity());
  EXPECT_EQ(listAttr.attr(), "description");
  EXPECT_EQ(listAttr.values(), std::vector<std::string>({"my", "list"}));

  // Bare create member.
  auto bareMember = BgpCommunityListConfig({"CL100", "community", "CM1"});
  EXPECT_TRUE(bareMember.hasCommunity());
  EXPECT_EQ(bareMember.communityName(), "CM1");
  EXPECT_TRUE(bareMember.attr().empty());

  // Member-level attribute.
  auto memberAttr = BgpCommunityListConfig(
      {"CL100", "community", "CM1", "value", "65000:100"});
  EXPECT_TRUE(memberAttr.hasCommunity());
  EXPECT_EQ(memberAttr.communityName(), "CM1");
  EXPECT_EQ(memberAttr.attr(), "value");
  EXPECT_EQ(memberAttr.values(), std::vector<std::string>({"65000:100"}));

  // Invalid: empty, empty name, community keyword without a name, unknown
  // list/member attributes.
  EXPECT_THROW(BgpCommunityListConfig({}), std::invalid_argument);
  EXPECT_THROW(BgpCommunityListConfig({""}), std::invalid_argument);
  EXPECT_THROW(
      BgpCommunityListConfig({"CL100", "community"}), std::invalid_argument);
  EXPECT_THROW(
      BgpCommunityListConfig({"CL100", "community", ""}),
      std::invalid_argument);
  EXPECT_THROW(
      BgpCommunityListConfig({"CL100", "no-such-attr", "1"}),
      std::invalid_argument);
  // value is a member attribute, not a list attribute.
  EXPECT_THROW(
      BgpCommunityListConfig({"CL100", "value", "65000:100"}),
      std::invalid_argument);
  // exact-match is a list attribute, not a member attribute.
  EXPECT_THROW(
      BgpCommunityListConfig(
          {"CL100", "community", "CM1", "exact-match", "true"}),
      std::invalid_argument);
  // description IS valid at both levels, so this must NOT throw.
  EXPECT_NO_THROW(BgpCommunityListConfig(
      {"CL100", "community", "CM1", "description", "x"}));
}

// ==============================================================================
// List-level handlers
// ==============================================================================

TEST_F(CmdConfigBgpPolicyCommunityListTestFixture, bareCreateList) {
  auto result = run({"CL100"});
  EXPECT_THAT(
      result, HasSubstr("Successfully created BGP community-list CL100"));
  ASSERT_EQ(lists().size(), 1);
  EXPECT_EQ(*lists()[0].name(), "CL100");
  EXPECT_FALSE(lists()[0].members().has_value());
  EXPECT_TRUE(sessionFileExists());

  run({"CL200"});
  ASSERT_EQ(lists().size(), 2);
  EXPECT_EQ(*lists()[1].name(), "CL200");
}

TEST_F(CmdConfigBgpPolicyCommunityListTestFixture, setListDescription) {
  auto result = run({"CL100", "description", "spine", "communities"});
  EXPECT_THAT(result, HasSubstr("Successfully set description"));
  ASSERT_EQ(lists().size(), 1);
  // Multi-token description is re-joined.
  EXPECT_EQ(*lists()[0].description(), "spine communities");
}

TEST_F(CmdConfigBgpPolicyCommunityListTestFixture, setBooleanOperator) {
  auto result = run({"CL100", "boolean-operator", "AND"});
  EXPECT_THAT(result, HasSubstr("Successfully set boolean-operator"));
  EXPECT_EQ(*lists()[0].boolean_operator(), BooleanOperator::AND);

  run({"CL100", "boolean-operator", "NOT"});
  EXPECT_EQ(*lists()[0].boolean_operator(), BooleanOperator::NOT);
}

TEST_F(CmdConfigBgpPolicyCommunityListTestFixture, booleanOperatorDefaultIsOr) {
  run({"CL100"});
  EXPECT_EQ(*lists()[0].boolean_operator(), BooleanOperator::OR);
}

TEST_F(CmdConfigBgpPolicyCommunityListTestFixture, setExactMatch) {
  auto result = run({"CL100", "exact-match", "true"});
  EXPECT_THAT(result, HasSubstr("Successfully enabled exact-match"));
  ASSERT_TRUE(lists()[0].exact_match().has_value());
  EXPECT_TRUE(*lists()[0].exact_match());

  auto disabled = run({"CL100", "exact-match", "false"});
  EXPECT_THAT(disabled, HasSubstr("Successfully disabled exact-match"));
  EXPECT_FALSE(*lists()[0].exact_match());
}

TEST_F(CmdConfigBgpPolicyCommunityListTestFixture, namedListsAreDistinct) {
  run({"CL100", "description", "one"});
  run({"CL200", "description", "two"});
  ASSERT_EQ(lists().size(), 2);
  // Re-referencing an existing list by name updates it, not appends.
  run({"CL100", "description", "one-updated"});
  ASSERT_EQ(lists().size(), 2);
  EXPECT_EQ(*lists()[0].description(), "one-updated");
}

// ==============================================================================
// Community (member)-level handlers
// ==============================================================================

TEST_F(CmdConfigBgpPolicyCommunityListTestFixture, bareCreateCommunity) {
  auto result = run({"CL100", "community", "CM1"});
  EXPECT_THAT(
      result,
      HasSubstr("Successfully created BGP community-list CL100 community CM1"));
  ASSERT_EQ(lists().size(), 1);
  ASSERT_EQ(lists()[0].members()->size(), 1);
  EXPECT_EQ(*member(0, 0).name(), "CM1");
}

TEST_F(CmdConfigBgpPolicyCommunityListTestFixture, setCommunityAttributes) {
  run({"CL100", "community", "CM1", "value", "65000:100"});
  run({"CL100", "community", "CM1", "type", "LARGE"});
  run({"CL100", "community", "CM1", "description", "spine", "tag"});

  ASSERT_EQ(lists().size(), 1);
  ASSERT_EQ(lists()[0].members()->size(), 1);
  const auto& community = member(0, 0);
  EXPECT_EQ(*community.name(), "CM1");
  EXPECT_EQ(*community.value(), "65000:100");
  EXPECT_EQ(*community.type(), bgp::bgp_policy::CommunityType::LARGE);
  EXPECT_EQ(*community.description(), "spine tag");
}

TEST_F(
    CmdConfigBgpPolicyCommunityListTestFixture,
    communitiesAccumulateAndAreKeyed) {
  run({"CL100", "community", "CM1", "value", "65000:100"});
  run({"CL100", "community", "CM2", "value", "65000:200"});
  ASSERT_EQ(lists().size(), 1);
  ASSERT_EQ(lists()[0].members()->size(), 2);

  // Re-referencing an existing member by name updates it, not appends.
  run({"CL100", "community", "CM1", "type", "NORMAL"});
  EXPECT_EQ(lists()[0].members()->size(), 2);
  EXPECT_EQ(*member(0, 0).value(), "65000:100");
  EXPECT_EQ(*member(0, 0).type(), bgp::bgp_policy::CommunityType::NORMAL);
}

TEST_F(CmdConfigBgpPolicyCommunityListTestFixture, reReferenceReportsExisting) {
  run({"CL100"});
  // A second bare reference must not claim to have created it again.
  EXPECT_THAT(run({"CL100"}), HasSubstr("already exists"));
  ASSERT_EQ(lists().size(), 1);

  run({"CL100", "community", "CM1"});
  EXPECT_THAT(
      run({"CL100", "community", "CM1"}),
      HasSubstr("community CM1 already exists"));
  EXPECT_EQ(lists()[0].members()->size(), 1);
}

// ==============================================================================
// Reject paths — the error is surfaced and nothing is persisted
// ==============================================================================

TEST_F(
    CmdConfigBgpPolicyCommunityListTestFixture,
    invalidBooleanOperatorRejected) {
  auto result = run({"CL100", "boolean-operator", "XOR"});
  // The full message matters: the accepted-values suffix once printed garbage
  // (a string_view capture dangling over a fmt::format temporary).
  EXPECT_THAT(
      result,
      HasSubstr("Invalid boolean-operator value 'XOR'; expected AND|OR|NOT"));
  // The rejected value must not leave a phantom list behind.
  EXPECT_TRUE(lists().empty());
  EXPECT_FALSE(sessionFileExists())
      << "session file should not exist after rejected input";
}

TEST_F(
    CmdConfigBgpPolicyCommunityListTestFixture,
    invalidCommunityTypeRejected) {
  auto result = run({"CL100", "community", "CM1", "type", "MEDIUM"});
  EXPECT_THAT(
      result,
      HasSubstr("Invalid type value 'MEDIUM'; expected NORMAL|EXTENDED|LARGE"));
  EXPECT_TRUE(lists().empty());
  EXPECT_FALSE(sessionFileExists())
      << "session file should not exist after rejected input";
}

TEST_F(CmdConfigBgpPolicyCommunityListTestFixture, invalidExactMatchRejected) {
  auto result = run({"CL100", "exact-match", "maybe"});
  EXPECT_THAT(result, HasSubstr("Invalid"));
  EXPECT_TRUE(lists().empty());
  EXPECT_FALSE(sessionFileExists())
      << "session file should not exist after rejected input";
}

TEST_F(
    CmdConfigBgpPolicyCommunityListTestFixture,
    rejectedCommunityOnExistingListKeepsList) {
  // Create the list first, then reject a member value on it.
  run({"CL100", "description", "keep-me"});
  ASSERT_EQ(lists().size(), 1);

  auto result = run({"CL100", "community", "CM1", "type", "MEDIUM"});
  EXPECT_THAT(result, HasSubstr("Invalid"));
  // The pre-existing list survives; only the phantom member is rolled back —
  // including the members field itself, which was unset before.
  ASSERT_EQ(lists().size(), 1);
  EXPECT_FALSE(lists()[0].members().has_value());
  EXPECT_EQ(*lists()[0].description(), "keep-me");
}

TEST_F(
    CmdConfigBgpPolicyCommunityListTestFixture,
    rejectedCommunityKeepsExistingMembers) {
  run({"CL100", "community", "CM1", "value", "65000:100"});
  ASSERT_EQ(lists()[0].members()->size(), 1);

  auto result = run({"CL100", "community", "CM2", "type", "MEDIUM"});
  EXPECT_THAT(result, HasSubstr("Invalid"));
  // Only the phantom member is rolled back; the existing one survives.
  ASSERT_EQ(lists()[0].members()->size(), 1);
  EXPECT_EQ(*member(0, 0).name(), "CM1");
}

} // namespace facebook::fboss
