/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <gtest/gtest.h>

#include "fboss/agent/FibHelpers.h"
#include "fboss/agent/LookupClassRouteUpdater.h"
#include "fboss/agent/NeighborUpdater.h"
#include "fboss/agent/SwSwitchRouteUpdateWrapper.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/RouteUpdater.h"
#include "fboss/agent/state/Vlan.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/TestUtils.h"

#include <folly/IPAddressV4.h>
#include <folly/IPAddressV6.h>

using folly::IPAddressV4;
using folly::IPAddressV6;

namespace facebook::fboss {

template <typename AddrTAndRib>
class FibHelperTest : public ::testing::Test {
 public:
  using Func = std::function<void()>;
  using StateUpdateFn = SwSwitch::StateUpdateFn;
  using AddrT = typename AddrTAndRib::AddrT;
  static constexpr auto hasStandAloneRib = AddrTAndRib::hasStandAloneRib;

  void SetUp() override {
    auto config = testConfigA();
    auto flags = hasStandAloneRib ? SwitchFlags::ENABLE_STANDALONE_RIB
                                  : SwitchFlags::DEFAULT;
    handle_ = createTestHandle(&config, flags);
    sw_ = handle_->getSw();
    resolveNeighbor(kIpAddressA(), kMacAddressA());
    // Install kPrefix1 in FIB
    programRoute(kPrefix1());
  }

  void programRoute(const folly::CIDRNetwork& prefix) {
    auto routeUpdater = sw_->getRouteUpdater();
    RouteNextHopSet nexthops{
        UnresolvedNextHop(kIpAddressA(), UCMP_DEFAULT_WEIGHT)};
    routeUpdater.addRoute(
        kRid(),
        prefix.first,
        prefix.second,
        kClientID(),
        RouteNextHopEntry(nexthops, AdminDistance::MAX_ADMIN_DISTANCE));
    routeUpdater.program();
  }

  void updateState(folly::StringPiece name, StateUpdateFn func) {
    sw_->updateStateBlocking(name, func);
  }

  VlanID kVlan() const {
    return VlanID(1);
  }

  PortID kPortID() const {
    return PortID(1);
  }

  ClientID kClientID() const {
    return ClientID(1);
  }

  RouterID kRid() const {
    return RouterID{0};
  }

  InterfaceID kInterfaceID() const {
    return InterfaceID(1);
  }

  MacAddress kMacAddressA() const {
    return MacAddress("01:02:03:04:05:06");
  }

  AddrT kIpAddressA() {
    if constexpr (std::is_same_v<AddrT, folly::IPAddressV4>) {
      return IPAddressV4("10.0.0.2");
    } else {
      return IPAddressV6("2401:db00:2110:3001::0002");
    }
  }

  folly::CIDRNetwork kPrefix1() const {
    if constexpr (std::is_same_v<AddrT, folly::IPAddressV4>) {
      return {folly::IPAddressV4{"10.1.4.0"}, 24};
    } else {
      return {folly::IPAddressV6{"2803:6080:d038:3066::"}, 64};
    }
  }
  AddrT kInSubnetIp() const {
    if constexpr (std::is_same_v<AddrT, folly::IPAddressV4>) {
      return folly::IPAddressV4{"10.1.4.1"};
    } else {
      return folly::IPAddressV6{"2803:6080:d038:3066::1"};
    }
  }
  AddrT kNotInSubnetIp() const {
    if constexpr (std::is_same_v<AddrT, folly::IPAddressV4>) {
      return folly::IPAddressV4{"10.1.5.1"};
    } else {
      return folly::IPAddressV6{"2803:6080:d038:4066::1"};
    }
  }

  folly::CIDRNetwork kPrefix2() const {
    if constexpr (std::is_same_v<AddrT, folly::IPAddressV4>) {
      return {folly::IPAddressV4{"11.1.4.0"}, 24};
    } else {
      return {folly::IPAddressV6{"2803:6080:d038:3067::"}, 64};
    }
  }

  void resolveNeighbor(AddrT ipAddress, MacAddress macAddress) {
    /*
     * Cause a neighbor entry to resolve by receiving appropriate ARP/NDP, and
     * assert if valid CLASSID is associated with the newly resolved neighbor.
     */
    if constexpr (std::is_same<AddrT, folly::IPAddressV4>::value) {
      sw_->getNeighborUpdater()->receivedArpMine(
          kVlan(),
          ipAddress,
          macAddress,
          PortDescriptor(kPortID()),
          ArpOpCode::ARP_OP_REPLY);
    } else {
      sw_->getNeighborUpdater()->receivedNdpMine(
          kVlan(),
          ipAddress,
          macAddress,
          PortDescriptor(kPortID()),
          ICMPv6Type::ICMPV6_TYPE_NDP_NEIGHBOR_ADVERTISEMENT,
          0);
    }

    sw_->getNeighborUpdater()->waitForPendingUpdates();
    waitForBackgroundThread(sw_);
    waitForStateUpdates(sw_);
    sw_->getNeighborUpdater()->waitForPendingUpdates();
    waitForStateUpdates(sw_);
  }

 protected:
  std::unique_ptr<HwTestHandle> handle_;
  SwSwitch* sw_;
};

template <typename AddrType, bool StandAloneRib>
struct FibHelperTestTypeT {
  using AddrT = AddrType;
  static constexpr auto hasStandAloneRib = StandAloneRib;
};

using V4NoRib = FibHelperTestTypeT<folly::IPAddressV4, false>;
using V4Rib = FibHelperTestTypeT<folly::IPAddressV4, true>;
using V6NoRib = FibHelperTestTypeT<folly::IPAddressV6, false>;
using V6Rib = FibHelperTestTypeT<folly::IPAddressV6, true>;
using FibHelperTestTypes = ::testing::Types<V4NoRib, V4Rib, V6NoRib, V6Rib>;

TYPED_TEST_CASE(FibHelperTest, FibHelperTestTypes);

TYPED_TEST(FibHelperTest, findRoute) {
  auto route = findRoute<typename TestFixture::AddrT>(
      this->sw_->isStandaloneRibEnabled(),
      this->kRid(),
      this->kPrefix1(),
      this->sw_->getState());
  EXPECT_NE(route, nullptr);

  route = findRoute<typename TestFixture::AddrT>(
      this->sw_->isStandaloneRibEnabled(),
      this->kRid(),
      this->kPrefix2(),
      this->sw_->getState());
  EXPECT_EQ(route, nullptr);
}

TYPED_TEST(FibHelperTest, findLongestMatchRoute) {
  auto route = findRoute<typename TestFixture::AddrT>(
      this->sw_->isStandaloneRibEnabled(),
      this->kRid(),
      this->kPrefix1(),
      this->sw_->getState());
  EXPECT_NE(route, nullptr);
  auto route2 = findLongestMatchRoute<typename TestFixture::AddrT>(
      this->sw_->getRib(),
      this->kRid(),
      this->kInSubnetIp(),
      this->sw_->getState());

  EXPECT_EQ(route, route2);

  auto route3 = findLongestMatchRoute<typename TestFixture::AddrT>(
      this->sw_->getRib(),
      this->kRid(),
      this->kNotInSubnetIp(),
      this->sw_->getState());
  EXPECT_NE(route2, route3);
}
TYPED_TEST(FibHelperTest, forAllRoutes) {
  int count = 0;
  auto countRoutes = [&count](RouterID rid, auto& route) { ++count; };
  forAllRoutes(
      this->sw_->isStandaloneRibEnabled(), this->sw_->getState(), countRoutes);
  auto prevCount = count;
  // Program prefix1 again, should not change the count
  this->programRoute(this->kPrefix1());
  count = 0;
  forAllRoutes(
      this->sw_->isStandaloneRibEnabled(), this->sw_->getState(), countRoutes);
  EXPECT_EQ(count, prevCount);
  // Add one more route, now count should increment
  this->programRoute(this->kPrefix2());
  count = 0;
  forAllRoutes(
      this->sw_->isStandaloneRibEnabled(), this->sw_->getState(), countRoutes);
  EXPECT_EQ(count, prevCount + 1);
}
} // namespace facebook::fboss
