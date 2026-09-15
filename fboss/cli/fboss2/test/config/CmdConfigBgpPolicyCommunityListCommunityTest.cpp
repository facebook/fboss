// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <configerator/structs/neteng/fboss/bgp/gen-cpp2/bgp_config_types.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

#include "configerator/structs/neteng/bgp_policy/thrift/gen-cpp2/bgp_policy_types.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/policy/community-list/CmdConfigProtocolBgpPolicyCommunityList.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/policy/community-list/community/CmdConfigProtocolBgpPolicyCommunityListCommunity.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/test/config/CmdConfigTestBase.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

using namespace ::testing;

namespace facebook::fboss {

// The community dispatcher only touches the BGP side of ConfigSession, which
// seeds from thrift schema defaults when neither a staged session nor a system
// bgpcpp.conf exists — so no seed agent config is needed (mirrors
// CmdConfigBgpPolicyCommunityListTest).
class CmdConfigBgpPolicyCommunityListCommunityTestFixture
    : public CmdConfigTestBase {
 public:
  CmdConfigBgpPolicyCommunityListCommunityTestFixture()
      : CmdConfigTestBase("bgp_community_list_community_test_%%%%-%%%%", "") {}

  void SetUp() override {
    CmdConfigTestBase::SetUp();
    setupTestableConfigSession();
  }

  // Invoke the community handler the way the framework does: the parent's
  // parsed args plus the community's own tokens.
  std::string runCommunity(
      const std::vector<std::string>& listTokens,
      const std::vector<std::string>& communityTokens) {
    CmdConfigProtocolBgpPolicyCommunityListCommunity cmd;
    HostInfo hostInfo("testhost");
    return cmd.queryClient(
        hostInfo,
        BgpCommunityListConfig(listTokens),
        BgpCommunityListCommunityConfig(communityTokens));
  }

  std::string runList(const std::vector<std::string>& tokens) {
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

  // The inline Community held by members[idx] of list[listIdx].
  const bgp::bgp_policy::Community& member(size_t listIdx, size_t idx) {
    return *lists()[listIdx].members()->at(idx).community_ref();
  }

  bool sessionFileExists() {
    return std::filesystem::exists(
        ConfigSession::getInstance().getBgpSessionConfigPath());
  }
};

// ==============================================================================
// BgpCommunityListCommunityConfig (arg) validation
// ==============================================================================

TEST_F(CmdConfigBgpPolicyCommunityListCommunityTestFixture, argValidation) {
  // Bare create.
  auto bare = BgpCommunityListCommunityConfig({"CM1"});
  EXPECT_EQ(bare.communityName(), "CM1");
  EXPECT_TRUE(bare.attr().empty());

  // Attribute with values.
  auto attr = BgpCommunityListCommunityConfig({"CM1", "value", "65000:100"});
  EXPECT_EQ(attr.communityName(), "CM1");
  EXPECT_EQ(attr.attr(), "value");
  EXPECT_EQ(attr.values(), std::vector<std::string>({"65000:100"}));

  // Invalid: empty, empty name, unknown attribute.
  EXPECT_THROW(BgpCommunityListCommunityConfig({}), std::invalid_argument);
  EXPECT_THROW(BgpCommunityListCommunityConfig({""}), std::invalid_argument);
  EXPECT_THROW(
      BgpCommunityListCommunityConfig({"CM1", "no-such-attr", "1"}),
      std::invalid_argument);
  // exact-match is a list attribute, not a community attribute.
  EXPECT_THROW(
      BgpCommunityListCommunityConfig({"CM1", "exact-match", "true"}),
      std::invalid_argument);
}

// ==============================================================================
// Community-level handlers
// ==============================================================================

TEST_F(
    CmdConfigBgpPolicyCommunityListCommunityTestFixture,
    bareCreateCommunity) {
  auto result = runCommunity({"CL100"}, {"CM1"});
  EXPECT_THAT(
      result,
      HasSubstr("Successfully created BGP community-list CL100 community CM1"));
  ASSERT_EQ(lists().size(), 1);
  ASSERT_EQ(lists()[0].members()->size(), 1);
  EXPECT_EQ(*member(0, 0).name(), "CM1");
}

TEST_F(
    CmdConfigBgpPolicyCommunityListCommunityTestFixture,
    setCommunityAttributes) {
  runCommunity({"CL100"}, {"CM1", "value", "65000:100"});
  runCommunity({"CL100"}, {"CM1", "type", "LARGE"});
  runCommunity({"CL100"}, {"CM1", "description", "spine", "tag"});

  ASSERT_EQ(lists().size(), 1);
  ASSERT_EQ(lists()[0].members()->size(), 1);
  const auto& community = member(0, 0);
  EXPECT_EQ(*community.name(), "CM1");
  EXPECT_EQ(*community.value(), "65000:100");
  EXPECT_EQ(*community.type(), bgp::bgp_policy::CommunityType::LARGE);
  EXPECT_EQ(*community.description(), "spine tag");
}

TEST_F(
    CmdConfigBgpPolicyCommunityListCommunityTestFixture,
    communitiesAccumulateAndAreKeyed) {
  runCommunity({"CL100"}, {"CM1", "value", "65000:100"});
  runCommunity({"CL100"}, {"CM2", "value", "65000:200"});
  ASSERT_EQ(lists().size(), 1);
  ASSERT_EQ(lists()[0].members()->size(), 2);

  // Re-referencing an existing member by name updates it, not appends.
  runCommunity({"CL100"}, {"CM1", "type", "NORMAL"});
  EXPECT_EQ(lists()[0].members()->size(), 2);
  EXPECT_EQ(*member(0, 0).value(), "65000:100");
  EXPECT_EQ(*member(0, 0).type(), bgp::bgp_policy::CommunityType::NORMAL);
}

TEST_F(
    CmdConfigBgpPolicyCommunityListCommunityTestFixture,
    reReferenceReportsExisting) {
  runCommunity({"CL100"}, {"CM1"});
  EXPECT_THAT(
      runCommunity({"CL100"}, {"CM1"}),
      HasSubstr("community CM1 already exists"));
  EXPECT_EQ(lists()[0].members()->size(), 1);
}

// ==============================================================================
// Reject paths — the error is surfaced and nothing is persisted
// ==============================================================================

TEST_F(
    CmdConfigBgpPolicyCommunityListCommunityTestFixture,
    invalidCommunityTypeRejected) {
  auto result = runCommunity({"CL100"}, {"CM1", "type", "MEDIUM"});
  // The full message matters: the accepted-values suffix once printed garbage
  // (a string_view capture dangling over a fmt::format temporary).
  EXPECT_THAT(
      result,
      HasSubstr("Invalid type value 'MEDIUM'; expected NORMAL|EXTENDED|LARGE"));
  EXPECT_TRUE(lists().empty());
  EXPECT_FALSE(sessionFileExists())
      << "session file should not exist after rejected input";
}

TEST_F(
    CmdConfigBgpPolicyCommunityListCommunityTestFixture,
    rejectedCommunityOnExistingListKeepsList) {
  // Create the list first, then reject a member value on it.
  runList({"CL100", "description", "keep-me"});
  ASSERT_EQ(lists().size(), 1);

  auto result = runCommunity({"CL100"}, {"CM1", "type", "MEDIUM"});
  EXPECT_THAT(result, HasSubstr("Invalid"));
  // The pre-existing list survives; only the phantom member is rolled back —
  // including the members field itself, which was unset before.
  ASSERT_EQ(lists().size(), 1);
  EXPECT_FALSE(lists()[0].members().has_value());
  EXPECT_EQ(*lists()[0].description(), "keep-me");
}

TEST_F(
    CmdConfigBgpPolicyCommunityListCommunityTestFixture,
    rejectedCommunityKeepsExistingMembers) {
  runCommunity({"CL100"}, {"CM1", "value", "65000:100"});
  ASSERT_EQ(lists()[0].members()->size(), 1);

  auto result = runCommunity({"CL100"}, {"CM2", "type", "MEDIUM"});
  EXPECT_THAT(result, HasSubstr("Invalid"));
  // Only the phantom member is rolled back; the existing one survives.
  ASSERT_EQ(lists()[0].members()->size(), 1);
  EXPECT_EQ(*member(0, 0).name(), "CM1");
}

// A list-level attribute alongside a community must be rejected: only the leaf
// command runs, so the list attribute would be silently dropped.
TEST_F(
    CmdConfigBgpPolicyCommunityListCommunityTestFixture,
    listAttributeMixedWithCommunityRejected) {
  auto result = runCommunity({"CL100", "description", "mixed"}, {"CM1"});
  EXPECT_THAT(result, HasSubstr("separate commands"));
  EXPECT_TRUE(lists().empty());
  EXPECT_FALSE(sessionFileExists());
}

} // namespace facebook::fboss
