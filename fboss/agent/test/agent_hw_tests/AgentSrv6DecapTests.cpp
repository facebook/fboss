// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/AsicUtils.h"
#include "fboss/agent/SwSwitchRouteUpdateWrapper.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/packet/Ethertype.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/state/AggregatePort.h"
#include "fboss/agent/state/RouteNextHop.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/agent/test/TrunkUtils.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/CoppTestUtils.h"
#include "fboss/agent/test/utils/LoadBalancerTestUtils.h"
#include "fboss/agent/test/utils/OlympicTestUtils.h"
#include "fboss/agent/test/utils/PacketSnooper.h"
#include "fboss/agent/test/utils/PortTestUtils.h"
#include "fboss/agent/test/utils/QosTestUtils.h"
#include "fboss/agent/test/utils/Srv6TestUtils.h"
#include "fboss/agent/test/utils/TrapPacketUtils.h"
#include "fboss/lib/CommonUtils.h"

namespace facebook::fboss {

struct PhysicalPortSrv6Decap {
  static constexpr bool isTrunk = false;
};
struct AggregatePortSrv6Decap {
  static constexpr bool isTrunk = true;
};
using Srv6DecapPortTypes =
    ::testing::Types<PhysicalPortSrv6Decap, AggregatePortSrv6Decap>;

template <typename PortType>
class AgentSrv6DecapTest : public AgentHwTest {
 protected:
  static constexpr bool kIsTrunk = PortType::isTrunk;

  // Route prefixes used for forwarding after decap
  const folly::IPAddressV6 kV6RoutePrefix{"2800:2::"};
  static constexpr uint8_t kV6RoutePrefixLen{64};
  const folly::IPAddressV6 kV6RouteDstIp{"2800:2::1"};
  const folly::IPAddressV4 kV4RoutePrefix{"100.0.0.0"};
  const folly::IPAddressV4 kV4RouteDstIp{"100.0.0.1"};
  const folly::IPAddressV6 kMySidAddr{"3001:db8:7fff::"};
  static constexpr uint8_t kMySidPrefixLen{48};
  static constexpr uint8_t kECT1{1};

  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    if constexpr (kIsTrunk) {
      return {
          ProductionFeature::SRV6_DECAP,
          ProductionFeature::L3_QOS,
          ProductionFeature::ECN,
          ProductionFeature::LAG};
    }
    return {
        ProductionFeature::SRV6_DECAP,
        ProductionFeature::L3_QOS,
        ProductionFeature::ECN};
  }

  void setCmdLineFlagOverrides() const override {
    AgentHwTest::setCmdLineFlagOverrides();
    FLAGS_enable_nexthop_id_manager = true;
    FLAGS_resolve_nexthops_from_id = true;
  }

  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    constexpr auto kNumNextHops = 4;
    auto cfg = utility::onePortPerInterfaceConfig(
        ensemble.getSw(),
        ensemble.masterLogicalPortIds(),
        true /*interfaceHasSubnet*/);
    if constexpr (kIsTrunk) {
      for (int i = 0; i < kNumNextHops; ++i) {
        utility::addAggPort(
            i + 1,
            {static_cast<int32_t>(ensemble.masterLogicalPortIds()[i])},
            &cfg);
      }
    }
    cfg.loadBalancers() =
        utility::getEcmpFullWithFlowLabelTrunkFullWithFlowLabelHashConfig(
            ensemble.getL3Asics());
    // Add trap ACLs for inner packet destinations so snooper can capture
    // the decapped and forwarded packets
    auto asic = checkSameAndGetAsicForTesting(ensemble.getL3Asics());
    utility::addTrapPacketAcl(
        asic,
        &cfg,
        std::set<folly::CIDRNetwork>{
            {kV6RouteDstIp, 128}, {kV4RouteDstIp, 32}});
    utility::addOlympicQueueConfig(
        &cfg,
        ensemble.getL3Asics(),
        /*addWredConfig=*/false,
        /*addEcnConfig=*/true);
    utility::addOlympicQosMaps(cfg, ensemble.getL3Asics());
    return cfg;
  }

  void applyConfigAndEnableTrunks(const cfg::SwitchConfig& config) {
    this->applyNewConfig(config);
    this->applyNewState(
        [](const std::shared_ptr<SwitchState> state) {
          return utility::enableTrunkPorts(state);
        },
        "enable trunk ports");
  }

  template <typename IPAddrT = folly::IPAddressV6>
  utility::EcmpSetupAnyNPorts<IPAddrT> makeEcmpHelper() {
    return utility::EcmpSetupAnyNPorts<IPAddrT>(
        this->getProgrammedState(), this->getSw()->needL2EntryForNeighbor());
  }

  void resolveNextHops(int numNextHops) {
    auto ecmpHelper6 = makeEcmpHelper<folly::IPAddressV6>();
    this->resolveNeighbors(ecmpHelper6, numNextHops, true /* useLinkLocal */);
  }

  void setupHelper(bool resolveNeighbors = true, bool addMySid = true) {
    if constexpr (kIsTrunk) {
      applyConfigAndEnableTrunks(
          this->initialConfig(*this->getAgentEnsemble()));
    }
    if (resolveNeighbors) {
      resolveNextHops(2);
    }
    // IPv6 route with regular next hops (no SID lists)
    addRoute<folly::CIDRNetworkV6>(
        {kV6RoutePrefix, kV6RoutePrefixLen}, 1 /*numNextHops*/);
    // IPv4 route with regular next hops (no SID lists)
    addRoute<folly::CIDRNetworkV4>(
        {folly::IPAddressV4("100.0.0.0"), 24}, 1 /*numNextHops*/);
    if (addMySid) {
      utility::addDecapMySidEntry(this->getSw(), kMySidAddr, kMySidPrefixLen);
    }
  }

  template <typename CIDRNetworkT>
  void addRoute(const CIDRNetworkT& prefix, int numNextHops) {
    RouteNextHopSet nhops;
    auto ecmpHelper = makeEcmpHelper<folly::IPAddressV6>();
    for (auto i = 0; i < numNextHops; ++i) {
      auto nhop = ecmpHelper.nhop(i);
      CHECK(nhop.linkLocalNhopIp.has_value());
      nhops.insert(ResolvedNextHop(
          folly::IPAddress(*nhop.linkLocalNhopIp), nhop.intf, ECMP_WEIGHT));
    }
    auto routeUpdater = this->getSw()->getRouteUpdater();
    routeUpdater.addRoute(
        RouterID(0),
        prefix.first,
        prefix.second,
        ClientID::BGPD,
        RouteNextHopEntry(nhops, AdminDistance::EBGP));
    routeUpdater.program();
  }

  PortID getEgressPort(const PortDescriptor& portDesc) const {
    if (portDesc.isPhysicalPort()) {
      return portDesc.phyPortID();
    }
    auto aggPort = this->getProgrammedState()->getAggregatePorts()->getNodeIf(
        portDesc.aggPortID());
    return aggPort->sortedSubports().front().portID;
  }

  void verifyDecapPacket(
      PortID egressPort,
      bool ecnMarked,
      bool isV4,
      std::optional<PortID> injectPort = std::nullopt) {
    auto portStatsBefore = this->getLatestPortStats(egressPort);
    auto bytesBefore = *portStatsBefore.outBytes_();

    auto intfMac =
        getMacForFirstInterfaceWithPortsForTesting(this->getProgrammedState());
    constexpr uint16_t kSrcPort{8000};
    constexpr uint16_t kDstPort{8001};
    constexpr uint8_t kInnerHopLimit{64};
    constexpr uint8_t kOuterHopLimit{24};
    constexpr uint8_t kInnerPktTc{42};
    constexpr uint8_t kOuterPktTc{24};
    auto outerTcField = ecnMarked
        ? static_cast<uint8_t>((kOuterPktTc << 2) | 0x3)
        : static_cast<uint8_t>(kOuterPktTc << 2);

    // Outer IPv6: dst = mySid address (triggers decap)
    // Inner IP: dst matches route prefix (forwarded after decap)
    auto outerSrcIp = folly::IPAddressV6("1::1");
    std::unique_ptr<facebook::fboss::TxPacket> txPacket;
    if (isV4) {
      txPacket = utility::makeIpInIpTxPacket(
          this->getSw(),
          this->getVlanIDForTx().value(),
          intfMac,
          intfMac,
          outerSrcIp,
          kMySidAddr,
          folly::IPAddressV4("10.0.0.1"),
          kV4RouteDstIp,
          kSrcPort,
          kDstPort,
          outerTcField,
          kInnerPktTc /* innerDscp */,
          kOuterHopLimit,
          kInnerHopLimit);
    } else {
      txPacket = utility::makeIpInIpTxPacket(
          this->getSw(),
          this->getVlanIDForTx().value(),
          intfMac,
          intfMac,
          outerSrcIp,
          kMySidAddr,
          folly::IPAddressV6("1::10"),
          kV6RouteDstIp,
          kSrcPort,
          kDstPort,
          outerTcField,
          kInnerPktTc << 2 /* innerTrafficClass */,
          kOuterHopLimit,
          kInnerHopLimit);
    }

    // Extract expected inner UDP payload before sending (txPacket gets moved)
    auto origFrame = utility::makeEthFrame(*txPacket);
    auto origOuterV6 = origFrame.v6PayLoad();
    ASSERT_TRUE(origOuterV6.has_value());
    std::optional<utility::UDPDatagram> expectedUdp;
    if (isV4) {
      auto innerV4 = origOuterV6->v4PayLoad();
      ASSERT_NE(innerV4, nullptr);
      expectedUdp = innerV4->udpPayload();
    } else {
      auto innerV6 = origOuterV6->v6PayLoad();
      ASSERT_NE(innerV6, nullptr);
      expectedUdp = innerV6->udpPayload();
    }
    ASSERT_TRUE(expectedUdp.has_value());

    utility::SwSwitchPacketSnooper snooper(this->getSw(), "srv6DecapSnooper");

    if (injectPort.has_value()) {
      this->getSw()->sendPacketOutOfPortAsync(
          std::move(txPacket), injectPort.value());
    } else {
      this->sendPacketSwitchedAsync(std::move(txPacket));
    }

    auto frameRx = snooper.waitForPacket(1);
    WITH_RETRIES({
      auto portStatsAfter = this->getLatestPortStats(egressPort);
      auto bytesAfter = *portStatsAfter.outBytes_();
      EXPECT_EVENTUALLY_GT(bytesAfter, bytesBefore);
      if (!frameRx.has_value()) {
        frameRx = snooper.waitForPacket(1);
      }
      EXPECT_EVENTUALLY_TRUE(frameRx.has_value());
    });
    ASSERT_TRUE(frameRx.has_value());
    folly::io::Cursor cursor((*frameRx).get());
    utility::EthFrame frame(cursor);

    // After decap, the forwarded packet should be the inner IP packet
    std::optional<utility::UDPDatagram> rxUdp;
    if (isV4) {
      EXPECT_EQ(
          frame.header().etherType,
          static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV4));
      auto rxV4 = frame.v4PayLoad();
      ASSERT_TRUE(rxV4.has_value());
      auto v4Hdr = rxV4->header();
      EXPECT_EQ(v4Hdr.srcAddr, folly::IPAddressV4("10.0.0.1"));
      EXPECT_EQ(v4Hdr.dstAddr, kV4RouteDstIp);
      // Outer header hop limit should get decremented and
      // copied
      EXPECT_EQ(v4Hdr.ttl, kOuterHopLimit - 1);
      // DSCP is preserved
      EXPECT_EQ(v4Hdr.dscp, kOuterPktTc);
      EXPECT_EQ(v4Hdr.ecn, ecnMarked ? 0x3 : 0);
      rxUdp = rxV4->udpPayload();
    } else {
      EXPECT_EQ(
          frame.header().etherType,
          static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV6));
      auto rxV6 = frame.v6PayLoad();
      ASSERT_TRUE(rxV6.has_value());
      auto v6Hdr = rxV6->header();
      EXPECT_EQ(v6Hdr.srcAddr, folly::IPAddressV6("1::10"));
      EXPECT_EQ(v6Hdr.dstAddr, kV6RouteDstIp);
      // Outer header hop limit should get decremented and
      // copied
      EXPECT_EQ(v6Hdr.hopLimit, kOuterHopLimit - 1);
      // DSCP is preserved
      EXPECT_EQ(v6Hdr.trafficClass >> 2, kOuterPktTc);
      EXPECT_EQ(v6Hdr.trafficClass & 0x3, ecnMarked ? 0x3 : 0);
      rxUdp = rxV6->udpPayload();
    }
    ASSERT_TRUE(rxUdp.has_value());
    EXPECT_EQ(*rxUdp, *expectedUdp);
  }

  void verifyDecapCpuAndFrontPanel(PortID egressPort) {
    auto injectPort = findInjectPort(egressPort);
    for (bool isV4 : {false, true}) {
      // ECN not marked
      verifyDecapPacket(egressPort, false, isV4);
      verifyDecapPacket(egressPort, false, isV4, injectPort);
      // ECN marked
      verifyDecapPacket(egressPort, true, isV4);
      verifyDecapPacket(egressPort, true, isV4, injectPort);
    }
  }

  PortID findInjectPort(PortID egressPort) {
    for (const auto& portMap :
         std::as_const(*this->getProgrammedState()->getPorts())) {
      for (const auto& [_, port] : std::as_const(*portMap.second)) {
        if (port->getID() != egressPort && port->isPortUp()) {
          return port->getID();
        }
      }
    }
    throw FbossError("No UP port found besides egress port");
  }
};

TYPED_TEST_SUITE(AgentSrv6DecapTest, Srv6DecapPortTypes);

TYPED_TEST(AgentSrv6DecapTest, sendPacketForDecap) {
  auto setup = [this]() { this->setupHelper(); };

  auto verify = [this]() {
    auto ecmpHelper = this->makeEcmpHelper();
    auto egressPort = this->getEgressPort(ecmpHelper.nhop(0).portDesc);
    this->verifyDecapCpuAndFrontPanel(egressPort);
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(AgentSrv6DecapTest, verifySrv6DecapEcnMarking) {
  auto setup = [this]() { this->setupHelper(); };

  auto verify = [this]() {
    auto ecmpHelper = this->makeEcmpHelper();
    auto egressPort = this->getEgressPort(ecmpHelper.nhop(0).portDesc);
    auto intfMac =
        getMacForFirstInterfaceWithPortsForTesting(this->getProgrammedState());

    // Send 512 IPv6-in-IPv6 packets (outer DSCP 5 + ECN ECT1, inner TC = 0).
    // UNIFORM decap copies outer DSCP+ECN to inner.
    auto sendFloodPackets = [this, &intfMac]() {
      auto outerTcField = static_cast<uint8_t>((5 << 2) | this->kECT1);
      for (int i = 0; i < 512; ++i) {
        auto txPacket = utility::makeIpInIpTxPacket(
            this->getSw(),
            this->getVlanIDForTx().value(),
            intfMac,
            intfMac,
            folly::IPAddressV6("1::1"),
            this->kMySidAddr,
            folly::IPAddressV6("1::10"),
            this->kV6RouteDstIp,
            8000,
            8001,
            outerTcField,
            0 /* innerTrafficClass */,
            64,
            std::optional<uint8_t>(64),
            std::vector<uint8_t>(7000, 0xff));
        this->getSw()->sendPacketSwitchedAsync(std::move(txPacket));
      }
    };

    // After decap, inner (now outer) dst = route dst, ECN CE = 0x3
    auto isEcnMarked = [this](
                           const folly::IPAddressV6& dstAddr, uint8_t ecnBits) {
      return dstAddr == this->kV6RouteDstIp && ecnBits == 0x3;
    };

    utility::verifySrv6EcnMarking(
        this->getAgentEnsemble(), egressPort, sendFloodPackets, isEcnMarked);
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

// Verify that SRv6-decapsulated packets are placed in the correct egress
// queue based on the outer packet's DSCP. The outer DSCP determines queue
// classification at ingress. In UNIFORM mode, outer DSCP is also copied
// to the inner (forwarded) packet.
TYPED_TEST(AgentSrv6DecapTest, VerifyDscpQueueMapping) {
  auto setup = [this]() { this->setupHelper(); };

  auto verify = [this]() {
    auto ecmpHelper = this->makeEcmpHelper();
    auto egressPort = this->getEgressPort(ecmpHelper.nhop(0).portDesc);
    auto injectPort = this->findInjectPort(egressPort);
    auto intfMac =
        getMacForFirstInterfaceWithPortsForTesting(this->getProgrammedState());

    auto sendPacket = [this, &intfMac](int dscp, bool frontPanel, PortID port) {
      // Outer DSCP determines queue classification.
      // Inner TC is 0 — overwritten by UNIFORM decap.
      auto txPacket = utility::makeIpInIpTxPacket(
          this->getSw(),
          this->getVlanIDForTx().value(),
          intfMac,
          intfMac,
          folly::IPAddressV6("1::1"),
          this->kMySidAddr,
          folly::IPAddressV6("1::10"),
          this->kV6RouteDstIp,
          8000,
          8001,
          dscp << 2,
          0 /* innerTrafficClass */,
          64,
          64);
      if (frontPanel) {
        this->getSw()->sendPacketOutOfPortAsync(std::move(txPacket), port);
      } else {
        this->getSw()->sendPacketSwitchedAsync(std::move(txPacket));
      }
    };

    // Send all packets first, then verify queue counters
    for (bool frontPanel : {false, true}) {
      auto portStatsBefore = this->getLatestPortStats(egressPort);
      for (const auto& [queue, dscps] : utility::kOlympicQueueToDscp()) {
        for (auto dscp : dscps) {
          sendPacket(dscp, frontPanel, injectPort);
        }
      }
      for (const auto& [queue, dscps] : utility::kOlympicQueueToDscp()) {
        utility::verifyQueueHit(
            portStatsBefore, queue, this->getSw(), egressPort, dscps.size());
      }
    }
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(AgentSrv6DecapTest, sendDecapPacketNonLastSegmentDropped) {
  auto setup = [this]() { this->setupHelper(); };

  auto verify = [this]() {
    auto ecmpHelper = this->makeEcmpHelper();
    auto egressPort = this->getEgressPort(ecmpHelper.nhop(0).portDesc);
    auto injectPort = this->findInjectPort(egressPort);

    auto portStatsBefore = this->getLatestPortStats(injectPort);

    auto intfMac =
        getMacForFirstInterfaceWithPortsForTesting(this->getProgrammedState());

    // Outer dst IP matches the mySid /48 prefix but is not the last uSid.
    // The packet should be dropped.
    const folly::IPAddressV6 kNonLastSegmentDst{"3001:db8:ffff:1:2::"};

    auto txPacket = utility::makeIpInIpTxPacket(
        this->getSw(),
        this->getVlanIDForTx().value(),
        intfMac,
        intfMac,
        folly::IPAddressV6("1::1"),
        kNonLastSegmentDst,
        folly::IPAddressV6("1::10"),
        this->kV6RouteDstIp,
        8000,
        8001,
        0 /* outerTcField */,
        0 /* innerTrafficClass */,
        64,
        64);

    this->getSw()->sendPacketOutOfPortAsync(std::move(txPacket), injectPort);

    WITH_RETRIES({
      auto portStatsAfter = this->getLatestPortStats(injectPort);
      EXPECT_EVENTUALLY_GT(
          *portStatsAfter.inDiscards_(), *portStatsBefore.inDiscards_());
      EXPECT_EVENTUALLY_TRUE(portStatsAfter.inSrv6MySidDiscards_().has_value());
      EXPECT_EVENTUALLY_GT(
          portStatsAfter.inSrv6MySidDiscards_().value_or(0),
          portStatsBefore.inSrv6MySidDiscards_().value_or(0));
    });
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

// Verify that when an SRv6 encapsulated packet arrives with an inner
// destination matching a local IP, the ASIC decapsulates (strips
// outer header) before punting to CPU. Tests both IPv6 and IPv4 inner
// destinations. Reproduces the RBB issue where the ASIC punted
// packets with the outer SRv6 header still present, causing the agent
// to see a double-header packet and fail to deliver it to kernel.
TYPED_TEST(AgentSrv6DecapTest, verifyDecapPuntStripsOuterHeader) {
  auto setup = [this]() {
    this->setupHelper();
    // Add COPP config to trap packets destined to device's own IPs.
    // Applied only in this test to avoid affecting other tests.
    auto config = this->initialConfig(*this->getAgentEnsemble());
    utility::setDefaultCpuTrafficPolicyConfig(
        config,
        this->getAgentEnsemble()->getL3Asics(),
        this->getAgentEnsemble()->isSai());
    utility::addCpuQueueConfig(
        config,
        this->getAgentEnsemble()->getL3Asics(),
        this->getAgentEnsemble()->isSai());
    this->applyNewConfig(config);
  };

  auto verify = [this]() {
    auto intfMac =
        getMacForFirstInterfaceWithPortsForTesting(this->getProgrammedState());
    auto ecmpHelper = this->makeEcmpHelper();
    auto egressPort = this->getEgressPort(ecmpHelper.nhop(0).portDesc);
    auto injectPort = this->findInjectPort(egressPort);

    auto intfId =
        firstInterfaceIDWithPortsForTesting(this->getProgrammedState());
    auto intf = this->getProgrammedState()->getInterfaces()->getNodeIf(intfId);
    ASSERT_NE(intf, nullptr);

    folly::IPAddressV6 myIntfIpV6;
    folly::IPAddressV4 myIntfIpV4;
    for (const auto& [addr, mask] : std::as_const(*intf->getAddresses())) {
      auto ip = folly::IPAddress(addr);
      if (ip.isV6() && !ip.isLinkLocal() && myIntfIpV6.isZero()) {
        myIntfIpV6 = ip.asV6();
      } else if (ip.isV4() && myIntfIpV4.isZero()) {
        myIntfIpV4 = ip.asV4();
      }
    }
    ASSERT_FALSE(myIntfIpV6.isZero()) << "No IPv6 interface address found";
    ASSERT_NE(myIntfIpV4, folly::IPAddressV4())
        << "No IPv4 interface address found";

    constexpr uint16_t kSrcPort{9000};
    constexpr uint16_t kDstPort{9001};
    constexpr uint8_t kOuterHopLimit{64};
    constexpr uint8_t kInnerHopLimit{64};

    for (bool isV4 : {false, true}) {
      SCOPED_TRACE(isV4 ? "IPv4 inner" : "IPv6 inner");

      std::unique_ptr<facebook::fboss::TxPacket> txPacket;
      if (isV4) {
        txPacket = utility::makeIpInIpTxPacket(
            this->getSw(),
            this->getVlanIDForTx().value(),
            intfMac,
            intfMac,
            folly::IPAddressV6("1::1"),
            this->kMySidAddr,
            folly::IPAddressV4("10.0.0.1"),
            myIntfIpV4,
            kSrcPort,
            kDstPort,
            0 /* outerTrafficClass */,
            0 /* innerDscp */,
            kOuterHopLimit,
            kInnerHopLimit);
      } else {
        txPacket = utility::makeIpInIpTxPacket(
            this->getSw(),
            this->getVlanIDForTx().value(),
            intfMac,
            intfMac,
            folly::IPAddressV6("1::1"),
            this->kMySidAddr,
            folly::IPAddressV6("1::10"),
            myIntfIpV6,
            kSrcPort,
            kDstPort,
            0 /* outerTrafficClass */,
            0 /* innerTrafficClass */,
            kOuterHopLimit,
            kInnerHopLimit);
      }

      auto origFrame = utility::makeEthFrame(*txPacket);
      auto origOuterV6 = origFrame.v6PayLoad();
      ASSERT_TRUE(origOuterV6.has_value());
      std::optional<utility::UDPDatagram> expectedUdp;
      if (isV4) {
        auto innerV4 = origOuterV6->v4PayLoad();
        ASSERT_NE(innerV4, nullptr);
        expectedUdp = innerV4->udpPayload();
      } else {
        auto innerV6 = origOuterV6->v6PayLoad();
        ASSERT_NE(innerV6, nullptr);
        expectedUdp = innerV6->udpPayload();
      }
      ASSERT_TRUE(expectedUdp.has_value());

      utility::SwSwitchPacketSnooper snooper(
          this->getSw(), "srv6DecapPuntSnooper");

      this->getSw()->sendPacketOutOfPortAsync(std::move(txPacket), injectPort);

      auto frameRx = snooper.waitForPacket(10);
      ASSERT_TRUE(frameRx.has_value()) << "Timed out waiting for punted packet";

      folly::io::Cursor cursor((*frameRx).get());
      utility::EthFrame frame(cursor);

      // 1. Confirm the outer header was stripped
      std::optional<utility::UDPDatagram> rxUdp;
      if (isV4) {
        EXPECT_EQ(
            frame.header().etherType,
            static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV4));
        auto rxV4 = frame.v4PayLoad();
        ASSERT_TRUE(rxV4.has_value());
        // 2. Confirm inner IP fields match what we constructed
        EXPECT_EQ(rxV4->header().srcAddr, folly::IPAddressV4("10.0.0.1"));
        EXPECT_EQ(rxV4->header().dstAddr, myIntfIpV4);
        rxUdp = rxV4->udpPayload();
      } else {
        EXPECT_EQ(
            frame.header().etherType,
            static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV6));
        auto rxV6 = frame.v6PayLoad();
        ASSERT_TRUE(rxV6.has_value());
        // 2. Confirm inner IP fields match what we constructed
        EXPECT_EQ(rxV6->header().srcAddr, folly::IPAddressV6("1::10"));
        EXPECT_EQ(rxV6->header().dstAddr, myIntfIpV6);
        rxUdp = rxV6->udpPayload();
      }
      ASSERT_TRUE(rxUdp.has_value());
      EXPECT_EQ(*rxUdp, *expectedUdp);
    }
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
