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
    auto cfg = utility::onePortPerInterfaceConfig(
        getHwSwitch(),
        masterLogicalPortIds(),
        getAsic()->desiredLoopbackModes());
    cfg.interfaces()[0].mtu() = 9000;
    return cfg;
  }

  void sendPkt(int payloadSize) {
    auto mac = utility::getFirstInterfaceMac(getProgrammedState());
    auto txPacket = utility::makeUDPTxPacket(
        getHwSwitch(),
        utility::firstVlanID(getProgrammedState()),
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
      resolveNeigborAndProgramRoutes(ecmpHelper, 1);
    };

    auto verify = [=]() {
      utility::EcmpSetupAnyNPorts6 ecmpHelper(
          getProgrammedState(), RouterID(0));
      auto port = ecmpHelper.ecmpPortDescriptorAt(0).phyPortID();
      auto portStatsBefore = getLatestPortStats(port);
      auto pktsBefore = getPortOutPkts(portStatsBefore);
      auto bytesBefore = *portStatsBefore.outBytes_();
      sendPkt(payloadSize);
      auto portStatsAfter = getLatestPortStats(port);
      auto pktsAfter = getPortOutPkts(portStatsAfter);
      auto bytesAfter = *portStatsAfter.outBytes_();
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
  // PKTIO has an internal check in src/bcm/esw/pktio/pktio.c
  // disallowing frame over "#define MAX_FRAME_SIZE_DEF (9472)
  // The max frame in our system is 9412.
  // Setting the jumbo packet size to the max of PKTIO max
  // makes it exceed MTU and get dropped.
  // This test works in both PKTIO and non-PKTIO cases.
  runJumboFrameTest(9472, true);
}

} // namespace facebook::fboss
