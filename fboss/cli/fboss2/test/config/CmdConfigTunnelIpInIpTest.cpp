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

#include "fboss/cli/fboss2/commands/config/tunnel/ip_in_ip/CmdConfigTunnelIpInIp.h"
#include "fboss/cli/fboss2/session/ConfigSession.h"
#include "fboss/cli/fboss2/test/config/CmdConfigTestBase.h"

using namespace ::testing;

namespace facebook::fboss {

class CmdConfigTunnelIpInIpTestFixture : public CmdConfigTestBase {
 public:
  CmdConfigTunnelIpInIpTestFixture()
      : CmdConfigTestBase(
            "fboss_tunnel_ipinip_test_%%%%-%%%%-%%%%-%%%%",
            R"({
  "sw": {
    "interfaces": [
      {
        "intfID": 1,
        "routerID": 0,
        "vlanID": 1,
        "name": "eth1/1/1",
        "mtu": 1500
      },
      {
        "intfID": 2,
        "routerID": 0,
        "vlanID": 2,
        "name": "eth1/2/1",
        "mtu": 1500
      }
    ],
    "ipInIpTunnels": [
      {
        "ipInIpTunnelId": "tunnel0",
        "underlayIntfID": 1,
        "dstIp": "2001:db8::1",
        "srcIp": "2001:db8::2"
      }
    ]
  }
})") {}

  void SetUp() override {
    CmdConfigTestBase::SetUp();
    setupTestableConfigSession();
  }
};

// ============================================================================
// TunnelIpInIpConfig arg-type parser tests
// ============================================================================

TEST_F(CmdConfigTunnelIpInIpTestFixture, configBareIdOnly) {
  TunnelIpInIpConfig cfg({"tunnel0"});
  EXPECT_EQ(cfg.getTunnelId(), "tunnel0");
  EXPECT_FALSE(cfg.hasAttrs());
}

TEST_F(CmdConfigTunnelIpInIpTestFixture, configEmptyThrows) {
  EXPECT_THROW(TunnelIpInIpConfig({}), std::invalid_argument);
}

TEST_F(CmdConfigTunnelIpInIpTestFixture, configAttrAsFirstTokenThrows) {
  EXPECT_THROW(TunnelIpInIpConfig({"destination"}), std::invalid_argument);
}

TEST_F(CmdConfigTunnelIpInIpTestFixture, configMaskAsFirstTokenThrows) {
  // 'mask' is a sub-keyword of destination/source — never a valid tunnel ID.
  EXPECT_THROW(
      TunnelIpInIpConfig({"mask", "2001:db8::1"}), std::invalid_argument);
}

TEST_F(CmdConfigTunnelIpInIpTestFixture, configCaseInsensitiveAttrs) {
  TunnelIpInIpConfig cfg(
      {"Tunnel0", "TTL-Mode", "UNIFORM", "Termination-Type", "P2P"});
  EXPECT_EQ(cfg.getTunnelId(), "Tunnel0"); // ID case is preserved
  EXPECT_EQ(cfg.getAttrs().at("ttl-mode"), "uniform");
  EXPECT_EQ(cfg.getAttrs().at("termination-type"), "p2p");
}

TEST_F(CmdConfigTunnelIpInIpTestFixture, configUnknownAttrThrows) {
  EXPECT_THROW(
      TunnelIpInIpConfig({"tunnel0", "bogus-attr", "val"}),
      std::invalid_argument);
}

TEST_F(CmdConfigTunnelIpInIpTestFixture, configMissingValueThrows) {
  EXPECT_THROW(
      TunnelIpInIpConfig({"tunnel0", "ttl-mode"}), std::invalid_argument);
}

TEST_F(CmdConfigTunnelIpInIpTestFixture, configDestinationIpv6Only) {
  TunnelIpInIpConfig cfg({"tunnel0", "destination", "2001:db8::1"});
  ASSERT_EQ(cfg.getAttrs().size(), 1u);
  EXPECT_EQ(cfg.getAttrs().at("destination"), "2001:db8::1");
}

TEST_F(CmdConfigTunnelIpInIpTestFixture, configDestinationWithPrefixLen) {
  TunnelIpInIpConfig cfg(
      {"tunnel0", "destination", "2001:db8::", "mask", "32"});
  ASSERT_EQ(cfg.getAttrs().size(), 2u);
  EXPECT_EQ(cfg.getAttrs().at("destination"), "2001:db8::");
  EXPECT_EQ(cfg.getAttrs().at("destination-mask"), "32");
}

TEST_F(CmdConfigTunnelIpInIpTestFixture, configDestinationBadIpThrows) {
  EXPECT_THROW(
      TunnelIpInIpConfig({"tunnel0", "destination", "not-an-ip"}),
      std::invalid_argument);
}

TEST_F(CmdConfigTunnelIpInIpTestFixture, configDestinationIpv4Rejected) {
  EXPECT_THROW(
      TunnelIpInIpConfig({"tunnel0", "destination", "10.0.0.1"}),
      std::invalid_argument);
}

TEST_F(CmdConfigTunnelIpInIpTestFixture, configSourceIpv4Rejected) {
  EXPECT_THROW(
      TunnelIpInIpConfig({"tunnel0", "source", "192.168.1.1"}),
      std::invalid_argument);
}

TEST_F(CmdConfigTunnelIpInIpTestFixture, configSourceWithPrefixLen) {
  TunnelIpInIpConfig cfg({"tunnel0", "source", "2001:db8::1", "mask", "64"});
  ASSERT_EQ(cfg.getAttrs().size(), 2u);
  EXPECT_EQ(cfg.getAttrs().at("source"), "2001:db8::1");
  EXPECT_EQ(cfg.getAttrs().at("source-mask"), "64");
}

TEST_F(CmdConfigTunnelIpInIpTestFixture, configPrefixLenOutOfRangeThrows) {
  EXPECT_THROW(
      TunnelIpInIpConfig(
          {"tunnel0", "destination", "2001:db8::", "mask", "129"}),
      std::invalid_argument);
}

TEST_F(CmdConfigTunnelIpInIpTestFixture, configPrefixLenNegativeThrows) {
  EXPECT_THROW(
      TunnelIpInIpConfig(
          {"tunnel0", "destination", "2001:db8::", "mask", "-1"}),
      std::invalid_argument);
}

TEST_F(CmdConfigTunnelIpInIpTestFixture, configPrefixLenNonIntThrows) {
  EXPECT_THROW(
      TunnelIpInIpConfig(
          {"tunnel0", "destination", "2001:db8::", "mask", "abc"}),
      std::invalid_argument);
}

TEST_F(CmdConfigTunnelIpInIpTestFixture, configSourceBadMaskKeywordThrows) {
  EXPECT_THROW(
      TunnelIpInIpConfig({"tunnel0", "source", "2001:db8::1", "prefix", "64"}),
      std::invalid_argument);
}

TEST_F(CmdConfigTunnelIpInIpTestFixture, configTtlModeUniform) {
  TunnelIpInIpConfig cfg({"tunnel0", "ttl-mode", "uniform"});
  ASSERT_EQ(cfg.getAttrs().size(), 1u);
  EXPECT_EQ(cfg.getAttrs().at("ttl-mode"), "uniform");
}

TEST_F(CmdConfigTunnelIpInIpTestFixture, configTtlModeInvalidThrows) {
  EXPECT_THROW(
      TunnelIpInIpConfig({"tunnel0", "ttl-mode", "user"}),
      std::invalid_argument);
}

TEST_F(CmdConfigTunnelIpInIpTestFixture, configDscpModePipe) {
  TunnelIpInIpConfig cfg({"tunnel0", "dscp-mode", "pipe"});
  EXPECT_EQ(cfg.getAttrs().at("dscp-mode"), "pipe");
}

TEST_F(CmdConfigTunnelIpInIpTestFixture, configEcnModePipe) {
  TunnelIpInIpConfig cfg({"tunnel0", "ecn-mode", "pipe"});
  EXPECT_EQ(cfg.getAttrs().at("ecn-mode"), "pipe");
}

TEST_F(CmdConfigTunnelIpInIpTestFixture, configTerminationTypeP2p) {
  TunnelIpInIpConfig cfg({"tunnel0", "termination-type", "p2p"});
  EXPECT_EQ(cfg.getAttrs().at("termination-type"), "p2p");
}

TEST_F(CmdConfigTunnelIpInIpTestFixture, configTerminationTypeInvalidThrows) {
  EXPECT_THROW(
      TunnelIpInIpConfig({"tunnel0", "termination-type", "p2p2p"}),
      std::invalid_argument);
}

TEST_F(CmdConfigTunnelIpInIpTestFixture, configMultiAttrInOneInvocation) {
  TunnelIpInIpConfig cfg(
      {"tunnel0", "destination", "2001:db8::1", "ttl-mode", "pipe"});
  ASSERT_EQ(cfg.getAttrs().size(), 2u);
  EXPECT_EQ(cfg.getAttrs().at("destination"), "2001:db8::1");
  EXPECT_EQ(cfg.getAttrs().at("ttl-mode"), "pipe");
}

// ============================================================================
// CmdConfigTunnelIpInIp::queryClient tests
// ============================================================================

TEST_F(CmdConfigTunnelIpInIpTestFixture, setDestinationSuccess) {
  CmdConfigTunnelIpInIp cmd;
  TunnelIpInIpConfig cfg({"tunnel0", "destination", "2001:db8::2"});

  auto result = cmd.queryClient(localhost(), cfg);

  EXPECT_THAT(result, HasSubstr("tunnel0"));
  EXPECT_THAT(result, HasSubstr("2001:db8::2"));

  auto& swConfig = *ConfigSession::getInstance().getAgentConfig().sw();
  for (const auto& t : *swConfig.ipInIpTunnels()) {
    if (*t.ipInIpTunnelId() == "tunnel0") {
      EXPECT_EQ(*t.dstIp(), "2001:db8::2");
      break;
    }
  }
}

TEST_F(CmdConfigTunnelIpInIpTestFixture, setDestinationWithPrefixLenSuccess) {
  CmdConfigTunnelIpInIp cmd;
  TunnelIpInIpConfig cfg(
      {"tunnel0", "destination", "2001:db8::", "mask", "32"});

  auto result = cmd.queryClient(localhost(), cfg);

  EXPECT_THAT(result, HasSubstr("2001:db8::"));
  EXPECT_THAT(result, HasSubstr("prefix-len=32"));
}

TEST_F(CmdConfigTunnelIpInIpTestFixture, setSourceIpv6Success) {
  CmdConfigTunnelIpInIp cmd;
  TunnelIpInIpConfig cfg({"tunnel0", "source", "2001:db8::3"});

  auto result = cmd.queryClient(localhost(), cfg);

  EXPECT_THAT(result, HasSubstr("2001:db8::3"));
  EXPECT_THAT(result, HasSubstr("tunnel0"));

  auto& swConfig = *ConfigSession::getInstance().getAgentConfig().sw();
  for (const auto& t : *swConfig.ipInIpTunnels()) {
    if (*t.ipInIpTunnelId() == "tunnel0") {
      EXPECT_EQ(*t.srcIp(), "2001:db8::3");
      break;
    }
  }
}

TEST_F(CmdConfigTunnelIpInIpTestFixture, setSourceWithPrefixLenSuccess) {
  CmdConfigTunnelIpInIp cmd;
  TunnelIpInIpConfig cfg({"tunnel0", "source", "2001:db8::1", "mask", "64"});

  auto result = cmd.queryClient(localhost(), cfg);
  EXPECT_THAT(result, HasSubstr("2001:db8::1"));
  EXPECT_THAT(result, HasSubstr("prefix-len=64"));
}

TEST_F(CmdConfigTunnelIpInIpTestFixture, setTtlModeUniform) {
  CmdConfigTunnelIpInIp cmd;
  TunnelIpInIpConfig cfg({"tunnel0", "ttl-mode", "uniform"});

  auto result = cmd.queryClient(localhost(), cfg);
  EXPECT_THAT(result, HasSubstr("ttl-mode=uniform"));

  auto& swConfig = *ConfigSession::getInstance().getAgentConfig().sw();
  for (const auto& t : *swConfig.ipInIpTunnels()) {
    if (*t.ipInIpTunnelId() == "tunnel0") {
      ASSERT_TRUE(t.ttlMode().has_value());
      EXPECT_EQ(*t.ttlMode(), cfg::TunnelMode::UNIFORM);
      break;
    }
  }
}

TEST_F(CmdConfigTunnelIpInIpTestFixture, setTtlModePipe) {
  CmdConfigTunnelIpInIp cmd;
  TunnelIpInIpConfig cfg({"tunnel0", "ttl-mode", "pipe"});

  auto result = cmd.queryClient(localhost(), cfg);
  EXPECT_THAT(result, HasSubstr("ttl-mode=pipe"));
}

TEST_F(CmdConfigTunnelIpInIpTestFixture, setDscpModeUniform) {
  CmdConfigTunnelIpInIp cmd;
  TunnelIpInIpConfig cfg({"tunnel0", "dscp-mode", "uniform"});

  auto result = cmd.queryClient(localhost(), cfg);
  EXPECT_THAT(result, HasSubstr("dscp-mode=uniform"));
}

TEST_F(CmdConfigTunnelIpInIpTestFixture, setEcnModePipe) {
  CmdConfigTunnelIpInIp cmd;
  TunnelIpInIpConfig cfg({"tunnel0", "ecn-mode", "pipe"});

  auto result = cmd.queryClient(localhost(), cfg);
  EXPECT_THAT(result, HasSubstr("ecn-mode=pipe"));
}

TEST_F(CmdConfigTunnelIpInIpTestFixture, setTerminationTypeP2p) {
  CmdConfigTunnelIpInIp cmd;
  TunnelIpInIpConfig cfg({"tunnel0", "termination-type", "p2p"});

  auto result = cmd.queryClient(localhost(), cfg);
  EXPECT_THAT(result, HasSubstr("termination-type=p2p"));
  EXPECT_THAT(result, HasSubstr("tunnel0"));

  auto& swConfig = *ConfigSession::getInstance().getAgentConfig().sw();
  for (const auto& t : *swConfig.ipInIpTunnels()) {
    if (*t.ipInIpTunnelId() == "tunnel0") {
      ASSERT_TRUE(t.tunnelTermType().has_value());
      EXPECT_EQ(*t.tunnelTermType(), cfg::TunnelTerminationType::P2P);
      break;
    }
  }
}

TEST_F(CmdConfigTunnelIpInIpTestFixture, setTerminationTypeP2mp) {
  CmdConfigTunnelIpInIp cmd;
  TunnelIpInIpConfig cfg({"tunnel0", "termination-type", "p2mp"});
  auto result = cmd.queryClient(localhost(), cfg);
  EXPECT_THAT(result, HasSubstr("p2mp"));
}

TEST_F(CmdConfigTunnelIpInIpTestFixture, setTerminationTypeMp2p) {
  CmdConfigTunnelIpInIp cmd;
  TunnelIpInIpConfig cfg({"tunnel0", "termination-type", "mp2p"});
  auto result = cmd.queryClient(localhost(), cfg);
  EXPECT_THAT(result, HasSubstr("mp2p"));
}

TEST_F(CmdConfigTunnelIpInIpTestFixture, setTerminationTypeMp2mp) {
  CmdConfigTunnelIpInIp cmd;
  TunnelIpInIpConfig cfg({"tunnel0", "termination-type", "mp2mp"});
  auto result = cmd.queryClient(localhost(), cfg);
  EXPECT_THAT(result, HasSubstr("mp2mp"));
}

TEST_F(CmdConfigTunnelIpInIpTestFixture, multiAttrInOneInvocation) {
  CmdConfigTunnelIpInIp cmd;
  TunnelIpInIpConfig cfg(
      {"tunnel0", "destination", "2001:db8::1", "ttl-mode", "pipe"});

  auto result = cmd.queryClient(localhost(), cfg);
  EXPECT_THAT(result, HasSubstr("destination=2001:db8::1"));
  EXPECT_THAT(result, HasSubstr("ttl-mode=pipe"));
}

TEST_F(CmdConfigTunnelIpInIpTestFixture, autoCreateBareId) {
  CmdConfigTunnelIpInIp cmd;
  TunnelIpInIpConfig cfg({"new-tunnel"});

  auto result = cmd.queryClient(localhost(), cfg);
  EXPECT_THAT(result, HasSubstr("new-tunnel"));

  auto& swConfig = *ConfigSession::getInstance().getAgentConfig().sw();
  bool found = false;
  for (const auto& t : *swConfig.ipInIpTunnels()) {
    if (*t.ipInIpTunnelId() == "new-tunnel") {
      found = true;
      EXPECT_EQ(*t.tunnelType(), ::facebook::fboss::TunnelType::IP_IN_IP);
      break;
    }
  }
  EXPECT_TRUE(found);
}

TEST_F(CmdConfigTunnelIpInIpTestFixture, autoCreateWithAttrs) {
  CmdConfigTunnelIpInIp cmd;
  TunnelIpInIpConfig cfg({"brand-new", "ttl-mode", "uniform"});

  auto result = cmd.queryClient(localhost(), cfg);
  EXPECT_THAT(result, HasSubstr("brand-new"));
  EXPECT_THAT(result, HasSubstr("ttl-mode=uniform"));

  auto& swConfig = *ConfigSession::getInstance().getAgentConfig().sw();
  bool found = false;
  for (const auto& t : *swConfig.ipInIpTunnels()) {
    if (*t.ipInIpTunnelId() == "brand-new") {
      found = true;
      ASSERT_TRUE(t.ttlMode().has_value());
      EXPECT_EQ(*t.ttlMode(), cfg::TunnelMode::UNIFORM);
      break;
    }
  }
  EXPECT_TRUE(found);
}

TEST_F(CmdConfigTunnelIpInIpTestFixture, configUnderlayIntfId) {
  TunnelIpInIpConfig cfg({"tunnel0", "underlay-intf-id", "10"});
  EXPECT_EQ(cfg.getAttrs().at("underlay-intf-id"), "10");
}

TEST_F(CmdConfigTunnelIpInIpTestFixture, configUnderlayIntfIdInvalidThrows) {
  EXPECT_THROW(
      TunnelIpInIpConfig({"tunnel0", "underlay-intf-id", "notanint"}),
      std::invalid_argument);
}

TEST_F(CmdConfigTunnelIpInIpTestFixture, setUnderlayIntfId) {
  CmdConfigTunnelIpInIp cmd;
  // intfID 2 exists in the test fixture config
  TunnelIpInIpConfig cfg({"tunnel0", "underlay-intf-id", "2"});
  auto result = cmd.queryClient(localhost(), cfg);
  EXPECT_THAT(result, HasSubstr("underlay-intf-id=2"));

  auto& swConfig = *ConfigSession::getInstance().getAgentConfig().sw();
  for (const auto& t : *swConfig.ipInIpTunnels()) {
    if (*t.ipInIpTunnelId() == "tunnel0") {
      ASSERT_TRUE(t.underlayIntfID().has_value());
      EXPECT_EQ(*t.underlayIntfID(), 2);
      break;
    }
  }
}

TEST_F(CmdConfigTunnelIpInIpTestFixture, setUnderlayIntfIdNonExistentThrows) {
  CmdConfigTunnelIpInIp cmd;
  // intfID 999 does not exist in the test fixture config
  TunnelIpInIpConfig cfg({"tunnel0", "underlay-intf-id", "999"});
  EXPECT_THROW(cmd.queryClient(localhost(), cfg), std::invalid_argument);
}

// Verify that auto-creating a tunnel without setting underlay-intf-id leaves
// underlayIntfID at 0, which is detected by the commit validation.
TEST_F(
    CmdConfigTunnelIpInIpTestFixture,
    newTunnelWithoutUnderlayIntfIdHasZero) {
  CmdConfigTunnelIpInIp cmd;
  TunnelIpInIpConfig cfg({"brand-new-no-intf"});
  cmd.queryClient(localhost(), cfg);

  auto& swConfig = *ConfigSession::getInstance().getAgentConfig().sw();
  for (const auto& t : *swConfig.ipInIpTunnels()) {
    if (*t.ipInIpTunnelId() == "brand-new-no-intf") {
      EXPECT_EQ(*t.underlayIntfID(), 0);
      break;
    }
  }
}

TEST_F(CmdConfigTunnelIpInIpTestFixture, idempotentReInvocation) {
  CmdConfigTunnelIpInIp cmd;

  TunnelIpInIpConfig cfg1({"tunnel0", "ttl-mode", "uniform"});
  cmd.queryClient(localhost(), cfg1);

  TunnelIpInIpConfig cfg2({"tunnel0", "ttl-mode", "pipe"});
  auto result = cmd.queryClient(localhost(), cfg2);

  EXPECT_THAT(result, HasSubstr("ttl-mode=pipe"));
  auto& swConfig = *ConfigSession::getInstance().getAgentConfig().sw();
  for (const auto& t : *swConfig.ipInIpTunnels()) {
    if (*t.ipInIpTunnelId() == "tunnel0") {
      ASSERT_TRUE(t.ttlMode().has_value());
      EXPECT_EQ(*t.ttlMode(), cfg::TunnelMode::PIPE);
      break;
    }
  }
}

} // namespace facebook::fboss
