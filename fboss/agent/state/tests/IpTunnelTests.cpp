// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>
#include "fboss/agent/state/IpTunnel.h"
#include "folly/IPAddressV6.h"

using namespace facebook::fboss;

std::shared_ptr<IpTunnel> makeTunnel(std::string tunnelId = "tunnel0") {
  auto tunnel = std::make_shared<IpTunnel>(tunnelId);
  tunnel->setType(IPINIP);
  tunnel->setUnderlayIntfId(InterfaceID(42));
  tunnel->setTTLMode(1);
  tunnel->setDscpMode(1);
  tunnel->setEcnMode(1);
  tunnel->setTunnelTermType(MP2MP);
  tunnel->setDstIP(folly::IPAddressV6("2401:db00:11c:8202:0:0:0:100"));
  tunnel->setSrcIP(folly::IPAddressV6("::"));
  tunnel->setDstIPMask(folly::IPAddressV6("::"));
  tunnel->setSrcIPMask(folly::IPAddressV6("::"));
  return tunnel;
}

TEST(Tunnel, SerDeserTunnel) {
  auto tunn = makeTunnel();
  auto serialized = tunn->toFollyDynamic();
  auto tunnBack = IpTunnel::fromFollyDynamic(serialized);
  EXPECT_TRUE(*tunn == *tunnBack);
}
