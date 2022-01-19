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
#include <folly/IPAddressV4.h>
#include <folly/IPAddressV6.h>
#include <folly/json.h>
#include <gtest/gtest.h>

using namespace facebook::fboss;

const RouterID kRid0(0);

cfg::SwitchConfig interfaceAndStaticRoutesWithNextHopsConfig() {
  cfg::SwitchConfig config;
  config.vlans_ref()->resize(2);
  config.vlans_ref()[0].id_ref() = 1;
  config.vlans_ref()[1].id_ref() = 2;

  config.interfaces_ref()->resize(2);
  config.interfaces_ref()[0].intfID_ref() = 1;
  config.interfaces_ref()[0].vlanID_ref() = 1;
  config.interfaces_ref()[0].routerID_ref() = 0;
  config.interfaces_ref()[0].mac_ref() = "00:00:00:00:00:11";
  config.interfaces_ref()[0].ipAddresses_ref()->resize(2);
  config.interfaces_ref()[0].ipAddresses_ref()[0] = "1.1.1.1/24";
  config.interfaces_ref()[0].ipAddresses_ref()[1] = "1::1/48";
  config.interfaces_ref()[1].intfID_ref() = 2;
  config.interfaces_ref()[1].vlanID_ref() = 2;
  config.interfaces_ref()[1].routerID_ref() = 0;
  config.interfaces_ref()[1].mac_ref() = "00:00:00:00:00:22";
  config.interfaces_ref()[1].ipAddresses_ref()->resize(2);
  config.interfaces_ref()[1].ipAddresses_ref()[0] = "2.2.2.2/24";
  config.interfaces_ref()[1].ipAddresses_ref()[1] = "2::1/48";

  config.staticRoutesWithNhops_ref()->resize(4);
  config.staticRoutesWithNhops_ref()[0].nexthops_ref()->resize(1);
  config.staticRoutesWithNhops_ref()[0].prefix_ref() = "2001::/64";
  config.staticRoutesWithNhops_ref()[0].nexthops_ref()[0] = "2::2";
  config.staticRoutesWithNhops_ref()[1].nexthops_ref()->resize(1);
  config.staticRoutesWithNhops_ref()[1].prefix_ref() = "20.20.20.0/24";
  config.staticRoutesWithNhops_ref()[1].nexthops_ref()[0] = "2.2.2.3";
  // Unresolved routes - circular reference via nhops
  config.staticRoutesWithNhops_ref()[2].nexthops_ref()->resize(1);
  config.staticRoutesWithNhops_ref()[2].prefix_ref() = "3::1/64";
  config.staticRoutesWithNhops_ref()[2].nexthops_ref()[0] = "2001::1";
  config.staticRoutesWithNhops_ref()[3].nexthops_ref()->resize(1);
  config.staticRoutesWithNhops_ref()[3].prefix_ref() = "2001::1/64";
  config.staticRoutesWithNhops_ref()[3].nexthops_ref()[0] = "3::1";

  config.staticMplsRoutesWithNhops_ref()->resize(2);
  config.staticMplsRoutesWithNhops_ref()[0].ingressLabel_ref() = 100;
  config.staticMplsRoutesWithNhops_ref()[0].nexthops_ref()->resize(1);
  config.staticMplsRoutesWithNhops_ref()[0].nexthops_ref()[0].address_ref() =
      facebook::network::toBinaryAddress(folly::IPAddress("2::2"));
  MplsAction swap;
  swap.action_ref() = MplsActionCode::SWAP;
  swap.swapLabel_ref() = 101;
  config.staticMplsRoutesWithNhops_ref()[0].nexthops_ref()[0].mplsAction_ref() =
      swap;
  // Unresolved route
  config.staticMplsRoutesWithNhops_ref()[1].ingressLabel_ref() = 101;
  config.staticMplsRoutesWithNhops_ref()[1].nexthops_ref()->resize(1);
  config.staticMplsRoutesWithNhops_ref()[1].nexthops_ref()[0].address_ref() =
      facebook::network::toBinaryAddress(folly::IPAddress("3::2"));
  config.staticMplsRoutesWithNhops_ref()[1].nexthops_ref()[0].mplsAction_ref() =
      swap;

  return config;
}
bool ribEqual(
    const RoutingInformationBase& l,
    const RoutingInformationBase& r) {
  return l.getRouteTableDetails(kRid0) == r.getRouteTableDetails(kRid0);
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
  auto deserializedRib = RoutingInformationBase::fromFollyDynamic(
      rib.toFollyDynamic(), nullptr, nullptr);

  EXPECT_TRUE(ribEqual(rib, *deserializedRib));
}

TEST_F(RibSerializationTest, serializeOnlyUnresolvedRoutes) {
  auto deserializedRib = RoutingInformationBase::fromFollyDynamic(
      rib.unresolvedRoutesFollyDynamic(),
      curState->getFibs(),
      curState->getLabelForwardingInformationBase());

  EXPECT_TRUE(ribEqual(rib, *deserializedRib));
}

TEST_F(RibSerializationTest, deserializeOnlyUnresolvedRoutes) {
  auto deserializedRibEmptyFib = RoutingInformationBase::fromFollyDynamic(
      rib.unresolvedRoutesFollyDynamic(),
      std::make_shared<ForwardingInformationBaseMap>(),
      std::make_shared<LabelForwardingInformationBase>());

  auto deserializedRibNoFib = RoutingInformationBase::fromFollyDynamic(
      rib.unresolvedRoutesFollyDynamic(), nullptr, nullptr);

  EXPECT_FALSE(ribEqual(rib, *deserializedRibEmptyFib));
  EXPECT_FALSE(ribEqual(rib, *deserializedRibNoFib));
  EXPECT_TRUE(ribEqual(*deserializedRibEmptyFib, *deserializedRibNoFib));
  EXPECT_EQ(2, deserializedRibEmptyFib->getRouteTableDetails(kRid0).size());
  EXPECT_EQ(1, deserializedRibEmptyFib->getMplsRouteTableDetails().size());

  auto deserializedRibWithFib = RoutingInformationBase::fromFollyDynamic(
      rib.unresolvedRoutesFollyDynamic(),
      curState->getFibs(),
      curState->getLabelForwardingInformationBase());
  EXPECT_TRUE(ribEqual(rib, *deserializedRibWithFib));
  EXPECT_EQ(8, deserializedRibWithFib->getRouteTableDetails(kRid0).size());
  EXPECT_EQ(2, deserializedRibWithFib->getMplsRouteTableDetails().size());
}
