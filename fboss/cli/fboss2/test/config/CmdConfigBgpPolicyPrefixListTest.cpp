// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <configerator/structs/neteng/fboss/bgp/gen-cpp2/bgp_config_types.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

#include "configerator/structs/neteng/bgp_policy/thrift/gen-cpp2/routing_policy_types.h"
#include "fboss/cli/fboss2/commands/config/protocol/bgp/policy/prefix-list/CmdConfigProtocolBgpPolicyPrefixList.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/test/config/CmdConfigTestBase.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

using namespace ::testing;
using facebook::bgp::routing_policy::BooleanOperator;
using facebook::bgp::routing_policy::ComparisonOperator;

namespace facebook::fboss {

// The prefix-list dispatcher only touches the BGP side of ConfigSession,
// which seeds from thrift schema defaults when neither a staged session nor a
// system bgpcpp.conf exists — so no seed agent config is needed (mirrors
// CmdConfigBgpPolicyCommunityListTest).
class CmdConfigBgpPolicyPrefixListTestFixture : public CmdConfigTestBase {
 public:
  CmdConfigBgpPolicyPrefixListTestFixture()
      : CmdConfigTestBase("bgp_prefix_list_test_%%%%-%%%%-%%%%", "") {}

  void SetUp() override {
    CmdConfigTestBase::SetUp();
    setupTestableConfigSession();
  }

  std::string run(const std::vector<std::string>& tokens) {
    CmdConfigProtocolBgpPolicyPrefixList cmd;
    HostInfo hostInfo("testhost");
    return cmd.queryClient(hostInfo, BgpPrefixListConfig(tokens));
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
// BgpPrefixListConfig (arg) validation
// ==============================================================================

TEST_F(CmdConfigBgpPolicyPrefixListTestFixture, argValidation) {
  // Bare create list.
  auto bareList = BgpPrefixListConfig({"PL100"});
  EXPECT_EQ(bareList.listName(), "PL100");
  EXPECT_TRUE(bareList.attr().empty());

  // List-level attribute.
  auto listAttr = BgpPrefixListConfig({"PL100", "description", "my", "list"});
  EXPECT_EQ(listAttr.attr(), "description");
  EXPECT_EQ(listAttr.values(), std::vector<std::string>({"my", "list"}));

  // Invalid: empty, empty name, unknown attribute.
  EXPECT_THROW(BgpPrefixListConfig({}), std::invalid_argument);
  EXPECT_THROW(BgpPrefixListConfig({""}), std::invalid_argument);
  EXPECT_THROW(
      BgpPrefixListConfig({"PL100", "no-such-attr", "1"}),
      std::invalid_argument);
  // base-prefix is an entry attribute, not a list attribute.
  EXPECT_THROW(
      BgpPrefixListConfig({"PL100", "base-prefix", "10.0.0.0/8"}),
      std::invalid_argument);
}

// ==============================================================================
// List-level handlers
// ==============================================================================

TEST_F(CmdConfigBgpPolicyPrefixListTestFixture, bareCreateList) {
  auto result = run({"PL100"});
  EXPECT_THAT(result, HasSubstr("Successfully created BGP prefix-list PL100"));
  ASSERT_EQ(lists().size(), 1);
  EXPECT_EQ(*lists()[0].name(), "PL100");
  EXPECT_TRUE(lists()[0].prefixes()->empty());
  EXPECT_TRUE(sessionFileExists());

  run({"PL200"});
  ASSERT_EQ(lists().size(), 2);
  EXPECT_EQ(*lists()[1].name(), "PL200");
}

TEST_F(CmdConfigBgpPolicyPrefixListTestFixture, setListDescription) {
  auto result = run({"PL100", "description", "spine", "prefixes"});
  EXPECT_THAT(result, HasSubstr("Successfully set description"));
  ASSERT_EQ(lists().size(), 1);
  // Multi-token description is re-joined.
  EXPECT_EQ(*lists()[0].description(), "spine prefixes");
}

TEST_F(CmdConfigBgpPolicyPrefixListTestFixture, setBooleanOperator) {
  auto result = run({"PL100", "boolean-operator", "AND"});
  EXPECT_THAT(result, HasSubstr("Successfully set boolean-operator"));
  EXPECT_EQ(*lists()[0].boolean_operator(), BooleanOperator::AND);

  run({"PL100", "boolean-operator", "NOT"});
  EXPECT_EQ(*lists()[0].boolean_operator(), BooleanOperator::NOT);
}

TEST_F(CmdConfigBgpPolicyPrefixListTestFixture, booleanOperatorDefaultIsOr) {
  run({"PL100"});
  EXPECT_EQ(*lists()[0].boolean_operator(), BooleanOperator::OR);
}

TEST_F(CmdConfigBgpPolicyPrefixListTestFixture, setCompareOperator) {
  auto result = run({"PL100", "compare-operator", "GE"});
  EXPECT_THAT(result, HasSubstr("Successfully set compare-operator"));
  ASSERT_TRUE(lists()[0].compare_operator().has_value());
  EXPECT_EQ(*lists()[0].compare_operator(), ComparisonOperator::GE);

  // RG is only documented for the entry-level prefix-len-range.
  auto rejected = run({"PL100", "compare-operator", "RG"});
  EXPECT_THAT(
      rejected,
      HasSubstr(
          "Invalid compare-operator value 'RG'; expected EQ|GE|LE|NE|GT|LT"));
  EXPECT_EQ(*lists()[0].compare_operator(), ComparisonOperator::GE);
}

TEST_F(CmdConfigBgpPolicyPrefixListTestFixture, setIpVersion) {
  auto result = run({"PL100", "ip-version", "v4"});
  EXPECT_THAT(result, HasSubstr("Successfully set ip-version"));
  ASSERT_TRUE(lists()[0].version().has_value());
  EXPECT_EQ(*lists()[0].version(), 4);

  run({"PL100", "ip-version", "v6"});
  EXPECT_EQ(*lists()[0].version(), 6);
}

TEST_F(CmdConfigBgpPolicyPrefixListTestFixture, namedListsAreDistinct) {
  run({"PL100", "description", "one"});
  run({"PL200", "description", "two"});
  ASSERT_EQ(lists().size(), 2);
  // Re-referencing an existing list by name updates it, not appends.
  run({"PL100", "description", "one-updated"});
  ASSERT_EQ(lists().size(), 2);
  EXPECT_EQ(*lists()[0].description(), "one-updated");
}

// ==============================================================================
// Entry-level handlers
// ==============================================================================

TEST_F(CmdConfigBgpPolicyPrefixListTestFixture, reReferenceReportsExisting) {
  run({"PL100"});
  // A second bare reference must not claim to have created it again.
  EXPECT_THAT(run({"PL100"}), HasSubstr("already exists"));
  ASSERT_EQ(lists().size(), 1);
}

// ==============================================================================
// Reject paths — the error is surfaced and nothing is persisted
// ==============================================================================

TEST_F(
    CmdConfigBgpPolicyPrefixListTestFixture,
    invalidBooleanOperatorRejected) {
  auto result = run({"PL100", "boolean-operator", "XOR"});
  EXPECT_THAT(
      result,
      HasSubstr("Invalid boolean-operator value 'XOR'; expected AND|OR|NOT"));
  // The rejected value must not leave a phantom list behind.
  EXPECT_TRUE(lists().empty());
  EXPECT_FALSE(sessionFileExists())
      << "session file should not exist after rejected input";
}

TEST_F(CmdConfigBgpPolicyPrefixListTestFixture, invalidIpVersionRejected) {
  auto result = run({"PL100", "ip-version", "v5"});
  EXPECT_THAT(
      result, HasSubstr("Invalid ip-version value 'v5'; expected v4|v6"));
  EXPECT_TRUE(lists().empty());
  EXPECT_FALSE(sessionFileExists())
      << "session file should not exist after rejected input";
}

} // namespace facebook::fboss
