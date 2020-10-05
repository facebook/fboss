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

namespace facebook::fboss {

// Random test state with 2 ports and 2 vlans
std::shared_ptr<SwitchState> createTestState(Platform* platform) {
  cfg::SwitchConfig cfg;
  cfg.ports_ref()->resize(64);
  cfg.vlans_ref()->resize(64);
  cfg.vlanPorts_ref()->resize(64);
  cfg.interfaces_ref()->resize(64);
  for (int i = 0; i < 64; ++i) {
    auto id = i + 1;
    // port
    *cfg.ports_ref()[i].logicalID_ref() = id;
    cfg.ports_ref()[i].name_ref() = folly::to<string>("port", id);
    cfg.ports_ref()[i].state_ref() = cfg::PortState::ENABLED;
    // vlans
    *cfg.vlans_ref()[i].id_ref() = id;
    *cfg.vlans_ref()[i].name_ref() = folly::to<string>("Vlan", id);
    cfg.vlans_ref()[i].intfID_ref() = id;
    // vlan ports
    *cfg.vlanPorts_ref()[i].logicalPort_ref() = id;
    *cfg.vlanPorts_ref()[i].vlanID_ref() = id;
    // interfaces
    *cfg.interfaces_ref()[i].intfID_ref() = id;
    *cfg.interfaces_ref()[i].routerID_ref() = 0;
    *cfg.interfaces_ref()[i].vlanID_ref() = id;
    cfg.interfaces_ref()[i].name_ref() = folly::to<string>("interface", id);
    cfg.interfaces_ref()[i].mac_ref() =
        folly::to<string>("00:02:00:00:00:", id);
    cfg.interfaces_ref()[i].mtu_ref() = 9000;
    cfg.interfaces_ref()[i].ipAddresses_ref()->resize(2);
    cfg.interfaces_ref()[i].ipAddresses_ref()[0] =
        folly::to<std::string>("10.0.", id, ".0/24");
    cfg.interfaces_ref()[i].ipAddresses_ref()[1] =
        folly::to<std::string>("2400:", id, "::/64");
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

uint64_t getRouteCount(
    const utility::RouteDistributionGenerator::RouteChunks& routeChunks) {
  uint64_t routeCount = 0;
  std::for_each(
      routeChunks.begin(),
      routeChunks.end(),
      [&routeCount](const auto& routeChunk) {
        routeCount += routeChunk.size();
      });
  return routeCount;
}

void verifyRouteCount(
    const utility::RouteDistributionGenerator& routeDistributionGen,
    uint64_t alreadyExistingRoutes,
    uint64_t expectedNewRoutes) {
  const auto& switchStates = routeDistributionGen.getSwitchStates();
  const auto& routeChunks = routeDistributionGen.get();

  EXPECT_EQ(
      getRouteCount(switchStates.back()),
      expectedNewRoutes + alreadyExistingRoutes);
  EXPECT_EQ(getRouteCount(routeChunks), expectedNewRoutes);
}

void verifyChunking(
    const utility::RouteDistributionGenerator::SwitchStates& switchStates,
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

void verifyChunking(
    const utility::RouteDistributionGenerator::RouteChunks& routeChunks,
    unsigned int totalRoutes,
    unsigned int chunkSize) {
  auto remainingRoutes = totalRoutes;
  for (const auto& routeChunk : routeChunks) {
    EXPECT_EQ(routeChunk.size(), std::min(remainingRoutes, chunkSize));
    remainingRoutes -= routeChunk.size();
  }
  EXPECT_EQ(remainingRoutes, 0);
}

void verifyChunking(
    const utility::RouteDistributionGenerator& routeDistributionGen,
    unsigned int totalRoutes,
    unsigned int chunkSize) {
  verifyChunking(
      routeDistributionGen.getSwitchStates(), totalRoutes, chunkSize);
  verifyChunking(routeDistributionGen.get(), totalRoutes, chunkSize);
}

} // namespace facebook::fboss
