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

#include "fboss/agent/Platform.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/test/EcmpSetupHelper.h"

#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/lib/CommonUtils.h"

using folly::IPAddress;
using folly::IPAddressV6;
using std::string;

namespace facebook::fboss {

class HwInDiscardsCounterTest : public HwLinkStateDependentTest {
 private:
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::onePortPerInterfaceConfig(
        getHwSwitch(),
        masterLogicalPortIds(),
        getAsic()->desiredLoopbackModes());
    cfg.staticRoutesToNull()->resize(2);
    *cfg.staticRoutesToNull()[0].routerID() =
        *cfg.staticRoutesToNull()[1].routerID() = 0;
    *cfg.staticRoutesToNull()[0].prefix() = "0.0.0.0/0";
    *cfg.staticRoutesToNull()[1].prefix() = "::/0";
    return cfg;
  }

 protected:
  void pumpTraffic(bool isV6) {
    auto vlanId = utility::firstVlanID(getProgrammedState());
    auto intfMac = utility::getFirstInterfaceMac(getProgrammedState());
    auto srcIp = IPAddress(isV6 ? "1001::1" : "10.0.0.1");
    auto dstIp = IPAddress(isV6 ? "100:100:100::1" : "100.100.100.1");
    auto pkt = utility::makeUDPTxPacket(
        getHwSwitch(), vlanId, intfMac, intfMac, srcIp, dstIp, 10000, 10001);
    getHwSwitch()->sendPacketOutOfPortAsync(
        std::move(pkt), PortID(masterLogicalInterfacePortIds()[0]));
  }
};

TEST_F(HwInDiscardsCounterTest, nullRouteHit) {
  auto setup = [=]() {};
  auto verify = [=, this]() {
    PortID portId = masterLogicalInterfacePortIds()[0];
    auto portStatsBefore = getLatestPortStats(portId);
    PortID otherPortId = masterLogicalInterfacePortIds()[1];
    auto otherPortStatsBefore = getLatestPortStats(otherPortId);
    auto switchDropStatsBefore = getHwSwitch()->getSwitchDropStats();
    pumpTraffic(true);
    pumpTraffic(false);
    WITH_RETRIES({
      auto portStatsAfter = getLatestPortStats(portId);
      EXPECT_EVENTUALLY_EQ(
          2,
          *portStatsAfter.inDiscardsRaw_() - *portStatsBefore.inDiscardsRaw_());
      EXPECT_EVENTUALLY_EQ(
          2,
          *portStatsAfter.inDstNullDiscards_() -
              *portStatsBefore.inDstNullDiscards_());
      EXPECT_EVENTUALLY_EQ(
          *portStatsAfter.inDiscardsRaw_(),
          *portStatsAfter.inDstNullDiscards_());
      if (getAsic()->getAsicType() == cfg::AsicType::ASIC_TYPE_JERICHO3) {
        auto switchDropStats = getHwSwitch()->getSwitchDropStats();
        EXPECT_EVENTUALLY_EQ(
            2,
            *switchDropStats.ingressPacketPipelineRejectDrops() -
                *switchDropStatsBefore.ingressPacketPipelineRejectDrops());
      }
    });
    // Do a -ve test as well, discard counters should not increment
    auto otherPortStatsAfter = getLatestPortStats(otherPortId);
    EXPECT_EQ(
        0,
        *otherPortStatsAfter.inDiscardsRaw_() -
            *otherPortStatsBefore.inDiscardsRaw_());
    EXPECT_EQ(
        0,
        *otherPortStatsAfter.inDstNullDiscards_() -
            *otherPortStatsBefore.inDstNullDiscards_());
    EXPECT_EQ(
        0,
        *otherPortStatsAfter.inDiscards_() -
            *otherPortStatsBefore.inDiscards_());
  };
  verifyAcrossWarmBoots(setup, verify);
} // namespace facebook::fboss
} // namespace facebook::fboss
