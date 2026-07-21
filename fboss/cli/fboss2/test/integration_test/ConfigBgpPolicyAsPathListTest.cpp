// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/**
 * End-to-end tests for `fboss2-dev config protocol bgp policy as-path-list
 * <name> [entry <seq-num>] [<attribute> <value> ...]`.
 *
 * Scope: the AS-path list and its entries. The commit-path tests stage the
 * change AND commit it, then assert the value landed at the correct thrift
 * field path inside the matching .policies.aspath_lists[] entry of bgpd's
 * running config (via getRunningConfig RPC) — which also confirms bgpd
 * accepts and adopts a `.policies` blob at all. The remaining attributes are
 * verified in the staged session JSON (~/.fboss2/bgp_config.json) to keep the
 * suite's daemon-restart count down; the promote mechanics are identical for
 * every attribute and are covered by the commit tests + ConfigBgpSessionTest.
 *
 * Requirements:
 *   - The fboss2-dev binary under test (config subcommand tree).
 *   - HOME is set (the session file lives under $HOME/.fboss2).
 *   - bgpd is installed/active (commit restarts it).
 */

#include <gtest/gtest.h>
#include <filesystem>
#include <string>
#include <vector>

#include "fboss/cli/fboss2/test/integration_test/ConfigBgpTestBase.h"
#include "folly/json/dynamic.h"
#include "gmock/gmock.h"

using namespace facebook::fboss;
using ::testing::HasSubstr;
using ::testing::Not;

namespace {
// Test-only AS-path-list names, unlikely to collide with a real list in the
// device's running BGP config.
const std::string kList = "FBOSS2-TEST-ASPATH";
const std::string kList2 = "FBOSS2-TEST-ASPATH-2";
} // namespace

class ConfigBgpPolicyAsPathListTest : public ConfigBgpTestBase {
 protected:
  // Run `config protocol bgp policy as-path-list <tokens...>` WITHOUT clearing
  // the staged session, so attributes can accumulate across invocations.
  auto runAsPathList(const std::vector<std::string>& tokens) {
    std::vector<std::string> args = {
        "config", "protocol", "bgp", "policy", "as-path-list"};
    args.insert(args.end(), tokens.begin(), tokens.end());
    auto result = runCli(args);
    EXPECT_EQ(result.exitCode, 0)
        << "stdout=" << result.stdout << " stderr=" << result.stderr;
    return result;
  }

  // Stage an as-path-list command expected to succeed and return the staged
  // session JSON.
  folly::dynamic stageAsPathList(const std::vector<std::string>& tokens) {
    auto result = runAsPathList(tokens);
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

TEST_F(ConfigBgpPolicyAsPathListTest, SetListDescriptionAndCommit) {
  discardSession();
  clearBgpSession();
  stageAsPathList({kList, "description", "test", "spine", "as-paths"});
  commitAndGetSha();
  ASSERT_TRUE(waitForBgpDaemonActive())
      << "bgpd did not return active after commit; state="
      << bgpDaemonActiveState();
  // Two layers: the promoted file proves what the commit wrote; the daemon's
  // getRunningConfig RPC proves bgpd parsed and adopted the .policies blob.
  auto config = readSystemBgpConfig();
  const auto* list = findList(config, kList);
  ASSERT_NE(list, nullptr) << "committed config has no as-path-list " << kList;
  EXPECT_EQ((*list)["description"].asString(), "test spine as-paths");

  auto running = readRunningBgpConfigViaRpc();
  const auto* runningList = findList(running, kList);
  ASSERT_NE(runningList, nullptr)
      << "bgpd's running config has no as-path-list " << kList;
  EXPECT_EQ((*runningList)["description"].asString(), "test spine as-paths");
}

TEST_F(ConfigBgpPolicyAsPathListTest, SetEntryAsnRegexpAndCommit) {
  discardSession();
  clearBgpSession();
  stageAsPathList({kList, "entry", "10", "asn-regexp", "^65000_"});
  commitAndGetSha();
  ASSERT_TRUE(waitForBgpDaemonActive())
      << "bgpd did not return active after commit; state="
      << bgpDaemonActiveState();
  // Verify the nested union path .as_path.as_path.asn_regexp through the
  // daemon's own view of its config.
  auto running = readRunningBgpConfigViaRpc();
  const auto* list = findList(running, kList);
  ASSERT_NE(list, nullptr) << "bgpd's running config has no as-path-list "
                           << kList;
  const auto* entry = findEntry(*list, 10);
  ASSERT_NE(entry, nullptr) << "running config has no entry 10";
  EXPECT_EQ((*entry)["as_path"]["as_path"]["asn_regexp"].asString(), "^65000_");
}

TEST_F(ConfigBgpPolicyAsPathListTest, BareCreateListStagesList) {
  clearBgpSession();
  auto config = stageAsPathList({kList});
  const auto* list = findList(config, kList);
  ASSERT_NE(list, nullptr) << "bare `as-path-list <name>` did not create it";
}

TEST_F(ConfigBgpPolicyAsPathListTest, EntryAttributesAccumulate) {
  clearBgpSession();
  stageAsPathList({kList, "entry", "10", "asn-regexp", "^65000_"});
  stageAsPathList({kList, "entry", "10", "description", "match", "65000"});
  auto config =
      stageAsPathList({kList, "entry", "10", "match-logic", "NOT_EQUAL"});

  const auto* list = findList(config, kList);
  ASSERT_NE(list, nullptr);
  const auto* entry = findEntry(*list, 10);
  ASSERT_NE(entry, nullptr);
  EXPECT_EQ((*entry)["as_path"]["as_path"]["asn_regexp"].asString(), "^65000_");
  EXPECT_EQ((*entry)["description"].asString(), "match 65000");
  // MatchValueLogicOperator.NOT_EQUAL = 1.
  EXPECT_EQ((*entry)["match_logic_type"].asInt(), 1);
  // Exactly one entry despite three invocations.
  EXPECT_EQ((*list)["as_path_list"].size(), 1);
}

TEST_F(ConfigBgpPolicyAsPathListTest, MultipleEntriesAreKeyedBySeq) {
  clearBgpSession();
  stageAsPathList({kList, "entry", "10", "asn-regexp", "^65000_"});
  auto config =
      stageAsPathList({kList, "entry", "20", "asn-regexp", "^65001_"});
  const auto* list = findList(config, kList);
  ASSERT_NE(list, nullptr);
  EXPECT_EQ((*list)["as_path_list"].size(), 2);
  ASSERT_NE(findEntry(*list, 10), nullptr);
  ASSERT_NE(findEntry(*list, 20), nullptr);
}

TEST_F(ConfigBgpPolicyAsPathListTest, InvalidMatchLogicRejected) {
  clearBgpSession();
  auto result = runAsPathList({kList, "entry", "10", "match-logic", "MAYBE"});
  EXPECT_THAT(result.stdout, HasSubstr("Invalid"));
  EXPECT_FALSE(std::filesystem::exists(bgpSessionPath()))
      << "session file should not exist after rejected input";
}

TEST_F(ConfigBgpPolicyAsPathListTest, UnknownAttributeRejected) {
  clearBgpSession();
  // asn-regexp is an entry attribute; used at the list level it is unknown and
  // the CLI must refuse it instead of persisting dead config. An unknown
  // attribute is rejected at arg-parse time (a thrown std::invalid_argument),
  // so the message is surfaced on stderr rather than stdout.
  auto result = runCli(
      {"config",
       "protocol",
       "bgp",
       "policy",
       "as-path-list",
       kList,
       "asn-regexp",
       "^65000_"});
  EXPECT_THAT(result.stderr, HasSubstr("unknown"));
  EXPECT_FALSE(std::filesystem::exists(bgpSessionPath()))
      << "session file should not exist after rejected input";
}

TEST_F(ConfigBgpPolicyAsPathListTest, DeleteStagedList) {
  clearBgpSession();
  stageAsPathList({kList, "description", "one"});
  stageAsPathList({kList2, "description", "two"});

  auto result =
      runCli({"delete", "protocol", "bgp", "policy", "as-path-list", kList});
  EXPECT_EQ(result.exitCode, 0)
      << "stdout=" << result.stdout << " stderr=" << result.stderr;
  EXPECT_THAT(result.stdout, HasSubstr("Successfully deleted"));

  auto config = readBgpSessionConfig();
  EXPECT_EQ(findList(config, kList), nullptr)
      << "deleted as-path-list still present in the staged session";
  EXPECT_NE(findList(config, kList2), nullptr)
      << "unrelated as-path-list was removed";
}

TEST_F(ConfigBgpPolicyAsPathListTest, DeleteUnknownListRejected) {
  clearBgpSession();
  auto result = runCli(
      {"delete", "protocol", "bgp", "policy", "as-path-list", "NO-SUCH-LIST"});
  EXPECT_THAT(result.stdout, HasSubstr("not found"));
  EXPECT_FALSE(std::filesystem::exists(bgpSessionPath()))
      << "session file should not exist after rejected delete";
}

TEST_F(ConfigBgpPolicyAsPathListTest, DeleteListAndCommit) {
  // Land an as-path-list in the system config, then delete it through a second
  // commit and verify it is gone from bgpd's running config.
  discardSession();
  clearBgpSession();
  stageAsPathList({kList, "entry", "10", "asn-regexp", "^65000_"});
  commitAndGetSha();
  ASSERT_TRUE(waitForBgpDaemonActive())
      << "bgpd did not return active after commit; state="
      << bgpDaemonActiveState();
  ASSERT_NE(findList(readRunningBgpConfigViaRpc(), kList), nullptr)
      << "setup commit did not land the as-path-list in bgpd's running config";

  clearBgpSession();
  auto result =
      runCli({"delete", "protocol", "bgp", "policy", "as-path-list", kList});
  EXPECT_THAT(result.stdout, HasSubstr("Successfully deleted"));
  commitAndGetSha();
  ASSERT_TRUE(waitForBgpDaemonActive())
      << "bgpd did not return active after delete commit; state="
      << bgpDaemonActiveState();
  EXPECT_EQ(findList(readRunningBgpConfigViaRpc(), kList), nullptr)
      << "deleted as-path-list still present in bgpd's running config";
}
