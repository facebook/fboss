// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/**
 * End-to-end tests for `fboss2-dev config protocol bgp peer-group <name>
 * [<attribute> <value> ...]`.
 *
 * Scope: the per-peer-group attributes. The commit-path tests stage the change
 * AND commit it, then assert the value landed at the correct thrift field
 * path inside the matching .peer_groups[] entry of bgpd's running config
 * (via getRunningConfig RPC). The remaining attributes are verified in the
 * staged session JSON (~/.fboss2/bgp_config.json) to keep the suite's
 * daemon-restart count down; the promote mechanics are identical for every
 * attribute and are covered by the commit tests + ConfigBgpSessionTest.
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
// Test-only peer-group names, unlikely to collide with a real group in the
// device's running BGP config.
const std::string kGroup = "FBOSS2-TEST-GROUP";
const std::string kGroup2 = "FBOSS2-TEST-GROUP-2";
} // namespace

class ConfigBgpPeerGroupTest : public ConfigBgpTestBase {
 protected:
  // Run `config protocol bgp peer-group <tokens...>` WITHOUT clearing the
  // staged session, so attributes can accumulate across invocations.
  auto runPeerGroup(const std::vector<std::string>& tokens) {
    std::vector<std::string> args = {"config", "protocol", "bgp", "peer-group"};
    args.insert(args.end(), tokens.begin(), tokens.end());
    auto result = runCli(args);
    EXPECT_EQ(result.exitCode, 0)
        << "stdout=" << result.stdout << " stderr=" << result.stderr;
    return result;
  }

  // Stage a peer-group command expected to succeed and return the staged
  // session JSON.
  folly::dynamic stagePeerGroup(const std::vector<std::string>& tokens) {
    auto result = runPeerGroup(tokens);
    EXPECT_THAT(result.stdout, Not(HasSubstr("Error:")));
    return readBgpSessionConfig();
  }

  // The .peer_groups[] entry named `name`, or nullptr.
  static const folly::dynamic* findGroup(
      const folly::dynamic& config,
      const std::string& name) {
    if (config.count("peer_groups") == 0) {
      return nullptr;
    }
    for (const auto& group : config["peer_groups"]) {
      if (group.count("name") && group["name"].asString() == name) {
        return &group;
      }
    }
    return nullptr;
  }
};

TEST_F(ConfigBgpPeerGroupTest, SetRemoteAsnAndCommit) {
  discardSession();
  clearBgpSession();
  stagePeerGroup({kGroup, "remote-asn", "65000"});
  commitAndGetSha();
  ASSERT_TRUE(waitForBgpDaemonActive())
      << "bgpd did not return active after commit; state="
      << bgpDaemonActiveState();
  // Two layers: the promoted file proves what the commit wrote; the daemon's
  // getRunningConfig RPC proves bgpd parsed and adopted it after the restart.
  auto config = readSystemBgpConfig();
  const auto* group = findGroup(config, kGroup);
  ASSERT_NE(group, nullptr) << "committed config has no peer-group " << kGroup;
  ASSERT_TRUE(group->count("remote_as_4_byte"));
  EXPECT_EQ((*group)["remote_as_4_byte"].asInt(), 65000);

  auto running = readRunningBgpConfigViaRpc();
  const auto* runningGroup = findGroup(running, kGroup);
  ASSERT_NE(runningGroup, nullptr)
      << "bgpd's running config has no peer-group " << kGroup;
  EXPECT_EQ((*runningGroup)["remote_as_4_byte"].asInt(), 65000);
}

TEST_F(ConfigBgpPeerGroupTest, SetTimersHoldTimeAndCommit) {
  discardSession();
  clearBgpSession();
  stagePeerGroup({kGroup, "timers", "hold-time", "90"});
  commitAndGetSha();
  ASSERT_TRUE(waitForBgpDaemonActive())
      << "bgpd did not return active after commit; state="
      << bgpDaemonActiveState();
  // Verify nested timer fields through the daemon's own view of its config.
  auto running = readRunningBgpConfigViaRpc();
  const auto* group = findGroup(running, kGroup);
  ASSERT_NE(group, nullptr)
      << "bgpd's running config has no peer-group " << kGroup;
  ASSERT_TRUE(group->count("bgp_peer_timers"));
  EXPECT_EQ((*group)["bgp_peer_timers"]["hold_time_seconds"].asInt(), 90);
}

TEST_F(ConfigBgpPeerGroupTest, BareCreateStagesGroup) {
  clearBgpSession();
  auto config = stagePeerGroup({kGroup});
  const auto* group = findGroup(config, kGroup);
  ASSERT_NE(group, nullptr) << "bare `peer-group <name>` did not create it";
}

TEST_F(ConfigBgpPeerGroupTest, AttributesAccumulateOnOneGroup) {
  clearBgpSession();
  stagePeerGroup({kGroup, "description", "test", "spine", "group"});
  stagePeerGroup({kGroup, "connect-mode", "PASSIVE"});
  stagePeerGroup({kGroup, "afi", "disable-ipv4-afi", "true"});
  stagePeerGroup({kGroup, "graceful-restart", "stateful-ha", "true"});
  stagePeerGroup({kGroup, "max-route", "pre-filter", "45000"});
  auto config = stagePeerGroup({kGroup, "local-asn", "64512"});

  const auto* group = findGroup(config, kGroup);
  ASSERT_NE(group, nullptr);
  EXPECT_EQ((*group)["description"].asString(), "test spine group");
  EXPECT_TRUE((*group)["is_passive"].asBool());
  EXPECT_TRUE((*group)["disable_ipv4_afi"].asBool());
  EXPECT_TRUE((*group)["enable_stateful_ha"].asBool());
  EXPECT_EQ((*group)["pre_filter"]["max_routes"].asInt(), 45000);
  EXPECT_EQ((*group)["local_as_4_byte"].asInt(), 64512);
  // Exactly one entry for this group despite six invocations.
  int count = 0;
  for (const auto& g : config["peer_groups"]) {
    count += (g["name"].asString() == kGroup);
  }
  EXPECT_EQ(count, 1);
}

TEST_F(ConfigBgpPeerGroupTest, AddPathMergesDirections) {
  clearBgpSession();
  // AddPath enum: RECEIVE=1, SEND=2, BOTH=3.
  auto config = stagePeerGroup({kGroup, "add-path", "receive", "true"});
  const auto* group = findGroup(config, kGroup);
  ASSERT_NE(group, nullptr);
  EXPECT_EQ((*group)["add_path"].asInt(), 1);

  config = stagePeerGroup({kGroup, "add-path", "send", "true"});
  group = findGroup(config, kGroup);
  EXPECT_EQ((*group)["add_path"].asInt(), 3);
}

TEST_F(ConfigBgpPeerGroupTest, InvalidBoolValueRejected) {
  clearBgpSession();
  auto result = runPeerGroup({kGroup, "rr-client", "maybe"});
  EXPECT_THAT(result.stdout, HasSubstr("Invalid"));
  EXPECT_FALSE(std::filesystem::exists(bgpSessionPath()))
      << "session file should not exist after rejected input";
}

TEST_F(ConfigBgpPeerGroupTest, UnsupportedAttributeRejected) {
  clearBgpSession();
  // peer-port has no per-peer-group thrift field; the CLI must refuse it
  // instead of persisting dead config.
  auto result = runPeerGroup({kGroup, "peer-port", "1179"});
  EXPECT_THAT(result.stdout, HasSubstr("unknown"));
  EXPECT_FALSE(std::filesystem::exists(bgpSessionPath()))
      << "session file should not exist after rejected input";
}

TEST_F(ConfigBgpPeerGroupTest, ConnectModeBothRejected) {
  clearBgpSession();
  auto result = runPeerGroup({kGroup, "connect-mode", "BOTH"});
  EXPECT_THAT(result.stdout, HasSubstr("Error"));
  EXPECT_FALSE(std::filesystem::exists(bgpSessionPath()))
      << "session file should not exist after rejected input";
}

TEST_F(ConfigBgpPeerGroupTest, DeleteStagedGroup) {
  clearBgpSession();
  stagePeerGroup({kGroup, "remote-asn", "65000"});
  stagePeerGroup({kGroup2, "remote-asn", "65001"});

  auto result = runCli({"delete", "protocol", "bgp", "peer-group", kGroup});
  EXPECT_EQ(result.exitCode, 0)
      << "stdout=" << result.stdout << " stderr=" << result.stderr;
  EXPECT_THAT(result.stdout, HasSubstr("Successfully deleted"));

  auto config = readBgpSessionConfig();
  EXPECT_EQ(findGroup(config, kGroup), nullptr)
      << "deleted peer-group still present in the staged session";
  EXPECT_NE(findGroup(config, kGroup2), nullptr)
      << "unrelated peer-group was removed";
}

TEST_F(ConfigBgpPeerGroupTest, DeleteUnknownGroupRejected) {
  clearBgpSession();
  auto result =
      runCli({"delete", "protocol", "bgp", "peer-group", "NO-SUCH-GROUP"});
  EXPECT_THAT(result.stdout, HasSubstr("not found"));
  EXPECT_FALSE(std::filesystem::exists(bgpSessionPath()))
      << "session file should not exist after rejected delete";
}

TEST_F(ConfigBgpPeerGroupTest, DeleteGroupAndCommit) {
  // Land a peer-group in the system config, then delete it through a second
  // commit and verify it is gone from bgpd's running config.
  discardSession();
  clearBgpSession();
  stagePeerGroup({kGroup, "remote-asn", "65000"});
  commitAndGetSha();
  ASSERT_TRUE(waitForBgpDaemonActive())
      << "bgpd did not return active after commit; state="
      << bgpDaemonActiveState();
  ASSERT_NE(findGroup(readRunningBgpConfigViaRpc(), kGroup), nullptr)
      << "setup commit did not land the peer-group in bgpd's running config";

  clearBgpSession();
  auto result = runCli({"delete", "protocol", "bgp", "peer-group", kGroup});
  EXPECT_THAT(result.stdout, HasSubstr("Successfully deleted"));
  commitAndGetSha();
  ASSERT_TRUE(waitForBgpDaemonActive())
      << "bgpd did not return active after delete commit; state="
      << bgpDaemonActiveState();
  EXPECT_EQ(findGroup(readRunningBgpConfigViaRpc(), kGroup), nullptr)
      << "deleted peer-group still present in bgpd's running config";
}
