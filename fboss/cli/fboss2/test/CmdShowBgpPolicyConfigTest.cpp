// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <thrift/lib/cpp2/protocol/Serializer.h>
#include "configerator/structs/neteng/fboss/bgp/gen-cpp2/bgp_config_types.h"
#include "fboss/cli/fboss2/commands/show/bgp/CmdShowUtils.h"
#include "fboss/cli/fboss2/test/CmdHandlerTestBase.h"

using namespace ::testing;

namespace facebook::fboss {
namespace {

facebook::bgp::bgp_policy::BgpPolicies makePolicies(std::string name) {
  facebook::bgp::bgp_policy::BgpPolicyStatement statement;
  statement.name() = std::move(name);
  statement.policy_version() = "1";

  facebook::bgp::bgp_policy::BgpPolicies policies;
  policies.bgp_policy_statements()->push_back(std::move(statement));
  return policies;
}

std::string serializePolicyConfig(
    const facebook::bgp::bgp_policy::BgpPolicies& policies) {
  facebook::bgp::thrift::BgpPolicyConfig config;
  config.policies() = policies;
  return apache::thrift::SimpleJSONSerializer::serialize<std::string>(config);
}

class CmdShowBgpPolicyConfigTest : public CmdHandlerTestBase {};

TEST_F(CmdShowBgpPolicyConfigTest, PrefersStandalonePolicyConfig) {
  setupMockedBgpServer();
  EXPECT_CALL(getMockBgp(), getPolicyConfig(_))
      .WillOnce([](std::string& config) {
        config = serializePolicyConfig(makePolicies("standalone"));
      });
  EXPECT_CALL(getMockBgp(), getRunningConfigStruct(_)).Times(0);

  const auto policies = getRunningBgpPolicies(localhost());

  ASSERT_TRUE(policies.has_value());
  ASSERT_EQ(1, policies->bgp_policy_statements()->size());
  EXPECT_EQ("standalone", policies->bgp_policy_statements()->front().name());
}

TEST_F(CmdShowBgpPolicyConfigTest, FallsBackToRunningConfig) {
  setupMockedBgpServer();
  EXPECT_CALL(getMockBgp(), getPolicyConfig(_))
      .WillOnce([](std::string& config) { config.clear(); });
  EXPECT_CALL(getMockBgp(), getRunningConfigStruct(_))
      .WillOnce([](facebook::bgp::thrift::BgpConfig& config) {
        config.policies() = makePolicies("fallback");
      });

  const auto policies = getRunningBgpPolicies(localhost());

  ASSERT_TRUE(policies.has_value());
  ASSERT_EQ(1, policies->bgp_policy_statements()->size());
  EXPECT_EQ("fallback", policies->bgp_policy_statements()->front().name());
}

TEST_F(CmdShowBgpPolicyConfigTest, EmptyStandalonePolicyDoesNotFallBack) {
  setupMockedBgpServer();
  EXPECT_CALL(getMockBgp(), getPolicyConfig(_))
      .WillOnce([](std::string& config) {
        config = apache::thrift::SimpleJSONSerializer::serialize<std::string>(
            facebook::bgp::thrift::BgpPolicyConfig{});
      });
  EXPECT_CALL(getMockBgp(), getRunningConfigStruct(_)).Times(0);

  EXPECT_FALSE(getRunningBgpPolicies(localhost()).has_value());
}

} // namespace
} // namespace facebook::fboss
