/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/test/RouteGeneratorTestUtils.h"

#include "fboss/agent/ApplyThriftConfig.h"
#include "fboss/agent/test/RouteDistributionGenerator.h"

#include <gtest/gtest.h>
#include <string>

using std::string;

namespace facebook {
namespace fboss {
// Random test state with 2 ports and 2 vlans
std::shared_ptr<SwitchState> createTestState(Platform* platform) {
  cfg::SwitchConfig cfg;

  std::array<string, 2> v4IntefacePrefixes = {
      "10.0.0.1/24",
      "20.20.20.2/24",
  };
  std::array<string, 2> v6IntefacePrefixes = {
      "2400::1/64",
      "2620::1/64",
  };

  cfg.ports.resize(2);
  cfg.vlans.resize(2);
  cfg.vlanPorts.resize(2);
  cfg.interfaces.resize(2);
  for (int i = 0; i < 2; ++i) {
    auto id = i + 1;
    // port
    cfg.ports[i].logicalID = id;
    cfg.ports[i].name_ref().value_unchecked() = folly::to<string>("port", id);
    // vlans
    cfg.vlans[i].id = id;
    cfg.vlans[i].name = folly::to<string>("Vlan", id);
    cfg.vlans[i].intfID_ref().value_unchecked() = id;
    // vlan ports
    cfg.vlanPorts[i].logicalPort = id;
    cfg.vlanPorts[i].vlanID = id;
    // interfaces
    cfg.interfaces[i].intfID = id;
    cfg.interfaces[i].routerID = 0;
    cfg.interfaces[i].vlanID = id;
    cfg.interfaces[i].name_ref().value_unchecked() =
        folly::to<string>("interface", id);
    cfg.interfaces[i].mac_ref().value_unchecked() =
        folly::to<string>("00:02:00:00:00:", id);
    cfg.interfaces[i].mtu_ref().value_unchecked() = 9000;
    cfg.interfaces[i].ipAddresses.resize(2);
    cfg.interfaces[i].ipAddresses[0] = v4IntefacePrefixes[i];
    cfg.interfaces[i].ipAddresses[1] = v6IntefacePrefixes[i];
  }
  return applyThriftConfig(std::make_shared<SwitchState>(), &cfg, platform);
}

uint64_t getRouteCount(const std::shared_ptr<SwitchState>& state) {
  uint64_t v4, v6;
  state->getRouteTables()->getRouteCount(&v4, &v6);
  return v4 + v6;
}

uint64_t getNewRouteCount(const StateDelta& delta) {
  return getNewRouteCount<folly::IPAddressV6>(delta) +
      getNewRouteCount<folly::IPAddressV4>(delta);
}

void verifyChunking(
    const utility::RouteDistributionSwitchStatesGenerator::SwitchStates&
        switchStates,
    unsigned int totalRoutes,
    unsigned int chunkSize) {
  auto remainingRoutes = totalRoutes;
  XLOG(INFO) << " Switch states : " << switchStates.size();
  for (auto i = 0; i < switchStates.size() - 1; ++i) {
    auto routeCount =
        getNewRouteCount(StateDelta(switchStates[i], switchStates[i + 1]));
    XLOG(INFO) << " Route count : " << routeCount;
    EXPECT_EQ(routeCount, std::min(remainingRoutes, chunkSize));
    remainingRoutes -= routeCount;
  }
  EXPECT_EQ(remainingRoutes, 0);
}
} // namespace fboss
} // namespace facebook
