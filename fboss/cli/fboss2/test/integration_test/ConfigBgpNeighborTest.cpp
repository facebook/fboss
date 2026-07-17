// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/**
 * End-to-end tests for `fboss2-dev config protocol bgp neighbor <ip>
 * [<attribute> <value> ...]`.
 *
 * Scope: the per-neighbor attributes. The commit-path tests stage the change
 * AND commit it, then assert the value landed at the correct thrift field
 * path inside the matching .peers[] entry of the promoted system config
 * (/etc/coop/bgpcpp/bgpcpp.conf). The remaining attributes are verified in
 * the staged session JSON (~/.fboss2/bgp_config.json) to keep the suite's
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
// Test-only neighbor addresses, from the documentation range (RFC 3849) so
// they never collide with a real peer in the device's running BGP config.
const std::string kNeighbor = "2001:db8::1234";
const std::string kNeighborV4 = "192.0.2.101";
} // namespace

class ConfigBgpNeighborTest : public ConfigBgpTestBase {
 protected:
  // Run `config protocol bgp neighbor <tokens...>` WITHOUT clearing the
  // staged session, so attributes can accumulate across invocations.
  auto runNeighbor(const std::vector<std::string>& tokens) {
    std::vector<std::string> args = {"config", "protocol", "bgp", "neighbor"};
    args.insert(args.end(), tokens.begin(), tokens.end());
    auto result = runCli(args);
    EXPECT_EQ(result.exitCode, 0)
        << "stdout=" << result.stdout << " stderr=" << result.stderr;
    return result;
  }

  // Stage a neighbor command expected to succeed and return the staged
  // session JSON.
  folly::dynamic stageNeighbor(const std::vector<std::string>& tokens) {
    auto result = runNeighbor(tokens);
    EXPECT_THAT(result.stdout, Not(HasSubstr("Error:")));
    return readBgpSessionConfig();
  }

  // The .peers[] entry for addr, or nullptr.
  static const folly::dynamic* findPeer(
      const folly::dynamic& config,
      const std::string& addr) {
    if (config.count("peers") == 0) {
      return nullptr;
    }
    for (const auto& peer : config["peers"]) {
      if (peer.count("peer_addr") && peer["peer_addr"].asString() == addr) {
        return &peer;
      }
    }
    return nullptr;
  }
};

TEST_F(ConfigBgpNeighborTest, SetRemoteAsnAndCommit) {
  discardSession();
  clearBgpSession();
  stageNeighbor({kNeighbor, "remote-asn", "65000"});
  commitAndGetSha();
  ASSERT_TRUE(waitForBgpDaemonActive())
      << "bgpd did not return active after commit; state="
      << bgpDaemonActiveState();
  // Two layers: the promoted file proves what the commit wrote; the daemon's
  // getRunningConfig RPC proves bgpd parsed and adopted it after the restart.
  auto config = readSystemBgpConfig();
  const auto* peer = findPeer(config, kNeighbor);
  ASSERT_NE(peer, nullptr) << "committed config has no peer " << kNeighbor;
  ASSERT_TRUE(peer->count("remote_as_4_byte"));
  EXPECT_EQ((*peer)["remote_as_4_byte"].asInt(), 65000);

  auto running = readRunningBgpConfigViaRpc();
  const auto* runningPeer = findPeer(running, kNeighbor);
  ASSERT_NE(runningPeer, nullptr)
      << "bgpd's running config has no peer " << kNeighbor;
  EXPECT_EQ((*runningPeer)["remote_as_4_byte"].asInt(), 65000);
}

TEST_F(ConfigBgpNeighborTest, SetTimersHoldTimeAndCommit) {
  discardSession();
  clearBgpSession();
  stageNeighbor({kNeighbor, "timers", "hold-time", "90"});
  commitAndGetSha();
  ASSERT_TRUE(waitForBgpDaemonActive())
      << "bgpd did not return active after commit; state="
      << bgpDaemonActiveState();
  // Verify nested timer fields through the daemon's own view of its config.
  auto running = readRunningBgpConfigViaRpc();
  const auto* peer = findPeer(running, kNeighbor);
  ASSERT_NE(peer, nullptr) << "bgpd's running config has no peer " << kNeighbor;
  ASSERT_TRUE(peer->count("bgp_peer_timers"));
  EXPECT_EQ((*peer)["bgp_peer_timers"]["hold_time_seconds"].asInt(), 90);
}

TEST_F(ConfigBgpNeighborTest, BareCreateStagesPeer) {
  clearBgpSession();
  auto config = stageNeighbor({kNeighborV4});
  const auto* peer = findPeer(config, kNeighborV4);
  ASSERT_NE(peer, nullptr) << "bare `neighbor <ip>` did not create the peer";
}

TEST_F(ConfigBgpNeighborTest, AttributesAccumulateOnOnePeer) {
  clearBgpSession();
  stageNeighbor({kNeighbor, "description", "test", "spine", "peer"});
  stageNeighbor({kNeighbor, "connect-mode", "PASSIVE"});
  stageNeighbor({kNeighbor, "afi", "disable-ipv4-afi", "true"});
  stageNeighbor({kNeighbor, "graceful-restart", "stateful-ha", "true"});
  stageNeighbor({kNeighbor, "max-route", "pre-filter", "45000"});
  stageNeighbor({kNeighbor, "peer-group", "TEST-GROUP"});
  auto config = stageNeighbor({kNeighbor, "local-asn", "64512"});

  const auto* peer = findPeer(config, kNeighbor);
  ASSERT_NE(peer, nullptr);
  EXPECT_EQ((*peer)["description"].asString(), "test spine peer");
  EXPECT_TRUE((*peer)["is_passive"].asBool());
  EXPECT_TRUE((*peer)["disable_ipv4_afi"].asBool());
  EXPECT_TRUE((*peer)["enable_stateful_ha"].asBool());
  EXPECT_EQ((*peer)["pre_filter"]["max_routes"].asInt(), 45000);
  EXPECT_EQ((*peer)["peer_group_name"].asString(), "TEST-GROUP");
  EXPECT_EQ((*peer)["local_as_4_byte"].asInt(), 64512);
  // Exactly one entry for this neighbor despite seven invocations.
  int count = 0;
  for (const auto& p : config["peers"]) {
    count += (p["peer_addr"].asString() == kNeighbor);
  }
  EXPECT_EQ(count, 1);
}

TEST_F(ConfigBgpNeighborTest, AddPathMergesDirections) {
  clearBgpSession();
  // AddPath enum: RECEIVE=1, SEND=2, BOTH=3.
  auto config = stageNeighbor({kNeighbor, "add-path", "receive", "true"});
  const auto* peer = findPeer(config, kNeighbor);
  ASSERT_NE(peer, nullptr);
  EXPECT_EQ((*peer)["add_path"].asInt(), 1);

  config = stageNeighbor({kNeighbor, "add-path", "send", "true"});
  peer = findPeer(config, kNeighbor);
  EXPECT_EQ((*peer)["add_path"].asInt(), 3);
}

TEST_F(ConfigBgpNeighborTest, InvalidBoolValueRejected) {
  clearBgpSession();
  auto result = runNeighbor({kNeighbor, "rr-client", "maybe"});
  EXPECT_THAT(result.stdout, HasSubstr("Invalid"));
  EXPECT_FALSE(std::filesystem::exists(bgpSessionPath()))
      << "session file should not exist after rejected input";
}

TEST_F(ConfigBgpNeighborTest, UnsupportedAttributeRejected) {
  clearBgpSession();
  // peer-port has no per-neighbor thrift field; the CLI must refuse it
  // instead of persisting dead config.
  auto result = runNeighbor({kNeighbor, "peer-port", "1179"});
  EXPECT_THAT(result.stdout, HasSubstr("not supported"));
  EXPECT_FALSE(std::filesystem::exists(bgpSessionPath()))
      << "session file should not exist after rejected input";
}

TEST_F(ConfigBgpNeighborTest, ConnectModeBothRejected) {
  clearBgpSession();
  auto result = runNeighbor({kNeighbor, "connect-mode", "BOTH"});
  EXPECT_THAT(result.stdout, HasSubstr("Error"));
  EXPECT_FALSE(std::filesystem::exists(bgpSessionPath()))
      << "session file should not exist after rejected input";
}

TEST_F(ConfigBgpNeighborTest, DeleteStagedNeighbor) {
  clearBgpSession();
  stageNeighbor({kNeighbor, "remote-asn", "65000"});
  stageNeighbor({kNeighborV4, "remote-asn", "65001"});

  auto result = runCli({"delete", "protocol", "bgp", "neighbor", kNeighbor});
  EXPECT_EQ(result.exitCode, 0)
      << "stdout=" << result.stdout << " stderr=" << result.stderr;
  EXPECT_THAT(result.stdout, HasSubstr("Successfully deleted"));

  auto config = readBgpSessionConfig();
  EXPECT_EQ(findPeer(config, kNeighbor), nullptr)
      << "deleted neighbor still present in the staged session";
  EXPECT_NE(findPeer(config, kNeighborV4), nullptr)
      << "unrelated neighbor was removed";
}

TEST_F(ConfigBgpNeighborTest, DeleteUnknownNeighborRejected) {
  clearBgpSession();
  auto result =
      runCli({"delete", "protocol", "bgp", "neighbor", "192.0.2.222"});
  EXPECT_THAT(result.stdout, HasSubstr("not found"));
  EXPECT_FALSE(std::filesystem::exists(bgpSessionPath()))
      << "session file should not exist after rejected delete";
}

TEST_F(ConfigBgpNeighborTest, DeleteNeighborAndCommit) {
  // Land a neighbor in the system config, then delete it through a second
  // commit and verify it is gone from the promoted config.
  discardSession();
  clearBgpSession();
  stageNeighbor({kNeighbor, "remote-asn", "65000"});
  commitAndGetSha();
  ASSERT_TRUE(waitForBgpDaemonActive())
      << "bgpd did not return active after commit; state="
      << bgpDaemonActiveState();
  ASSERT_NE(findPeer(readRunningBgpConfigViaRpc(), kNeighbor), nullptr)
      << "setup commit did not land the neighbor in bgpd's running config";

  clearBgpSession();
  auto result = runCli({"delete", "protocol", "bgp", "neighbor", kNeighbor});
  EXPECT_THAT(result.stdout, HasSubstr("Successfully deleted"));
  commitAndGetSha();
  ASSERT_TRUE(waitForBgpDaemonActive())
      << "bgpd did not return active after delete commit; state="
      << bgpDaemonActiveState();
  EXPECT_EQ(findPeer(readRunningBgpConfigViaRpc(), kNeighbor), nullptr)
      << "deleted neighbor still present in bgpd's running config";
}
