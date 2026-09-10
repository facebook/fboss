// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/**
 * End-to-end tests for `fboss2-dev config protocol bgp policy as-path-list
 * <name> entry <seq-num> [<attribute> <value> ...]`.
 *
 * Scope: the entry level. Every test stages the change AND commits it, then
 * asserts the value landed at the correct thrift field path inside the
 * matching .policies.aspath_lists[].as_path_list[] entry of bgpd's running
 * config (via getRunningConfig RPC). Stage-only behavior (attribute parsing,
 * validation, rejection) is covered by the unit tests; an integration test
 * that never commits exercises no daemon.
 *
 * Requirements:
 *   - The fboss2-dev binary under test (config subcommand tree).
 *   - HOME is set (the session file lives under $HOME/.fboss2).
 *   - bgpd is installed/active (commit restarts it).
 */

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
// Test-only AS-path-list name, unlikely to collide with a real list in the
// device's running BGP config.
const std::string kList = "FBOSS2-TEST-ASPATH-ENTRY";
} // namespace

class ConfigBgpPolicyAsPathListEntryTest : public ConfigBgpTestBase {
 protected:
  // Stage `config protocol bgp policy as-path-list <name> entry <tokens...>`
  // WITHOUT clearing the staged session, so attributes can accumulate across
  // invocations. Returns the staged session JSON.
  folly::dynamic stageEntry(const std::vector<std::string>& tokens) {
    std::vector<std::string> args = {
        "config", "protocol", "bgp", "policy", "as-path-list", kList, "entry"};
    args.insert(args.end(), tokens.begin(), tokens.end());
    auto result = runCli(args);
    EXPECT_EQ(result.exitCode, 0)
        << "stdout=" << result.stdout << " stderr=" << result.stderr;
    EXPECT_THAT(result.stdout, Not(HasSubstr("Error:")));
    return readBgpSessionConfig();
  }

  // The .policies.aspath_lists[] entry named `name`, or nullptr.
  static const folly::dynamic* findList(
      const folly::dynamic& config,
      const std::string& name) {
    if (config.count("policies") == 0 ||
        config["policies"].count("aspath_lists") == 0) {
      return nullptr;
    }
    for (const auto& list : config["policies"]["aspath_lists"]) {
      if (list.count("name") && list["name"].asString() == name) {
        return &list;
      }
    }
    return nullptr;
  }

  // The .as_path_list[] entry with sequence_number `seq` inside `list`, or
  // nullptr.
  static const folly::dynamic* findEntry(
      const folly::dynamic& list,
      int64_t seq) {
    if (list.count("as_path_list") == 0) {
      return nullptr;
    }
    for (const auto& entry : list["as_path_list"]) {
      if (entry.count("sequence_number") &&
          entry["sequence_number"].asInt() == seq) {
        return &entry;
      }
    }
    return nullptr;
  }
};

TEST_F(ConfigBgpPolicyAsPathListEntryTest, SetEntryAsnRegexpAndCommit) {
  discardSession();
  clearBgpSession();
  stageEntry({"10", "asn-regexp", "^65000_"});
  commitAndGetSha();
  ASSERT_TRUE(waitForBgpDaemonActive())
      << "bgpd did not return active after commit; state="
      << bgpDaemonActiveState();
  // Verify the nested union path .as_path.as_path.asn_regexp through the
  // daemon's own view of its config — which also confirms the entry
  // subcommand's implicit creation of its parent list reaches bgpd.
  auto running = readRunningBgpConfigViaRpc();
  const auto* list = findList(running, kList);
  ASSERT_NE(list, nullptr) << "bgpd's running config has no as-path-list "
                           << kList;
  const auto* entry = findEntry(*list, 10);
  ASSERT_NE(entry, nullptr) << "running config has no entry 10";
  EXPECT_EQ((*entry)["as_path"]["as_path"]["asn_regexp"].asString(), "^65000_");
}

TEST_F(ConfigBgpPolicyAsPathListEntryTest, SetEntryMatchLogicAndCommit) {
  discardSession();
  clearBgpSession();
  stageEntry({"20", "asn-regexp", "^65001_"});
  stageEntry({"20", "match-logic", "NOT_EQUAL"});
  commitAndGetSha();
  ASSERT_TRUE(waitForBgpDaemonActive())
      << "bgpd did not return active after commit; state="
      << bgpDaemonActiveState();
  auto running = readRunningBgpConfigViaRpc();
  const auto* list = findList(running, kList);
  ASSERT_NE(list, nullptr) << "bgpd's running config has no as-path-list "
                           << kList;
  const auto* entry = findEntry(*list, 20);
  ASSERT_NE(entry, nullptr) << "running config has no entry 20";
  // Enums ride the SimpleJSON wire format as integers:
  // routing_policy.MatchValueLogicOperator.NOT_EQUAL = 1.
  EXPECT_EQ((*entry)["match_logic_type"].asInt(), 1);
}
