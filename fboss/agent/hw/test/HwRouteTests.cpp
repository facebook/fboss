/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"

#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwTestMplsUtils.h"
#include "fboss/agent/hw/test/HwTestRouteUtils.h"
#include "fboss/agent/state/NodeBase-defs.h"
#include "fboss/agent/test/EcmpSetupHelper.h"

#include <string>

namespace facebook::fboss {

template <typename AddrT>
class HwRouteTest : public HwLinkStateDependentTest {
 public:
  using Type = AddrT;

 protected:
  cfg::SwitchConfig initialConfig() const override {
    return utility::onePortPerVlanConfig(
        getHwSwitch(),
        {masterLogicalPortIds()[0],
         masterLogicalPortIds()[1],
         masterLogicalPortIds()[2],
         masterLogicalPortIds()[3]},
        cfg::PortLoopbackMode::MAC);
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

  const AddrT kStaticIp2MplsNextHop() const {
    if constexpr (std::is_same_v<AddrT, folly::IPAddressV4>) {
      return folly::IPAddressV4{"10.10.1.1"};
    } else {
      return folly::IPAddressV6{"2803:6080:d038:3063::1"};
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
    applyNewState(ecmpHelper.resolveNextHops(getProgrammedState(), kEcmpWidth));
    ecmpHelper.programRoutes(
        this->getRouteUpdater(), kEcmpWidth, routePrefixes);
    return getProgrammedState();
  }

  void verifyClassIDHelper(
      RoutePrefix<AddrT> routePrefix,
      std::optional<cfg::AclLookupClass> classID) {
    EXPECT_EQ(
        utility::getHwRouteClassID(
            this->getHwSwitch(),
            kRouterID(),
            folly::CIDRNetwork(routePrefix.network, routePrefix.mask)),
        classID);
  }
};

using IpTypes = ::testing::Types<folly::IPAddressV4, folly::IPAddressV6>;

TYPED_TEST_SUITE(HwRouteTest, IpTypes);

TYPED_TEST(HwRouteTest, VerifyClassID) {
  auto setup = [=]() {
    // 3 routes r0, r1, r2. r0 & r1 have classID, r2 does not.
    auto state = this->applyNewConfig(this->initialConfig());
    auto state2 = this->addRoutes(
        state,
        {this->kGetRoutePrefix0(),
         this->kGetRoutePrefix1(),
         this->kGetRoutePrefix2()});
    auto updater = this->getHwSwitchEnsemble()->getRouteUpdater();
    updater.programClassID(
        this->kRouterID(),
        {this->kGetRoutePrefix0().toCidrNetwork(),
         this->kGetRoutePrefix1().toCidrNetwork()},
        this->kLookupClass(),
        false /*sync*/);

    // verify classID programming
    this->verifyClassIDHelper(this->kGetRoutePrefix0(), this->kLookupClass());
    this->verifyClassIDHelper(this->kGetRoutePrefix1(), this->kLookupClass());
    this->verifyClassIDHelper(this->kGetRoutePrefix2(), std::nullopt);

    // remove r1's classID, add classID for r2
    updater.programClassID(
        this->kRouterID(),
        {this->kGetRoutePrefix1().toCidrNetwork()},
        std::nullopt,
        false /*sync*/);
    updater.programClassID(
        this->kRouterID(),
        {this->kGetRoutePrefix2().toCidrNetwork()},
        this->kLookupClass(),
        false /*sync*/);
  };

  auto verify = [=]() {
    this->verifyClassIDHelper(this->kGetRoutePrefix0(), this->kLookupClass());
    this->verifyClassIDHelper(this->kGetRoutePrefix1(), std::nullopt);
    this->verifyClassIDHelper(this->kGetRoutePrefix2(), this->kLookupClass());
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwRouteTest, VerifyClassIdWithNhopResolutionFlap) {
  auto setup = [=]() {
    auto state = this->applyNewConfig(this->initialConfig());
    this->addRoutes(state, {this->kGetRoutePrefix0()});
    auto updater = this->getHwSwitchEnsemble()->getRouteUpdater();
    updater.programClassID(
        this->kRouterID(),
        {this->kGetRoutePrefix0().toCidrNetwork()},
        this->kLookupClass(),
        false /*sync*/);

    // verify classID programming
    this->verifyClassIDHelper(this->kGetRoutePrefix0(), this->kLookupClass());
    auto kEcmpWidth = 1;
    using AddrT = typename TestFixture::Type;
    utility::EcmpSetupAnyNPorts<AddrT> ecmpHelper(
        this->getProgrammedState(), this->kRouterID());
    // Unresolve nhop
    this->applyNewState(
        ecmpHelper.unresolveNextHops(this->getProgrammedState(), kEcmpWidth));
    // Resolve nhop
    this->applyNewState(
        ecmpHelper.resolveNextHops(this->getProgrammedState(), kEcmpWidth));
  };
  auto verify = [=]() {
    this->verifyClassIDHelper(this->kGetRoutePrefix0(), this->kLookupClass());
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwRouteTest, UnresolvedAndResolvedNextHop) {
  using AddrT = typename TestFixture::Type;
  auto setup = [=]() {
    this->applyNewConfig(this->initialConfig());
    utility::EcmpSetupTargetedPorts<AddrT> ecmpHelper(
        this->getProgrammedState(), this->kRouterID());
    ecmpHelper.programRoutes(
        this->getRouteUpdater(),
        {PortDescriptor(this->masterLogicalPortIds()[0])},
        {this->kGetRoutePrefix0()});

    this->applyNewState(ecmpHelper.resolveNextHops(
        this->getProgrammedState(),
        {PortDescriptor(this->masterLogicalPortIds()[1])}));
    ecmpHelper.programRoutes(
        this->getRouteUpdater(),
        {PortDescriptor(this->masterLogicalPortIds()[1])},
        {this->kGetRoutePrefix1()});
  };
  auto verify = [=]() {
    auto routePrefix0 = this->kGetRoutePrefix0();
    auto cidr0 = folly::CIDRNetwork(routePrefix0.network, routePrefix0.mask);
    EXPECT_TRUE(
        utility::isHwRouteToCpu(this->getHwSwitch(), this->kRouterID(), cidr0));
    EXPECT_FALSE(utility::isHwRouteMultiPath(
        this->getHwSwitch(), this->kRouterID(), cidr0));

    auto routePrefix1 = this->kGetRoutePrefix1();
    auto cidr1 = folly::CIDRNetwork(routePrefix1.network, routePrefix1.mask);
    if (std::is_same_v<AddrT, folly::IPAddressV4>) {
      EXPECT_TRUE(utility::isHwRouteToNextHop(
          this->getHwSwitch(),
          this->kRouterID(),
          cidr1,
          folly::IPAddress("2.0.0.2")));
    } else {
      EXPECT_TRUE(utility::isHwRouteToNextHop(
          this->getHwSwitch(),
          this->kRouterID(),
          cidr1,
          folly::IPAddress("2::2")));
    }
    EXPECT_FALSE(utility::isHwRouteMultiPath(
        this->getHwSwitch(), this->kRouterID(), cidr1));
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwRouteTest, UnresolveResolvedNextHop) {
  using AddrT = typename TestFixture::Type;

  auto setup = [=]() {
    this->applyNewConfig(this->initialConfig());
    utility::EcmpSetupAnyNPorts<AddrT> ecmpHelper(
        this->getProgrammedState(), this->kRouterID());
    this->applyNewState(
        ecmpHelper.resolveNextHops(this->getProgrammedState(), 1));

    ecmpHelper.programRoutes(
        this->getRouteUpdater(), 1, {this->kGetRoutePrefix0()});
    this->applyNewState(
        ecmpHelper.unresolveNextHops(this->getProgrammedState(), 1));
  };
  auto verify = [=]() {
    auto routePrefix = this->kGetRoutePrefix0();
    auto cidr = folly::CIDRNetwork(routePrefix.network, routePrefix.mask);
    EXPECT_TRUE(
        utility::isHwRouteToCpu(this->getHwSwitch(), this->kRouterID(), cidr));
    EXPECT_FALSE(utility::isHwRouteMultiPath(
        this->getHwSwitch(), this->kRouterID(), cidr));
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwRouteTest, UnresolvedAndResolvedMultiNextHop) {
  using AddrT = typename TestFixture::Type;
  auto setup = [=]() {
    this->applyNewConfig(this->initialConfig());
    utility::EcmpSetupTargetedPorts<AddrT> ecmpHelper(
        this->getProgrammedState(), this->kRouterID());
    ecmpHelper.programRoutes(
        this->getRouteUpdater(),
        {PortDescriptor(this->masterLogicalPortIds()[0]),
         PortDescriptor(this->masterLogicalPortIds()[1])},
        {this->kGetRoutePrefix0()});

    this->applyNewState(ecmpHelper.resolveNextHops(
        this->getProgrammedState(),
        {PortDescriptor(this->masterLogicalPortIds()[2]),
         PortDescriptor(this->masterLogicalPortIds()[3])}));
    ecmpHelper.programRoutes(
        this->getRouteUpdater(),
        {PortDescriptor(this->masterLogicalPortIds()[2]),
         PortDescriptor(this->masterLogicalPortIds()[3])},
        {this->kGetRoutePrefix1()});
  };
  auto verify = [=]() {
    auto routePrefix0 = this->kGetRoutePrefix0();
    auto cidr0 = folly::CIDRNetwork(routePrefix0.network, routePrefix0.mask);
    EXPECT_FALSE(
        utility::isHwRouteToCpu(this->getHwSwitch(), this->kRouterID(), cidr0));
    EXPECT_TRUE(utility::isHwRouteMultiPath(
        this->getHwSwitch(), this->kRouterID(), cidr0));
    if (std::is_same_v<AddrT, folly::IPAddressV4>) {
      EXPECT_FALSE(utility::isHwRouteToNextHop(
          this->getHwSwitch(),
          this->kRouterID(),
          cidr0,
          folly::IPAddress("1.0.0.1")));
      EXPECT_FALSE(utility::isHwRouteToNextHop(
          this->getHwSwitch(),
          this->kRouterID(),
          cidr0,
          folly::IPAddress("2.0.0.2")));
    } else {
      EXPECT_FALSE(utility::isHwRouteToNextHop(
          this->getHwSwitch(),
          this->kRouterID(),
          cidr0,
          folly::IPAddress("1::1")));
      EXPECT_FALSE(utility::isHwRouteToNextHop(
          this->getHwSwitch(),
          this->kRouterID(),
          cidr0,
          folly::IPAddress("2::1")));
    }

    auto routePrefix1 = this->kGetRoutePrefix1();
    auto cidr1 = folly::CIDRNetwork(routePrefix1.network, routePrefix1.mask);
    EXPECT_FALSE(
        utility::isHwRouteToCpu(this->getHwSwitch(), this->kRouterID(), cidr1));
    EXPECT_TRUE(utility::isHwRouteMultiPath(
        this->getHwSwitch(), this->kRouterID(), cidr1));
    if (std::is_same_v<AddrT, folly::IPAddressV4>) {
      EXPECT_TRUE(utility::isHwRouteToNextHop(
          this->getHwSwitch(),
          this->kRouterID(),
          cidr1,
          folly::IPAddress("3.0.0.3")));
      EXPECT_TRUE(utility::isHwRouteToNextHop(
          this->getHwSwitch(),
          this->kRouterID(),
          cidr1,
          folly::IPAddress("4.0.0.4")));
    } else {
      EXPECT_TRUE(utility::isHwRouteToNextHop(
          this->getHwSwitch(),
          this->kRouterID(),
          cidr1,
          folly::IPAddress("3::3")));
      EXPECT_TRUE(utility::isHwRouteToNextHop(
          this->getHwSwitch(),
          this->kRouterID(),
          cidr1,
          folly::IPAddress("4::4")));
    }
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwRouteTest, ResolvedMultiNexthopToUnresolvedSingleNexthop) {
  using AddrT = typename TestFixture::Type;
  auto setup = [=]() { this->applyNewConfig(this->initialConfig()); };
  auto verify = [=]() {
    utility::EcmpSetupTargetedPorts<AddrT> ecmpHelper(
        this->getProgrammedState(), this->kRouterID());
    this->applyNewState(ecmpHelper.resolveNextHops(
        this->getProgrammedState(),
        {PortDescriptor(this->masterLogicalPortIds()[0]),
         PortDescriptor(this->masterLogicalPortIds()[1])}));
    ecmpHelper.programRoutes(
        this->getRouteUpdater(),
        {PortDescriptor(this->masterLogicalPortIds()[0]),
         PortDescriptor(this->masterLogicalPortIds()[1])},
        {this->kGetRoutePrefix0()});
    auto routePrefix0 = this->kGetRoutePrefix0();
    auto cidr0 = folly::CIDRNetwork(routePrefix0.network, routePrefix0.mask);
    EXPECT_FALSE(
        utility::isHwRouteToCpu(this->getHwSwitch(), this->kRouterID(), cidr0));
    EXPECT_TRUE(utility::isHwRouteMultiPath(
        this->getHwSwitch(), this->kRouterID(), cidr0));
    if (std::is_same_v<AddrT, folly::IPAddressV4>) {
      EXPECT_TRUE(utility::isHwRouteToNextHop(
          this->getHwSwitch(),
          this->kRouterID(),
          cidr0,
          folly::IPAddress("1.0.0.1")));
      EXPECT_TRUE(utility::isHwRouteToNextHop(
          this->getHwSwitch(),
          this->kRouterID(),
          cidr0,
          folly::IPAddress("2.0.0.2")));
    } else {
      EXPECT_TRUE(utility::isHwRouteToNextHop(
          this->getHwSwitch(),
          this->kRouterID(),
          cidr0,
          folly::IPAddress("1::1")));
      EXPECT_TRUE(utility::isHwRouteToNextHop(
          this->getHwSwitch(),
          this->kRouterID(),
          cidr0,
          folly::IPAddress("2::2")));
    }
    this->applyNewState(ecmpHelper.unresolveNextHops(
        this->getProgrammedState(),
        {PortDescriptor(this->masterLogicalPortIds()[0]),
         PortDescriptor(this->masterLogicalPortIds()[1])}));
    this->applyNewState(ecmpHelper.resolveNextHops(
        this->getProgrammedState(),
        {PortDescriptor(this->masterLogicalPortIds()[0])}));
    ecmpHelper.programRoutes(
        this->getRouteUpdater(),
        {PortDescriptor(this->masterLogicalPortIds()[0])},
        {this->kGetRoutePrefix0()});
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwRouteTest, StaticIp2MplsRoutes) {
  using AddrT = typename TestFixture::Type;

  auto setup = [=]() {
    auto config = this->initialConfig();

    config.staticIp2MplsRoutes_ref()->resize(1);
    config.staticIp2MplsRoutes_ref()[0].prefix_ref() =
        this->kGetRoutePrefix1().str();

    config.staticIp2MplsRoutes_ref()[0].nexthops_ref()->resize(1);
    config.staticIp2MplsRoutes_ref()[0].nexthops_ref()[0].nexthop_ref() =
        this->kStaticIp2MplsNextHop().str(); // in prefix 0 subnet
    config.staticIp2MplsRoutes_ref()[0]
        .nexthops_ref()[0]
        .labelForwardingAction_ref()
        ->action_ref() = MplsActionCode::PUSH;
    config.staticIp2MplsRoutes_ref()[0]
        .nexthops_ref()[0]
        .labelForwardingAction_ref()
        ->pushLabels_ref() = {1001, 1002};
    this->applyNewConfig(config);

    // resolve prefix 0 subnet
    utility::EcmpSetupTargetedPorts<AddrT> ecmpHelper(
        this->getProgrammedState(), this->kRouterID());
    ecmpHelper.programRoutes(
        this->getRouteUpdater(),
        {PortDescriptor(this->masterLogicalPortIds()[0]),
         PortDescriptor(this->masterLogicalPortIds()[1])},
        {this->kGetRoutePrefix0()});

    this->applyNewState(ecmpHelper.resolveNextHops(
        this->getProgrammedState(),
        {PortDescriptor(this->masterLogicalPortIds()[0]),
         PortDescriptor(this->masterLogicalPortIds()[1])}));
  };
  auto verify = [=]() {
    // prefix 1 subnet reachable via prefix 0 with mpls stack over this stack
    utility::verifyProgrammedStack<AddrT>(
        this->getHwSwitch(),
        this->kGetRoutePrefix1(),
        InterfaceID(utility::kBaseVlanId),
        {1001, 1002},
        1);
    utility::verifyProgrammedStack<AddrT>(
        this->getHwSwitch(),
        this->kGetRoutePrefix1(),
        InterfaceID(utility::kBaseVlanId + 1),
        {1001, 1002},
        1);
  };
  this->verifyAcrossWarmBoots(setup, verify);
}
} // namespace facebook::fboss
