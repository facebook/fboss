/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <folly/IPAddressV6.h>

#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwLinkStateDependentTest.h"
#include "fboss/agent/hw/test/HwTestAclUtils.h"
#include "fboss/agent/hw/test/HwTestCoppUtils.h"
#include "fboss/agent/hw/test/HwTestMplsUtils.h"
#include "fboss/agent/hw/test/HwTestPacketSnooper.h"
#include "fboss/agent/hw/test/HwTestPacketTrapEntry.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/hw/test/TrafficPolicyUtils.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/packet/PktUtil.h"
#include "fboss/agent/state/LabelForwardingEntry.h"
#include "fboss/agent/state/PortDescriptor.h"
#include "fboss/agent/state/RouteNextHop.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/TrunkUtils.h"
#include "fboss/agent/types.h"

#include <gtest/gtest.h>
#include <memory>

namespace {
const facebook::fboss::LabelForwardingEntry::Label kTopLabel{1101};
constexpr auto kGetQueueOutPktsRetryTimes = 5;
using TestTypes =
    ::testing::Types<facebook::fboss::PortID, facebook::fboss::AggregatePortID>;
} // namespace

namespace facebook::fboss {

template <typename PortType>
class HwMPLSTest : public HwLinkStateDependentTest {
  struct HwPacketVerifier {
    HwPacketVerifier(HwSwitchEnsemble* ensemble, PortID port, MPLSHdr hdr)
        : ensemble_(ensemble), entry_{}, snooper_{}, expectedHdr_(hdr) {
      if (!ensemble->getPlatform()->getAsic()->isSupported(
              HwAsic::Feature::SAI_ACL_ENTRY_SRC_PORT_QUALIFIER)) {
        return;
      }
      // capture packet exiting port (entering back due to loopback)
      entry_ = std::make_unique<HwTestPacketTrapEntry>(
          ensemble->getHwSwitch(), port);
      snooper_ = std::make_unique<HwTestPacketSnooper>(ensemble_);
    }

    ~HwPacketVerifier() {
      if (!ensemble_->getPlatform()->getAsic()->isSupported(
              HwAsic::Feature::SAI_ACL_ENTRY_SRC_PORT_QUALIFIER)) {
        return;
      }
      auto pkt = snooper_->waitForPacket(10);
      auto mplsPayLoad = pkt ? pkt->mplsPayLoad() : std::nullopt;
      EXPECT_TRUE(mplsPayLoad.has_value());
      if (mplsPayLoad) {
        EXPECT_EQ(mplsPayLoad->header(), expectedHdr_);
      }
    }

    HwSwitchEnsemble* ensemble_;
    std::unique_ptr<HwTestPacketTrapEntry> entry_;
    std::unique_ptr<HwTestPacketSnooper> snooper_;
    MPLSHdr expectedHdr_;
  };

 protected:
  void SetUp() override {
    HwLinkStateDependentTest::SetUp();
    ecmpHelper_ = std::make_unique<utility::EcmpSetupTargetedPorts6>(
        getProgrammedState(), RouterID(0));

    ecmpSwapHelper_ = std::make_unique<
        utility::MplsEcmpSetupTargetedPorts<folly::IPAddressV6>>(
        getProgrammedState(),
        kTopLabel,
        LabelForwardingAction::LabelForwardingType::SWAP);
  }

  PortDescriptor getPortDescriptor(int index) {
    if constexpr (std::is_same_v<PortType, PortID>) {
      return PortDescriptor(masterLogicalPortIds()[index]);
    } else {
      return PortDescriptor(AggregatePortID(index + 1));
    }
  }

  cfg::SwitchConfig initialConfig() const override {
    std::vector<PortID> ports = {
        masterLogicalPortIds()[0],
        masterLogicalPortIds()[1],
    };
    auto config = utility::onePortPerVlanConfig(
        getHwSwitch(), std::move(ports), cfg::PortLoopbackMode::MAC, true);

    if constexpr (std::is_same_v<PortType, AggregatePortID>) {
      utility::addAggPort(1, {masterLogicalPortIds()[0]}, &config);
      utility::addAggPort(2, {masterLogicalPortIds()[1]}, &config);
    }
    cfg::QosMap qosMap;
    for (auto tc = 0; tc < 8; tc++) {
      // setup ingress qos map for dscp
      cfg::DscpQosMap dscpMap;
      *dscpMap.internalTrafficClass_ref() = tc;
      for (auto dscp = 0; dscp < 8; dscp++) {
        dscpMap.fromDscpToTrafficClass_ref()->push_back(8 * tc + dscp);
      }

      // setup egress qos map for tc
      cfg::ExpQosMap expMap;
      *expMap.internalTrafficClass_ref() = tc;
      expMap.fromExpToTrafficClass_ref()->push_back(tc);
      expMap.fromTrafficClassToExp_ref() = 7 - tc;

      qosMap.dscpMaps_ref()->push_back(dscpMap);
      qosMap.expMaps_ref()->push_back(expMap);
    }
    config.qosPolicies_ref()->resize(1);
    *config.qosPolicies_ref()[0].name_ref() = "qp";
    config.qosPolicies_ref()[0].qosMap_ref() = qosMap;

    cfg::TrafficPolicyConfig policy;
    policy.defaultQosPolicy_ref() = "qp";
    config.dataPlaneTrafficPolicy_ref() = policy;

    utility::setDefaultCpuTrafficPolicyConfig(config, getAsic());
    utility::addCpuQueueConfig(config, getAsic());

    return config;
  }

  HwSwitchEnsemble::Features featuresDesired() const override {
    return {HwSwitchEnsemble::LINKSCAN, HwSwitchEnsemble::PACKET_RX};
  }

  void addRoute(
      folly::IPAddressV6 prefix,
      uint8_t mask,
      PortDescriptor port,
      LabelForwardingAction::LabelStack stack = {}) {
    applyNewState(ecmpHelper_->resolveNextHops(
        getProgrammedState(),
        {
            port,
        }));

    if (stack.empty()) {
      ecmpHelper_->programRoutes(
          getRouteUpdater(), {port}, {RoutePrefixV6{prefix, mask}});
    } else {
      ecmpHelper_->programIp2MplsRoutes(
          getRouteUpdater(),
          {port},
          {{port, std::move(stack)}},
          {RoutePrefixV6{prefix, mask}});
    }
  }

  LabelForwardingEntry::Label programLabelSwap(PortDescriptor port) {
    auto state = ecmpHelper_->resolveNextHops(
        getProgrammedState(),
        {
            port,
        });
    applyNewState(ecmpSwapHelper_->setupECMPForwarding(state, {port}));
    auto swapNextHop = ecmpSwapHelper_->nhop(port);
    return swapNextHop.action.swapWith().value();
  }

  void programLabelPop(LabelForwardingEntry::Label label) {
    // program MPLS route to POP
    auto state = getProgrammedState();
    state = state->clone();
    auto* labelFib = state->getLabelForwardingInformationBase()->modify(&state);

    LabelForwardingAction popAndLookup(
        LabelForwardingAction::LabelForwardingType::POP_AND_LOOKUP);
    UnresolvedNextHop nexthop(folly::IPAddress("::1"), 1, popAndLookup);
    labelFib->programLabel(
        &state,
        label,
        ClientID::STATIC_ROUTE,
        AdminDistance::STATIC_ROUTE,
        {nexthop});
    applyNewState(state);
  }

  void programLabelPhp(PortDescriptor port) {
    utility::MplsEcmpSetupTargetedPorts<folly::IPAddressV6> phpEcmpHelper(
        getProgrammedState(),
        kTopLabel,
        LabelForwardingAction::LabelForwardingType::PHP);
    auto state = phpEcmpHelper.resolveNextHops(
        getProgrammedState(),
        {
            port,
        });
    applyNewState(phpEcmpHelper.setupECMPForwarding(state, {port}));
  }

  std::unique_ptr<utility::MplsEcmpSetupTargetedPorts<folly::IPAddressV6>>
      ecmpSwapHelper_;

  void sendL3Packet(
      folly::IPAddressV6 dst,
      PortID from,
      std::optional<DSCP> dscp = std::nullopt) {
    CHECK(ecmpHelper_);
    auto vlanId = utility::firstVlanID(initialConfig());
    // construct eth hdr
    const auto intfMac = utility::getInterfaceMac(getProgrammedState(), vlanId);

    EthHdr::VlanTags_t vlans{
        VlanTag(vlanId, static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_VLAN))};

    EthHdr eth{intfMac, intfMac, std::move(vlans), 0x86DD};
    // construct l3 hdr
    IPv6Hdr ip6{folly::IPAddressV6("1::10"), dst};
    ip6.nextHeader = 17; /* udp payload */
    if (dscp) {
      ip6.trafficClass = (dscp.value() << 2); // last two bits are ECN
    }
    UDPHeader udp(4049, 4050, 1);
    utility::UDPDatagram datagram(udp, {0xff});
    auto pkt = utility::EthFrame(
                   eth, utility::IPPacket<folly::IPAddressV6>(ip6, datagram))
                   .getTxPacket(getHwSwitch());
    XLOG(DBG2) << "sending packet: ";
    XLOG(DBG2) << PktUtil::hexDump(pkt->buf());
    // send pkt on src port, let it loop back in switch and be l3 switched
    getHwSwitchEnsemble()->ensureSendPacketOutOfPort(std::move(pkt), from);
  }

  void sendMplsPacket(
      uint32_t topLabel,
      PortID from,
      std::optional<EXP> exp = std::nullopt,
      uint8_t ttl = 128,
      folly::IPAddressV6 dstIp = folly::IPAddressV6("2001::")) {
    // construct eth hdr
    const auto srcMac = utility::kLocalCpuMac();
    const auto dstMac = utility::kLocalCpuMac(); /* for l3 switching */

    auto vlanId = utility::firstVlanID(initialConfig()) + 1;

    uint8_t tc = exp.has_value() ? static_cast<uint8_t>(exp.value()) : 0;
    MPLSHdr::Label mplsLabel{topLabel, tc, true, ttl};

    // construct l3 hdr
    auto srcIp = folly::IPAddressV6(("1001::"));

    // get tx packet
    auto frame = utility::getEthFrame(
        srcMac,
        dstMac,
        {mplsLabel},
        srcIp,
        dstIp,
        10000,
        20000,
        VlanID(vlanId));

    auto pkt = frame.getTxPacket(getHwSwitch());
    XLOG(DBG2) << "sending packet: ";
    XLOG(DBG2) << PktUtil::hexDump(pkt->buf());
    // send pkt on src port, let it loop back in switch and be l3 switched
    getHwSwitchEnsemble()->ensureSendPacketOutOfPort(std::move(pkt), from);
  }
  bool skipTest() {
    return !getPlatform()->getAsic()->isSupported(HwAsic::Feature::MPLS);
  }

  void sendMplsPktAndVerifyTrappedCpuQueue(
      int queueId,
      int label = 1101,
      const int numPktsToSend = 1,
      const int expectedPktDelta = 1,
      uint8_t ttl = 128) {
    auto beforeOutPkts = utility::getQueueOutPacketsWithRetry(
        getHwSwitch(), queueId, 0 /* retryTimes */, 0 /* expectedNumPkts */);
    for (int i = 0; i < numPktsToSend; i++) {
      sendMplsPacket(label, masterLogicalPortIds()[1], EXP(5), ttl);
    }
    auto afterOutPkts = utility::getQueueOutPacketsWithRetry(
        getHwSwitch(),
        queueId,
        kGetQueueOutPktsRetryTimes,
        beforeOutPkts + numPktsToSend);
    XLOG(DBG0) << ". Queue=" << queueId << ", before pkts:" << beforeOutPkts
               << ", after pkts:" << afterOutPkts;
    EXPECT_EQ(expectedPktDelta, afterOutPkts - beforeOutPkts);
  }

  void setup() {
    if constexpr (std::is_same_v<PortType, AggregatePortID>) {
      applyNewState(utility::enableTrunkPorts(getProgrammedState()));
    }
  }

  HwPacketVerifier getPacketVerifer(PortID port, MPLSHdr hdr) {
    return HwPacketVerifier(getHwSwitchEnsemble(), port, hdr);
  }

  std::unique_ptr<utility::EcmpSetupTargetedPorts6> ecmpHelper_;
};

TYPED_TEST_SUITE(HwMPLSTest, TestTypes);

TYPED_TEST(HwMPLSTest, Push) {
  if (this->skipTest()) {
    return;
  }
  auto setup = [=]() {
    this->setup();
    // setup ip2mpls route to 2401::201:ab00/120 through
    // port 0 w/ stack {101, 102}
    this->addRoute(
        folly::IPAddressV6("2401::201:ab00"),
        120,
        this->getPortDescriptor(0),
        {101, 102});
  };
  auto verify = [=]() {
    auto expectedMplsHdr = MPLSHdr({
        MPLSHdr::Label{102, 5, 0, 254}, // exp = 5 for tc = 2
        MPLSHdr::Label{101, 5, 1, 254}, // exp = 5 for tc = 2
    });
    [[maybe_unused]] auto verifier = this->getPacketVerifer(
        this->masterLogicalPortIds()[0], expectedMplsHdr);

    auto outPktsBefore = getPortOutPkts(
        this->getLatestPortStats(this->masterLogicalPortIds()[0]));

    // generate the packet entering  port 1
    this->sendL3Packet(
        folly::IPAddressV6("2401::201:ab01"),
        this->masterLogicalPortIds()[1],
        DSCP(16)); // tc = 2 for dscp = 16

    auto outPktsAfter = getPortOutPkts(
        this->getLatestPortStats(this->masterLogicalPortIds()[0]));
    EXPECT_EQ((outPktsAfter - outPktsBefore), 1);
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwMPLSTest, Swap) {
  if (this->skipTest()) {
    return;
  }
  auto setup = [=]() {
    this->setup();
    // setup ip2mpls route to 2401::201:ab00/120 through
    // port 0 w/ stack {101, 102}
    this->programLabelSwap(this->getPortDescriptor(0));
  };
  auto verify = [=]() {
    uint32_t expectedOutLabel =
        utility::getLabelSwappedWithForTopLabel(this->getHwSwitch(), kTopLabel);
    auto expectedMplsHdr = MPLSHdr({
        MPLSHdr::Label{expectedOutLabel, 2, true, 127}, // exp is remarked to 2
    });
    [[maybe_unused]] auto verifier = this->getPacketVerifer(
        this->masterLogicalPortIds()[0], expectedMplsHdr);

    auto outPktsBefore = getPortOutPkts(
        this->getLatestPortStats(this->masterLogicalPortIds()[0]));

    // generate the packet entering  port 1
    this->sendMplsPacket(
        1101,
        this->masterLogicalPortIds()[1],
        EXP(5)); // send packet with exp 5

    auto outPktsAfter = getPortOutPkts(
        this->getLatestPortStats(this->masterLogicalPortIds()[0]));
    EXPECT_EQ((outPktsAfter - outPktsBefore), 1);
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwMPLSTest, MplsNoMatchPktsToLowPriQ) {
  if (this->skipTest()) {
    return;
  }
  auto setup = [=]() { this->setup(); };

  auto verify = [=]() {
    auto statBefore = utility::getMplsDestNoMatchCounter(
        this->getHwSwitchEnsemble(),
        this->getProgrammedState(),
        this->masterLogicalPortIds()[1]);
    auto portStatsBefore =
        this->getLatestPortStats(this->masterLogicalPortIds()[1]);

    this->sendMplsPktAndVerifyTrappedCpuQueue(utility::kCoppLowPriQueueId);

    auto statAfter = utility::getMplsDestNoMatchCounter(
        this->getHwSwitchEnsemble(),
        this->getProgrammedState(),
        this->masterLogicalPortIds()[1]);
    auto portStatsAfter =
        this->getLatestPortStats(this->masterLogicalPortIds()[1]);

    EXPECT_EQ(statBefore + 1, statAfter);
    EXPECT_EQ(
        portStatsAfter.inDiscardsRaw__ref(),
        portStatsBefore.inDiscardsRaw__ref());
    EXPECT_EQ(
        portStatsAfter.inDstNullDiscards__ref(),
        portStatsBefore.inDstNullDiscards__ref());
    EXPECT_EQ(
        portStatsAfter.inDiscards__ref(), portStatsBefore.inDiscards__ref());
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwMPLSTest, MplsMatchPktsNottrapped) {
  if (this->skipTest()) {
    return;
  }
  auto setup = [=]() {
    this->setup();
    this->programLabelSwap(this->getPortDescriptor(0));
  };

  auto verify = [=]() {
    const auto& mplsNoMatchCounter = utility::getMplsDestNoMatchCounterName();
    auto statBefore = utility::getAclInOutPackets(
        this->getHwSwitch(),
        this->getProgrammedState(),
        "",
        mplsNoMatchCounter);

    this->sendMplsPktAndVerifyTrappedCpuQueue(
        utility::kCoppLowPriQueueId, 1101, 1 /* To send*/, 0 /* expected*/);

    auto statAfter = utility::getAclInOutPackets(
        this->getHwSwitch(),
        this->getProgrammedState(),
        "",
        mplsNoMatchCounter);
    EXPECT_EQ(statBefore, statAfter);
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwMPLSTest, Pop) {
  if (this->skipTest()) {
    return;
  }
  auto setup = [=]() {
    this->setup();
    // pop and lookup 1101
    this->programLabelPop(1101);
    // setup route for 2001::, dest ip under label 1101
    this->addRoute(
        folly::IPAddressV6("2001::"), 128, this->getPortDescriptor(0));
  };
  auto verify = [=]() {
    auto outPktsBefore = getPortOutPkts(
        this->getLatestPortStats(this->masterLogicalPortIds()[0]));
    // send mpls packet with label and let it pop
    this->sendMplsPacket(1101, this->masterLogicalPortIds()[1]);
    // ip packet should be forwarded as per route for 2001::/128
    auto outPktsAfter = getPortOutPkts(
        this->getLatestPortStats(this->masterLogicalPortIds()[0]));
    EXPECT_EQ((outPktsAfter - outPktsBefore), 1);
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwMPLSTest, Php) {
  if (this->skipTest()) {
    return;
  }
  auto setup = [=]() {
    this->setup();
    // php to exit out of port 0
    this->programLabelPhp(this->getPortDescriptor(0));
  };
  auto verify = [=]() {
    auto outPktsBefore = getPortOutPkts(
        this->getLatestPortStats(this->masterLogicalPortIds()[0]));
    // send mpls packet with label and let it forward with php
    this->sendMplsPacket(1101, this->masterLogicalPortIds()[1]);
    // ip packet should be forwarded through port 0
    auto outPktsAfter = getPortOutPkts(
        this->getLatestPortStats(this->masterLogicalPortIds()[0]));
    EXPECT_EQ((outPktsAfter - outPktsBefore), 1);
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwMPLSTest, Pop2Cpu) {
  if (this->skipTest()) {
    return;
  }
  auto setup = [=]() {
    this->setup();
    // pop and lookup 1101
    this->programLabelPop(1101);
  };
  auto verify = [=]() {
    HwTestPacketSnooper snooper(
        this->getHwSwitchEnsemble(), this->masterLogicalPortIds()[1]);

    // send mpls packet with label and let it pop
    this->sendMplsPacket(
        1101,
        this->masterLogicalPortIds()[1],
        EXP(0),
        128,
        folly::IPAddressV6("1::0"));
    // ip packet should be punted to cpu after Pop
    auto frame = snooper.waitForPacket(1);
    ASSERT_TRUE(frame.has_value());

    auto mplsPayLoad = frame->mplsPayLoad();
    ASSERT_FALSE(mplsPayLoad.has_value());

    auto v6PayLoad = frame->v6PayLoad();
    ASSERT_TRUE(v6PayLoad.has_value());

    auto udpPayload = v6PayLoad->payload();
    ASSERT_TRUE(udpPayload.has_value());

    auto hdr = v6PayLoad->header();
    EXPECT_EQ(hdr.srcAddr, folly::IPAddress("1001::"));
    EXPECT_EQ(hdr.dstAddr, folly::IPAddress("1::0"));
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwMPLSTest, ExpiringTTL) {
  auto setup = [=]() {
    this->setup();
    this->programLabelSwap(this->getPortDescriptor(0));
  };
  auto verify = [=]() {
    this->sendMplsPktAndVerifyTrappedCpuQueue(
        utility::kCoppLowPriQueueId, 1101, 1, 1, 1);
  };
  this->verifyAcrossWarmBoots(setup, verify);
}
} // namespace facebook::fboss
