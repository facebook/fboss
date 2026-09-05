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
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/test/config/CmdConfigTestBase.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

using namespace ::testing;

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
  EXPECT_TRUE(bareList.attr().empty());

  // List-level attribute.
  auto listAttr = BgpAsPathListConfig({"AS100", "description", "my", "list"});
  EXPECT_EQ(listAttr.attr(), "description");
  EXPECT_EQ(listAttr.values(), std::vector<std::string>({"my", "list"}));

  // Invalid: empty, empty name, unknown attribute.
  EXPECT_THROW(BgpAsPathListConfig({}), std::invalid_argument);
  EXPECT_THROW(BgpAsPathListConfig({""}), std::invalid_argument);
  EXPECT_THROW(
      BgpAsPathListConfig({"AS100", "no-such-attr", "1"}),
      std::invalid_argument);
  // asn-regexp is an entry attribute, not a list attribute.
  EXPECT_THROW(
      BgpAsPathListConfig({"AS100", "asn-regexp", "^65000_"}),
      std::invalid_argument);
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
// Reject paths — the error is surfaced and nothing is persisted
// ==============================================================================

TEST_F(CmdConfigBgpPolicyAsPathListTestFixture, reReferenceReportsExisting) {
  run({"AS100"});
  // A second bare reference must not claim to have created it again.
  EXPECT_THAT(run({"AS100"}), HasSubstr("already exists"));
  ASSERT_EQ(lists().size(), 1);
}

TEST_F(CmdConfigBgpPolicyAsPathListTestFixture, unknownAttributeRejected) {
  EXPECT_THROW(run({"AS100", "no-such-attr", "1"}), std::invalid_argument);
  // The rejected command must not leave a phantom list behind.
  EXPECT_TRUE(lists().empty());
  EXPECT_FALSE(sessionFileExists())
      << "session file should not exist after rejected input";
}

} // namespace facebook::fboss
