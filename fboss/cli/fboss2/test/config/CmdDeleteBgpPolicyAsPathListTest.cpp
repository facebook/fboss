// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <configerator/structs/neteng/fboss/bgp/gen-cpp2/bgp_config_types.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

#include "configerator/structs/neteng/bgp_policy/thrift/gen-cpp2/bgp_policy_types.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/policy/as-path-list/CmdConfigProtocolBgpPolicyAsPathList.h"
#include "fboss/cli/fboss2/commands/delete/protocol/bgp/policy/as-path-list/CmdDeleteProtocolBgpPolicyAsPathList.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/test/config/CmdConfigTestBase.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

using namespace ::testing;

namespace facebook::fboss {

// Deleting only touches the BGP side of ConfigSession, which seeds from thrift
// schema defaults when neither a staged session nor a system bgpcpp.conf
// exists — so no seed agent config is needed (mirrors
// CmdConfigBgpPeerGroupTest).
class CmdDeleteBgpPolicyAsPathListTestFixture : public CmdConfigTestBase {
 public:
  CmdDeleteBgpPolicyAsPathListTestFixture()
      : CmdConfigTestBase("bgp_aspath_list_delete_test_%%%%-%%%%-%%%%", "") {}

  void SetUp() override {
    CmdConfigTestBase::SetUp();
    setupTestableConfigSession();
  }

  std::string configure(const std::vector<std::string>& tokens) {
    CmdConfigProtocolBgpPolicyAsPathList cmd;
    HostInfo hostInfo("testhost");
    return cmd.queryClient(hostInfo, BgpAsPathListConfig(tokens));
  }

  std::string del(const std::vector<std::string>& tokens) {
    CmdDeleteProtocolBgpPolicyAsPathList cmd;
    HostInfo hostInfo("testhost");
    return cmd.queryClient(hostInfo, BgpAsPathListRef(tokens));
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
// BgpAsPathListRef (arg) validation
// ==============================================================================

TEST_F(CmdDeleteBgpPolicyAsPathListTestFixture, argValidation) {
  EXPECT_EQ(BgpAsPathListRef({"AS100"}).listName(), "AS100");

  // Invalid: empty, empty name, extra tokens.
  EXPECT_THROW(BgpAsPathListRef({}), std::invalid_argument);
  EXPECT_THROW(BgpAsPathListRef({""}), std::invalid_argument);
  EXPECT_THROW(BgpAsPathListRef({"AS100", "AS200"}), std::invalid_argument);
}

// ==============================================================================
// queryClient
// ==============================================================================

TEST_F(CmdDeleteBgpPolicyAsPathListTestFixture, deleteExistingList) {
  configure({"AS100", "entry", "10", "asn-regexp", "^65000_"});
  configure({"AS200", "description", "keep"});
  ASSERT_EQ(lists().size(), 2);

  auto result = del({"AS100"});
  EXPECT_THAT(result, HasSubstr("Successfully deleted BGP as-path-list AS100"));
  // The other list survives untouched.
  ASSERT_EQ(lists().size(), 1);
  EXPECT_EQ(*lists()[0].name(), "AS200");
  EXPECT_EQ(*lists()[0].description(), "keep");
  EXPECT_TRUE(sessionFileExists());
}

TEST_F(CmdDeleteBgpPolicyAsPathListTestFixture, deleteUnknownListRejected) {
  auto result = del({"NO-SUCH-LIST"});
  EXPECT_THAT(
      result, HasSubstr("Error: BGP as-path-list NO-SUCH-LIST not found"));
  // Nothing was persisted for the failed delete.
  EXPECT_FALSE(sessionFileExists())
      << "session file should not exist after rejected delete";
}

TEST_F(
    CmdDeleteBgpPolicyAsPathListTestFixture,
    deleteUnknownLeavesOthersIntact) {
  configure({"AS100", "description", "one"});
  auto result = del({"AS200"});
  EXPECT_THAT(result, HasSubstr("not found"));
  ASSERT_EQ(lists().size(), 1);
  EXPECT_EQ(*lists()[0].name(), "AS100");
}

} // namespace facebook::fboss
