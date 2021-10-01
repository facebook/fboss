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
#include "fboss/agent/platforms/common/wedge100/Wedge100PlatformMapping.h"
#include "fboss/agent/test/RouteDistributionGenerator.h"
#include "fboss/agent/test/TestUtils.h"

#include <gtest/gtest.h>
#include <string>

using std::string;

namespace {
template <typename Chunks>
uint64_t getRouteCountImpl(const Chunks& routeChunks) {
  uint64_t routeCount = 0;
  std::for_each(
      routeChunks.begin(),
      routeChunks.end(),
      [&routeCount](const auto& routeChunk) {
        routeCount += routeChunk.size();
      });
  return routeCount;
}
} // namespace
namespace facebook::fboss {

cfg::SwitchConfig getTestConfig() {
  // Because this test will eventually use MockPlatform which is using
  // Wedge100PlatformMapping for creating platform configs.
  auto platformMapping = std::make_unique<Wedge100PlatformMapping>();
  const auto& platformPorts = platformMapping->getPlatformPorts();
  cfg::SwitchConfig cfg;
  cfg.ports_ref()->resize(platformPorts.size());
  cfg.vlans_ref()->resize(platformPorts.size());
  cfg.vlanPorts_ref()->resize(platformPorts.size());
  cfg.interfaces_ref()->resize(platformPorts.size());
  auto i = 0;
  for (const auto& entry : platformPorts) {
    auto id = entry.first;
    // port
    preparedMockPortConfig(cfg.ports_ref()[i], id);
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
    cfg.interfaces_ref()[i].mac_ref() = fmt::format("00:02:00:00:00:{:x}", i);
    cfg.interfaces_ref()[i].mtu_ref() = 9000;
    cfg.interfaces_ref()[i].ipAddresses_ref()->resize(2);
    cfg.interfaces_ref()[i].ipAddresses_ref()[0] =
        folly::to<std::string>("10.0.", i, ".0/24");
    cfg.interfaces_ref()[i].ipAddresses_ref()[1] =
        folly::to<std::string>("2400:", i, "::/64");

    ++i;
  }

  XLOG(INFO) << "Created " << platformPorts.size() << " port config";

  return cfg;
}

uint64_t getRouteCount(
    const utility::RouteDistributionGenerator::RouteChunks& routeChunks) {
  return getRouteCountImpl(routeChunks);
}

uint64_t getRouteCount(
    const utility::RouteDistributionGenerator::ThriftRouteChunks& routeChunks) {
  return getRouteCountImpl(routeChunks);
}

void verifyRouteCount(
    const utility::RouteDistributionGenerator& routeDistributionGen,
    uint64_t alreadyExistingRoutes,
    uint64_t expectedNewRoutes) {
  const auto& routeChunks = routeDistributionGen.get();
  const auto& thriftRouteChunks = routeDistributionGen.getThriftRoutes();

  EXPECT_EQ(getRouteCount(routeChunks), expectedNewRoutes);
  EXPECT_EQ(routeDistributionGen.allRoutes().size(), expectedNewRoutes);
  EXPECT_EQ(getRouteCount(thriftRouteChunks), expectedNewRoutes);
  EXPECT_EQ(routeDistributionGen.allThriftRoutes().size(), expectedNewRoutes);
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
  verifyChunking(routeDistributionGen.get(), totalRoutes, chunkSize);
}

} // namespace facebook::fboss
