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
        getAsic()->desiredLoopbackMode());
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
    getHwSwitchEnsemble()->ensureSendPacketOutOfPort(
        std::move(pkt), PortID(masterLogicalInterfacePortIds()[0]));
  }
};

TEST_F(HwInDiscardsCounterTest, nullRouteHit) {
  auto setup = [=]() {};
  auto verify = [=]() {
    PortID portId = masterLogicalInterfacePortIds()[0];
    auto portStatsBefore = getLatestPortStats(portId);
    pumpTraffic(true);
    pumpTraffic(false);
    auto portStatsAfter = getLatestPortStats(portId);
    EXPECT_EQ(
        2,
        *portStatsAfter.inDiscardsRaw_() - *portStatsBefore.inDiscardsRaw_());
    EXPECT_EQ(
        2,
        *portStatsAfter.inDstNullDiscards_() -
            *portStatsBefore.inDstNullDiscards_());
    EXPECT_EQ(
        0, *portStatsAfter.inDiscards_() - *portStatsBefore.inDiscards_());
  };
  verifyAcrossWarmBoots(setup, verify);
} // namespace facebook::fboss
} // namespace facebook::fboss
