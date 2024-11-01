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

#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwTestRouteUtils.h"
#include "folly/IPAddressV4.h"
#include "folly/IPAddressV6.h"

using facebook::network::toBinaryAddress;

DECLARE_bool(intf_nbr_tables);
DECLARE_bool(classid_for_unresolved_routes);

namespace facebook::fboss {

template <typename AddrT>
class HwRouteTest : public HwLinkStateDependentTest {
 public:
  using Type = AddrT;

 protected:
  cfg::SwitchConfig initialConfig() const override {
    return utility::onePortPerInterfaceConfig(
        getHwSwitch(),
        masterLogicalPortIds(),
        getAsic()->desiredLoopbackModes(),
        true);
  }

  HwSwitchEnsemble::Features featuresDesired() const override {
    return {HwSwitchEnsemble::LINKSCAN, HwSwitchEnsemble::PACKET_RX};
  }

  RouterID kRouterID() const {
    return RouterID(0);
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
};

template <typename AddrType, bool enableIntfNbrTable>
struct IpAddrAndEnableIntfNbrTableT {
  using AddrT = AddrType;
  static constexpr auto intfNbrTable = enableIntfNbrTable;
};

using NeighborTableTypes = ::testing::Types<
    IpAddrAndEnableIntfNbrTableT<folly::IPAddressV4, false>,
    IpAddrAndEnableIntfNbrTableT<folly::IPAddressV4, true>,
    IpAddrAndEnableIntfNbrTableT<folly::IPAddressV6, false>,
    IpAddrAndEnableIntfNbrTableT<folly::IPAddressV6, true>>;

template <typename IpAddrAndEnableIntfNbrTableT>
class HwRouteNeighborTest
    : public HwRouteTest<typename IpAddrAndEnableIntfNbrTableT::AddrT> {
  static auto constexpr intfNbrTable =
      IpAddrAndEnableIntfNbrTableT::intfNbrTable;

 public:
  bool isIntfNbrTable() const {
    return intfNbrTable == true;
  }

  void SetUp() override {
    FLAGS_intf_nbr_tables = isIntfNbrTable();
    HwLinkStateDependentTest::SetUp();
  }
};

TYPED_TEST_SUITE(HwRouteNeighborTest, NeighborTableTypes);

TYPED_TEST(HwRouteNeighborTest, AddHostRouteAndNeighbor) {
  using AddrT = typename TestFixture::Type;
  auto setup = [=, this]() {
    auto ip = this->kGetRoutePrefix3().network();
    auto portId = this->masterLogicalInterfacePortIds()[0];
    auto port = this->getProgrammedState()->getPort(portId);
    auto state = this->getProgrammedState();
    auto getNeighborTable = [&]() {
      auto switchType = this->getSwitchType();

      if (this->isIntfNbrTable() || switchType == cfg::SwitchType::VOQ) {
        auto intfId = port->getInterfaceID();
        return state->getInterfaces()
            ->getNode(intfId)
            ->template getNeighborEntryTable<AddrT>()
            ->modify(intfId, &state);
      } else if (switchType == cfg::SwitchType::NPU) {
        auto vlanId = port->getVlans().begin()->first;
        return state->getVlans()
            ->getNode(vlanId)
            ->template getNeighborEntryTable<AddrT>()
            ->modify(vlanId, &state);
      }

      XLOG(FATAL) << "Unexpected switch type " << static_cast<int>(switchType);
    };
    // add neighbor
    auto nbrTable = getNeighborTable();
    folly::MacAddress neighborMac = folly::MacAddress("06:00:00:01:02:03");
    nbrTable->addEntry(
        ip,
        neighborMac,
        PortDescriptor(PortID(portId)),
        port->getInterfaceID());
    this->applyNewState(state);

    // add host route
    auto updater = this->getHwSwitchEnsemble()->getRouteUpdater();
    RouteNextHopSet nexthops;
    // @lint-ignore CLANGTIDY
    nexthops.emplace(ResolvedNextHop(ip, port->getInterfaceID(), ECMP_WEIGHT));
    updater.addRoute(
        this->kRouterID(),
        ip,
        AddrT::bitCount(),
        ClientID::BGPD,
        RouteNextHopEntry(nexthops, AdminDistance::EBGP));
    updater.program();
  };
  auto verify = [=, this]() {
    utility::isHwRoutePresent(
        this->getHwSwitch(),
        this->kRouterID(),
        this->kGetRoutePrefix3().toCidrNetwork());
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
