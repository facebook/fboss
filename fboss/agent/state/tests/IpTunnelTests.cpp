// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>
#include "fboss/agent/ApplyThriftConfig.h"
#include "fboss/agent/state/IpTunnel.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/TestUtils.h"
#include "folly/IPAddressV6.h"

using namespace facebook::fboss;

namespace {
HwSwitchMatcher scope() {
  return HwSwitchMatcher{std::unordered_set<SwitchID>{SwitchID(10)}};
}
} // namespace

std::shared_ptr<IpTunnel> makeTunnel(const std::string& tunnelId = "tunnel0") {
  auto tunnel = std::make_shared<IpTunnel>(tunnelId);
  tunnel->setType(TunnelType::IP_IN_IP);
  tunnel->setUnderlayIntfId(InterfaceID(42));
  tunnel->setTTLMode(cfg::TunnelMode::PIPE);
  tunnel->setDscpMode(cfg::TunnelMode::PIPE);
  tunnel->setEcnMode(cfg::TunnelMode::PIPE);
  tunnel->setTunnelTermType(cfg::TunnelTerminationType::MP2MP);
  tunnel->setDstIP(folly::IPAddressV6("2401:db00:11c:8202:0:0:0:100"));
  tunnel->setSrcIP(folly::IPAddressV6("::"));
  tunnel->setDstIPMask(folly::IPAddressV6("::"));
  tunnel->setSrcIPMask(folly::IPAddressV6("::"));
  return tunnel;
}

TEST(Tunnel, SerDeserTunnel) {
  auto tunn = makeTunnel();
  auto serialized = tunn->toThrift();
  auto tunnBack = std::make_shared<IpTunnel>(serialized);
  EXPECT_TRUE(*tunn == *tunnBack);
}

TEST(Tunnel, SerDeserSwitchState) {
  auto state = std::make_shared<SwitchState>();

  auto tunnel0 = makeTunnel("tunnel0");
  auto tunnel1 = makeTunnel("tunnel1");
  auto tunnels = state->getTunnels()->modify(&state);

  tunnels->addNode(tunnel0, scope());
  tunnels->addNode(tunnel1, scope());

  auto serialized = state->toThrift();
  auto stateBack = SwitchState::fromThrift(serialized);

  for (auto tunnelID : {"tunnel0", "tunnel1"}) {
    EXPECT_TRUE(
        *state->getTunnels()->getNode(tunnelID) ==
        *stateBack->getTunnels()->getNode(tunnelID));
  }
}

TEST(Tunnel, AddRemove) {
  auto state = std::make_shared<SwitchState>();

  auto tunnel0 = makeTunnel("tunnel0");
  auto tunnel1 = makeTunnel("tunnel1");

  auto tunnels = state->getTunnels()->modify(&state);
  tunnels->addNode(tunnel0, scope());
  tunnels->addNode(tunnel1, scope());
  tunnels->removeNode("tunnel0");
  EXPECT_EQ(state->getTunnels()->getNodeIf("tunnel0"), nullptr);
  EXPECT_NE(state->getTunnels()->getNodeIf("tunnel1"), nullptr);
}

TEST(Tunnel, ApplyConfigDecapSwapsIps) {
  auto platform = createMockPlatform();
  auto stateV0 = std::make_shared<SwitchState>();
  addSwitchInfo(stateV0);

  auto config = testConfigA();

  cfg::IpInIpTunnel tunnelCfg;
  tunnelCfg.ipInIpTunnelId() = "decapTunnel";
  tunnelCfg.underlayIntfID() = 1;
  tunnelCfg.dstIp() = "2401:db00::1";
  tunnelCfg.srcIp() = "2401:db00::2";
  tunnelCfg.tunnelType() = TunnelType::IP_IN_IP;
  tunnelCfg.tunnelTermType() = cfg::TunnelTerminationType::P2P;
  tunnelCfg.ttlMode() = cfg::TunnelMode::PIPE;
  tunnelCfg.dscpMode() = cfg::TunnelMode::PIPE;
  tunnelCfg.ecnMode() = cfg::TunnelMode::PIPE;
  config.ipInIpTunnels() = {tunnelCfg};

  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  ASSERT_NE(nullptr, stateV1);

  auto tunnel = stateV1->getTunnels()->getNodeIf("decapTunnel");
  ASSERT_NE(nullptr, tunnel);
  // Decap swaps: config dstIp -> state srcIP, config srcIp -> state dstIP
  EXPECT_EQ(tunnel->getSrcIP(), folly::IPAddressV6("2401:db00::1"));
  EXPECT_EQ(tunnel->getDstIP(), folly::IPAddressV6("2401:db00::2"));
}

TEST(Tunnel, ApplyConfigEncapNoSwap) {
  auto platform = createMockPlatform();
  auto stateV0 = std::make_shared<SwitchState>();
  addSwitchInfo(stateV0);

  auto config = testConfigA();

  cfg::IpInIpTunnel tunnelCfg;
  tunnelCfg.ipInIpTunnelId() = "encapTunnel";
  tunnelCfg.underlayIntfID() = 1;
  tunnelCfg.srcIp() = "2401:db00::1";
  tunnelCfg.dstIp() = "2401:db00::2";
  tunnelCfg.tunnelType() = TunnelType::IP_IN_IP_ENCAP;
  tunnelCfg.tunnelTermType() = cfg::TunnelTerminationType::P2P;
  tunnelCfg.ttlMode() = cfg::TunnelMode::PIPE;
  tunnelCfg.dscpMode() = cfg::TunnelMode::PIPE;
  tunnelCfg.ecnMode() = cfg::TunnelMode::UNIFORM;
  config.ipInIpTunnels() = {tunnelCfg};

  auto stateV1 = publishAndApplyConfig(stateV0, &config, platform.get());
  ASSERT_NE(nullptr, stateV1);

  auto tunnel = stateV1->getTunnels()->getNodeIf("encapTunnel");
  ASSERT_NE(nullptr, tunnel);
  // Encap: no swap, config srcIp -> state srcIP, config dstIp -> state dstIP
  EXPECT_EQ(tunnel->getSrcIP(), folly::IPAddressV6("2401:db00::1"));
  EXPECT_EQ(tunnel->getDstIP(), folly::IPAddressV6("2401:db00::2"));
  EXPECT_EQ(tunnel->getType(), TunnelType::IP_IN_IP_ENCAP);
  EXPECT_EQ(tunnel->getEcnMode(), cfg::TunnelMode::UNIFORM);
}
