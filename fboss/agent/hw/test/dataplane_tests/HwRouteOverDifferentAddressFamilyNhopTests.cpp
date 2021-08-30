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
#include "fboss/agent/hw/test/dataplane_tests/HwTestOlympicUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwTestQosUtils.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/ResourceLibUtil.h"
#include "fboss/agent/test/TestUtils.h"

namespace facebook::fboss {

class HwRouteOverDifferentAddressFamilyNhopTest
    : public HwLinkStateDependentTest {
 private:
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::onePortPerVlanConfig(
        getHwSwitch(), masterLogicalPortIds(), cfg::PortLoopbackMode::MAC);
    return cfg;
  }
  folly::IPAddressV6 kIpv6() const {
    return folly::IPAddressV6{"200::1"};
  }
  folly::IPAddressV4 kIpv4() const {
    return folly::IPAddressV4{"200.0.0.1"};
  }
  folly::CIDRNetwork kPrefixV6() const {
    return {kIpv6(), 64};
  }
  folly::CIDRNetwork kPrefixV4() const {
    return {kIpv4(), 24};
  }
  template <typename AddrT>
  folly::CIDRNetwork getPrefix() const {
    if constexpr (std::is_same_v<AddrT, folly::IPAddressV6>) {
      return kPrefixV6();
    } else {
      return kPrefixV4();
    }
  }

 protected:
  template <typename RouteAddrT, typename NhopAddrT>
  void runTest() {
    auto setup = [this] {
      auto addRoute = [this](auto nw, const auto& nhops) {
        auto updater = getRouteUpdater();
        updater->addRoute(
            RouterID(0),
            nw.first,
            nw.second,
            ClientID::BGPD,
            RouteNextHopEntry(nhops, AdminDistance::EBGP));
        updater->program();
      };
      utility::EcmpSetupTargetedPorts<NhopAddrT> ecmpHelper(
          getProgrammedState());
      PortDescriptor nhopPort(masterLogicalPortIds()[0]);
      applyNewState(
          ecmpHelper.resolveNextHops(getProgrammedState(), {nhopPort}));
      auto nhops = makeNextHops<NhopAddrT>({ecmpHelper.nhop(nhopPort).ip});
      addRoute(getPrefix<RouteAddrT>(), nhops);
    };
    auto verify = []() {
      // TODO
    };
    verifyAcrossWarmBoots(setup, verify);
  }
};

TEST_F(HwRouteOverDifferentAddressFamilyNhopTest, v4RouteV6Nhops) {
  runTest<folly::IPAddressV4, folly::IPAddressV6>();
}

TEST_F(HwRouteOverDifferentAddressFamilyNhopTest, v6RouteV4Nhops) {
  runTest<folly::IPAddressV6, folly::IPAddressV4>();
}
} // namespace facebook::fboss
