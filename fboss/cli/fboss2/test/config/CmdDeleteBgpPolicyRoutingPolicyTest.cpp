// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <configerator/structs/neteng/fboss/bgp/gen-cpp2/bgp_config_types.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

#include "configerator/structs/neteng/bgp_policy/thrift/gen-cpp2/bgp_policy_types.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/policy/routing-policy/CmdConfigProtocolBgpPolicyRoutingPolicy.h"
#include "fboss/cli/fboss2/commands/delete/protocol/bgp/policy/routing-policy/CmdDeleteProtocolBgpPolicyRoutingPolicy.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/test/config/CmdConfigTestBase.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

using namespace ::testing;

namespace facebook::fboss {

// Deleting only touches the BGP side of ConfigSession, which seeds from thrift
// schema defaults when neither a staged session nor a system bgpcpp.conf
// exists — so no seed agent config is needed (mirrors
// CmdDeleteBgpPolicyCommunityListTest).
class CmdDeleteBgpPolicyRoutingPolicyTestFixture : public CmdConfigTestBase {
 public:
  CmdDeleteBgpPolicyRoutingPolicyTestFixture()
      : CmdConfigTestBase("bgp_routing_policy_delete_test_%%%%-%%%%-%%%%", "") {
  }

  void SetUp() override {
    CmdConfigTestBase::SetUp();
    setupTestableConfigSession();
  }

  std::string configure(const std::vector<std::string>& tokens) {
    CmdConfigProtocolBgpPolicyRoutingPolicy cmd;
    HostInfo hostInfo("testhost");
    return cmd.queryClient(hostInfo, BgpRoutingPolicyConfig(tokens));
  }

  std::string del(const std::vector<std::string>& tokens) {
    CmdDeleteProtocolBgpPolicyRoutingPolicy cmd;
    HostInfo hostInfo("testhost");
    return cmd.queryClient(hostInfo, BgpRoutingPolicyRef(tokens));
  }

  const std::vector<bgp::bgp_policy::BgpPolicyStatement>& policies() {
    return *ConfigSession::getInstance()
                .getBgpConfig()
                .policies()
                .ensure()
                .bgp_policy_statements();
  }

  bool sessionFileExists() {
    return std::filesystem::exists(
        ConfigSession::getInstance().getBgpSessionConfigPath());
  }
};

// ==============================================================================
// BgpRoutingPolicyRef (arg) validation
// ==============================================================================

TEST_F(CmdDeleteBgpPolicyRoutingPolicyTestFixture, argValidation) {
  auto ref = BgpRoutingPolicyRef({"RM100"});
  EXPECT_EQ(ref.policyName(), "RM100");

  // Invalid: empty, empty name, extra tokens.
  EXPECT_THROW(BgpRoutingPolicyRef({}), std::invalid_argument);
  EXPECT_THROW(BgpRoutingPolicyRef({""}), std::invalid_argument);
  EXPECT_THROW(BgpRoutingPolicyRef({"RM100", "RM200"}), std::invalid_argument);
  EXPECT_THROW(
      BgpRoutingPolicyRef({"RM100", "description"}), std::invalid_argument);
}

// ==============================================================================
// queryClient
// ==============================================================================

TEST_F(CmdDeleteBgpPolicyRoutingPolicyTestFixture, deleteExistingPolicy) {
  configure({"RM100", "description", "delete-me"});
  configure({"RM200", "description", "keep"});
  ASSERT_EQ(policies().size(), 2);

  auto result = del({"RM100"});
  EXPECT_THAT(
      result, HasSubstr("Successfully deleted BGP routing-policy RM100"));
  // The other policy survives untouched.
  ASSERT_EQ(policies().size(), 1);
  EXPECT_EQ(*policies()[0].name(), "RM200");
  EXPECT_EQ(*policies()[0].description(), "keep");
  EXPECT_TRUE(sessionFileExists());
}

TEST_F(
    CmdDeleteBgpPolicyRoutingPolicyTestFixture,
    deleteUnknownPolicyRejected) {
  auto result = del({"NO-SUCH-POLICY"});
  EXPECT_THAT(
      result, HasSubstr("Error: BGP routing-policy NO-SUCH-POLICY not found"));
  // Nothing was persisted for the failed delete.
  EXPECT_FALSE(sessionFileExists())
      << "session file should not exist after rejected delete";
}

TEST_F(
    CmdDeleteBgpPolicyRoutingPolicyTestFixture,
    deleteUnknownLeavesOthersIntact) {
  configure({"RM100", "description", "one"});
  auto result = del({"RM200"});
  EXPECT_THAT(result, HasSubstr("not found"));
  ASSERT_EQ(policies().size(), 1);
  EXPECT_EQ(*policies()[0].name(), "RM100");
}

} // namespace facebook::fboss
