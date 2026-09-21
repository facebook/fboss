// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>
#include "fboss/agent/state/Srv6Tunnel.h"
#include "fboss/agent/state/SwitchState.h"
#include "folly/IPAddressV6.h"

using namespace facebook::fboss;

namespace {
HwSwitchMatcher scope() {
  return HwSwitchMatcher{std::unordered_set<SwitchID>{SwitchID(10)}};
}

std::shared_ptr<Srv6Tunnel> makeSrv6Tunnel(
    const std::string& tunnelId = "srv6tunnel0") {
  auto tunnel = std::make_shared<Srv6Tunnel>(tunnelId);
  tunnel->setType(TunnelType::SRV6_ENCAP);
  tunnel->setUnderlayIntfId(InterfaceID(42));
  tunnel->setTTLMode(cfg::TunnelMode::PIPE);
  tunnel->setDscpMode(cfg::TunnelMode::PIPE);
  tunnel->setEcnMode(cfg::TunnelMode::PIPE);
  tunnel->setTunnelTermType(cfg::TunnelTerminationType::MP2MP);
  tunnel->setSrcIP(folly::IPAddressV6("2401:db00:11c:8202::1"));
  tunnel->setDstIP(folly::IPAddressV6("2401:db00:11c:8202::100"));
  return tunnel;
}
} // namespace

TEST(Srv6Tunnel, GettersReturnSetValues) {
  auto tunnel = makeSrv6Tunnel();
  EXPECT_EQ(tunnel->getID(), "srv6tunnel0");
  EXPECT_EQ(tunnel->getType(), TunnelType::SRV6_ENCAP);
  EXPECT_EQ(tunnel->getUnderlayIntfId(), InterfaceID(42));
  EXPECT_EQ(tunnel->getTTLMode(), cfg::TunnelMode::PIPE);
  EXPECT_EQ(tunnel->getDscpMode(), cfg::TunnelMode::PIPE);
  EXPECT_EQ(tunnel->getEcnMode(), cfg::TunnelMode::PIPE);
  EXPECT_EQ(tunnel->getTunnelTermType(), cfg::TunnelTerminationType::MP2MP);
  EXPECT_EQ(tunnel->getSrcIP(), folly::IPAddressV6("2401:db00:11c:8202::1"));
  EXPECT_EQ(tunnel->getDstIP(), folly::IPAddressV6("2401:db00:11c:8202::100"));
}

TEST(Srv6Tunnel, DefaultOptionalFieldsAreNullopt) {
  const std::string id{"bare"};
  auto tunnel = std::make_shared<Srv6Tunnel>(id);
  EXPECT_EQ(tunnel->getID(), "bare");
  EXPECT_EQ(tunnel->getSrcIP(), std::nullopt);
  EXPECT_EQ(tunnel->getDstIP(), std::nullopt);
  EXPECT_EQ(tunnel->getTTLMode(), std::nullopt);
  EXPECT_EQ(tunnel->getDscpMode(), std::nullopt);
  EXPECT_EQ(tunnel->getEcnMode(), std::nullopt);
  EXPECT_EQ(tunnel->getTunnelTermType(), std::nullopt);
}

TEST(Srv6Tunnel, ResetOptionalFields) {
  auto tunnel = makeSrv6Tunnel();

  tunnel->setSrcIP(std::nullopt);
  EXPECT_EQ(tunnel->getSrcIP(), std::nullopt);

  tunnel->setDstIP(std::nullopt);
  EXPECT_EQ(tunnel->getDstIP(), std::nullopt);

  tunnel->setTTLMode(std::nullopt);
  EXPECT_EQ(tunnel->getTTLMode(), std::nullopt);

  tunnel->setDscpMode(std::nullopt);
  EXPECT_EQ(tunnel->getDscpMode(), std::nullopt);

  tunnel->setEcnMode(std::nullopt);
  EXPECT_EQ(tunnel->getEcnMode(), std::nullopt);

  tunnel->setTunnelTermType(std::nullopt);
  EXPECT_EQ(tunnel->getTunnelTermType(), std::nullopt);
}

TEST(Srv6Tunnel, ModifyFields) {
  auto tunnel = makeSrv6Tunnel();

  tunnel->setUnderlayIntfId(InterfaceID(99));
  EXPECT_EQ(tunnel->getUnderlayIntfId(), InterfaceID(99));

  tunnel->setType(TunnelType::IP_IN_IP_DECAP);
  EXPECT_EQ(tunnel->getType(), TunnelType::IP_IN_IP_DECAP);

  tunnel->setTTLMode(cfg::TunnelMode::UNIFORM);
  EXPECT_EQ(tunnel->getTTLMode(), cfg::TunnelMode::UNIFORM);

  tunnel->setDscpMode(cfg::TunnelMode::UNIFORM);
  EXPECT_EQ(tunnel->getDscpMode(), cfg::TunnelMode::UNIFORM);

  tunnel->setEcnMode(cfg::TunnelMode::UNIFORM);
  EXPECT_EQ(tunnel->getEcnMode(), cfg::TunnelMode::UNIFORM);

  tunnel->setTunnelTermType(cfg::TunnelTerminationType::P2MP);
  EXPECT_EQ(tunnel->getTunnelTermType(), cfg::TunnelTerminationType::P2MP);

  auto newSrc = folly::IPAddressV6("2401:db00:11c:8202::2");
  tunnel->setSrcIP(newSrc);
  EXPECT_EQ(tunnel->getSrcIP(), newSrc);

  auto newDst = folly::IPAddressV6("2401:db00:11c:8202::200");
  tunnel->setDstIP(newDst);
  EXPECT_EQ(tunnel->getDstIP(), newDst);
}

TEST(Srv6Tunnel, Inequality) {
  auto tunnel0 = makeSrv6Tunnel("srv6tunnel0");
  auto tunnel1 = makeSrv6Tunnel("srv6tunnel1");
  EXPECT_FALSE(*tunnel0 == *tunnel1);
}

TEST(Srv6Tunnel, SerDeserWithOptionalFieldsUnset) {
  const std::string id{"minimal"};
  auto tunnel = std::make_shared<Srv6Tunnel>(id);
  tunnel->setUnderlayIntfId(InterfaceID(1));

  auto serialized = tunnel->toThrift();
  auto tunnelBack = std::make_shared<Srv6Tunnel>(serialized);
  EXPECT_TRUE(*tunnel == *tunnelBack);
  EXPECT_EQ(tunnelBack->getSrcIP(), std::nullopt);
  EXPECT_EQ(tunnelBack->getDstIP(), std::nullopt);
  EXPECT_EQ(tunnelBack->getTTLMode(), std::nullopt);
}

TEST(Srv6Tunnel, SerDeserSrv6Tunnel) {
  auto tunnel = makeSrv6Tunnel();
  auto serialized = tunnel->toThrift();
  auto tunnelBack = std::make_shared<Srv6Tunnel>(serialized);
  EXPECT_TRUE(*tunnel == *tunnelBack);
}

TEST(Srv6Tunnel, SerDeserSwitchState) {
  auto state = std::make_shared<SwitchState>();

  auto tunnel0 = makeSrv6Tunnel("srv6tunnel0");
  auto tunnel1 = makeSrv6Tunnel("srv6tunnel1");
  auto tunnels = state->getSrv6Tunnels()->modify(&state);

  tunnels->addNode(tunnel0, scope());
  tunnels->addNode(tunnel1, scope());

  auto serialized = state->toThrift();
  auto stateBack = SwitchState::fromThrift(serialized);

  for (const auto& tunnelID : {"srv6tunnel0", "srv6tunnel1"}) {
    EXPECT_TRUE(
        *state->getSrv6Tunnels()->getNode(tunnelID) ==
        *stateBack->getSrv6Tunnels()->getNode(tunnelID));
  }
}

TEST(Srv6Tunnel, AddRemove) {
  auto state = std::make_shared<SwitchState>();

  auto tunnel0 = makeSrv6Tunnel("srv6tunnel0");
  auto tunnel1 = makeSrv6Tunnel("srv6tunnel1");

  auto tunnels = state->getSrv6Tunnels()->modify(&state);
  tunnels->addNode(tunnel0, scope());
  tunnels->addNode(tunnel1, scope());
  tunnels->removeNode("srv6tunnel0");
  EXPECT_EQ(state->getSrv6Tunnels()->getNodeIf("srv6tunnel0"), nullptr);
  EXPECT_NE(state->getSrv6Tunnels()->getNodeIf("srv6tunnel1"), nullptr);
}
