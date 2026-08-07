// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/**
 * End-to-end tests for `fboss2-dev config protocol bgp policy community-list
 * <name> [community <name>] [<attribute> <value> ...]`.
 *
 * Scope: the community-list and its inline community members. Every test
 * stages the change AND commits it, then asserts the value landed at the
 * correct thrift field path inside the matching .policies.community_lists[]
 * entry of bgpd's running config (via getRunningConfig RPC) — which also
 * confirms bgpd accepts and adopts a `.policies` blob at all. Stage-only
 * behavior (attribute parsing, validation, rejection) is covered by the unit
 * tests; an integration test that never commits exercises no daemon.
 *
 * Requirements:
 *   - The fboss2-dev binary under test (config subcommand tree).
 *   - HOME is set (the session file lives under $HOME/.fboss2).
 *   - bgpd is installed/active (commit restarts it).
 */

#include <fmt/format.h>
#include <gtest/gtest.h>
#include <string>
#include <vector>

#include "fboss/cli/fboss2/test/integration_test/ConfigBgpTestBase.h"
#include "folly/json/dynamic.h"
#include "gmock/gmock.h"

using namespace facebook::fboss;
using ::testing::HasSubstr;
using ::testing::Not;

namespace {
// Test-only community-list / member names, unlikely to collide with a real
// list in the device's running BGP config.
const std::string kList = "FBOSS2-TEST-COMMUNITY";
const std::string kMember = "FBOSS2-TEST-CM1";
const std::string kMember2 = "FBOSS2-TEST-CM2";
} // namespace

class ConfigBgpPolicyCommunityListTest : public ConfigBgpTestBase {
 protected:
  // Stage `config protocol bgp policy community-list <tokens...>` WITHOUT
  // clearing the staged session, so attributes can accumulate across
  // invocations. Returns the staged session JSON.
  folly::dynamic stageCommunityList(const std::vector<std::string>& tokens) {
    std::vector<std::string> args = {
        "config", "protocol", "bgp", "policy", "community-list"};
    args.insert(args.end(), tokens.begin(), tokens.end());
    auto result = runCli(args);
    EXPECT_EQ(result.exitCode, 0)
        << "stdout=" << result.stdout << " stderr=" << result.stderr;
    EXPECT_THAT(result.stdout, Not(HasSubstr("Error:")));
    return readBgpSessionConfig();
  }

  // The .policies.community_lists[] entry named `name`, or nullptr.
  static const folly::dynamic* findList(
      const folly::dynamic& config,
      const std::string& name) {
    if (config.count("policies") == 0 ||
        config["policies"].count("community_lists") == 0) {
      return nullptr;
    }
    for (const auto& list : config["policies"]["community_lists"]) {
      if (list.count("name") && list["name"].asString() == name) {
        return &list;
      }
    }
    return nullptr;
  }

  // The inline .members[].community named `name` inside `list`, or nullptr.
  static const folly::dynamic* findMember(
      const folly::dynamic& list,
      const std::string& name) {
    if (list.count("members") == 0) {
      return nullptr;
    }
    for (const auto& member : list["members"]) {
      if (member.count("community") && member["community"].count("name") &&
          member["community"]["name"].asString() == name) {
        return &member["community"];
      }
    }
    return nullptr;
  }
};

TEST_F(ConfigBgpPolicyCommunityListTest, SetListAttributesAndCommit) {
  discardSession();
  clearBgpSession();
  stageCommunityList({kList, "description", "test", "spine", "communities"});
  stageCommunityList({kList, "boolean-operator", "AND"});
  stageCommunityList({kList, "exact-match", "true"});
  commitAndGetSha();
  ASSERT_TRUE(waitForBgpDaemonActive())
      << "bgpd did not return active after commit; state="
      << bgpDaemonActiveState();
  // Two layers: the promoted file proves what the commit wrote; the daemon's
  // getRunningConfig RPC proves bgpd parsed and adopted the .policies blob.
  auto config = readSystemBgpConfig();
  const auto* list = findList(config, kList);
  ASSERT_NE(list, nullptr) << "committed config has no community-list "
                           << kList;
  EXPECT_EQ((*list)["description"].asString(), "test spine communities");

  auto running = readRunningBgpConfigViaRpc();
  const auto* runningList = findList(running, kList);
  ASSERT_NE(runningList, nullptr)
      << "bgpd's running config has no community-list " << kList;
  EXPECT_EQ((*runningList)["description"].asString(), "test spine communities");
  // Enums ride the SimpleJSON wire format as integers:
  // routing_policy.BooleanOperator.AND = 1.
  EXPECT_EQ((*runningList)["boolean_operator"].asInt(), 1);
  EXPECT_TRUE((*runningList)["exact_match"].asBool());
}

TEST_F(ConfigBgpPolicyCommunityListTest, SetCommunityAttributesAndCommit) {
  discardSession();
  clearBgpSession();
  stageCommunityList({kList, "community", kMember, "value", "65000:100"});
  stageCommunityList({kList, "community", kMember, "type", "NORMAL"});
  stageCommunityList({kList, "community", kMember, "description", "test"});
  commitAndGetSha();
  ASSERT_TRUE(waitForBgpDaemonActive())
      << "bgpd did not return active after commit; state="
      << bgpDaemonActiveState();
  // Verify the nested member path .members[].community.{value,type,
  // description} through the daemon's own view of its config.
  auto running = readRunningBgpConfigViaRpc();
  const auto* list = findList(running, kList);
  ASSERT_NE(list, nullptr) << "bgpd's running config has no community-list "
                           << kList;
  const auto* community = findMember(*list, kMember);
  ASSERT_NE(community, nullptr)
      << "running config has no member community " << kMember;
  EXPECT_EQ((*community)["value"].asString(), "65000:100");
  // bgp_policy.CommunityType.NORMAL = 1 (integer on the SimpleJSON wire).
  EXPECT_EQ((*community)["type"].asInt(), 1);
  EXPECT_EQ((*community)["description"].asString(), "test");
}

TEST_F(ConfigBgpPolicyCommunityListTest, DeleteListAndCommit) {
  // Land a community-list in the system config, then delete it through a
  // second commit and verify it is gone from bgpd's running config.
  discardSession();
  clearBgpSession();
  stageCommunityList({kList, "community", kMember, "value", "65000:100"});
  commitAndGetSha();
  ASSERT_TRUE(waitForBgpDaemonActive())
      << "bgpd did not return active after commit; state="
      << bgpDaemonActiveState();
  ASSERT_NE(findList(readRunningBgpConfigViaRpc(), kList), nullptr)
      << "setup commit did not land the community-list in bgpd's running "
         "config";

  clearBgpSession();
  auto result =
      runCli({"delete", "protocol", "bgp", "policy", "community-list", kList});
  EXPECT_THAT(result.stdout, HasSubstr("Successfully deleted"));
  commitAndGetSha();
  ASSERT_TRUE(waitForBgpDaemonActive())
      << "bgpd did not return active after delete commit; state="
      << bgpDaemonActiveState();
  EXPECT_EQ(findList(readRunningBgpConfigViaRpc(), kList), nullptr)
      << "deleted community-list still present in bgpd's running config";
}

TEST_F(ConfigBgpPolicyCommunityListTest, DeleteCommunityAndCommit) {
  // Land a community-list with two inline members, delete one member through
  // a second commit, and verify the list and the other member survive in
  // bgpd's running config.
  discardSession();
  clearBgpSession();
  stageCommunityList({kList, "community", kMember, "value", "65000:100"});
  stageCommunityList({kList, "community", kMember2, "value", "65000:200"});
  commitAndGetSha();
  ASSERT_TRUE(waitForBgpDaemonActive())
      << "bgpd did not return active after commit; state="
      << bgpDaemonActiveState();
  {
    auto running = readRunningBgpConfigViaRpc();
    const auto* list = findList(running, kList);
    ASSERT_NE(list, nullptr)
        << "setup commit did not land the community-list in bgpd's running "
           "config";
    ASSERT_NE(findMember(*list, kMember), nullptr);
    ASSERT_NE(findMember(*list, kMember2), nullptr);
  }

  clearBgpSession();
  auto result = runCli(
      {"delete",
       "protocol",
       "bgp",
       "policy",
       "community-list",
       kList,
       "community",
       kMember});
  EXPECT_THAT(
      result.stdout,
      HasSubstr(
          fmt::format(
              "Successfully deleted BGP community-list {} community {}",
              kList,
              kMember)));
  commitAndGetSha();
  ASSERT_TRUE(waitForBgpDaemonActive())
      << "bgpd did not return active after delete commit; state="
      << bgpDaemonActiveState();
  auto running = readRunningBgpConfigViaRpc();
  const auto* list = findList(running, kList);
  ASSERT_NE(list, nullptr)
      << "deleting one member must not delete the community-list";
  EXPECT_EQ(findMember(*list, kMember), nullptr)
      << "deleted member still present in bgpd's running config";
  EXPECT_NE(findMember(*list, kMember2), nullptr)
      << "surviving member missing from bgpd's running config";
}
