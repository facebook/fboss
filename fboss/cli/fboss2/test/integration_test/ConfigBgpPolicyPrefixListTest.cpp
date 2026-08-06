// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/**
 * End-to-end tests for `fboss2-dev config protocol bgp policy prefix-list
 * <name> [entry <seq-num>] [<attribute> <value> ...]`.
 *
 * Scope: the prefix-list and its seq-num-keyed entries. Every test stages the
 * change AND commits it, then asserts the value landed at the correct thrift
 * field path inside the matching .policies.prefix_lists[] entry of bgpd's
 * running config (via getRunningConfig RPC) — which also confirms bgpd
 * accepts and adopts a `.policies` blob at all. Stage-only behavior
 * (attribute parsing, validation, rejection) is covered by the unit tests; an
 * integration test that never commits exercises no daemon.
 *
 * Requirements:
 *   - The fboss2-dev binary under test (config subcommand tree).
 *   - HOME is set (the session file lives under $HOME/.fboss2).
 *   - bgpd is installed/active (commit restarts it).
 */

#include <fmt/format.h>
#include <gtest/gtest.h>
#include <cstdint>
#include <string>
#include <vector>

#include "fboss/cli/fboss2/test/integration_test/ConfigBgpTestBase.h"
#include "folly/json/dynamic.h"
#include "gmock/gmock.h"

using namespace facebook::fboss;
using ::testing::HasSubstr;
using ::testing::Not;

namespace {
// Test-only prefix-list name, unlikely to collide with a real list in the
// device's running BGP config.
const std::string kList = "FBOSS2-TEST-PREFIXES";
const std::string kEntrySeq = "10";
const std::string kEntrySeq2 = "20";
} // namespace

class ConfigBgpPolicyPrefixListTest : public ConfigBgpTestBase {
 protected:
  // Stage `config protocol bgp policy prefix-list <tokens...>` WITHOUT
  // clearing the staged session, so attributes can accumulate across
  // invocations. Returns the staged session JSON.
  folly::dynamic stagePrefixList(const std::vector<std::string>& tokens) {
    std::vector<std::string> args = {
        "config", "protocol", "bgp", "policy", "prefix-list"};
    args.insert(args.end(), tokens.begin(), tokens.end());
    auto result = runCli(args);
    EXPECT_EQ(result.exitCode, 0)
        << "stdout=" << result.stdout << " stderr=" << result.stderr;
    EXPECT_THAT(result.stdout, Not(HasSubstr("Error:")));
    return readBgpSessionConfig();
  }

  // The .policies.prefix_lists[] entry named `name`, or nullptr.
  static const folly::dynamic* findList(
      const folly::dynamic& config,
      const std::string& name) {
    if (config.count("policies") == 0 ||
        config["policies"].count("prefix_lists") == 0) {
      return nullptr;
    }
    for (const auto& list : config["policies"]["prefix_lists"]) {
      if (list.count("name") && list["name"].asString() == name) {
        return &list;
      }
    }
    return nullptr;
  }

  // The .prefixes[] entry with seq_num `seqNum` inside `list`, or nullptr.
  static const folly::dynamic* findEntry(
      const folly::dynamic& list,
      int64_t seqNum) {
    if (list.count("prefixes") == 0) {
      return nullptr;
    }
    for (const auto& entry : list["prefixes"]) {
      if (entry.count("seq_num") && entry["seq_num"].asInt() == seqNum) {
        return &entry;
      }
    }
    return nullptr;
  }
};

TEST_F(ConfigBgpPolicyPrefixListTest, SetListAttributesAndCommit) {
  discardSession();
  clearBgpSession();
  stagePrefixList({kList, "description", "test", "spine", "prefixes"});
  stagePrefixList({kList, "boolean-operator", "AND"});
  stagePrefixList({kList, "ip-version", "v4"});
  commitAndGetSha();
  ASSERT_TRUE(waitForBgpDaemonActive())
      << "bgpd did not return active after commit; state="
      << bgpDaemonActiveState();
  // Two layers: the promoted file proves what the commit wrote; the daemon's
  // getRunningConfig RPC proves bgpd parsed and adopted the .policies blob.
  auto config = readSystemBgpConfig();
  const auto* list = findList(config, kList);
  ASSERT_NE(list, nullptr) << "committed config has no prefix-list " << kList;
  EXPECT_EQ((*list)["description"].asString(), "test spine prefixes");

  auto running = readRunningBgpConfigViaRpc();
  const auto* runningList = findList(running, kList);
  ASSERT_NE(runningList, nullptr)
      << "bgpd's running config has no prefix-list " << kList;
  EXPECT_EQ((*runningList)["description"].asString(), "test spine prefixes");
  // Enums ride the SimpleJSON wire format as integers:
  // routing_policy.BooleanOperator.AND = 1.
  EXPECT_EQ((*runningList)["boolean_operator"].asInt(), 1);
  EXPECT_EQ((*runningList)["version"].asInt(), 4);
}

TEST_F(ConfigBgpPolicyPrefixListTest, SetEntryAttributesAndCommit) {
  discardSession();
  clearBgpSession();
  stagePrefixList({kList, "entry", kEntrySeq, "base-prefix", "10.0.0.0/8"});
  stagePrefixList({kList, "entry", kEntrySeq, "match-logic", "NOT_EQUAL"});
  stagePrefixList(
      {kList,
       "entry",
       kEntrySeq,
       "prefix-len-range",
       "compare-operator",
       "GE"});
  stagePrefixList(
      {kList, "entry", kEntrySeq, "prefix-len-range", "value", "24"});
  stagePrefixList({kList, "entry", kEntrySeq, "communities", "65000:100"});
  stagePrefixList({kList, "entry", kEntrySeq, "description", "test"});
  commitAndGetSha();
  ASSERT_TRUE(waitForBgpDaemonActive())
      << "bgpd did not return active after commit; state="
      << bgpDaemonActiveState();
  // Verify the nested entry paths .prefixes[].{base_prefix, match_logic,
  // prefix_len_ranges[0], communities, description} through the daemon's own
  // view of its config.
  auto running = readRunningBgpConfigViaRpc();
  const auto* list = findList(running, kList);
  ASSERT_NE(list, nullptr) << "bgpd's running config has no prefix-list "
                           << kList;
  const auto* entry = findEntry(*list, 10);
  ASSERT_NE(entry, nullptr) << "running config has no entry " << kEntrySeq;
  EXPECT_EQ((*entry)["base_prefix"].asString(), "10.0.0.0/8");
  // routing_policy.MatchValueLogicOperator.NOT_EQUAL = 1 (integer on the
  // SimpleJSON wire).
  EXPECT_EQ((*entry)["match_logic"].asInt(), 1);
  ASSERT_EQ((*entry)["prefix_len_ranges"].size(), 1);
  // routing_policy.ComparisonOperator.GE = 2.
  EXPECT_EQ((*entry)["prefix_len_ranges"][0]["compare_operator"].asInt(), 2);
  EXPECT_EQ((*entry)["prefix_len_ranges"][0]["value"].asInt(), 24);
  ASSERT_EQ((*entry)["communities"].size(), 1);
  EXPECT_EQ((*entry)["communities"][0].asString(), "65000:100");
  EXPECT_EQ((*entry)["description"].asString(), "test");
}

TEST_F(ConfigBgpPolicyPrefixListTest, DeleteListAndCommit) {
  // Land a prefix-list in the system config, then delete it through a second
  // commit and verify it is gone from bgpd's running config.
  discardSession();
  clearBgpSession();
  stagePrefixList({kList, "entry", kEntrySeq, "base-prefix", "10.0.0.0/8"});
  commitAndGetSha();
  ASSERT_TRUE(waitForBgpDaemonActive())
      << "bgpd did not return active after commit; state="
      << bgpDaemonActiveState();
  ASSERT_NE(findList(readRunningBgpConfigViaRpc(), kList), nullptr)
      << "setup commit did not land the prefix-list in bgpd's running config";

  clearBgpSession();
  auto result =
      runCli({"delete", "protocol", "bgp", "policy", "prefix-list", kList});
  EXPECT_THAT(result.stdout, HasSubstr("Successfully deleted"));
  commitAndGetSha();
  ASSERT_TRUE(waitForBgpDaemonActive())
      << "bgpd did not return active after delete commit; state="
      << bgpDaemonActiveState();
  EXPECT_EQ(findList(readRunningBgpConfigViaRpc(), kList), nullptr)
      << "deleted prefix-list still present in bgpd's running config";
}

TEST_F(ConfigBgpPolicyPrefixListTest, DeleteEntryAndCommit) {
  // Land a prefix-list with two entries, delete one entry through a second
  // commit, and verify the list and the other entry survive in bgpd's running
  // config.
  discardSession();
  clearBgpSession();
  stagePrefixList({kList, "entry", kEntrySeq, "base-prefix", "10.0.0.0/8"});
  stagePrefixList(
      {kList, "entry", kEntrySeq2, "base-prefix", "192.168.0.0/16"});
  commitAndGetSha();
  ASSERT_TRUE(waitForBgpDaemonActive())
      << "bgpd did not return active after commit; state="
      << bgpDaemonActiveState();
  {
    auto running = readRunningBgpConfigViaRpc();
    const auto* list = findList(running, kList);
    ASSERT_NE(list, nullptr)
        << "setup commit did not land the prefix-list in bgpd's running "
           "config";
    ASSERT_NE(findEntry(*list, 10), nullptr);
    ASSERT_NE(findEntry(*list, 20), nullptr);
  }

  clearBgpSession();
  auto result = runCli(
      {"delete",
       "protocol",
       "bgp",
       "policy",
       "prefix-list",
       kList,
       "entry",
       kEntrySeq});
  EXPECT_THAT(
      result.stdout,
      HasSubstr(
          fmt::format(
              "Successfully deleted BGP prefix-list {} entry {}",
              kList,
              kEntrySeq)));
  commitAndGetSha();
  ASSERT_TRUE(waitForBgpDaemonActive())
      << "bgpd did not return active after delete commit; state="
      << bgpDaemonActiveState();
  auto running = readRunningBgpConfigViaRpc();
  const auto* list = findList(running, kList);
  ASSERT_NE(list, nullptr)
      << "deleting one entry must not delete the prefix-list";
  EXPECT_EQ(findEntry(*list, 10), nullptr)
      << "deleted entry still present in bgpd's running config";
  EXPECT_NE(findEntry(*list, 20), nullptr)
      << "surviving entry missing from bgpd's running config";
}
