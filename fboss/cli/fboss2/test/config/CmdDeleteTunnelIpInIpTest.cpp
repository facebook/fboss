/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "fboss/cli/fboss2/commands/delete/tunnel/ip_in_ip/CmdDeleteTunnelIpInIp.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/test/config/CmdConfigTestBase.h"

using namespace ::testing;

namespace facebook::fboss {

// ============================================================================
// Test fixture — agent config has one tunnel with all attrs populated.
// ============================================================================

class CmdDeleteTunnelIpInIpTestFixture : public CmdConfigTestBase {
 public:
  CmdDeleteTunnelIpInIpTestFixture()
      : CmdConfigTestBase(
            "fboss_delete_tunnel_ipinip_test_%%%%-%%%%-%%%%-%%%%",
            R"({
  "sw": {
    "ipInIpTunnels": [
      {
        "ipInIpTunnelId": "tunnel0",
        "underlayIntfID": 1,
        "dstIp": "2001:db8::1",
        "dstIpMask": "64",
        "srcIp": "2001:db8::2",
        "srcIpMask": "48",
        "ttlMode": 1,
        "dscpMode": 0,
        "ecnMode": 1,
        "tunnelTermType": 0
      },
      {
        "ipInIpTunnelId": "tunnel1",
        "underlayIntfID": 2,
        "dstIp": "2001:db8::3"
      }
    ]
  }
})") {}

  void SetUp() override {
    CmdConfigTestBase::SetUp();
    setupTestableConfigSession();
  }

 protected:
  // Look up tunnel by id from current agent config.
  const cfg::IpInIpTunnel* findTunnel(const std::string& id) {
    auto& swConfig = *ConfigSession::getInstance().getAgentConfig().sw();
    if (!swConfig.ipInIpTunnels().has_value()) {
      return nullptr;
    }
    for (const auto& t : *swConfig.ipInIpTunnels()) {
      if (*t.ipInIpTunnelId() == id) {
        return &t;
      }
    }
    return nullptr;
  }
};

// ============================================================================
// TunnelIpInIpDeleteArgs parser tests
// ============================================================================

TEST_F(CmdDeleteTunnelIpInIpTestFixture, deleteArgsEmptyThrows) {
  EXPECT_THROW(TunnelIpInIpDeleteArgs({}), std::invalid_argument);
}

TEST_F(CmdDeleteTunnelIpInIpTestFixture, deleteArgsAttrAsFirstTokenThrows) {
  // First token is the tunnel ID; using an attribute name there is rejected.
  EXPECT_THROW(TunnelIpInIpDeleteArgs({"ttl-mode"}), std::invalid_argument);
}

TEST_F(CmdDeleteTunnelIpInIpTestFixture, deleteArgsBareIdMeansDeleteWhole) {
  TunnelIpInIpDeleteArgs args({"tunnel0"});
  EXPECT_EQ(args.getTunnelId(), "tunnel0");
  EXPECT_TRUE(args.deleteEntireTunnel());
  EXPECT_TRUE(args.getAttributes().empty());
}

TEST_F(CmdDeleteTunnelIpInIpTestFixture, deleteArgsUnknownAttrThrows) {
  try {
    TunnelIpInIpDeleteArgs args({"tunnel0", "bogus"});
    FAIL() << "Expected std::invalid_argument";
  } catch (const std::invalid_argument& e) {
    EXPECT_THAT(e.what(), HasSubstr("Unknown tunnel attribute"));
    EXPECT_THAT(e.what(), HasSubstr("bogus"));
  }
}

TEST_F(CmdDeleteTunnelIpInIpTestFixture, deleteArgsRequiredFieldRejected) {
  try {
    TunnelIpInIpDeleteArgs args({"tunnel0", "destination"});
    FAIL() << "Expected std::invalid_argument";
  } catch (const std::invalid_argument& e) {
    EXPECT_THAT(e.what(), HasSubstr("Cannot reset required field"));
  }
}

TEST_F(CmdDeleteTunnelIpInIpTestFixture, deleteArgsUnderlayIntfIdRejected) {
  EXPECT_THROW(
      TunnelIpInIpDeleteArgs({"tunnel0", "underlay-intf-id"}),
      std::invalid_argument);
}

TEST_F(CmdDeleteTunnelIpInIpTestFixture, deleteArgsCaseInsensitive) {
  TunnelIpInIpDeleteArgs args({"tunnel0", "TTL-Mode"});
  ASSERT_EQ(args.getAttributes().size(), 1u);
  EXPECT_EQ(args.getAttributes()[0], "ttl-mode");
}

TEST_F(CmdDeleteTunnelIpInIpTestFixture, deleteArgsMultipleAttrs) {
  TunnelIpInIpDeleteArgs args({"tunnel0", "ttl-mode", "dscp-mode", "ecn-mode"});
  ASSERT_EQ(args.getAttributes().size(), 3u);
  EXPECT_EQ(args.getAttributes()[0], "ttl-mode");
  EXPECT_EQ(args.getAttributes()[1], "dscp-mode");
  EXPECT_EQ(args.getAttributes()[2], "ecn-mode");
  EXPECT_FALSE(args.deleteEntireTunnel());
}

// ============================================================================
// CmdDeleteTunnelIpInIp::queryClient — per-attribute reset tests
// ============================================================================

TEST_F(CmdDeleteTunnelIpInIpTestFixture, resetSource) {
  CmdDeleteTunnelIpInIp cmd;
  TunnelIpInIpDeleteArgs args({"tunnel0", "source"});

  auto result = cmd.queryClient(localhost(), args);

  EXPECT_THAT(result, HasSubstr("Successfully reset"));
  EXPECT_THAT(result, HasSubstr("tunnel0"));

  const auto* t = findTunnel("tunnel0");
  ASSERT_NE(t, nullptr);
  EXPECT_FALSE(t->srcIp().has_value());
  EXPECT_FALSE(t->srcIpMask().has_value());
  // Destination must remain unchanged.
  EXPECT_EQ(*t->dstIp(), "2001:db8::1");
}

TEST_F(CmdDeleteTunnelIpInIpTestFixture, resetTtlMode) {
  CmdDeleteTunnelIpInIp cmd;
  TunnelIpInIpDeleteArgs args({"tunnel0", "ttl-mode"});

  auto result = cmd.queryClient(localhost(), args);

  EXPECT_THAT(result, HasSubstr("Successfully reset"));
  const auto* t = findTunnel("tunnel0");
  ASSERT_NE(t, nullptr);
  EXPECT_FALSE(t->ttlMode().has_value());
  // Other modes untouched.
  EXPECT_TRUE(t->dscpMode().has_value());
  EXPECT_TRUE(t->ecnMode().has_value());
}

TEST_F(CmdDeleteTunnelIpInIpTestFixture, resetDscpMode) {
  CmdDeleteTunnelIpInIp cmd;
  TunnelIpInIpDeleteArgs args({"tunnel0", "dscp-mode"});
  cmd.queryClient(localhost(), args);

  const auto* t = findTunnel("tunnel0");
  ASSERT_NE(t, nullptr);
  EXPECT_FALSE(t->dscpMode().has_value());
}

TEST_F(CmdDeleteTunnelIpInIpTestFixture, resetEcnMode) {
  CmdDeleteTunnelIpInIp cmd;
  TunnelIpInIpDeleteArgs args({"tunnel0", "ecn-mode"});
  cmd.queryClient(localhost(), args);

  const auto* t = findTunnel("tunnel0");
  ASSERT_NE(t, nullptr);
  EXPECT_FALSE(t->ecnMode().has_value());
}

TEST_F(CmdDeleteTunnelIpInIpTestFixture, resetTerminationType) {
  CmdDeleteTunnelIpInIp cmd;
  TunnelIpInIpDeleteArgs args({"tunnel0", "termination-type"});
  cmd.queryClient(localhost(), args);

  const auto* t = findTunnel("tunnel0");
  ASSERT_NE(t, nullptr);
  EXPECT_FALSE(t->tunnelTermType().has_value());
}

TEST_F(CmdDeleteTunnelIpInIpTestFixture, resetMultipleAttrs) {
  CmdDeleteTunnelIpInIp cmd;
  TunnelIpInIpDeleteArgs args(
      {"tunnel0", "ttl-mode", "dscp-mode", "termination-type"});

  auto result = cmd.queryClient(localhost(), args);

  EXPECT_THAT(result, HasSubstr("Successfully reset"));

  const auto* t = findTunnel("tunnel0");
  ASSERT_NE(t, nullptr);
  EXPECT_FALSE(t->ttlMode().has_value());
  EXPECT_FALSE(t->dscpMode().has_value());
  EXPECT_FALSE(t->tunnelTermType().has_value());
  // ecnMode untouched.
  EXPECT_TRUE(t->ecnMode().has_value());
}

// ============================================================================
// queryClient — delete-entire-tunnel tests
// ============================================================================

TEST_F(CmdDeleteTunnelIpInIpTestFixture, deleteEntireTunnel) {
  CmdDeleteTunnelIpInIp cmd;
  TunnelIpInIpDeleteArgs args({"tunnel0"});

  auto result = cmd.queryClient(localhost(), args);

  EXPECT_THAT(result, HasSubstr("Successfully deleted"));
  EXPECT_THAT(result, HasSubstr("tunnel0"));

  EXPECT_EQ(findTunnel("tunnel0"), nullptr);
  // Other tunnels remain.
  EXPECT_NE(findTunnel("tunnel1"), nullptr);
}

// ============================================================================
// queryClient — idempotent / not-found tests
// ============================================================================

TEST_F(CmdDeleteTunnelIpInIpTestFixture, deleteUnknownTunnelIsIdempotent) {
  CmdDeleteTunnelIpInIp cmd;
  TunnelIpInIpDeleteArgs args({"does-not-exist"});

  auto result = cmd.queryClient(localhost(), args);

  EXPECT_THAT(result, HasSubstr("not found"));
  // Existing tunnels still there.
  EXPECT_NE(findTunnel("tunnel0"), nullptr);
}

TEST_F(
    CmdDeleteTunnelIpInIpTestFixture,
    resetAttrsOnUnknownTunnelIsIdempotent) {
  CmdDeleteTunnelIpInIp cmd;
  TunnelIpInIpDeleteArgs args({"does-not-exist", "ttl-mode"});

  EXPECT_NO_THROW({
    auto result = cmd.queryClient(localhost(), args);
    EXPECT_THAT(result, HasSubstr("not found"));
  });
}

TEST_F(CmdDeleteTunnelIpInIpTestFixture, resetAttrAlreadyUnsetIsNoOp) {
  // tunnel1 has no ttl-mode set. Resetting it should silently succeed.
  CmdDeleteTunnelIpInIp cmd;
  TunnelIpInIpDeleteArgs args({"tunnel1", "ttl-mode"});

  auto result = cmd.queryClient(localhost(), args);
  EXPECT_THAT(result, HasSubstr("Successfully reset"));

  const auto* t = findTunnel("tunnel1");
  ASSERT_NE(t, nullptr);
  EXPECT_FALSE(t->ttlMode().has_value());
}

// ============================================================================
// printOutput
// ============================================================================

TEST_F(CmdDeleteTunnelIpInIpTestFixture, printOutput) {
  CmdDeleteTunnelIpInIp cmd;
  testing::internal::CaptureStdout();
  cmd.printOutput("test output");
  std::string output = testing::internal::GetCapturedStdout();
  EXPECT_THAT(output, HasSubstr("test output"));
}

} // namespace facebook::fboss
