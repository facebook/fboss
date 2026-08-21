// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <configerator/structs/neteng/fboss/bgp/gen-cpp2/bgp_config_types.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

#include "configerator/structs/neteng/bgp_policy/thrift/gen-cpp2/bgp_policy_types.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/policy/community-list/BgpCommunityListCliUtils.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/policy/community-list/CmdConfigProtocolBgpPolicyCommunityList.h"
#include "fboss/cli/fboss2/commands/delete/protocol/bgp/policy/community-list/CmdDeleteProtocolBgpPolicyCommunityList.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/test/config/CmdConfigTestBase.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

using namespace ::testing;

namespace facebook::fboss {

// Deleting only touches the BGP side of ConfigSession, which seeds from thrift
// schema defaults when neither a staged session nor a system bgpcpp.conf
// exists — so no seed agent config is needed (mirrors
// CmdDeleteBgpPolicyAsPathListTest).
class CmdDeleteBgpPolicyCommunityListTestFixture : public CmdConfigTestBase {
 public:
  CmdDeleteBgpPolicyCommunityListTestFixture()
      : CmdConfigTestBase("bgp_community_list_delete_test_%%%%-%%%%-%%%%", "") {
  }

  void SetUp() override {
    CmdConfigTestBase::SetUp();
    setupTestableConfigSession();
  }

  std::string configure(const std::vector<std::string>& tokens) {
    CmdConfigProtocolBgpPolicyCommunityList cmd;
    HostInfo hostInfo("testhost");
    return cmd.queryClient(hostInfo, BgpCommunityListConfig(tokens));
  }

  // Seed an inline Community member. The member level is its own subcommand
  // (CmdConfigProtocolBgpPolicyCommunityListCommunity), which lands above this
  // command in the stack — so stage it through the shared helpers rather than
  // depending on that handler.
  void configureCommunity(
      const std::string& listName,
      const std::string& communityName,
      const std::string& value) {
    auto& session = ConfigSession::getInstance();
    auto& list =
        bgpcli::findOrCreateCommunityList(session.getBgpConfig(), listName);
    bgpcli::findOrCreateCommunityMember(list, communityName).value() = value;
    session.saveBgpConfig();
  }

  std::string del(const std::vector<std::string>& tokens) {
    CmdDeleteProtocolBgpPolicyCommunityList cmd;
    HostInfo hostInfo("testhost");
    return cmd.queryClient(hostInfo, BgpCommunityListRef(tokens));
  }

  const std::vector<bgp::bgp_policy::CommunityList>& lists() {
    return *ConfigSession::getInstance()
                .getBgpConfig()
                .policies()
                .ensure()
                .community_lists();
  }

  bool sessionFileExists() {
    return std::filesystem::exists(
        ConfigSession::getInstance().getBgpSessionConfigPath());
  }
};

// ==============================================================================
// BgpCommunityListRef (arg) validation
// ==============================================================================

TEST_F(CmdDeleteBgpPolicyCommunityListTestFixture, argValidation) {
  auto listOnly = BgpCommunityListRef({"CL100"});
  EXPECT_EQ(listOnly.listName(), "CL100");
  EXPECT_FALSE(listOnly.hasCommunity());

  auto withMember = BgpCommunityListRef({"CL100", "community", "CM1"});
  EXPECT_EQ(withMember.listName(), "CL100");
  EXPECT_TRUE(withMember.hasCommunity());
  EXPECT_EQ(withMember.communityName(), "CM1");

  // Invalid: empty, empty names, non-`community` second token, missing
  // community name, extra tokens.
  EXPECT_THROW(BgpCommunityListRef({}), std::invalid_argument);
  EXPECT_THROW(BgpCommunityListRef({""}), std::invalid_argument);
  EXPECT_THROW(BgpCommunityListRef({"CL100", "CL200"}), std::invalid_argument);
  EXPECT_THROW(
      BgpCommunityListRef({"CL100", "community"}), std::invalid_argument);
  EXPECT_THROW(
      BgpCommunityListRef({"CL100", "community", ""}), std::invalid_argument);
  EXPECT_THROW(
      BgpCommunityListRef({"CL100", "community", "CM1", "extra"}),
      std::invalid_argument);
}

// ==============================================================================
// queryClient
// ==============================================================================

TEST_F(CmdDeleteBgpPolicyCommunityListTestFixture, deleteExistingList) {
  configureCommunity("CL100", "CM1", "65000:100");
  configure({"CL200", "description", "keep"});
  ASSERT_EQ(lists().size(), 2);

  auto result = del({"CL100"});
  EXPECT_THAT(
      result, HasSubstr("Successfully deleted BGP community-list CL100"));
  // The other list survives untouched.
  ASSERT_EQ(lists().size(), 1);
  EXPECT_EQ(*lists()[0].name(), "CL200");
  EXPECT_EQ(*lists()[0].description(), "keep");
  EXPECT_TRUE(sessionFileExists());
}

TEST_F(CmdDeleteBgpPolicyCommunityListTestFixture, deleteUnknownListRejected) {
  auto result = del({"NO-SUCH-LIST"});
  EXPECT_THAT(
      result, HasSubstr("Error: BGP community-list NO-SUCH-LIST not found"));
  // Nothing was persisted for the failed delete.
  EXPECT_FALSE(sessionFileExists())
      << "session file should not exist after rejected delete";
}

TEST_F(
    CmdDeleteBgpPolicyCommunityListTestFixture,
    deleteUnknownLeavesOthersIntact) {
  configure({"CL100", "description", "one"});
  auto result = del({"CL200"});
  EXPECT_THAT(result, HasSubstr("not found"));
  ASSERT_EQ(lists().size(), 1);
  EXPECT_EQ(*lists()[0].name(), "CL100");
}

// ==============================================================================
// queryClient: single community member deletion
// ==============================================================================

TEST_F(CmdDeleteBgpPolicyCommunityListTestFixture, deleteExistingCommunity) {
  configureCommunity("CL100", "CM1", "65000:100");
  configureCommunity("CL100", "CM2", "65000:200");

  auto result = del({"CL100", "community", "CM1"});
  EXPECT_THAT(
      result,
      HasSubstr("Successfully deleted BGP community-list CL100 community CM1"));
  // The list and its other member survive.
  ASSERT_EQ(lists().size(), 1);
  ASSERT_TRUE(lists()[0].members().has_value());
  ASSERT_EQ(lists()[0].members()->size(), 1);
  EXPECT_EQ(*(*lists()[0].members())[0].community_ref()->name(), "CM2");
  EXPECT_TRUE(sessionFileExists());
}

TEST_F(
    CmdDeleteBgpPolicyCommunityListTestFixture,
    deleteLastCommunityUnsetsMembers) {
  configureCommunity("CL100", "CM1", "65000:100");

  auto result = del({"CL100", "community", "CM1"});
  EXPECT_THAT(result, HasSubstr("Successfully deleted"));
  // The list stays, shaped like one that never had members.
  ASSERT_EQ(lists().size(), 1);
  EXPECT_EQ(*lists()[0].name(), "CL100");
  EXPECT_FALSE(lists()[0].members().has_value());
}

TEST_F(
    CmdDeleteBgpPolicyCommunityListTestFixture,
    deleteUnknownCommunityRejected) {
  configureCommunity("CL100", "CM1", "65000:100");
  // Consume the session file created by configure so we can assert the
  // rejected delete does not persist anything new.
  ASSERT_TRUE(sessionFileExists());

  auto result = del({"CL100", "community", "NO-SUCH-COMMUNITY"});
  EXPECT_THAT(
      result,
      HasSubstr(
          "Error: BGP community-list CL100 community NO-SUCH-COMMUNITY not "
          "found"));
  // The existing member is untouched.
  ASSERT_EQ(lists().size(), 1);
  ASSERT_TRUE(lists()[0].members().has_value());
  ASSERT_EQ(lists()[0].members()->size(), 1);
  EXPECT_EQ(*(*lists()[0].members())[0].community_ref()->name(), "CM1");
}

TEST_F(
    CmdDeleteBgpPolicyCommunityListTestFixture,
    deleteCommunityFromMemberlessListRejected) {
  configure({"CL100", "description", "no", "members"});

  auto result = del({"CL100", "community", "CM1"});
  EXPECT_THAT(
      result,
      HasSubstr("Error: BGP community-list CL100 community CM1 not found"));
  ASSERT_EQ(lists().size(), 1);
  EXPECT_FALSE(lists()[0].members().has_value());
}

TEST_F(
    CmdDeleteBgpPolicyCommunityListTestFixture,
    deleteCommunityFromUnknownListRejected) {
  auto result = del({"NO-SUCH-LIST", "community", "CM1"});
  EXPECT_THAT(
      result, HasSubstr("Error: BGP community-list NO-SUCH-LIST not found"));
  EXPECT_FALSE(sessionFileExists())
      << "session file should not exist after rejected delete";
}

} // namespace facebook::fboss
