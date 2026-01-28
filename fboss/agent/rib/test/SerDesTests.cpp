/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/mock/MockPlatform.h"
#include "fboss/agent/rib/RoutingInformationBase.h"
#include "fboss/agent/state/LabelForwardingInformationBase.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/TestUtils.h"

#include <folly/IPAddress.h>
#include <gtest/gtest.h>

using namespace facebook::fboss;

const RouterID kRid0(0);

cfg::SwitchConfig interfaceAndStaticRoutesWithNextHopsConfig() {
  cfg::SwitchConfig config;
  config.vlans()->resize(2);
  config.vlans()[0].id() = 1;
  config.vlans()[1].id() = 2;

  config.interfaces()->resize(2);
  config.interfaces()[0].intfID() = 1;
  config.interfaces()[0].vlanID() = 1;
  config.interfaces()[0].routerID() = 0;
  config.interfaces()[0].mac() = "00:00:00:00:00:11";
  config.interfaces()[0].ipAddresses()->resize(2);
  config.interfaces()[0].ipAddresses()[0] = "1.1.1.1/24";
  config.interfaces()[0].ipAddresses()[1] = "1::1/48";
  config.interfaces()[1].intfID() = 2;
  config.interfaces()[1].vlanID() = 2;
  config.interfaces()[1].routerID() = 0;
  config.interfaces()[1].mac() = "00:00:00:00:00:22";
  config.interfaces()[1].ipAddresses()->resize(2);
  config.interfaces()[1].ipAddresses()[0] = "2.2.2.2/24";
  config.interfaces()[1].ipAddresses()[1] = "2::1/48";

  config.staticRoutesWithNhops()->resize(4);
  config.staticRoutesWithNhops()[0].nexthops()->resize(1);
  config.staticRoutesWithNhops()[0].prefix() = "2001::/64";
  config.staticRoutesWithNhops()[0].nexthops()[0] = "2::2";
  config.staticRoutesWithNhops()[1].nexthops()->resize(1);
  config.staticRoutesWithNhops()[1].prefix() = "20.20.20.0/24";
  config.staticRoutesWithNhops()[1].nexthops()[0] = "2.2.2.3";
  // Unresolved routes - circular reference via nhops
  config.staticRoutesWithNhops()[2].nexthops()->resize(1);
  config.staticRoutesWithNhops()[2].prefix() = "3::1/64";
  config.staticRoutesWithNhops()[2].nexthops()[0] = "2001::1";
  config.staticRoutesWithNhops()[3].nexthops()->resize(1);
  config.staticRoutesWithNhops()[3].prefix() = "2001::1/64";
  config.staticRoutesWithNhops()[3].nexthops()[0] = "3::1";

  config.staticMplsRoutesWithNhops()->resize(2);
  config.staticMplsRoutesWithNhops()[0].ingressLabel() = 100;
  config.staticMplsRoutesWithNhops()[0].nexthops()->resize(1);
  config.staticMplsRoutesWithNhops()[0].nexthops()[0].address() =
      facebook::network::toBinaryAddress(folly::IPAddress("2::2"));
  MplsAction swap;
  swap.action() = MplsActionCode::SWAP;
  swap.swapLabel() = 101;
  config.staticMplsRoutesWithNhops()[0].nexthops()[0].mplsAction() = swap;
  // Unresolved route
  config.staticMplsRoutesWithNhops()[1].ingressLabel() = 101;
  config.staticMplsRoutesWithNhops()[1].nexthops()->resize(1);
  config.staticMplsRoutesWithNhops()[1].nexthops()[0].address() =
      facebook::network::toBinaryAddress(folly::IPAddress("3::2"));
  config.staticMplsRoutesWithNhops()[1].nexthops()[0].mplsAction() = swap;

  return config;
}
// Helper function to clear resolvedNextHopSetID fields from thrift for
// comparison. We clear resolvedNextHopSetID now because ID generation currently
// does not happen in RIB. Therefore, it is expected that initially the RIB
// routes wont have IDs. In the future we will remove this after moving ID
// generation logic to the RIB.
void clearNextHopSetIDsFromThrift(
    std::map<int32_t, state::RouteTableFields>& routeTables) {
  for (auto& [_, routeTable] : routeTables) {
    for (auto& [__, v4Route] : *routeTable.v4NetworkToRoute()) {
      v4Route.fwd()->resolvedNextHopSetID().reset();
      v4Route.fwd()->normalizedResolvedNextHopSetID().reset();
    }
    for (auto& [__, v6Route] : *routeTable.v6NetworkToRoute()) {
      v6Route.fwd()->resolvedNextHopSetID().reset();
      v6Route.fwd()->normalizedResolvedNextHopSetID().reset();
    }
  }
}

bool ribThriftEqual(
    const RoutingInformationBase& l,
    const RoutingInformationBase& r) {
  auto lThrift = l.toThrift();
  auto rThrift = r.toThrift();
  clearNextHopSetIDsFromThrift(lThrift);
  clearNextHopSetIDsFromThrift(rThrift);
  return lThrift == rThrift;
}

bool ribEqual(
    const RoutingInformationBase& l,
    const RoutingInformationBase& r) {
  // Compare route table details (doesn't include resolvedNextHopSetID)
  if (l.getRouteTableDetails(kRid0) != r.getRouteTableDetails(kRid0)) {
    return false;
  }
  return ribThriftEqual(l, r);
}

class RibSerializationTest : public ::testing::Test {
 public:
  void SetUp() override {
    FLAGS_mpls_rib = true;
    auto emptyState = std::make_shared<SwitchState>();
    auto platform = createMockPlatform();
    auto config = interfaceAndStaticRoutesWithNextHopsConfig();

    curState = publishAndApplyConfig(emptyState, &config, platform.get(), &rib);
    EXPECT_NE(nullptr, curState);
  }
  RoutingInformationBase rib;
  std::shared_ptr<SwitchState> curState;
};

TEST_F(RibSerializationTest, fullRibSerDeser) {
  auto deserializedRib =
      RoutingInformationBase::fromThrift(rib.toThrift(), nullptr, nullptr);

  EXPECT_TRUE(ribEqual(rib, *deserializedRib));
}

TEST_F(RibSerializationTest, serializeOnlyUnresolvedRoutes) {
  auto deserializedRibThrift = RoutingInformationBase::fromThrift(
      rib.warmBootState(),
      curState->getFibsInfoMap(),
      curState->getLabelForwardingInformationBase());
  // Use ribThriftEqual to compare excluding resolvedNextHopSetID
  EXPECT_TRUE(ribThriftEqual(rib, *deserializedRibThrift));
}

TEST_F(RibSerializationTest, deserializeOnlyUnresolvedRoutes) {
  auto deserializedRibEmptyFibThrift = RoutingInformationBase::fromThrift(
      rib.warmBootState(),
      std::make_shared<MultiSwitchFibInfoMap>(),
      std::make_shared<MultiLabelForwardingInformationBase>());

  auto deserializedRibNoFibThrift =
      RoutingInformationBase::fromThrift(rib.warmBootState(), nullptr, nullptr);

  EXPECT_FALSE(ribEqual(rib, *deserializedRibEmptyFibThrift));

  EXPECT_FALSE(ribEqual(rib, *deserializedRibNoFibThrift));

  EXPECT_TRUE(
      ribEqual(*deserializedRibEmptyFibThrift, *deserializedRibNoFibThrift));

  EXPECT_EQ(
      2, deserializedRibEmptyFibThrift->getRouteTableDetails(kRid0).size());
  EXPECT_EQ(
      1, deserializedRibEmptyFibThrift->getMplsRouteTableDetails().size());

  auto deserializedRibWithFibThrift = RoutingInformationBase::fromThrift(
      rib.warmBootState(),
      curState->getFibsInfoMap(),
      curState->getLabelForwardingInformationBase());
  EXPECT_TRUE(ribEqual(rib, *deserializedRibWithFibThrift));
  EXPECT_EQ(
      8, deserializedRibWithFibThrift->getRouteTableDetails(kRid0).size());
  EXPECT_EQ(2, deserializedRibWithFibThrift->getMplsRouteTableDetails().size());
}
