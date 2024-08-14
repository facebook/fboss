/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/packet/EthFrame.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"

#include "fboss/agent/test/utils/AclTestUtils.h"
#include "fboss/agent/test/utils/AsicUtils.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/CoppTestUtils.h"
#include "fboss/agent/test/utils/DscpMarkingUtils.h"
#include "fboss/agent/test/utils/OlympicTestUtils.h"

#include "fboss/agent/test/utils/QosTestUtils.h"
#include "fboss/agent/types.h"
#include "fboss/lib/CommonUtils.h"

#include <folly/IPAddress.h>
#include <gtest/gtest.h>

namespace facebook::fboss {

class AgentDscpMarkingTest : public AgentHwTest {
 protected:
  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    return {production_features::ProductionFeature::DSCP_REMARKING};
  }

  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto cfg = utility::onePortPerInterfaceConfig(
        ensemble.getSw(),
        ensemble.masterLogicalPortIds(),
        true /*interfaceHasSubnet*/);
    auto l3Asics = ensemble.getL3Asics();
    auto asic = utility::checkSameAndGetAsic(l3Asics);
    utility::addOlympicQosMaps(cfg, l3Asics);
    // drop packets are match to avoid packets matching multiple times in L3
    // loop case
    utility::addDscpCounterAcl(asic, &cfg, cfg::AclActionType::DENY);
    utility::addDscpMarkingAcls(asic, &cfg, ensemble.isSai());
    return cfg;
  }

  void verifyDscpMarking() {
    /*
     * ACL1:: Matcher: ICP DSCP. Action: Increment stat.
     * ACL2:: Matcher: L4SrcPort/L4DstPort. Action: SET_DSCP, SET_TC.
     *
     * ACL1 precedes ACL2.
     * ACL2 mimics prod ACL, ACL1 is used only for test verification.
     *
     * Inject a pkt with dscp = 0, l4SrcPort matching ACL2:
     *   - The packet will match ACL2, thus SET_DSCP, SET_TC.
     *   - Verify ICP queue counters (verifies SET_TC action).
     *   - Packet egress via front panel port which is in loopback mode.
     *   - Thus, packet gets looped back.
     *   - Verify ACL1 counter (verifies SET_DSCP action).
     *
     * Inject a pkt with dscp = ICP DSCP, l4SrcPort matching ACL2:
     *   - The packet will match ACL1, thus counter incremented.
     *   - Packet dropped after hitting ACL1
     */

    auto setup = [=, this]() {
      utility::EcmpSetupAnyNPorts6 ecmpHelper(
          getProgrammedState(),
          utility::getFirstInterfaceMac(getProgrammedState()),
          RouterID(0),
          false,
          {cfg::PortType::INTERFACE_PORT});
      resolveNeigborAndProgramRoutes(ecmpHelper, kEcmpWidth);
    };

    auto verify = [=, this]() {
      utility::EcmpSetupAnyNPorts6 ecmpHelper(
          getProgrammedState(),
          utility::getFirstInterfaceMac(getProgrammedState()),
          RouterID(0),
          false,
          {cfg::PortType::INTERFACE_PORT});
      auto portId = ecmpHelper.ecmpPortDescriptorAt(0).phyPortID();
      auto portStatsBefore = getLatestPortStats(portId);

      for (bool frontPanel : {false, true}) {
        XLOG(DBG2) << "verify send packets "
                   << (frontPanel ? "out of port" : "switched");
        auto beforeAclInOutPkts =
            utility::getAclInOutPackets(getSw(), utility::kCounterName());

        sendAllPackets(
            0 /* No Dscp */,
            frontPanel,
            IP_PROTO::IP_PROTO_UDP,
            utility::kUdpPorts());
        sendAllPackets(
            0 /* No Dscp */,
            frontPanel,
            IP_PROTO::IP_PROTO_TCP,
            utility::kTcpPorts());

        int64_t afterAclInOutPkts = 0;
        WITH_RETRIES_N(60, {
          afterAclInOutPkts =
              utility::getAclInOutPackets(getSw(), utility::kCounterName());

          // See detailed comment block at the beginning of this function
          EXPECT_EVENTUALLY_EQ(
              afterAclInOutPkts - beforeAclInOutPkts,
              (1 /* ACL hit once */ * 2 /* l4SrcPort, l4DstPort */ *
               utility::kUdpPorts().size()) +
                  (1 /* ACL hit once */ * 2 /* l4SrcPort, l4DstPort */ *
                   utility::kTcpPorts().size()));
        });
        utility::verifyQueueHit(
            portStatsBefore, utility::kOlympicICPQueueId, getSw(), portId);

        sendAllPackets(
            utility::kIcpDscp(),
            frontPanel,
            IP_PROTO::IP_PROTO_UDP,
            utility::kUdpPorts());
        sendAllPackets(
            utility::kIcpDscp(),
            frontPanel,
            IP_PROTO::IP_PROTO_TCP,
            utility::kTcpPorts());

        WITH_RETRIES_N(60, {
          auto afterAclInOutPkts2 =
              utility::getAclInOutPackets(getSw(), utility::kCounterName());

          // See detailed comment block at the beginning of this function
          EXPECT_EVENTUALLY_EQ(
              afterAclInOutPkts2 - afterAclInOutPkts,
              (1 /* ACL hit once */ * 2 /* l4SrcPort, l4DstPort */ *
               utility::kUdpPorts().size()) +
                  (1 /* ACL hit once */ * 2 /* l4SrcPort, l4DstPort */ *
                   utility::kTcpPorts().size()));
        });
      }
    };

    verifyAcrossWarmBoots(setup, verify);
  }

 private:
  void sendAllPackets(
      uint8_t dscp,
      bool frontPanel,
      IP_PROTO proto,
      const std::vector<uint32_t>& ports) {
    for (auto port : ports) {
      sendPacket(
          dscp,
          frontPanel,
          proto,
          port /* l4SrcPort */,
          std::nullopt /* l4DstPort */);
      sendPacket(
          dscp,
          frontPanel,
          proto,
          std::nullopt /* l4SrcPort */,
          port /* l4DstPort */);
    }
  }

  void sendPacket(
      uint8_t dscp,
      bool frontPanel,
      IP_PROTO proto,
      std::optional<uint16_t> l4SrcPort,
      std::optional<uint16_t> l4DstPort) {
    auto vlanId = utility::firstVlanID(getProgrammedState());
    auto intfMac = utility::getFirstInterfaceMac(getProgrammedState());
    auto srcMac = utility::MacAddressGenerator().get(intfMac.u64NBO() + 1);

    std::unique_ptr<facebook::fboss::TxPacket> txPacket;
    if (proto == IP_PROTO::IP_PROTO_UDP) {
      txPacket = utility::makeUDPTxPacket(
          getSw(),
          vlanId,
          srcMac, // src mac
          intfMac, // dst mac
          folly::IPAddressV6("2620:0:1cfe:face:b00c::3"), // src ip
          folly::IPAddressV6("2620:0:1cfe:face:b00c::4"), // dst ip
          l4SrcPort.has_value() ? l4SrcPort.value() : 8000,
          l4DstPort.has_value() ? l4DstPort.value() : 8001,
          dscp << 2, // shifted by 2 bits for ECN
          255 // ttl
      );
    } else if (proto == IP_PROTO::IP_PROTO_TCP) {
      txPacket = utility::makeTCPTxPacket(
          getSw(),
          vlanId,
          srcMac, // src mac
          intfMac, // dst mac
          folly::IPAddressV6("2620:0:1cfe:face:b00c::3"), // src ip
          folly::IPAddressV6("2620:0:1cfe:face:b00c::4"), // dst ip
          l4SrcPort.has_value() ? l4SrcPort.value() : 8000,
          l4DstPort.has_value() ? l4DstPort.value() : 8001,
          dscp << 2, // shifted by 2 bits for ECN
          255 // ttl
      );
    } else {
      CHECK(false);
    }

    // port is in LB mode, so it will egress and immediately loop back.
    // Since it is not re-written, it should hit the pipeline as if it
    // ingressed on the port, and be properly queued.
    if (frontPanel) {
      utility::EcmpSetupAnyNPorts6 ecmpHelper(
          getProgrammedState(),
          utility::getFirstInterfaceMac(getProgrammedState()),
          RouterID(0),
          false,
          {cfg::PortType::INTERFACE_PORT});
      auto outPort = ecmpHelper.ecmpPortDescriptorAt(kEcmpWidth).phyPortID();
      getSw()->sendPacketOutOfPortAsync(std::move(txPacket), outPort);
    } else {
      getSw()->sendPacketSwitchedAsync(std::move(txPacket));
    }
  }

  static inline constexpr auto kEcmpWidth = 1;
};

// Verify that the DSCP unmarked traffic to specific L4 src/dst ports that
// arrives on a forntl panel/cpu port is DSCP marked correctly as well as
// egresses via the right QoS queue.
TEST_F(AgentDscpMarkingTest, VerifyDscpMarking) {
  verifyDscpMarking();
}

} // namespace facebook::fboss
