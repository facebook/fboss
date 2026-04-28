// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/**
 * End-to-end test for the 'fboss2-dev config tunnel ip-in-ip' and
 * 'fboss2-dev delete tunnel ip-in-ip' commands.
 *
 * Positive tests create a complete production-style tunnel (all attributes
 * set, matching the shape of the netbouncer_tunnel in production) and verify
 * that create / delete / per-attribute reset commit cleanly with no agent
 * crash.
 *
 * Negative tests verify that the CLI and agent reject incomplete or invalid
 * configurations with a non-zero exit code.
 *
 * Requirements:
 * - FBOSS agent must be running with a valid configuration
 */

#include <folly/logging/xlog.h>
#include <gtest/gtest.h>
#include <string>
#include <vector>
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/test/integration_test/Fboss2IntegrationTest.h"

using namespace facebook::fboss;

class TunnelIpInIpTest : public Fboss2IntegrationTest {
 protected:
  void TearDown() override {
    for (const auto& id : createdTunnels_) {
      auto result = runCli({"delete", "tunnel", "ip-in-ip", id});
      if (result.exitCode == 0) {
        commitConfig();
      }
    }
    createdTunnels_.clear();
    Fboss2IntegrationTest::TearDown();
  }

  bool tunnelExists(const std::string& id) const {
    auto& session = ConfigSession::getInstance();
    auto& swConfig = *session.getAgentConfig().sw();
    if (!swConfig.ipInIpTunnels().has_value()) {
      return false;
    }
    for (const auto& t : *swConfig.ipInIpTunnels()) {
      if (*t.ipInIpTunnelId() == id) {
        return true;
      }
    }
    return false;
  }

  // Create a complete production-style tunnel (all attrs) and commit.
  // Mirrors netbouncer_tunnel: dstIp from virtual intf, srcIp fixed doc IPv6,
  // ttl/dscp/ecn=PIPE, termType=P2MP. Tracks the id for auto-cleanup.
  void createCompleteTunnel(
      const std::string& tunnelId,
      int intfId,
      const std::string& dstIp) {
    auto result = runCli(
        {"config",
         "tunnel",
         "ip-in-ip",
         tunnelId,
         "destination",
         dstIp,
         "source",
         "2001:db8:cafe::1",
         "ttl-mode",
         "pipe",
         "dscp-mode",
         "pipe",
         "ecn-mode",
         "pipe",
         "termination-type",
         "p2mp",
         "underlay-intf-id",
         std::to_string(intfId)});
    ASSERT_EQ(result.exitCode, 0)
        << "Failed to create complete tunnel: " << result.stderr;
    commitConfig();
    createdTunnels_.push_back(tunnelId);
    ASSERT_TRUE(tunnelExists(tunnelId))
        << "Tunnel not found in config after commit";
  }

  void trackTunnel(const std::string& id) {
    createdTunnels_.push_back(id);
  }

 private:
  std::vector<std::string> createdTunnels_;
};

// ---------------------------------------------------------------------------
// Positive: create a tunnel with all production attributes in one invocation,
// commit, and verify every field is recorded correctly.
//
// Mirrors the production netbouncer_tunnel shape:
//   underlayIntfID -> virtual interface (isVirtual=true)
//   dstIp          -> IPv6 on that virtual interface
//   srcIp          -> fixed documentation IPv6 (stand-in for 2401::0)
//   ttlMode=PIPE, dscpMode=PIPE, ecnMode=PIPE, tunnelTermType=P2MP
// ---------------------------------------------------------------------------

TEST_F(TunnelIpInIpTest, CreateCompleteProductionTunnel) {
  int intfId = ensureUnderlayIntfId();
  const std::string dstIp = findIpv6OnIntf(intfId);
  const std::string srcIp = "2001:db8:cafe::1";
  const std::string tunnelId = "integration-test-tunnel-complete";

  XLOG(INFO) << "[Step 1] Creating complete production-style tunnel "
             << tunnelId << " (underlayIntf=" << intfId << " dst=" << dstIp
             << ")...";
  createCompleteTunnel(tunnelId, intfId, dstIp);

  XLOG(INFO) << "[Step 2] Verifying all fields in committed config...";
  auto& session = ConfigSession::getInstance();
  auto& swConfig = *session.getAgentConfig().sw();
  bool found = false;
  for (const auto& tunnel : *swConfig.ipInIpTunnels()) {
    if (*tunnel.ipInIpTunnelId() != tunnelId) {
      continue;
    }
    EXPECT_EQ(*tunnel.dstIp(), dstIp);
    ASSERT_TRUE(tunnel.srcIp().has_value());
    EXPECT_EQ(*tunnel.srcIp(), srcIp);
    EXPECT_EQ(*tunnel.underlayIntfID(), intfId);
    ASSERT_TRUE(tunnel.ttlMode().has_value());
    EXPECT_EQ(*tunnel.ttlMode(), cfg::TunnelMode::PIPE);
    ASSERT_TRUE(tunnel.dscpMode().has_value());
    EXPECT_EQ(*tunnel.dscpMode(), cfg::TunnelMode::PIPE);
    ASSERT_TRUE(tunnel.ecnMode().has_value());
    EXPECT_EQ(*tunnel.ecnMode(), cfg::TunnelMode::PIPE);
    ASSERT_TRUE(tunnel.tunnelTermType().has_value());
    EXPECT_EQ(*tunnel.tunnelTermType(), cfg::TunnelTerminationType::P2MP);
    found = true;
    break;
  }
  EXPECT_TRUE(found) << "Tunnel '" << tunnelId << "' not found after commit";
}

// ---------------------------------------------------------------------------
// Negative: underlay-intf-id pointing to a non-existent interface must be
// rejected by the CLI (no commit needed).
// ---------------------------------------------------------------------------

TEST_F(TunnelIpInIpTest, SetUnderlayIntfIdInvalidRejected) {
  int intfId = ensureUnderlayIntfId();
  const std::string dstIp = findIpv6OnIntf(intfId);
  const std::string tunnelId = "integration-test-tunnel-invalid-underlay";

  XLOG(INFO) << "[Setup] Creating base tunnel " << tunnelId << "...";
  {
    auto result = runCli(
        {"config",
         "tunnel",
         "ip-in-ip",
         tunnelId,
         "destination",
         dstIp,
         "underlay-intf-id",
         std::to_string(intfId)});
    ASSERT_EQ(result.exitCode, 0) << "Setup create failed: " << result.stderr;
    commitConfig();
    trackTunnel(tunnelId);
  }

  // 2147483647 is INT_MAX — not a real interface ID.
  XLOG(INFO) << "[Test] Setting underlay-intf-id to INT_MAX (must fail)...";
  auto result = runCli(
      {"config",
       "tunnel",
       "ip-in-ip",
       tunnelId,
       "underlay-intf-id",
       "2147483647"});
  EXPECT_NE(result.exitCode, 0)
      << "Expected non-zero exit for non-existent underlay-intf-id";
  discardSession();
}

// ---------------------------------------------------------------------------
// Positive: create a complete production-style tunnel then delete it whole.
// ---------------------------------------------------------------------------

TEST_F(TunnelIpInIpTest, DeleteCompleteProductionTunnel) {
  int intfId = ensureUnderlayIntfId();
  const std::string dstIp = findIpv6OnIntf(intfId);
  const std::string tunnelId = "integration-test-tunnel-delete-complete";

  XLOG(INFO) << "[Step 1] Creating complete production-style tunnel "
             << tunnelId << "...";
  createCompleteTunnel(tunnelId, intfId, dstIp);

  XLOG(INFO) << "[Step 2] Deleting entire tunnel...";
  auto result = runCli({"delete", "tunnel", "ip-in-ip", tunnelId});
  ASSERT_EQ(result.exitCode, 0) << "Delete failed: " << result.stderr;
  commitConfig();
  EXPECT_FALSE(tunnelExists(tunnelId));
}

// ---------------------------------------------------------------------------
// Positive: create a complete tunnel, reset optional attrs (ttl-mode and
// dscp-mode), verify they become unset while the remaining attrs survive.
// ---------------------------------------------------------------------------

TEST_F(TunnelIpInIpTest, ResetOptionalAttrsOnCompleteTunnel) {
  int intfId = ensureUnderlayIntfId();
  const std::string dstIp = findIpv6OnIntf(intfId);
  const std::string tunnelId = "integration-test-tunnel-reset-optional";

  XLOG(INFO) << "[Step 1] Creating complete production-style tunnel "
             << tunnelId << "...";
  createCompleteTunnel(tunnelId, intfId, dstIp);

  XLOG(INFO) << "[Step 2] Resetting ttl-mode and dscp-mode...";
  {
    auto result = runCli(
        {"delete", "tunnel", "ip-in-ip", tunnelId, "ttl-mode", "dscp-mode"});
    ASSERT_EQ(result.exitCode, 0) << "Reset failed: " << result.stderr;
    commitConfig();
  }

  XLOG(INFO)
      << "[Step 3] Verifying ttl-mode/dscp-mode unset, other attrs intact...";
  auto& session = ConfigSession::getInstance();
  auto& swConfig = *session.getAgentConfig().sw();
  bool checked = false;
  for (const auto& tunnel : *swConfig.ipInIpTunnels()) {
    if (*tunnel.ipInIpTunnelId() != tunnelId) {
      continue;
    }
    EXPECT_FALSE(tunnel.ttlMode().has_value())
        << "ttl-mode should be unset after reset";
    EXPECT_FALSE(tunnel.dscpMode().has_value())
        << "dscp-mode should be unset after reset";
    ASSERT_TRUE(tunnel.ecnMode().has_value());
    EXPECT_EQ(*tunnel.ecnMode(), cfg::TunnelMode::PIPE);
    ASSERT_TRUE(tunnel.tunnelTermType().has_value());
    EXPECT_EQ(*tunnel.tunnelTermType(), cfg::TunnelTerminationType::P2MP);
    checked = true;
    break;
  }
  EXPECT_TRUE(checked) << "Tunnel '" << tunnelId << "' missing after reset";
}

// ---------------------------------------------------------------------------
// Positive: deleting a non-existent tunnel is idempotent (exit 0).
// ---------------------------------------------------------------------------

TEST_F(TunnelIpInIpTest, DeleteNonExistentIsIdempotent) {
  const std::string ghostId = "tunnel-that-does-not-exist-XYZ-12345";
  ASSERT_FALSE(tunnelExists(ghostId));

  auto result = runCli({"delete", "tunnel", "ip-in-ip", ghostId});
  EXPECT_EQ(result.exitCode, 0)
      << "Idempotent delete of non-existent tunnel should succeed: "
      << result.stderr;
}

// ---------------------------------------------------------------------------
// Negative: resetting a required field (destination) must be rejected.
// ---------------------------------------------------------------------------

TEST_F(TunnelIpInIpTest, ResetRequiredFieldRejected) {
  int intfId = ensureUnderlayIntfId();
  const std::string dstIp = findIpv6OnIntf(intfId);
  const std::string tunnelId = "integration-test-tunnel-reset-required";

  XLOG(INFO) << "[Setup] Creating complete tunnel " << tunnelId << "...";
  createCompleteTunnel(tunnelId, intfId, dstIp);

  XLOG(INFO)
      << "[Test] Attempting to reset required field 'destination' (must fail)...";
  auto result =
      runCli({"delete", "tunnel", "ip-in-ip", tunnelId, "destination"});
  EXPECT_NE(result.exitCode, 0)
      << "Expected non-zero exit when resetting required field";
}

// ---------------------------------------------------------------------------
// Negative: passing an unknown attribute must be rejected by the CLI.
// ---------------------------------------------------------------------------

TEST_F(TunnelIpInIpTest, UnknownAttrRejected) {
  int intfId = ensureUnderlayIntfId();
  const std::string dstIp = findIpv6OnIntf(intfId);
  const std::string tunnelId = "integration-test-tunnel-unknown-attr";

  XLOG(INFO) << "[Setup] Creating complete tunnel " << tunnelId << "...";
  createCompleteTunnel(tunnelId, intfId, dstIp);

  auto result =
      runCli({"delete", "tunnel", "ip-in-ip", tunnelId, "bogus-attr"});
  EXPECT_NE(result.exitCode, 0)
      << "Expected non-zero exit for unknown tunnel attribute";
}
