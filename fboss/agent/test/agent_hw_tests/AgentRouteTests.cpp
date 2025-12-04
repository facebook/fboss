/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/AsicUtils.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/packet/PktUtil.h"
#include "fboss/agent/state/LabelForwardingAction.h"
#include "fboss/agent/state/NodeBase-defs.h"
#include "fboss/agent/state/Port.h"
#include "fboss/agent/state/StateUtils.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/CoppTestUtils.h"
#include "fboss/agent/test/utils/OlympicTestUtils.h"
#include "fboss/agent/test/utils/PacketTestUtils.h"
#include "fboss/agent/test/utils/RouteTestUtils.h"
#include "fboss/lib/CommonUtils.h"

#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/if/gen-cpp2/common_types.h"
#include "folly/IPAddressV4.h"
#include "folly/IPAddressV6.h"

#include <string>

using facebook::network::toBinaryAddress;

DECLARE_bool(intf_nbr_tables);
DECLARE_bool(classid_for_unresolved_routes);

namespace {
template <typename AddrT>
bool verifyProgrammedStack(
    typename facebook::fboss::Route<AddrT>::Prefix routePrefix,
    const facebook::fboss::InterfaceID& intfID,
    const facebook::fboss::LabelForwardingAction::LabelStack& stack,
    long refCount,
    facebook::fboss::AgentEnsemble& ensemble) {
  auto switchId = ensemble.getSw()
                      ->getScopeResolver()
                      ->scope(ensemble.masterLogicalPortIds())
                      .switchId();
  facebook::fboss::IpPrefix prefix;
  prefix.ip() = toBinaryAddress(routePrefix.network());
  prefix.prefixLength() = routePrefix.mask();
  auto client = ensemble.getHwAgentTestClient(switchId);
  return client->sync_isProgrammedInHw(intfID, prefix, stack, refCount);
}
} // namespace

namespace facebook::fboss {

template <typename AddrT>
class AgentRouteTest : public AgentHwTest {
 public:
  using Type = AddrT;

 protected:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    return utility::onePortPerInterfaceConfig(
        ensemble.getSw(),
        ensemble.masterLogicalPortIds(),
        true /*interfaceHasSubnet*/);
  }

  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {ProductionFeature::L3_FORWARDING};
  }

  RouterID kRouterID() const {
    return RouterID(0);
  }

  cfg::AclLookupClass kLookupClass() const {
    return cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_2;
  }

  std::vector<PortDescriptor> portDescs() const {
    std::vector<PortDescriptor> ports;
    for (auto i = 0; i < 4; ++i) {
      if (FLAGS_hyper_port) {
        ports.push_back(PortDescriptor(masterLogicalHyperPortIds()[i]));
      } else {
        ports.push_back(PortDescriptor(masterLogicalInterfacePortIds()[i]));
      }
    }
    return ports;
  }

  RoutePrefix<AddrT> getSubnetIpForInterface() const {
    auto state = this->getProgrammedState();
    InterfaceID intfID = utility::firstInterfaceIDWithPorts(state);
    auto interface = state->getInterfaces()->getNodeIf(intfID);
    if (interface) {
      for (auto iter : std::as_const(*interface->getAddresses())) {
        std::pair<folly::IPAddress, uint8_t> address(
            folly::IPAddress(iter.first), iter.second->ref());
        if constexpr (std::is_same_v<AddrT, folly::IPAddressV4>) {
          if (address.first.isV4()) {
            return RoutePrefix<folly::IPAddressV4>{
                address.first.asV4(), address.second};
          }
        } else {
          if (address.first.isV6()) {
            return RoutePrefix<folly::IPAddressV6>{
                address.first.asV6(), address.second};
          }
        }
      }
    }
    XLOG(FATAL) << "No interface found";
  }
  const std::vector<RoutePrefix<AddrT>> kGetRoutePrefixes() const {
    if constexpr (std::is_same_v<AddrT, folly::IPAddressV4>) {
      static const std::vector<RoutePrefix<AddrT>> routePrefixes = {
          RoutePrefix<folly::IPAddressV4>{folly::IPAddressV4{"10.10.1.0"}, 24},
          RoutePrefix<folly::IPAddressV4>{folly::IPAddressV4{"10.20.1.0"}, 24},
          RoutePrefix<folly::IPAddressV4>{folly::IPAddressV4{"10.30.1.0"}, 24},
          RoutePrefix<folly::IPAddressV4>{folly::IPAddressV4{"10.40.1.1"}, 32}};

      return routePrefixes;
    } else {
      static const std::vector<RoutePrefix<AddrT>> routePrefixes = {
          RoutePrefix<folly::IPAddressV6>{
              folly::IPAddressV6{"2803:6080:d038:3063::"}, 64},
          RoutePrefix<folly::IPAddressV6>{
              folly::IPAddressV6{"2803:6080:d038:3064::"}, 64},
          RoutePrefix<folly::IPAddressV6>{
              folly::IPAddressV6{"2803:6080:d038:3065::"}, 64},
          RoutePrefix<folly::IPAddressV6>{
              folly::IPAddressV6{"2803:6080:d038:3065::1"}, 128}};

      return routePrefixes;
    }
  }

  RoutePrefix<AddrT> kDefaultPrefix() const {
    if constexpr (std::is_same_v<AddrT, folly::IPAddressV4>) {
      return RoutePrefix<folly::IPAddressV4>{folly::IPAddressV4{"0.0.0.0"}, 0};

    } else {
      return RoutePrefix<folly::IPAddressV6>{folly::IPAddressV6{"::"}, 0};
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

  RoutePrefix<AddrT> kGetRoutePrefix3() const {
    return kGetRoutePrefixes()[3];
  }

  std::shared_ptr<SwitchState> addRoutes(
      const std::shared_ptr<SwitchState>& inState,
      const std::vector<RoutePrefix<AddrT>>& routePrefixes) {
    auto kEcmpWidth = 1;
    utility::EcmpSetupAnyNPorts<AddrT> ecmpHelper(
        inState, getSw()->needL2EntryForNeighbor(), kRouterID());
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      auto newState = ecmpHelper.resolveNextHops(in, kEcmpWidth);
      return newState;
    });
    auto wrapper = this->getSw()->getRouteUpdater();
    ecmpHelper.programRoutes(&wrapper, kEcmpWidth, routePrefixes);
    return getProgrammedState();
  }

  void verifyClassIDHelper(
      RoutePrefix<AddrT> routePrefix,
      std::optional<cfg::AclLookupClass> classID) {
    auto routeInfo = utility::getRouteInfo(
        routePrefix.network(), routePrefix.mask(), *this->getAgentEnsemble());
    std::optional<cfg::AclLookupClass> classIDFromHw;
    if (routeInfo.classId().has_value()) {
      classIDFromHw =
          facebook::fboss::cfg::AclLookupClass(routeInfo.classId().value());
    }
    EXPECT_EQ(classIDFromHw, classID);
  }
};

template <typename AddrT>
class AgentClassIDRouteTest : public AgentRouteTest<AddrT> {
 public:
  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    auto features = AgentRouteTest<AddrT>::getProductionFeaturesVerified();
    features.push_back(ProductionFeature::CLASS_ID_FOR_CONNECTED_ROUTE);
    return features;
  }
};

template <typename AddrT>
class AgentMplsRouteTest : public AgentRouteTest<AddrT> {
 public:
  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    auto features = AgentRouteTest<AddrT>::getProductionFeaturesVerified();
    features.push_back(ProductionFeature::MPLS);
    return features;
  }
};

using IpTypes = ::testing::Types<folly::IPAddressV4, folly::IPAddressV6>;

TYPED_TEST_SUITE(AgentRouteTest, IpTypes);
TYPED_TEST_SUITE(AgentClassIDRouteTest, IpTypes);
TYPED_TEST_SUITE(AgentMplsRouteTest, IpTypes);

TYPED_TEST(AgentRouteTest, VerifyClassID) {
  auto setup = [=, this]() {
    // 3 routes r0, r1, r2. r0 & r1 have classID, r2 does not.
    this->addRoutes(
        this->getProgrammedState(),
        {this->kGetRoutePrefix0(),
         this->kGetRoutePrefix1(),
         this->kGetRoutePrefix2()});
    auto updater = this->getSw()->getRouteUpdater();
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

  auto verify = [=, this]() {
    this->verifyClassIDHelper(this->kGetRoutePrefix0(), this->kLookupClass());
    this->verifyClassIDHelper(this->kGetRoutePrefix1(), std::nullopt);
    this->verifyClassIDHelper(this->kGetRoutePrefix2(), this->kLookupClass());
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(AgentRouteTest, VerifyClassIdWithNhopResolutionFlap) {
  auto setup = [=, this]() {
    this->addRoutes(this->getProgrammedState(), {this->kGetRoutePrefix0()});
    auto updater = this->getSw()->getRouteUpdater();
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
        this->getProgrammedState(),
        this->getSw()->needL2EntryForNeighbor(),
        this->kRouterID());
    // Unresolve nhop
    this->applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      auto newState = ecmpHelper.unresolveNextHops(in, kEcmpWidth);
      return newState;
    });
    // Resolve nhop
    this->applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      auto newState = ecmpHelper.resolveNextHops(in, kEcmpWidth);
      return newState;
    });
  };
  auto verify = [=, this]() {
    this->verifyClassIDHelper(this->kGetRoutePrefix0(), this->kLookupClass());
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(AgentRouteTest, UnresolvedAndResolvedNextHop) {
  using AddrT = typename TestFixture::Type;
  auto ports = this->portDescs();
  auto setup = [=, this]() {
    utility::EcmpSetupTargetedPorts<AddrT> ecmpHelper(
        this->getProgrammedState(),
        this->getSw()->needL2EntryForNeighbor(),
        this->kRouterID());
    auto wrapper = this->getSw()->getRouteUpdater();
    ecmpHelper.programRoutes(&wrapper, {ports[0]}, {this->kGetRoutePrefix0()});

    this->applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      auto newState = ecmpHelper.resolveNextHops(in, {ports[1]});
      return newState;
    });
    ecmpHelper.programRoutes(&wrapper, {ports[1]}, {this->kGetRoutePrefix1()});
  };
  auto verify = [=, this]() {
    auto routePrefix0 = this->kGetRoutePrefix0();
    utility::EcmpSetupTargetedPorts<AddrT> ecmpHelper(
        this->getProgrammedState(),
        this->getSw()->needL2EntryForNeighbor(),
        this->kRouterID());
    auto routeInfo = utility::getRouteInfo(
        routePrefix0.network(), routePrefix0.mask(), *this->getAgentEnsemble());
    EXPECT_TRUE(*routeInfo.isProgrammedToCpu());
    EXPECT_FALSE(*routeInfo.isMultiPath());

    auto routePrefix1 = this->kGetRoutePrefix1();
    EXPECT_TRUE(
        utility::isRouteToNexthop(
            routePrefix1.network(),
            routePrefix1.mask(),
            ecmpHelper.nhop(ports[1]).ip,
            *this->getAgentEnsemble()));

    auto routeInfo1 = utility::getRouteInfo(
        routePrefix1.network(), routePrefix1.mask(), *this->getAgentEnsemble());
    EXPECT_FALSE(*routeInfo1.isMultiPath());
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(AgentRouteTest, UnresolveResolvedNextHop) {
  using AddrT = typename TestFixture::Type;

  auto setup = [=, this]() {
    utility::EcmpSetupAnyNPorts<AddrT> ecmpHelper(
        this->getProgrammedState(),
        this->getSw()->needL2EntryForNeighbor(),
        this->kRouterID());
    this->applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      auto newState = ecmpHelper.resolveNextHops(in, 1);
      return newState;
    });

    auto wrapper = this->getSw()->getRouteUpdater();
    ecmpHelper.programRoutes(&wrapper, 1, {this->kGetRoutePrefix0()});
    this->applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      auto newState = ecmpHelper.unresolveNextHops(in, 1);
      return newState;
    });
  };
  auto verify = [=, this]() {
    auto routePrefix = this->kGetRoutePrefix0();
    auto routeInfo = utility::getRouteInfo(
        routePrefix.network(), routePrefix.mask(), *this->getAgentEnsemble());
    EXPECT_TRUE(*routeInfo.isProgrammedToCpu());
    EXPECT_FALSE(*routeInfo.isMultiPath());
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(AgentRouteTest, UnresolvedAndResolvedMultiNextHop) {
  using AddrT = typename TestFixture::Type;
  auto ports = this->portDescs();
  auto setup = [=, this]() {
    utility::EcmpSetupTargetedPorts<AddrT> ecmpHelper(
        this->getProgrammedState(),
        this->getSw()->needL2EntryForNeighbor(),
        this->kRouterID());
    auto wrapper = this->getSw()->getRouteUpdater();
    ecmpHelper.programRoutes(
        &wrapper, {ports[0], ports[1]}, {this->kGetRoutePrefix0()});

    this->applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      auto newState = ecmpHelper.resolveNextHops(in, {ports[2], ports[3]});
      return newState;
    });
    ecmpHelper.programRoutes(
        &wrapper, {ports[2], ports[3]}, {this->kGetRoutePrefix1()});
  };
  auto verify = [=, this]() {
    auto routePrefix0 = this->kGetRoutePrefix0();
    auto routeInfo = utility::getRouteInfo(
        routePrefix0.network(), routePrefix0.mask(), *this->getAgentEnsemble());
    EXPECT_FALSE(*routeInfo.isProgrammedToCpu());
    EXPECT_TRUE(*routeInfo.isMultiPath());
    utility::EcmpSetupTargetedPorts<AddrT> ecmpHelper(
        this->getProgrammedState(),
        this->getSw()->needL2EntryForNeighbor(),
        this->kRouterID());
    EXPECT_FALSE(
        utility::isRouteToNexthop(
            routePrefix0.network(),
            routePrefix0.mask(),
            ecmpHelper.nhop(ports[0]).ip,
            *this->getAgentEnsemble()));
    EXPECT_FALSE(
        utility::isRouteToNexthop(
            routePrefix0.network(),
            routePrefix0.mask(),
            ecmpHelper.nhop(ports[1]).ip,
            *this->getAgentEnsemble()));

    auto routePrefix1 = this->kGetRoutePrefix1();
    auto routeInfo1 = utility::getRouteInfo(
        routePrefix1.network(), routePrefix1.mask(), *this->getAgentEnsemble());
    EXPECT_FALSE(*routeInfo1.isProgrammedToCpu());
    EXPECT_TRUE(*routeInfo1.isMultiPath());
    EXPECT_TRUE(
        utility::isRouteToNexthop(
            routePrefix1.network(),
            routePrefix1.mask(),
            ecmpHelper.nhop(ports[2]).ip,
            *this->getAgentEnsemble()));
    EXPECT_TRUE(
        utility::isRouteToNexthop(
            routePrefix1.network(),
            routePrefix1.mask(),
            ecmpHelper.nhop(ports[3]).ip,
            *this->getAgentEnsemble()));
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(AgentRouteTest, ResolvedMultiNexthopToUnresolvedSingleNexthop) {
  auto ports = this->portDescs();
  using AddrT = typename TestFixture::Type;
  auto verify = [=, this]() {
    utility::EcmpSetupTargetedPorts<AddrT> ecmpHelper(
        this->getProgrammedState(),
        this->getSw()->needL2EntryForNeighbor(),
        this->kRouterID());
    this->applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      auto newState = ecmpHelper.resolveNextHops(in, {ports[0], ports[1]});
      return newState;
    });
    auto wrapper = this->getSw()->getRouteUpdater();
    ecmpHelper.programRoutes(
        &wrapper, {ports[0], ports[1]}, {this->kGetRoutePrefix0()});
    auto routePrefix0 = this->kGetRoutePrefix0();
    auto routeInfo = utility::getRouteInfo(
        routePrefix0.network(), routePrefix0.mask(), *this->getAgentEnsemble());
    EXPECT_FALSE(*routeInfo.isProgrammedToCpu());
    EXPECT_TRUE(*routeInfo.isMultiPath());
    EXPECT_TRUE(
        utility::isRouteToNexthop(
            routePrefix0.network(),
            routePrefix0.mask(),
            ecmpHelper.nhop(ports[0]).ip,
            *this->getAgentEnsemble()));
    EXPECT_TRUE(
        utility::isRouteToNexthop(
            routePrefix0.network(),
            routePrefix0.mask(),
            ecmpHelper.nhop(ports[1]).ip,
            *this->getAgentEnsemble()));

    this->applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      auto newState = ecmpHelper.unresolveNextHops(
          in, {this->portDescs()[0], this->portDescs()[1]});
      return newState;
    });
    this->applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      auto newState = ecmpHelper.resolveNextHops(in, {this->portDescs()[0]});
      return newState;
    });
    ecmpHelper.programRoutes(
        &wrapper, {this->portDescs()[0]}, {this->kGetRoutePrefix0()});
  };
  this->verifyAcrossWarmBoots([] {}, verify);
}

TYPED_TEST(AgentRouteTest, VerifyRouting) {
  using AddrT = typename TestFixture::Type;
  auto constexpr isV4 = std::is_same_v<AddrT, folly::IPAddressV4>;
  auto ports = this->portDescs();
  auto setup = [=, this]() {
    utility::EcmpSetupTargetedPorts<AddrT> ecmpHelper(
        this->getProgrammedState(),
        this->getSw()->needL2EntryForNeighbor(),
        this->kRouterID());

    this->applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      auto newState = ecmpHelper.resolveNextHops(in, {ports[0]});
      return newState;
    });

    auto wrapper = this->getSw()->getRouteUpdater();
    ecmpHelper.programRoutes(&wrapper, {ports[0]}, {this->kDefaultPrefix()});
  };
  auto verify = [=, this]() {
    const auto egressPort = ports[0].phyPortID();
    auto vlanId = this->getVlanIDForTx();
    auto intfMac =
        utility::getMacForFirstInterfaceWithPorts(this->getProgrammedState());

    auto beforeOutPkts =
        *this->getLatestPortStats(egressPort).outUnicastPkts__ref();
    auto v4TxPkt = utility::makeUDPTxPacket(
        this->getSw(),
        vlanId,
        intfMac,
        intfMac,
        folly::IPAddressV4("101.0.0.1"),
        folly::IPAddressV4("201.0.0.1"),
        1234,
        4321);

    auto v6TxPkt = utility::makeUDPTxPacket(
        this->getSw(),
        vlanId,
        intfMac,
        intfMac,
        folly::IPAddressV6("101::1"),
        folly::IPAddressV6("201::1"),
        1234,
        4321);

    if (isV4) {
      this->getAgentEnsemble()->ensureSendPacketOutOfPort(
          std::move(v4TxPkt), ports[1].phyPortID());
    } else {
      this->getAgentEnsemble()->ensureSendPacketOutOfPort(
          std::move(v6TxPkt), ports[1].phyPortID());
    }
    WITH_RETRIES({
      auto afterOutPkts =
          *this->getLatestPortStats(egressPort).outUnicastPkts__ref();
      XLOG(DBG2) << "Stats:: beforeOutPkts: " << beforeOutPkts
                 << " afterOutPkts: " << afterOutPkts;
      EXPECT_EVENTUALLY_EQ(afterOutPkts - 1, beforeOutPkts);
    });
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(AgentRouteTest, verifyHostRouteChange) {
  using AddrT = typename TestFixture::Type;
  auto ports = this->portDescs();

  auto setup = [=, this]() {
    utility::EcmpSetupTargetedPorts<AddrT> ecmpHelper(
        this->getProgrammedState(),
        this->getSw()->needL2EntryForNeighbor(),
        this->kRouterID());
    this->applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      auto newState = ecmpHelper.resolveNextHops(in, {ports[0], ports[1]});
      return newState;
    });
    auto wrapper = this->getSw()->getRouteUpdater();
    ecmpHelper.programRoutes(&wrapper, {ports[1]}, {this->kGetRoutePrefix3()});
    ecmpHelper.programRoutes(
        &wrapper, {ports[0], ports[1]}, {this->kGetRoutePrefix3()});
  };

  auto verify = [=, this]() {
    auto routePrefix = this->kGetRoutePrefix3();
    utility::EcmpSetupTargetedPorts<AddrT> ecmpHelper(
        this->getProgrammedState(),
        this->getSw()->needL2EntryForNeighbor(),
        this->kRouterID());
    EXPECT_TRUE(
        utility::isRouteToNexthop(
            routePrefix.network(),
            routePrefix.mask(),
            ecmpHelper.nhop(ports[0]).ip,
            *this->getAgentEnsemble()));
    EXPECT_TRUE(
        utility::isRouteToNexthop(
            routePrefix.network(),
            routePrefix.mask(),
            ecmpHelper.nhop(ports[1]).ip,
            *this->getAgentEnsemble()));
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(AgentRouteTest, verifyCpuRouteChange) {
  using AddrT = typename TestFixture::Type;
  auto ports = this->portDescs();

  auto setup = [=, this]() {
    auto cfg = this->initialConfig(*this->getAgentEnsemble());
    auto ensemble = this->getAgentEnsemble();
    utility::addOlympicQosMaps(cfg, ensemble->getL3Asics());
    utility::setDefaultCpuTrafficPolicyConfig(
        cfg, ensemble->getL3Asics(), ensemble->isSai());
    utility::addCpuQueueConfig(cfg, ensemble->getL3Asics(), ensemble->isSai());
    this->applyNewConfig(cfg);

    utility::EcmpSetupTargetedPorts<AddrT> ecmpHelper(
        this->getProgrammedState(),
        this->getSw()->needL2EntryForNeighbor(),
        this->kRouterID());
    // Next hops unresolved - route should point to CPU
    auto wrapper = this->getSw()->getRouteUpdater();
    ecmpHelper.programRoutes(&wrapper, {ports[1]}, {this->kGetRoutePrefix3()});
  };

  auto verify = [=, this]() {
    auto routePrefix = this->kGetRoutePrefix3();
    auto routeInfo = utility::getRouteInfo(
        routePrefix.network(), routePrefix.mask(), *this->getAgentEnsemble());
    EXPECT_TRUE(*routeInfo.isProgrammedToCpu());
    if (FLAGS_classid_for_unresolved_routes) {
      EXPECT_TRUE(*routeInfo.isRouteUnresolvedToClassId());
    }

    // Resolve next hops
    utility::EcmpSetupTargetedPorts<AddrT> ecmpHelper(
        this->getProgrammedState(),
        this->getSw()->needL2EntryForNeighbor(),
        this->kRouterID());
    this->applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      auto newState = ecmpHelper.resolveNextHops(in, {ports[1]});
      return newState;
    });
    WITH_RETRIES({
      EXPECT_EVENTUALLY_TRUE(
          utility::isRouteToNexthop(
              routePrefix.network(),
              routePrefix.mask(),
              ecmpHelper.nhop(ports[1]).ip,
              *this->getAgentEnsemble()));
      if (FLAGS_classid_for_unresolved_routes) {
        auto routeInfo2 = utility::getRouteInfo(
            routePrefix.network(),
            routePrefix.mask(),
            *this->getAgentEnsemble());
        EXPECT_EVENTUALLY_FALSE(*routeInfo2.isRouteUnresolvedToClassId());
      }
    });

    // Verify routing
    const auto egressPort = ports[1].phyPortID();
    auto vlanId = this->getVlanIDForTx();
    auto intfMac =
        utility::getMacForFirstInterfaceWithPorts(this->getProgrammedState());
    auto beforeOutPkts =
        *this->getLatestPortStats(egressPort).outUnicastPkts__ref();
    auto v6TxPkt = utility::makeUDPTxPacket(
        this->getSw(),
        vlanId,
        intfMac,
        intfMac,
        this->kGetRoutePrefix0().network(), // Randomly pick src IP
        routePrefix.network(),
        1234,
        4321);
    this->getAgentEnsemble()->ensureSendPacketOutOfPort(
        std::move(v6TxPkt), ports[0].phyPortID());
    WITH_RETRIES({
      auto afterOutPkts =
          *this->getLatestPortStats(egressPort).outUnicastPkts__ref();
      XLOG(DBG2) << "Stats:: beforeOutPkts: " << beforeOutPkts
                 << " afterOutPkts: " << afterOutPkts;
      EXPECT_EVENTUALLY_EQ(afterOutPkts - 1, beforeOutPkts);
    });

    // Unresolve next hops
    this->applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      auto newState = ecmpHelper.unresolveNextHops(in, {ports[1]});
      return newState;
    });
    WITH_RETRIES({
      auto routeInfo3 = utility::getRouteInfo(
          routePrefix.network(), routePrefix.mask(), *this->getAgentEnsemble());
      EXPECT_EVENTUALLY_TRUE(*routeInfo3.isProgrammedToCpu());
      if (FLAGS_classid_for_unresolved_routes) {
        EXPECT_EVENTUALLY_TRUE(*routeInfo3.isRouteUnresolvedToClassId());
      }
    });

    // Resolve next hops
    this->applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      auto newState = ecmpHelper.resolveNextHops(in, {ports[1]});
      return newState;
    });
    WITH_RETRIES({
      EXPECT_EVENTUALLY_TRUE(
          utility::isRouteToNexthop(
              routePrefix.network(),
              routePrefix.mask(),
              ecmpHelper.nhop(ports[1]).ip,
              *this->getAgentEnsemble()));
      if (FLAGS_classid_for_unresolved_routes) {
        auto routeInfo4 = utility::getRouteInfo(
            routePrefix.network(),
            routePrefix.mask(),
            *this->getAgentEnsemble());
        EXPECT_EVENTUALLY_FALSE(*routeInfo4.isRouteUnresolvedToClassId());
      }
    });

    // Unresolve next hops
    this->applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      auto newState = ecmpHelper.unresolveNextHops(in, {ports[1]});
      return newState;
    });
    WITH_RETRIES({
      auto routeInfo5 = utility::getRouteInfo(
          routePrefix.network(), routePrefix.mask(), *this->getAgentEnsemble());
      EXPECT_EVENTUALLY_TRUE(*routeInfo5.isProgrammedToCpu());
      if (FLAGS_classid_for_unresolved_routes) {
        EXPECT_EVENTUALLY_TRUE(*routeInfo5.isRouteUnresolvedToClassId());
      }
    });
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(AgentRouteTest, VerifyDefaultRoute) {
  auto verify = [=, this]() {
    // default routes should exist always.
    auto routeInfo = utility::getRouteInfo(
        folly::IPAddress("::"), 0, *this->getAgentEnsemble());
    EXPECT_TRUE(*routeInfo.exists());
    auto routeInfo1 = utility::getRouteInfo(
        folly::IPAddress("0.0.0.0"), 0, *this->getAgentEnsemble());
    EXPECT_TRUE(*routeInfo1.exists());
  };
  this->verifyAcrossWarmBoots([] {}, verify);
}

TYPED_TEST(AgentClassIDRouteTest, VerifyClassIDForConnectedRoute) {
  auto verify = [=, this]() {
    auto ipAddr = this->getSubnetIpForInterface();
    // verify if the connected route of the interface is present
    auto routeInfo = utility::getRouteInfo(
        ipAddr.network(), ipAddr.mask(), *this->getAgentEnsemble());
    EXPECT_TRUE(*routeInfo.exists());
    auto asic = checkSameAndGetAsic(this->getAgentEnsemble()->getL3Asics());
    if (asic->getAsicVendor() != HwAsic::AsicVendor::ASIC_VENDOR_TAJO) {
      if (FLAGS_set_classid_for_my_subnet_and_ip_routes) {
        this->verifyClassIDHelper(
            ipAddr, cfg::AclLookupClass::DST_CLASS_L3_LOCAL_2);
      }
    }
  };

  this->verifyAcrossWarmBoots([] {}, verify);
}

TYPED_TEST(AgentMplsRouteTest, StaticIp2MplsRoutes) {
  using AddrT = typename TestFixture::Type;

  auto setup = [=, this]() {
    auto config = this->initialConfig(*this->getAgentEnsemble());

    config.staticIp2MplsRoutes()->resize(1);
    config.staticIp2MplsRoutes()[0].prefix() = this->kGetRoutePrefix1().str();

    NextHopThrift nexthop;
    nexthop.address() = toBinaryAddress(
        folly::IPAddress(
            this->kStaticIp2MplsNextHop().str())); // in prefix 0 subnet
    MplsAction action;
    action.action() = MplsActionCode::PUSH;
    action.pushLabels() = {1001, 1002};
    nexthop.mplsAction() = action;

    config.staticIp2MplsRoutes()[0].nexthops()->resize(1);
    config.staticIp2MplsRoutes()[0].nexthops()[0] = nexthop;
    this->applyNewConfig(config);

    // resolve prefix 0 subnet
    utility::EcmpSetupTargetedPorts<AddrT> ecmpHelper(
        this->getProgrammedState(),
        this->getSw()->needL2EntryForNeighbor(),
        this->kRouterID());
    auto wrapper = this->getSw()->getRouteUpdater();
    ecmpHelper.programRoutes(
        &wrapper,
        {this->portDescs()[0], this->portDescs()[1]},
        {this->kGetRoutePrefix0()});

    this->applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      auto newState = ecmpHelper.resolveNextHops(
          in, {this->portDescs()[0], this->portDescs()[1]});
      return newState;
    });
  };
  auto verify = [=, this]() {
    // prefix 1 subnet reachable via prefix 0 with mpls stack over this stack
    WITH_RETRIES({
      EXPECT_EVENTUALLY_TRUE(
          verifyProgrammedStack<AddrT>(
              this->kGetRoutePrefix1(),
              InterfaceID(utility::kBaseVlanId),
              {1001, 1002},
              1,
              *this->getAgentEnsemble()));
      EXPECT_EVENTUALLY_TRUE(
          verifyProgrammedStack<AddrT>(
              this->kGetRoutePrefix1(),
              InterfaceID(utility::kBaseVlanId + 1),
              {1001, 1002},
              1,
              *this->getAgentEnsemble()));
    });
  };
  this->verifyAcrossWarmBoots(setup, verify);
}
} // namespace facebook::fboss
