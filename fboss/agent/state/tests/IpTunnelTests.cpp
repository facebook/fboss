// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>
#include "fboss/agent/state/IpTunnel.h"
#include "fboss/agent/state/SwitchState.h"
#include "folly/IPAddressV6.h"

using namespace facebook::fboss;

std::shared_ptr<IpTunnel> makeTunnel(const std::string& tunnelId = "tunnel0") {
  auto tunnel = std::make_shared<IpTunnel>(tunnelId);
  tunnel->setType(cfg::TunnelType::IP_IN_IP);
  tunnel->setUnderlayIntfId(InterfaceID(42));
  tunnel->setTTLMode(cfg::IpTunnelMode::PIPE);
  tunnel->setDscpMode(cfg::IpTunnelMode::PIPE);
  tunnel->setEcnMode(cfg::IpTunnelMode::PIPE);
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

  state->addTunnel(tunnel0);
  state->addTunnel(tunnel1);

  auto serialized = state->toThrift();
  auto stateBack = SwitchState::fromThrift(serialized);

  for (auto tunnelID : {"tunnel0", "tunnel1"}) {
    EXPECT_TRUE(
        *state->getTunnels()->getTunnel(tunnelID) ==
        *stateBack->getTunnels()->getTunnel(tunnelID));
  }
}

TEST(Tunnel, AddRemove) {
  auto state = std::make_shared<SwitchState>();

  auto tunnel0 = makeTunnel("tunnel0");
  auto tunnel1 = makeTunnel("tunnel1");

  state->addTunnel(tunnel0);
  state->addTunnel(tunnel1);
  state->getTunnels()->removeTunnel("tunnel0");
  EXPECT_EQ(state->getTunnels()->getTunnelIf("tunnel0"), nullptr);
  EXPECT_NE(state->getTunnels()->getTunnelIf("tunnel1"), nullptr);
}
