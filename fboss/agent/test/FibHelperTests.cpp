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
#include "fboss/agent/state/FibInfo.h"
#include "fboss/agent/state/FibInfoMap.h"
#include "fboss/agent/state/RouteNextHopEntry.h"

#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/TestUtils.h"

#include <folly/IPAddressV4.h>
#include <folly/IPAddressV6.h>

DECLARE_bool(enable_nexthop_id_manager);

using folly::IPAddressV4;
using folly::IPAddressV6;

namespace facebook::fboss {

namespace {
HwSwitchMatcher scope() {
  return HwSwitchMatcher{std::unordered_set<SwitchID>{SwitchID(0)}};
}
} // namespace

template <typename AddrType>
class FibHelperTest : public ::testing::Test {
 public:
  using Func = std::function<void()>;
  using StateUpdateFn = SwSwitch::StateUpdateFn;
  using AddrT = AddrType;
  static constexpr bool hasStandAloneRib = true;

  void SetUp() override {
    FLAGS_enable_nexthop_id_manager = true;
    auto config = testConfigA();
    handle_ = createTestHandle(&config);
    sw_ = handle_->getSw();

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

  void programRouteWithNexthops(
      const folly::CIDRNetwork& prefix,
      const std::vector<std::string>& nexthopStrs) {
    auto routeUpdater = sw_->getRouteUpdater();
    RouteNextHopSet nexthops;
    for (const auto& nexthopStr : nexthopStrs) {
      nexthops.emplace(
          UnresolvedNextHop(folly::IPAddress(nexthopStr), ECMP_WEIGHT));
    }
    routeUpdater.addRoute(
        kRid(),
        prefix.first,
        prefix.second,
        kClientID(),
        RouteNextHopEntry(nexthops, AdminDistance::EBGP));
    routeUpdater.program();
  }

  std::shared_ptr<FibInfo> getFibInfo() {
    auto state = sw_->getState();
    return state->getFibsInfoMap()->getFibInfo(scope());
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

  AddrT kIpAddressB() {
    if constexpr (std::is_same_v<AddrT, folly::IPAddressV4>) {
      return IPAddressV4("10.0.0.3");
    } else {
      return IPAddressV6("2401:db00:2110:3001::0003");
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

  std::shared_ptr<ForwardingInformationBase<AddrT>> getFib(
      const std::shared_ptr<ForwardingInformationBaseContainer>& fibContainer) {
    if constexpr (std::is_same_v<AddrT, folly::IPAddressV4>) {
      return fibContainer->getFibV4();
    } else {
      return fibContainer->getFibV6();
    }
  }

 protected:
  std::unique_ptr<HwTestHandle> handle_;
  SwSwitch* sw_;
};

using FibHelperTestTypes =
    ::testing::Types<folly::IPAddressV4, folly::IPAddressV6>;

TYPED_TEST_CASE(FibHelperTest, FibHelperTestTypes);

TYPED_TEST(FibHelperTest, findRoute) {
  auto route = findRoute<typename TestFixture::AddrT>(
      this->kRid(), this->kPrefix1(), this->sw_->getState());
  EXPECT_NE(route, nullptr);

  route = findRoute<typename TestFixture::AddrT>(
      this->kRid(), this->kPrefix2(), this->sw_->getState());
  EXPECT_EQ(route, nullptr);
}

TYPED_TEST(FibHelperTest, findLongestMatchRoute) {
  auto route = findRoute<typename TestFixture::AddrT>(
      this->kRid(), this->kPrefix1(), this->sw_->getState());
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
  forAllRoutes(this->sw_->getState(), countRoutes);
  auto prevCount = count;
  // Program prefix1 again, should not change the count
  this->programRoute(this->kPrefix1());
  count = 0;
  forAllRoutes(this->sw_->getState(), countRoutes);
  EXPECT_EQ(count, prevCount);
  // Add one more route, now count should increment
  this->programRoute(this->kPrefix2());
  count = 0;
  forAllRoutes(this->sw_->getState(), countRoutes);
  EXPECT_EQ(count, prevCount + 1);
}

TYPED_TEST(FibHelperTest, getNexthopsResolvesRouteIds) {
  // Route with two nexthops
  this->programRouteWithNexthops(
      this->kPrefix2(), {this->kIpAddressA().str(), this->kIpAddressB().str()});

  auto state = this->sw_->getState();
  auto fibInfo = this->getFibInfo();
  ASSERT_NE(fibInfo, nullptr);

  auto fibContainer = fibInfo->getFibContainerIf(this->kRid());
  ASSERT_NE(fibContainer, nullptr);

  // Verify routes can be resolved using getNextHops
  auto fib = this->getFib(fibContainer);
  for (const auto& [_, route] : std::as_const(*fib)) {
    const auto& fwdInfo = route->getForwardInfo();
    auto resolvedSetId = fwdInfo.getResolvedNextHopSetID();
    const auto& expectedNextHopSet = fwdInfo.getNextHopSet();

    // Skip routes without nexthops (TO_CPU/DROP actions)
    if (expectedNextHopSet.empty()) {
      continue;
    }

    ASSERT_TRUE(resolvedSetId.has_value())
        << "Route should have resolvedNextHopSetID";

    // Use the FibHelper getNextHops function to resolve the ID
    auto resolvedNextHops =
        getNextHops(state, static_cast<NextHopSetId>(*resolvedSetId));

    // Sort for comparison
    std::sort(resolvedNextHops.begin(), resolvedNextHops.end());
    std::vector<NextHop> expectedNextHops(
        expectedNextHopSet.begin(), expectedNextHopSet.end());

    EXPECT_EQ(resolvedNextHops, expectedNextHops)
        << "NextHops mismatch for route " << route->prefix().str();
  }
}

TYPED_TEST(FibHelperTest, getNexthopsInvalidIdThrows) {
  auto state = this->sw_->getState();

  // Try to resolve a non-existent NextHopSetId
  NextHopSetId invalidId = 999999;
  EXPECT_THROW(getNextHops(state, invalidId), FbossError);
}

} // namespace facebook::fboss
