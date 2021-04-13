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
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/hw/test/HwTestRouteUtils.h"

#include "fboss/agent/hw/test/ConfigFactory.h"

using folly::IPAddress;
using folly::IPAddressV6;

namespace facebook::fboss {

template <typename AddrT>
class HwL3Test : public HwLinkStateDependentTest {
 protected:
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::oneL3IntfConfig(
        getHwSwitch(), masterLogicalPortIds()[0], cfg::PortLoopbackMode::MAC);
    return cfg;
  }

  RoutePrefix<AddrT> kGetRoutePrefix() const {
    if constexpr (std::is_same_v<AddrT, folly::IPAddressV4>) {
      return RoutePrefix<folly::IPAddressV4>{folly::IPAddressV4{"1.1.1.0"}, 24};
    } else {
      return RoutePrefix<folly::IPAddressV6>{folly::IPAddressV6{"1::"}, 64};
    }
  }

  AddrT kSrcIP() {
    if constexpr (std::is_same<AddrT, folly::IPAddressV4>::value) {
      return folly::IPAddressV4("1.1.1.1");
    } else {
      return folly::IPAddressV6("1::1");
    }
  }

  AddrT kDstIP() {
    if constexpr (std::is_same<AddrT, folly::IPAddressV4>::value) {
      return folly::IPAddressV4("1.1.1.3");
    } else {
      return folly::IPAddressV6("1::3");
    }
  }

  void checkRouteHit() {
    auto setup = [=]() {};

    auto verify = [=]() {
      auto vlanId = utility::firstVlanID(initialConfig());
      auto intfMac = utility::getInterfaceMac(getProgrammedState(), vlanId);
      RoutePrefix<AddrT> prefix(kGetRoutePrefix());
      auto cidr = folly::CIDRNetwork(prefix.network, prefix.mask);

      // Ensure hit bit is NOT set
      EXPECT_FALSE(
          utility::isHwRouteHit(this->getHwSwitch(), RouterID(0), cidr));

      // Construct and send packet
      auto pkt = utility::makeIpTxPacket(
          getHwSwitch(),
          vlanId,
          intfMac,
          intfMac,
          this->kSrcIP(),
          this->kDstIP());
      getHwSwitchEnsemble()->ensureSendPacketSwitched(std::move(pkt));

      // Verify hit bit is set
      EXPECT_TRUE(
          utility::isHwRouteHit(this->getHwSwitch(), RouterID(0), cidr));

      // Clear hit bit and verify
      utility::clearHwRouteHit(this->getHwSwitch(), RouterID(0), cidr);
      EXPECT_FALSE(
          utility::isHwRouteHit(this->getHwSwitch(), RouterID(0), cidr));
    };

    // TODO(vsp): Once hitbit is set, ensure its preserved after warm boot.
    verifyAcrossWarmBoots(setup, verify);
  }
};

using IpTypes = ::testing::Types<folly::IPAddressV4, folly::IPAddressV6>;
TYPED_TEST_SUITE(HwL3Test, IpTypes);

TYPED_TEST(HwL3Test, HitBit) {
  this->checkRouteHit();
}

} // namespace facebook::fboss
