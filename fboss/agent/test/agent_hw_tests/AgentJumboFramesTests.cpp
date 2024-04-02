/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/test/AgentHwTest.h"

#include "fboss/agent/packet/PktFactory.h"
#include "fboss/lib/CommonUtils.h"

#include "fboss/agent/test/gen-cpp2/production_features_types.h"

#include "fboss/agent/packet/EthHdr.h"
#include "fboss/agent/packet/IPv4Hdr.h"
#include "fboss/agent/packet/IPv6Hdr.h"
#include "fboss/agent/packet/UDPHeader.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/lib/CommonUtils.h"

#include <folly/IPAddress.h>
#include "fboss/agent/TxPacket.h"

using folly::IPAddress;
using folly::IPAddressV6;
using std::string;

namespace facebook::fboss {

class AgentJumboFramesTest : public AgentHwTest {
 private:
  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    return {production_features::ProductionFeature::JUMBO_FRAMES};
  }

  void sendPkt(int payloadSize) {
    auto mac = utility::getFirstInterfaceMac(getProgrammedState());
    auto txPacket = utility::makeUDPTxPacket(
        getSw(),
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
    getSw()->sendPacketSwitchedAsync(std::move(txPacket));
  }

 protected:
  void runJumboFrameTest(int payloadSize, bool expectPacketDrop) {
    auto setup = [=, this]() {
      utility::EcmpSetupAnyNPorts6 ecmpHelper(
          getProgrammedState(), RouterID(0));
      resolveNeigborAndProgramRoutes(ecmpHelper, 1);
    };

    auto verify = [=, this]() {
      utility::EcmpSetupAnyNPorts6 ecmpHelper(
          getProgrammedState(), RouterID(0));
      auto port = ecmpHelper.ecmpPortDescriptorAt(0).phyPortID();
      auto portStatsBefore = getLatestPortStats(port);
      auto pktsBefore = *portStatsBefore.outUnicastPkts_();
      auto bytesBefore = *portStatsBefore.outBytes_();
      sendPkt(payloadSize);
      WITH_RETRIES({
        auto portStatsAfter = getLatestPortStats(port);
        auto pktsAfter = *portStatsAfter.outUnicastPkts_();
        auto bytesAfter = *portStatsAfter.outBytes_();
        XLOG(INFO) << " Before, pkts: " << pktsBefore
                   << " bytes: " << bytesBefore << " After, pkts: " << pktsAfter
                   << " bytes: " << bytesAfter;
        if (expectPacketDrop) {
          EXPECT_EVENTUALLY_EQ(pktsBefore, pktsAfter);
          EXPECT_EVENTUALLY_EQ(bytesBefore, bytesAfter);
        } else {
          EXPECT_EVENTUALLY_EQ(pktsBefore + 1, pktsAfter);
          EXPECT_EVENTUALLY_EQ(
              bytesBefore + EthHdr::SIZE + IPv6Hdr::SIZE + UDPHeader::size() +
                  payloadSize,
              bytesAfter);
        }
      });
    };
    verifyAcrossWarmBoots(setup, verify);
  }
};

TEST_F(AgentJumboFramesTest, JumboFramesGetThrough) {
  runJumboFrameTest(6000, false);
}

TEST_F(AgentJumboFramesTest, SuperJumboFramesGetDropped) {
  // 10k frame size leads to pkt buffer allocation failure on TH3/TH4
  runJumboFrameTest(9472, true);
}

} // namespace facebook::fboss
