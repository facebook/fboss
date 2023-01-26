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
const facebook::fboss::Label kTopLabel{1101};
constexpr auto kGetQueueOutPktsRetryTimes = 5;
const std::string kAclName = "acl0";
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
        masterLogicalPortIds()[2],
    };
    auto config = utility::onePortPerInterfaceConfig(
        getHwSwitch(),
        std::move(ports),
        getAsic()->desiredLoopbackMode(),
        true);

    if constexpr (std::is_same_v<PortType, AggregatePortID>) {
      utility::addAggPort(1, {masterLogicalPortIds()[0]}, &config);
      utility::addAggPort(2, {masterLogicalPortIds()[1]}, &config);
      utility::addAggPort(3, {masterLogicalPortIds()[2]}, &config);
    }
    cfg::QosMap qosMap;
    for (auto tc = 0; tc < 8; tc++) {
      // setup ingress qos map for dscp
      cfg::DscpQosMap dscpMap;
      *dscpMap.internalTrafficClass() = tc;
      for (auto dscp = 0; dscp < 8; dscp++) {
        dscpMap.fromDscpToTrafficClass()->push_back(8 * tc + dscp);
      }

      // setup egress qos map for tc
      cfg::ExpQosMap expMap;
      *expMap.internalTrafficClass() = tc;
      expMap.fromExpToTrafficClass()->push_back(tc);
      expMap.fromTrafficClassToExp() = 7 - tc;

      qosMap.dscpMaps()->push_back(dscpMap);
      qosMap.expMaps()->push_back(expMap);
    }
    config.qosPolicies()->resize(1);
    *config.qosPolicies()[0].name() = "qp";
    config.qosPolicies()[0].qosMap() = qosMap;

    cfg::TrafficPolicyConfig policy;
    policy.defaultQosPolicy() = "qp";
    config.dataPlaneTrafficPolicy() = policy;

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

  void addRoute(
      LabelID label,
      PortDescriptor port,
      LabelForwardingAction::LabelStack stack = {},
      LabelForwardingAction::LabelForwardingType type =
          LabelForwardingAction::LabelForwardingType::SWAP) {
    applyNewState(ecmpHelper_->resolveNextHops(
        getProgrammedState(),
        {
            port,
        }));

    ecmpHelper_->programMplsRoutes(
        getRouteUpdater(), {port}, {{port, std::move(stack)}}, {label}, type);
  }

  void addRoute(LabelID label, LabelNextHopEntry& nexthop) {
    auto updater = getRouteUpdater();
    MplsRoute route;
    route.topLabel() = label;
    route.nextHops() = util::fromRouteNextHopSet(nexthop.getNextHopSet());
    updater->addRoute(ClientID::BGPD, route);
    updater->program();
  }

  std::unique_ptr<utility::MplsEcmpSetupTargetedPorts<folly::IPAddressV6>>
      ecmpSwapHelper_;

  void sendL3Packet(
      folly::IPAddressV6 dst,
      PortID from,
      std::optional<DSCP> dscp = std::nullopt) {
    CHECK(ecmpHelper_);
    // TODO: Remove the dependency on VLAN below
    auto vlan = utility::firstVlanID(initialConfig());
    if (!vlan) {
      throw FbossError("VLAN id unavailable for test");
    }
    auto vlanId = *vlan;

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

    // TODO: Remove the dependency on VLAN below
    auto vlan = utility::firstVlanID(initialConfig());
    if (!vlan) {
      throw FbossError("VLAN id unavailable for test");
    }
    auto vlanId = *vlan;

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

  void getNexthops(
      const std::vector<std::pair<PortDescriptor, InterfaceID>>& portIntfs,
      const std::vector<LabelForwardingAction::LabelStack>& stacks,
      RouteNextHopSet& nhops) {
    int idx = 0;
    for (auto portIntf : portIntfs) {
      nhops.emplace(ResolvedNextHop(
          ecmpHelper_->ip(portIntf.first),
          portIntf.second,
          UCMP_DEFAULT_WEIGHT,
          LabelForwardingAction(
              LabelForwardingAction::LabelForwardingType::PUSH,
              stacks[idx++])));
    }
  }

  void addRedirectToNexthopAcl(
      const std::string& aclName,
      uint32_t ingressVlanId,
      const std::string& dstPrefix,
      const std::vector<std::string>& redirectNexthopIps,
      const std::vector<std::pair<PortDescriptor, InterfaceID>>& portIntfs,
      const std::vector<LabelForwardingAction::LabelStack>& stacks) {
    RouteNextHopSet recursiveNexthops;
    getNexthops(portIntfs, stacks, recursiveNexthops);
    boost::container::flat_set<PortDescriptor> ports;
    for (auto portIntf : portIntfs) {
      ports.emplace(portIntf.first);
    }
    std::shared_ptr<SwitchState> newState;
    if (ports.size() > 0) {
      newState = ecmpHelper_->resolveNextHops(getProgrammedState(), ports);
    } else {
      newState = getProgrammedState()->clone();
    }
    auto newAcl =
        std::make_shared<AclEntry>(AclTable::kDataplaneAclMaxPriority, aclName);
    newAcl->setDstIp(folly::IPAddress::tryCreateNetwork(dstPrefix).value());
    newAcl->setVlanID(ingressVlanId);
    auto cfgRedirectToNextHop = cfg::RedirectToNextHopAction();
    for (auto nhIp : redirectNexthopIps) {
      cfg::RedirectNextHop nhop;
      nhop.ip() = nhIp;
      cfgRedirectToNextHop.redirectNextHops()->push_back(nhop);
    }
    auto redirectToNextHop = MatchAction::RedirectToNextHopAction();
    redirectToNextHop.first = cfgRedirectToNextHop;
    redirectToNextHop.second = recursiveNexthops;
    MatchAction action = MatchAction();
    action.setRedirectToNextHop(redirectToNextHop);
    newAcl->setAclAction(action);
    newState->addAcl(newAcl);
    applyNewState(newState);
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
    this->addRoute(LabelID(1101), this->getPortDescriptor(0), {11});
  };
  auto verify = [=]() {
    uint32_t expectedOutLabel = utility::getLabelSwappedWithForTopLabel(
        this->getHwSwitch(), kTopLabel.value());
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
        portStatsAfter.inDiscardsRaw_(), portStatsBefore.inDiscardsRaw_());
    EXPECT_EQ(
        portStatsAfter.inDstNullDiscards_(),
        portStatsBefore.inDstNullDiscards_());
    EXPECT_EQ(portStatsAfter.inDiscards_(), portStatsBefore.inDiscards_());
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwMPLSTest, MplsMatchPktsNottrapped) {
  if (this->skipTest()) {
    return;
  }
  auto setup = [=]() {
    this->setup();
    this->addRoute(LabelID(1101), this->getPortDescriptor(0), {11});
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
    this->addRoute(
        LabelID(1101),
        this->getPortDescriptor(0),
        {},
        LabelForwardingAction::LabelForwardingType::POP_AND_LOOKUP);
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
    this->addRoute(
        LabelID(1101),
        this->getPortDescriptor(0),
        {},
        LabelForwardingAction::LabelForwardingType::PHP);
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
    this->addRoute(
        LabelID(1101),
        this->getPortDescriptor(0),
        {},
        LabelForwardingAction::LabelForwardingType::POP_AND_LOOKUP);
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

TYPED_TEST(HwMPLSTest, punt2Cpu) {
  if (this->skipTest()) {
    return;
  }
  auto setup = [=]() {
    this->setup();
    LabelNextHopEntry nexthop{
        LabelNextHopEntry::Action::TO_CPU, AdminDistance::MAX_ADMIN_DISTANCE};
    this->addRoute(LabelID(1101), nexthop);
  };
  auto verify = [=]() {
    HwTestPacketSnooper snooper(
        this->getHwSwitchEnsemble(), this->masterLogicalPortIds()[1]);

    // send mpls packet with label
    this->sendMplsPacket(
        1101,
        this->masterLogicalPortIds()[1],
        EXP(0),
        128,
        folly::IPAddressV6("1::0"));
    // mpls packet should be punted to cpu
    auto frame = snooper.waitForPacket(1);
    ASSERT_TRUE(frame.has_value());

    auto mplsPayLoad = frame->mplsPayLoad();
    ASSERT_TRUE(mplsPayLoad.has_value());
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwMPLSTest, ExpiringTTL) {
  auto setup = [=]() {
    this->setup();
    this->addRoute(LabelID(1101), this->getPortDescriptor(0), {11});
  };
  auto verify = [=]() {
    this->sendMplsPktAndVerifyTrappedCpuQueue(
        utility::kCoppLowPriQueueId, 1101, 1, 1, 1);
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwMPLSTest, AclRedirectToNexthop) {
  if (this->skipTest()) {
    return;
  }
  auto setup = [=]() {
    this->setup();
    std::string dstIp{"2401::201:ab00"};
    uint8_t mask = 120;
    this->addRoute(
        folly::IPAddressV6(dstIp),
        mask,
        this->getPortDescriptor(0),
        {101, 102});
    std::string dstPrefix{fmt::format("{}/{}", dstIp, mask)};
    // We are using port 1 as ingress port. Which has ingress vlan_id
    // kBaseVlanId + 1
    uint32_t ingressVlan = utility::kBaseVlanId + 1;
    auto config = this->initialConfig();
    std::vector<std::pair<PortDescriptor, InterfaceID>> portIntfs{
        std::make_pair(
            this->getPortDescriptor(0),
            InterfaceID(*config.interfaces()[0].intfID()))};
    this->addRedirectToNexthopAcl(
        kAclName, ingressVlan, dstPrefix, {"1000::1"}, portIntfs, {{201, 202}});
  };
  auto verify = [=]() {
    // Use different labels from that of the rib route to verify that the
    // redirect ACL is in effect
    auto expectedMplsHdr = MPLSHdr({
        MPLSHdr::Label{202, 5, 0, 254},
        MPLSHdr::Label{201, 5, 1, 254},
    });
    [[maybe_unused]] auto verifier = this->getPacketVerifer(
        this->masterLogicalPortIds()[0], expectedMplsHdr);
    auto outPktsBefore = getPortOutPkts(
        this->getLatestPortStats(this->masterLogicalPortIds()[0]));
    this->sendL3Packet(
        folly::IPAddressV6("2401::201:ab01"),
        this->masterLogicalPortIds()[1],
        DSCP(16));
    auto outPktsAfter = getPortOutPkts(
        this->getLatestPortStats(this->masterLogicalPortIds()[0]));
    EXPECT_EQ((outPktsAfter - outPktsBefore), 1);
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwMPLSTest, AclRedirectToNexthopDrop) {
  // Packets must be dropped if  there are no resolved nexthops
  // in the redirect ACL entry
  if (this->skipTest()) {
    return;
  }
  auto setup = [=]() {
    this->setup();
    std::string dstIp{"2401::201:ab00"};
    uint8_t mask = 120;
    this->addRoute(
        folly::IPAddressV6(dstIp),
        mask,
        this->getPortDescriptor(0),
        {101, 102});
    std::string dstPrefix{fmt::format("{}/{}", dstIp, mask)};
    // We are using port 1 as ingress port. Which has ingress vlan_id
    // kBaseVlanId + 1
    uint32_t ingressVlan = utility::kBaseVlanId + 1;
    std::vector<std::pair<PortDescriptor, InterfaceID>> portIntfs;
    this->addRedirectToNexthopAcl(
        kAclName, ingressVlan, dstPrefix, {"1000::1"}, portIntfs, {});
  };
  auto verify = [=]() {
    auto outPktsBefore = getPortOutPkts(
        this->getLatestPortStats(this->masterLogicalPortIds()[0]));
    this->sendL3Packet(
        folly::IPAddressV6("2401::201:ab01"),
        this->masterLogicalPortIds()[1],
        DSCP(16));
    auto outPktsAfter = getPortOutPkts(
        this->getLatestPortStats(this->masterLogicalPortIds()[0]));
    // Packet drop expected
    EXPECT_EQ(outPktsAfter, outPktsBefore);
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

// Verfiy that redirect does not happen when ACL qualifiers do not match
TYPED_TEST(HwMPLSTest, AclRedirectToNexthopMismatch) {
  if (this->skipTest()) {
    return;
  }
  auto setup = [=]() {
    this->setup();
    std::string dstIp{"2401::201:ab00"};
    uint8_t mask = 120;
    this->addRoute(
        folly::IPAddressV6(dstIp),
        mask,
        this->getPortDescriptor(0),
        {101, 102});
    std::string dstPrefix{fmt::format("{}/{}", dstIp, mask)};
    auto config = this->initialConfig();
    std::vector<std::pair<PortDescriptor, InterfaceID>> portIntfs{
        std::make_pair(
            this->getPortDescriptor(0),
            InterfaceID(*config.interfaces()[0].intfID()))};
    // Use VLAN ID qualifier value that does not match the actual
    // ingress vlan.
    uint32_t ingressVlan = utility::kBaseVlanId + 100;
    this->addRedirectToNexthopAcl(
        kAclName, ingressVlan, dstPrefix, {"1000::1"}, portIntfs, {{201, 202}});
  };
  auto verify = [=]() {
    auto expectedMplsHdr = MPLSHdr({
        MPLSHdr::Label{102, 5, 0, 254},
        MPLSHdr::Label{101, 5, 1, 254},
    });
    // Since ACL qualifiers do not match packet fields, packet must
    // exit via RIB route nexthops
    [[maybe_unused]] auto verifier = this->getPacketVerifer(
        this->masterLogicalPortIds()[0], expectedMplsHdr);
    auto outPktsBefore = getPortOutPkts(
        this->getLatestPortStats(this->masterLogicalPortIds()[0]));
    this->sendL3Packet(
        folly::IPAddressV6("2401::201:ab01"),
        this->masterLogicalPortIds()[1],
        DSCP(16));
    auto outPktsAfter = getPortOutPkts(
        this->getLatestPortStats(this->masterLogicalPortIds()[0]));
    EXPECT_EQ((outPktsAfter - outPktsBefore), 1);
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

// Test ACL redirect to an ECMP nexthop
TYPED_TEST(HwMPLSTest, AclRedirectToNexthopMultipleNexthops) {
  if (this->skipTest()) {
    return;
  }
  auto setup = [=]() {
    this->setup();
    std::string dstIp{"2401::201:ab00"};
    uint8_t mask = 120;
    std::string dstPrefix{fmt::format("{}/{}", dstIp, mask)};
    // We are using port 2 as ingress port. Which has ingress vlan_id
    // kBaseVlanId + 2
    uint32_t ingressVlan = utility::kBaseVlanId + 2;
    auto config = this->initialConfig();
    std::vector<std::pair<PortDescriptor, InterfaceID>> portIntfs{
        std::make_pair(
            this->getPortDescriptor(0),
            InterfaceID(*config.interfaces()[0].intfID())),
        std::make_pair(
            this->getPortDescriptor(1),
            InterfaceID(*config.interfaces()[1].intfID())),
    };
    this->addRedirectToNexthopAcl(
        kAclName,
        ingressVlan,
        dstPrefix,
        {"1000::1"},
        portIntfs,
        {{201, 202}, {301, 302}});
  };
  auto verify = [=]() {
    std::vector<PortID> ports{
        this->masterLogicalPortIds()[0], this->masterLogicalPortIds()[1]};
    auto outPktsBefore = getPortOutPkts(this->getLatestPortStats(ports));
    this->sendL3Packet(
        folly::IPAddressV6("2401::201:ab01"),
        this->masterLogicalPortIds()[2],
        DSCP(16));
    auto outPktsAfter = getPortOutPkts(this->getLatestPortStats(ports));
    EXPECT_EQ((outPktsAfter - outPktsBefore), 1);
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
