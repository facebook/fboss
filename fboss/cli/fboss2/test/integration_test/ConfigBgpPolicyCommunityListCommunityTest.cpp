// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/**
 * End-to-end tests for `fboss2-dev config protocol bgp policy community-list
 * <name> community <name> [<attribute> <value> ...]`.
 *
 * Scope: the inline community-member level. Every test stages the change AND
 * commits it, then asserts the value landed at the correct thrift field path
 * inside the matching .policies.community_lists[].members[] entry of bgpd's
 * running config (via getRunningConfig RPC). Stage-only behavior (attribute
 * parsing, validation, rejection) is covered by the unit tests; an integration
 * test that never commits exercises no daemon.
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
const std::string kList = "FBOSS2-TEST-COMMUNITY-MEMBER";
const std::string kMember = "FBOSS2-TEST-CM1";
const std::string kMember2 = "FBOSS2-TEST-CM2";
} // namespace

class ConfigBgpPolicyCommunityListCommunityTest : public ConfigBgpTestBase {
 protected:
  // Stage `config protocol bgp policy community-list <list> community
  // <tokens...>` WITHOUT clearing the staged session, so attributes can
  // accumulate across invocations. Returns the staged session JSON.
  folly::dynamic stageCommunity(const std::vector<std::string>& tokens) {
    std::vector<std::string> args = {
        "config",
        "protocol",
        "bgp",
        "policy",
        "community-list",
        kList,
        "community"};
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

  // The inline Community held by a .members[] entry named `name`, or nullptr.
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

TEST_F(ConfigBgpPolicyCommunityListCommunityTest, SetAttributesAndCommit) {
  discardSession();
  clearBgpSession();
  stageCommunity({kMember, "value", "65000:100"});
  stageCommunity({kMember, "type", "NORMAL"});
  stageCommunity({kMember, "description", "test"});
  commitAndGetSha();
  ASSERT_TRUE(waitForBgpDaemonActive())
      << "bgpd did not return active after commit; state="
      << bgpDaemonActiveState();
  // Verify the nested member path .members[].community.{value,type,
  // description} through the daemon's own view of its config — which also
  // confirms the community subcommand's implicit creation of its parent list
  // reaches bgpd.
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

TEST_F(ConfigBgpPolicyCommunityListCommunityTest, DeleteCommunityAndCommit) {
  // Land a community-list with two inline members, delete one member through
  // a second commit, and verify the list and the other member survive in
  // bgpd's running config.
  discardSession();
  clearBgpSession();
  stageCommunity({kMember, "value", "65000:100"});
  stageCommunity({kMember2, "value", "65000:200"});
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
