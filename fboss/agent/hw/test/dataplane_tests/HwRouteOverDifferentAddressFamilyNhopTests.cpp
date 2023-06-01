/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/state/RouteNextHop.h"
#include "fboss/agent/state/RouteNextHopEntry.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/ResourceLibUtil.h"

namespace facebook::fboss {

class HwRouteOverDifferentAddressFamilyNhopTest
    : public HwLinkStateDependentTest {
 private:
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::onePortPerInterfaceConfig(
        getHwSwitch(),
        masterLogicalPortIds(),
        getAsic()->desiredLoopbackModes());
    return cfg;
  }
  folly::IPAddressV6 kIpv6(bool ecmp) const {
    return ecmp ? folly::IPAddressV6{"200::1"} : folly::IPAddressV6{"201::1"};
  }
  folly::IPAddressV4 kIpv4(bool ecmp) const {
    return ecmp ? folly::IPAddressV4{"200.0.0.1"}
                : folly::IPAddressV4{"201.0.0.1"};
  }
  folly::CIDRNetwork kPrefixV6(bool ecmp) const {
    return {kIpv6(ecmp), 64};
  }
  folly::CIDRNetwork kPrefixV4(bool ecmp) const {
    return {kIpv4(ecmp), 24};
  }
  template <typename AddrT>
  folly::CIDRNetwork getPrefix(bool ecmp) const {
    if constexpr (std::is_same_v<AddrT, folly::IPAddressV6>) {
      return kPrefixV6(ecmp);
    } else {
      return kPrefixV4(ecmp);
    }
  }
  template <typename AddrT>
  AddrT getDstIp(bool ecmp) const {
    if constexpr (std::is_same_v<AddrT, folly::IPAddressV6>) {
      return kIpv6(ecmp);
    } else {
      return kIpv4(ecmp);
    }
  }
  std::unique_ptr<TxPacket> makeTxPacket(const folly::IPAddress& dstIp) const {
    auto vlanId = utility::firstVlanID(getProgrammedState());
    auto intfMac = utility::getFirstInterfaceMac(getProgrammedState());
    auto srcMac = utility::MacAddressGenerator().get(intfMac.u64HBO() + 1);
    auto srcIp = dstIp.isV6() ? folly::IPAddress("100::1")
                              : folly::IPAddress("100.0.0.1");
    return utility::makeUDPTxPacket(
        getHwSwitch(),
        vlanId,
        srcMac, // src mac
        intfMac, // dst mac
        srcIp,
        dstIp,
        8000, // l4 src port
        8001 // l4 dst port
    );
  }

 protected:
  template <typename RouteAddrT, typename NhopAddrT>
  void runTest(bool useLinkLocalNhop = false) {
    if (useLinkLocalNhop) {
      if constexpr (std::is_same_v<NhopAddrT, folly::IPAddressV4>) {
        CHECK(false) << "Link local hop only supported for V6";
      }
    }
    auto setup = [this, useLinkLocalNhop] {
      utility::EcmpSetupTargetedPorts<NhopAddrT> ecmpHelper(
          getProgrammedState());
      auto addRoute = [this, &ecmpHelper, useLinkLocalNhop](
                          auto nw, const auto& nhopPorts) {
        RouteNextHopEntry::NextHopSet nhops;
        for (auto nhopPort : nhopPorts) {
          auto nhop = ecmpHelper.nhop(nhopPort);
          if (useLinkLocalNhop) {
            nhops.insert(
                ResolvedNextHop(*nhop.linkLocalNhopIp, nhop.intf, ECMP_WEIGHT));
          } else {
            nhops.insert(UnresolvedNextHop(nhop.ip, ECMP_WEIGHT));
          }
        }
        auto updater = getRouteUpdater();
        updater->addRoute(
            RouterID(0),
            nw.first,
            nw.second,
            ClientID::BGPD,
            RouteNextHopEntry(nhops, AdminDistance::EBGP));
        updater->program();
      };
      boost::container::flat_set<PortDescriptor> ecmpNhopPorts(
          {PortDescriptor(masterLogicalPortIds()[0]),
           PortDescriptor(masterLogicalPortIds()[1])});
      boost::container::flat_set<PortDescriptor> nonEcmpNhopPorts(
          {PortDescriptor(masterLogicalPortIds()[2])});
      for (auto ecmp : {true, false}) {
        auto nhopPorts = ecmp ? ecmpNhopPorts : nonEcmpNhopPorts;
        applyNewState(ecmpHelper.resolveNextHops(
            getProgrammedState(), nhopPorts, useLinkLocalNhop));
        addRoute(getPrefix<RouteAddrT>(ecmp), nhopPorts);
      }
    };
    auto verify = [this]() {
      for (auto ecmp : {true, false}) {
        getHwSwitchEnsemble()->ensureSendPacketSwitched(
            makeTxPacket(getDstIp<RouteAddrT>(ecmp)));
      }
    };
    verifyAcrossWarmBoots(setup, verify);
  }
};

TEST_F(HwRouteOverDifferentAddressFamilyNhopTest, v4RouteV6Nhops) {
  runTest<folly::IPAddressV4, folly::IPAddressV6>();
}

TEST_F(HwRouteOverDifferentAddressFamilyNhopTest, v4RouteV6LinkLocalNhops) {
  runTest<folly::IPAddressV4, folly::IPAddressV6>(
      true /*use link local nhops*/);
}

TEST_F(HwRouteOverDifferentAddressFamilyNhopTest, v6RouteV4Nhops) {
  runTest<folly::IPAddressV6, folly::IPAddressV4>();
}
} // namespace facebook::fboss
