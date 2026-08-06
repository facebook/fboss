// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <configerator/structs/neteng/fboss/bgp/gen-cpp2/bgp_config_types.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

#include "configerator/structs/neteng/bgp_policy/thrift/gen-cpp2/routing_policy_types.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/policy/prefix-list/BgpPrefixListCliUtils.h"
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

  // Seed an entry. The entry level is its own subcommand
  // (CmdConfigProtocolBgpPolicyPrefixListEntry), which lands above this
  // command in the stack — so stage it through the shared helpers rather than
  // depending on that handler.
  void configureEntry(
      const std::string& listName,
      int32_t seqNum,
      const std::string& basePrefix) {
    auto& session = ConfigSession::getInstance();
    auto& list =
        bgpcli::findOrCreatePrefixList(session.getBgpConfig(), listName);
    bgpcli::findOrCreatePrefixListEntry(list, seqNum).base_prefix() =
        basePrefix;
    session.saveBgpConfig();
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
  EXPECT_FALSE(listOnly.hasEntry());

  auto withEntry = BgpPrefixListRef({"PL100", "entry", "10"});
  EXPECT_EQ(withEntry.listName(), "PL100");
  EXPECT_TRUE(withEntry.hasEntry());
  EXPECT_EQ(withEntry.seqNum(), 10);

  // Invalid: empty, empty name, non-`entry` second token, missing seq-num,
  // non-integer and negative seq-nums, extra tokens.
  EXPECT_THROW(BgpPrefixListRef({}), std::invalid_argument);
  EXPECT_THROW(BgpPrefixListRef({""}), std::invalid_argument);
  EXPECT_THROW(BgpPrefixListRef({"PL100", "PL200"}), std::invalid_argument);
  EXPECT_THROW(BgpPrefixListRef({"PL100", "entry"}), std::invalid_argument);
  EXPECT_THROW(
      BgpPrefixListRef({"PL100", "entry", "ten"}), std::invalid_argument);
  EXPECT_THROW(
      BgpPrefixListRef({"PL100", "entry", "-1"}), std::invalid_argument);
  EXPECT_THROW(
      BgpPrefixListRef({"PL100", "entry", "10", "extra"}),
      std::invalid_argument);
}

// ==============================================================================
// queryClient: whole-list deletion
// ==============================================================================

TEST_F(CmdDeleteBgpPolicyPrefixListTestFixture, deleteExistingList) {
  configureEntry("PL100", 10, "10.0.0.0/8");
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

TEST_F(CmdDeleteBgpPolicyPrefixListTestFixture, deleteUnknownListRejected) {
  auto result = del({"NO-SUCH-LIST"});
  EXPECT_THAT(
      result, HasSubstr("Error: BGP prefix-list NO-SUCH-LIST not found"));
  // Nothing was persisted for the failed delete.
  EXPECT_FALSE(sessionFileExists())
      << "session file should not exist after rejected delete";
}

TEST_F(
    CmdDeleteBgpPolicyPrefixListTestFixture,
    deleteUnknownLeavesOthersIntact) {
  configure({"PL100", "description", "one"});
  auto result = del({"PL200"});
  EXPECT_THAT(result, HasSubstr("not found"));
  ASSERT_EQ(lists().size(), 1);
  EXPECT_EQ(*lists()[0].name(), "PL100");
}

// ==============================================================================
// queryClient: single entry deletion
// ==============================================================================

TEST_F(CmdDeleteBgpPolicyPrefixListTestFixture, deleteExistingEntry) {
  configureEntry("PL100", 10, "10.0.0.0/8");
  configureEntry("PL100", 20, "192.168.0.0/16");

  auto result = del({"PL100", "entry", "10"});
  EXPECT_THAT(
      result, HasSubstr("Successfully deleted BGP prefix-list PL100 entry 10"));
  // The list and its other entry survive.
  ASSERT_EQ(lists().size(), 1);
  ASSERT_EQ(lists()[0].prefixes()->size(), 1);
  EXPECT_EQ(*(*lists()[0].prefixes())[0].seq_num(), 20);
  EXPECT_TRUE(sessionFileExists());
}

TEST_F(CmdDeleteBgpPolicyPrefixListTestFixture, deleteLastEntryKeepsList) {
  configureEntry("PL100", 10, "10.0.0.0/8");

  auto result = del({"PL100", "entry", "10"});
  EXPECT_THAT(result, HasSubstr("Successfully deleted"));
  // The list stays, shaped like one that never had entries (prefixes is a
  // non-optional list, so empty — not unset — is that shape).
  ASSERT_EQ(lists().size(), 1);
  EXPECT_EQ(*lists()[0].name(), "PL100");
  EXPECT_TRUE(lists()[0].prefixes()->empty());
}

TEST_F(CmdDeleteBgpPolicyPrefixListTestFixture, deleteUnknownEntryRejected) {
  configureEntry("PL100", 10, "10.0.0.0/8");
  // Remove the session file created by configure so its absence afterwards
  // proves the rejected delete did not persist anything new.
  ASSERT_TRUE(sessionFileExists());
  std::filesystem::remove(
      ConfigSession::getInstance().getBgpSessionConfigPath());

  auto result = del({"PL100", "entry", "99"});
  EXPECT_THAT(
      result, HasSubstr("Error: BGP prefix-list PL100 entry 99 not found"));
  EXPECT_FALSE(sessionFileExists())
      << "rejected delete must not persist a session file";
  // The existing entry is untouched.
  ASSERT_EQ(lists().size(), 1);
  ASSERT_EQ(lists()[0].prefixes()->size(), 1);
  EXPECT_EQ(*(*lists()[0].prefixes())[0].seq_num(), 10);
}

TEST_F(
    CmdDeleteBgpPolicyPrefixListTestFixture,
    deleteEntryFromUnknownListRejected) {
  auto result = del({"NO-SUCH-LIST", "entry", "10"});
  EXPECT_THAT(
      result, HasSubstr("Error: BGP prefix-list NO-SUCH-LIST not found"));
  EXPECT_FALSE(sessionFileExists())
      << "session file should not exist after rejected delete";
}

} // namespace facebook::fboss
