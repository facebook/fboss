/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/test/HwTest.h"

#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwTestRouteUtils.h"
#include "fboss/agent/state/NodeBase-defs.h"
#include "fboss/agent/test/EcmpSetupHelper.h"

#include <string>

namespace facebook::fboss {

template <typename AddrT>
class HwRouteTest : public HwTest {
 protected:
  cfg::SwitchConfig initialConfig() const {
    return utility::twoL3IntfConfig(
        getHwSwitch(), masterLogicalPortIds()[0], masterLogicalPortIds()[1]);
  }

  RouterID kRouterID() const {
    return RouterID(0);
  }

  cfg::AclLookupClass kLookupClass() const {
    return cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_2;
  }

  const std::vector<RoutePrefix<AddrT>> kGetRoutePrefixes() const {
    if constexpr (std::is_same_v<AddrT, folly::IPAddressV4>) {
      static const std::vector<RoutePrefix<AddrT>> routePrefixes = {
          RoutePrefix<folly::IPAddressV4>{folly::IPAddressV4{"10.10.1.0"}, 24},
          RoutePrefix<folly::IPAddressV4>{folly::IPAddressV4{"10.20.1.0"}, 24},
          RoutePrefix<folly::IPAddressV4>{folly::IPAddressV4{"10.30.1.0"}, 24}};

      return routePrefixes;
    } else {
      static const std::vector<RoutePrefix<AddrT>> routePrefixes = {
          RoutePrefix<folly::IPAddressV6>{
              folly::IPAddressV6{"2803:6080:d038:3063::"}, 64},
          RoutePrefix<folly::IPAddressV6>{
              folly::IPAddressV6{"2803:6080:d038:3064::"}, 64},
          RoutePrefix<folly::IPAddressV6>{
              folly::IPAddressV6{"2803:6080:d038:3065::"}, 64}};

      return routePrefixes;
    }
  }

  RoutePrefix<AddrT> kGetRoutePrefix0() const {
    return kGetRoutePrefixes()[0];
  }

  RoutePrefix<AddrT> kGetRoutePrefix1() const {
    return kGetRoutePrefixes()[1];
  }

  RoutePrefix<AddrT> kGetRoutePrefix2() const {
    return kGetRoutePrefixes()[2];
  }

  std::shared_ptr<SwitchState> addRoutes(
      const std::shared_ptr<SwitchState>& inState,
      const std::vector<RoutePrefix<AddrT>>& routePrefixes) {
    auto kEcmpWidth = 1;
    utility::EcmpSetupAnyNPorts<AddrT> ecmpHelper(inState, kRouterID());

    return ecmpHelper.setupECMPForwarding(
        ecmpHelper.resolveNextHops(this->getProgrammedState(), kEcmpWidth),
        kEcmpWidth,
        routePrefixes);
  }

  std::shared_ptr<SwitchState> updateRoutesClassID(
      const std::shared_ptr<SwitchState>& inState,
      const std::map<RoutePrefix<AddrT>, std::optional<cfg::AclLookupClass>>&
          routePrefix2ClassID) {
    auto outState{inState->clone()};
    auto routeTable = outState->getRouteTables()->getRouteTable(kRouterID());
    auto routeTableRib =
        routeTable->template getRib<AddrT>()->modify(kRouterID(), &outState);

    for (const auto& [routePrefix, classID] : routePrefix2ClassID) {
      auto route = routeTableRib->routes()->getRouteIf(routePrefix);
      CHECK(route);
      route = route->clone();
      route->updateClassID(classID);
      routeTableRib->updateRoute(route);
    }
    return outState;
  }

  void verifyClassIDHelper(
      RoutePrefix<AddrT> routePrefix,
      std::optional<cfg::AclLookupClass> classID) {
    EXPECT_EQ(
        utility::getHwRouteClassID(
            this->getHwSwitch(),
            folly::CIDRNetwork(routePrefix.network, routePrefix.mask)),
        classID);
  }
};

using IpTypes = ::testing::Types<folly::IPAddressV4, folly::IPAddressV6>;

TYPED_TEST_CASE(HwRouteTest, IpTypes);

TYPED_TEST(HwRouteTest, VerifyClassID) {
  auto setup = [=]() {
    // 3 routes r0, r1, r2. r0 & r1 have classID, r2 does not.
    auto state = this->applyNewConfig(this->initialConfig());
    auto state2 = this->addRoutes(
        state,
        {this->kGetRoutePrefix0(),
         this->kGetRoutePrefix1(),
         this->kGetRoutePrefix2()});
    auto state3 = this->updateRoutesClassID(
        state2,
        {{this->kGetRoutePrefix0(), this->kLookupClass()},
         {this->kGetRoutePrefix1(), this->kLookupClass()}});
    this->applyNewState(state3);

    // verify classID programming
    this->verifyClassIDHelper(this->kGetRoutePrefix0(), this->kLookupClass());
    this->verifyClassIDHelper(this->kGetRoutePrefix1(), this->kLookupClass());
    this->verifyClassIDHelper(this->kGetRoutePrefix2(), std::nullopt);

    // remove r1's classID, add classID for r2
    auto state4 = this->updateRoutesClassID(
        this->getProgrammedState(),
        {{this->kGetRoutePrefix1(), std::nullopt},
         {this->kGetRoutePrefix2(), this->kLookupClass()}});
    this->applyNewState(state4);
  };

  auto verify = [=]() {
    this->verifyClassIDHelper(this->kGetRoutePrefix0(), this->kLookupClass());
    this->verifyClassIDHelper(this->kGetRoutePrefix1(), std::nullopt);
    this->verifyClassIDHelper(this->kGetRoutePrefix2(), this->kLookupClass());
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
