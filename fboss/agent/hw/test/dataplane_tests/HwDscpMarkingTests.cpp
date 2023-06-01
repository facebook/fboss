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
#include "fboss/agent/hw/test/HwTestAclUtils.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/hw/test/TrafficPolicyUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwTestDscpMarkingUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwTestOlympicUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwTestQosUtils.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/ResourceLibUtil.h"

namespace facebook::fboss {

class HwDscpMarkingTest : public HwLinkStateDependentTest {
 protected:
  void SetUp() override {
    HwLinkStateDependentTest::SetUp();
    helper_ = std::make_unique<utility::EcmpSetupAnyNPorts6>(
        getProgrammedState(), RouterID(0));
  }
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::onePortPerInterfaceConfig(
        getHwSwitch(),
        masterLogicalPortIds(),
        getAsic()->desiredLoopbackModes());
    return cfg;
  }

  void verifyDscpMarking(bool frontPanel) {
    if (!isSupported(HwAsic::Feature::L3_QOS)) {
      return;
    }

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
     *   - Packet egress via front panel port which is in loopback mode.
     *   - Thus, packet gets looped back.
     *   - Hits ACL1 again, and thus counter incremented twice.
     */

    auto setup = [=]() {
      resolveNeigborAndProgramRoutes(*helper_, kEcmpWidth);

      auto newCfg{initialConfig()};
      utility::addOlympicQosMaps(newCfg, getAsic());
      utility::addDscpCounterAcl(&newCfg, getAsic());
      utility::addDscpMarkingAcls(&newCfg, getAsic());

      applyNewConfig(newCfg);
    };

    auto verify = [=]() {
      auto portId = helper_->ecmpPortDescriptorAt(0).phyPortID();
      auto portStatsBefore = getLatestPortStats(portId);

      auto beforeAclInOutPkts = utility::getAclInOutPackets(
          getHwSwitch(),
          getProgrammedState(),
          utility::kDscpCounterAclName(),
          utility::kCounterName());

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

      auto afterAclInOutPkts = utility::getAclInOutPackets(
          getHwSwitch(),
          getProgrammedState(),
          utility::kDscpCounterAclName(),
          utility::kCounterName());

      // See detailed comment block at the beginning of this function
      EXPECT_EQ(
          afterAclInOutPkts - beforeAclInOutPkts,
          (1 /* ACL hit once */ * 2 /* l4SrcPort, l4DstPort */ *
           utility::kUdpPorts().size()) +
              (1 /* ACL hit once */ * 2 /* l4SrcPort, l4DstPort */ *
               utility::kTcpPorts().size()));

      EXPECT_TRUE(utility::verifyQueueMappings(
          portStatsBefore,
          {{utility::kOlympicICPQueueId, {utility::kIcpDscp(getAsic())}}},
          getHwSwitchEnsemble(),
          portId));

      sendAllPackets(
          utility::kIcpDscp(getAsic()),
          frontPanel,
          IP_PROTO::IP_PROTO_UDP,
          utility::kUdpPorts());
      sendAllPackets(
          utility::kIcpDscp(getAsic()),
          frontPanel,
          IP_PROTO::IP_PROTO_TCP,
          utility::kTcpPorts());

      auto afterAclInOutPkts2 = utility::getAclInOutPackets(
          getHwSwitch(),
          getProgrammedState(),
          utility::kDscpCounterAclName(),
          utility::kCounterName());

      // See detailed comment block at the beginning of this function
      EXPECT_EQ(
          afterAclInOutPkts2 - afterAclInOutPkts,
          (2 /* ACL hit twice */ * 2 /* l4SrcPort, l4DstPort */ *
           utility::kUdpPorts().size()) +
              (2 /* ACL hit twice */ * 2 /* l4SrcPort, l4DstPort */ *
               utility::kTcpPorts().size()));
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
    auto vlanId = VlanID(*initialConfig().vlanPorts()[0].vlanID());
    auto intfMac = utility::getInterfaceMac(getProgrammedState(), vlanId);
    auto srcMac = utility::MacAddressGenerator().get(intfMac.u64NBO() + 1);

    std::unique_ptr<facebook::fboss::TxPacket> txPacket;
    if (proto == IP_PROTO::IP_PROTO_UDP) {
      txPacket = utility::makeUDPTxPacket(
          getHwSwitch(),
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
          getHwSwitch(),
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
      auto outPort = helper_->ecmpPortDescriptorAt(kEcmpWidth).phyPortID();
      getHwSwitchEnsemble()->getHwSwitch()->sendPacketOutOfPortSync(
          std::move(txPacket), outPort);
    } else {
      getHwSwitchEnsemble()->getHwSwitch()->sendPacketSwitchedSync(
          std::move(txPacket));
    }
  }

  static inline constexpr auto kEcmpWidth = 1;
  std::unique_ptr<utility::EcmpSetupAnyNPorts6> helper_;
};

// Verify that the DSCP unmarked traffic to specific L4 src/dst ports that
// arrives on a forntl panel port is DSCP marked correctly as well as egresses
// via the right QoS queue.
TEST_F(HwDscpMarkingTest, VerifyDscpMarkingFrontPanel) {
  verifyDscpMarking(true);
}

// Verify that the DSCP unmarked traffic to specific L4 src/dst ports that
// originates from a CPU port is  DSCP marked correctly as well as egresses
// via the right QoS queue.
TEST_F(HwDscpMarkingTest, VerifyDscpMarkingCpu) {
  verifyDscpMarking(false);
}

} // namespace facebook::fboss
