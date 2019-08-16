/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/ApplyThriftConfig.h"

#include "fboss/agent/hw/mock/MockPlatform.h"
#include "fboss/agent/state/DeltaFunctions.h"
#include "fboss/agent/state/Route.h"
#include "fboss/agent/state/RouteDelta.h"
#include "fboss/agent/state/RouteTable.h"
#include "fboss/agent/state/RouteTableMap.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/RouteDistributionGenerator.h"

#include <array>
#include <memory>
#include <string>

namespace facebook {
namespace fboss {
class Platfform;
}
} // namespace facebook

using std::shared_ptr;
using std::string;

using namespace facebook::fboss;
namespace {
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

constexpr auto kExtraRoutes = 4 /*interface route*/ + 1 /*link local route*/;

uint64_t getRouteCount(const std::shared_ptr<SwitchState>& state) {
  uint64_t v4, v6;
  state->getRouteTables()->getRouteCount(&v4, &v6);
  return v4 + v6;
}

template <typename AddrT>
uint64_t getNewRouteCount(const StateDelta& delta) {
  uint64_t newRoutes = 0;
  using RouteT = Route<AddrT>;
  for (const auto& rtDelta : delta.getRouteTablesDelta()) {
    DeltaFunctions::forEachAdded(
        rtDelta.template getRoutesDelta<AddrT>(),
        [&newRoutes](const shared_ptr<RouteT>& /*addedRoute*/) {
          ++newRoutes;
        });
  }
  return newRoutes;
}

uint64_t getNewRouteCount(const StateDelta& delta) {
  return getNewRouteCount<folly::IPAddressV6>(delta) +
      getNewRouteCount<folly::IPAddressV4>(delta);
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
} // namespace
namespace facebook {
namespace fboss {
namespace utility {

TEST(RouteScaleGeneratorsTest, v4AndV6DistributionSingleChunk) {
  auto mockPlatform = std::make_unique<testing::NiceMock<MockPlatform>>();
  auto switchStates = RouteDistributionGenerator(
                          createTestState(mockPlatform.get()),
                          {
                              {65, 5},
                              {127, 5},
                          },
                          {
                              {25, 5},
                              {32, 5},
                          },
                          4000,
                          2)
                          .get();

  EXPECT_EQ(getRouteCount(switchStates.back()), 20 + kExtraRoutes);
  verifyChunking(switchStates, 20, 4000);
}

TEST(RouteScaleGeneratorsTest, v4AndV6DistributionMultipleChunks) {
  auto mockPlatform = std::make_unique<testing::NiceMock<MockPlatform>>();
  auto switchStates = RouteDistributionGenerator(
                          createTestState(mockPlatform.get()),
                          {
                              {65, 5},
                              {127, 5},
                          },
                          {
                              {25, 5},
                              {32, 5},
                          },
                          10,
                          2)
                          .get();

  EXPECT_EQ(getRouteCount(switchStates.back()), 20 + kExtraRoutes);
  verifyChunking(switchStates, 20, 10);
}

TEST(RouteScaleGeneratorsTest, v4AndV6DistributionChunksSpillOverMaskLens) {
  auto mockPlatform = std::make_unique<testing::NiceMock<MockPlatform>>();
  auto switchStates = RouteDistributionGenerator(
                          createTestState(mockPlatform.get()),
                          {
                              {65, 3},
                              {127, 5},
                          },
                          {
                              {25, 3},
                              {32, 7},
                          },
                          4,
                          2)
                          .get();

  EXPECT_EQ(getRouteCount(switchStates.back()), 18 + kExtraRoutes);
  verifyChunking(switchStates, 18, 4);
}

TEST(
    RouteScaleGeneratorsTest,
    v4AndV6DistributionChunksSpillOverAddressFamilies) {
  auto mockPlatform = std::make_unique<testing::NiceMock<MockPlatform>>();
  auto switchStates = RouteDistributionGenerator(
                          createTestState(mockPlatform.get()),
                          {
                              {65, 5},
                              {127, 6},
                          },
                          {
                              {25, 4},
                              {32, 5},
                          },
                          5,
                          2)
                          .get();

  EXPECT_EQ(getRouteCount(switchStates.back()), 20 + kExtraRoutes);
  verifyChunking(switchStates, 20, 5);
}
} // namespace utility
} // namespace fboss
} // namespace facebook
