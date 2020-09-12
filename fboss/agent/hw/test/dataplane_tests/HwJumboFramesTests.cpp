/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/Platform.h"
#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/test/EcmpSetupHelper.h"

#include "fboss/agent/hw/test/ConfigFactory.h"

#include "fboss/agent/packet/EthHdr.h"
#include "fboss/agent/packet/IPv4Hdr.h"
#include "fboss/agent/packet/IPv6Hdr.h"
#include "fboss/agent/packet/UDPHeader.h"
#include "fboss/agent/state/SwitchState.h"

#include <folly/IPAddress.h>

using folly::IPAddress;
using folly::IPAddressV6;
using std::string;

namespace facebook::fboss {

class HwJumboFramesTest : public HwLinkStateDependentTest {
 private:
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::oneL3IntfConfig(
        getHwSwitch(), masterLogicalPortIds()[0], cfg::PortLoopbackMode::MAC);
    cfg.interfaces_ref()[0].mtu_ref() = 9000;
    return cfg;
  }

  void sendPkt(int payloadSize) {
    auto vlanId = VlanID(*initialConfig().vlanPorts_ref()[0].vlanID_ref());
    auto mac = utility::getInterfaceMac(getProgrammedState(), vlanId);
    auto txPacket = utility::makeUDPTxPacket(
        getHwSwitch(),
        vlanId,
        mac,
        mac,
        folly::IPAddressV6("2620:0:1cfe:face:b00c::3"),
        folly::IPAddressV6("2620:0:1cfe:face:b00c::4"),
        8000,
        8001,
        0,
        255,
        std::vector<uint8_t>(payloadSize, 0xff));
    getHwSwitchEnsemble()->ensureSendPacketSwitched(std::move(txPacket));
  }

 protected:
  void runJumboFrameTest(int payloadSize, bool expectPacketDrop) {
    auto setup = [=]() {
      utility::EcmpSetupAnyNPorts6 ecmpHelper(
          getProgrammedState(), RouterID(0));
      applyNewState(ecmpHelper.setupECMPForwarding(
          ecmpHelper.resolveNextHops(getProgrammedState(), 1), 1));
    };

    auto verify = [=]() {
      auto portStatsBefore = getLatestPortStats(masterLogicalPortIds()[0]);
      auto pktsBefore = getPortOutPkts(portStatsBefore);
      auto bytesBefore = *portStatsBefore.outBytes__ref();
      sendPkt(payloadSize);
      auto portStatsAfter = getLatestPortStats(masterLogicalPortIds()[0]);
      auto pktsAfter = getPortOutPkts(portStatsAfter);
      auto bytesAfter = *portStatsAfter.outBytes__ref();
      if (expectPacketDrop) {
        EXPECT_EQ(pktsBefore, pktsAfter);
        EXPECT_EQ(bytesBefore, bytesAfter);
      } else {
        EXPECT_EQ(pktsBefore + 1, pktsAfter);
        EXPECT_EQ(
            bytesBefore + EthHdr::SIZE + IPv6Hdr::SIZE + UDPHeader::size() +
                payloadSize,
            bytesAfter);
      }
    };
    verifyAcrossWarmBoots(setup, verify);
  }
};

TEST_F(HwJumboFramesTest, JumboFramesGetThrough) {
  runJumboFrameTest(6000, false);
}

TEST_F(HwJumboFramesTest, SuperJumboFramesGetDropped) {
  runJumboFrameTest(60000, true);
}

} // namespace facebook::fboss
