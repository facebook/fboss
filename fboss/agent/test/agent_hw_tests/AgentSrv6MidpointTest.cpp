// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/AsicUtils.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/packet/Ethertype.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/state/AggregatePort.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/agent/test/TrunkUtils.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/LoadBalancerTestUtils.h"
#include "fboss/agent/test/utils/PacketSnooper.h"
#include "fboss/agent/test/utils/Srv6TestUtils.h"
#include "fboss/agent/test/utils/TrapPacketUtils.h"
#include "fboss/lib/CommonUtils.h"

namespace facebook::fboss {

struct PhysicalPortSrv6Midpoint {
  static constexpr bool isTrunk = false;
};
struct AggregatePortSrv6Midpoint {
  static constexpr bool isTrunk = true;
};
using Srv6MidpointPortTypes =
    ::testing::Types<PhysicalPortSrv6Midpoint, AggregatePortSrv6Midpoint>;

template <typename PortType>
class AgentSrv6MidpointTest : public AgentHwTest {
 protected:
  static constexpr bool kIsTrunk = PortType::isTrunk;

  // MySid prefix: fdad:ffff:1:: /48
  // Locator block: fdad:ffff:: (32 bits), function: 1 (16 bits)
  const folly::IPAddressV6 kMySidPrefix{"fdad:ffff:1::"};
  static constexpr uint8_t kMySidPrefixLen{48};

  // Input outer dst: the SID being processed at this midpoint node.
  // uSID format: [locator fdad:ffff:][active uSID 1:][next uSIDs e002::]
  const folly::IPAddressV6 kPktOuterDst{"fdad:ffff:1:2::"};

  // After the active uSID 1 is processed and shifted out,
  // the rewritten outer dst forwarded to the next hop.
  const folly::IPAddressV6 kExpectedOuterDst{"fdad:ffff:2::"};

  // Active uSID 3 sits in the same locator block but has no entry in
  // mySidConfig (function 1 is the only one configured), so nothing in
  // hardware claims it. Function 3 also keeps it clear of kExpectedOuterDst
  // (fdad:ffff:2::), which the trap ACL owns.
  const folly::IPAddressV6 kUnconfiguredSidOuterDst{"fdad:ffff:3:4::"};

  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    if constexpr (kIsTrunk) {
      return {ProductionFeature::SRV6_MIDPOINT, ProductionFeature::LAG};
    }
    return {ProductionFeature::SRV6_MIDPOINT};
  }

  void setCmdLineFlagOverrides() const override {
    AgentHwTest::setCmdLineFlagOverrides();
    FLAGS_enable_nexthop_id_manager = true;
    FLAGS_resolve_nexthops_from_id = true;
  }

  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    constexpr auto kNumNextHops = 4;
    auto masterLogicalPorts = ensemble.masterLogicalPortIds();
    cfg::SwitchConfig cfg;
    if constexpr (kIsTrunk) {
      std::vector<utility::AggregatePortInfo> aggPorts;
      aggPorts.reserve(kNumNextHops);
      for (int i = 0; i < kNumNextHops; ++i) {
        aggPorts.push_back({AggregatePortID(i + 1), {masterLogicalPorts[i]}});
      }
      cfg = utility::oneAggregatePortPerInterfaceConfig(
          ensemble.getSw(),
          masterLogicalPorts,
          aggPorts,
          true /*interfaceHasSubnet*/);
    } else {
      cfg = utility::onePortPerInterfaceConfig(
          ensemble.getSw(), masterLogicalPorts, true /*interfaceHasSubnet*/);
    }
    cfg.loadBalancers() =
        utility::getEcmpFullWithFlowLabelTrunkFullWithFlowLabelHashConfig(
            ensemble.getL3Asics());
    cfg.mySidConfig() = makeAdjacencyMySidConfig(cfg, ensemble);
    std::vector<cfg::Srv6Tunnel> tunnelList;
    tunnelList.push_back(
        utility::makeSrv6TunnelConfig(
            "srv6Tunnel0", InterfaceID(cfg.interfaces()[0].intfID().value())));
    cfg.srv6Tunnels() = tunnelList;
    // Trap packets with the rewritten outer dst so the snooper can capture
    // the forwarded (uSID-shifted) packet.
    auto asic = checkSameAndGetAsicForTesting(ensemble.getL3Asics());
    utility::addTrapPacketAcl(
        asic,
        &cfg,
        std::set<folly::CIDRNetwork>{{folly::IPAddress("fdad:ffff:2::"), 128}});
    return cfg;
  }

  cfg::MySidConfig makeAdjacencyMySidConfig(
      const cfg::SwitchConfig& cfg,
      const AgentEnsemble& ensemble) const {
    std::string portName;
    if constexpr (kIsTrunk) {
      // utility::addAggPort default name is "AGG-<key>"; key=1 above.
      portName = "AGG-1";
    } else {
      portName =
          utility::portNameForConfig(cfg, ensemble.masterLogicalPortIds()[0]);
    }
    // Function ID 1 → SID fdad:ffff:1::/48 (matches kMySidPrefix).
    return utility::makeAdjacencyMySidConfig(
        portName, "fdad:ffff::/32", /*functionId=*/1);
  }

  void applyConfigAndEnableTrunks(const cfg::SwitchConfig& config) {
    this->applyNewConfig(config);
    this->applyNewState(
        [](const std::shared_ptr<SwitchState> state) {
          return utility::enableTrunkPorts(state);
        },
        "enable trunk ports");
  }

  utility::EcmpSetupAnyNPorts<folly::IPAddressV6> makeEcmpHelper() {
    return utility::EcmpSetupAnyNPorts<folly::IPAddressV6>(
        this->getProgrammedState(),
        this->getSw()->needL2EntryForNeighbor(),
        getLocalMacAddress());
  }

  // The PortDescriptor the uA mysid is wired to. Tests must resolve their
  // ecmp neighbor on this exact descriptor and verify forwarding via this
  // descriptor — picking ecmpHelper.nhop(0) blindly lands on a standalone
  // physical port (PortDescriptor sorts PHYSICAL < AGGREGATE, so any
  // non-aggregated master port comes first in the flat_map).
  PortDescriptor mySidPortDesc() const {
    if constexpr (kIsTrunk) {
      // Matches "AGG-1" baked into makeAdjacencyMySidConfig().
      return PortDescriptor(AggregatePortID(1));
    }
    return PortDescriptor(this->masterLogicalPortIds()[0]);
  }

  void setupHelper() {
    if constexpr (kIsTrunk) {
      applyConfigAndEnableTrunks(
          this->initialConfig(*this->getAgentEnsemble()));
    }
    auto ecmpHelper = makeEcmpHelper();
    auto portDesc = mySidPortDesc();
    this->applyNewState(
        [&ecmpHelper, portDesc](std::shared_ptr<SwitchState> in) {
          return ecmpHelper.resolveNextHops(
              in, {portDesc}, /*useLinkLocal=*/true);
        },
        "resolve mysid neighbor");
    waitForMySidResolveOrUnresolve(/*resolved=*/true);
  }

  // MySidNeighborObserver binds/unbinds the mysid asynchronously: it sees
  // a neighbor delta, queues a RIB update, RIB processes on its own thread,
  // and a state delta lands. SAI's add/remove MySidEntry is gated on
  // mysid->resolved(), so verify() must wait for the binding to settle in
  // either direction or it'll race the first packet.
  void waitForMySidResolveOrUnresolve(bool resolved) {
    auto getStateFn = [this]() { return this->getProgrammedState(); };
    utility::waitForMySidResolveOrUnresolve(
        getStateFn, kMySidPrefix, kMySidPrefixLen, resolved);
  }

  PortID getEgressPort(const PortDescriptor& portDesc) const {
    if (portDesc.isPhysicalPort()) {
      return portDesc.phyPortID();
    }
    auto aggPort = this->getProgrammedState()->getAggregatePorts()->getNodeIf(
        portDesc.aggPortID());
    return aggPort->sortedSubports().front().portID;
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

  // Build and send an IPv6-in-IPv6 midpoint packet. Returns the original
  // EthFrame (for inner-packet comparison) before the txPacket is moved.
  utility::EthFrame sendMidpointPacket(
      bool ecnMarked,
      bool isV4,
      std::optional<PortID> injectPort = std::nullopt,
      std::optional<folly::IPAddressV6> outerDst = std::nullopt) {
    auto dstAddr = outerDst.value_or(kPktOuterDst);
    auto intfMac =
        getMacForFirstInterfaceWithPortsForTesting(this->getProgrammedState());
    constexpr uint8_t kHopLimit{24};
    constexpr uint8_t kTc{42};
    auto tcField = ecnMarked ? static_cast<uint8_t>((kTc << 2) | 0x3)
                             : static_cast<uint8_t>(kTc << 2);

    std::unique_ptr<TxPacket> txPacket;
    if (isV4) {
      txPacket = utility::makeIpInIpTxPacket(
          this->getSw(),
          this->getVlanIDForTx().value(),
          intfMac,
          intfMac,
          folly::IPAddressV6("100::1") /* outerSrc */,
          dstAddr /* outerDst */,
          folly::IPAddressV4("10.0.0.1") /* innerSrc */,
          folly::IPAddressV4("10.0.0.2") /* innerDst */,
          8000 /* srcPort */,
          8001 /* dstPort */,
          tcField /* outerTrafficClass */,
          0 /* innerTrafficClass */,
          kHopLimit,
          64 /* innerHopLimit */);
    } else {
      txPacket = utility::makeIpInIpTxPacket(
          this->getSw(),
          this->getVlanIDForTx().value(),
          intfMac,
          intfMac,
          folly::IPAddressV6("100::1") /* outerSrc */,
          dstAddr /* outerDst */,
          folly::IPAddressV6("2001:db8::1") /* innerSrc */,
          folly::IPAddressV6("2001:db8::2") /* innerDst */,
          8000 /* srcPort */,
          8001 /* dstPort */,
          tcField /* outerTrafficClass */,
          0 /* innerTrafficClass */,
          kHopLimit,
          64 /* innerHopLimit */);
    }

    auto origFrame = utility::makeEthFrame(*txPacket);

    if (injectPort.has_value()) {
      this->getSw()->sendPacketOutOfPortAsync(
          std::move(txPacket), injectPort.value());
    } else {
      this->getSw()->sendPacketSwitchedAsync(std::move(txPacket));
    }

    return origFrame;
  }

  void assertMidpointForwarding(
      PortID egressPort,
      bool ecnMarked,
      bool isV4,
      std::optional<PortID> injectPort = std::nullopt) {
    constexpr uint8_t kHopLimit{24};
    constexpr uint8_t kTc{42};

    auto portStatsBefore = this->getLatestPortStats(egressPort);
    auto bytesBefore = *portStatsBefore.outBytes_();

    // Only filter snooper by egress port if ACL port qualifier is supported.
    auto asic =
        checkSameAndGetAsicForTesting(this->getAgentEnsemble()->getL3Asics());
    std::optional<PortID> snooperPort =
        asic->isSupported(HwAsic::Feature::SAI_ACL_ENTRY_SRC_PORT_QUALIFIER)
        ? std::make_optional(egressPort)
        : std::nullopt;
    utility::SwSwitchPacketSnooper snooper(
        this->getSw(), "srv6MidpointSnooper", snooperPort);

    auto origFrame = sendMidpointPacket(ecnMarked, isV4, injectPort);

    auto frameRx = snooper.waitForPacket(1);
    WITH_RETRIES({
      auto portStatsAfter = this->getLatestPortStats(egressPort);
      EXPECT_EVENTUALLY_GT(*portStatsAfter.outBytes_(), bytesBefore);
      if (!frameRx.has_value()) {
        frameRx = snooper.waitForPacket(1);
      }
      EXPECT_EVENTUALLY_TRUE(frameRx.has_value());
    });

    ASSERT_TRUE(frameRx.has_value());
    folly::io::Cursor cursor((*frameRx).get());
    utility::EthFrame frame(cursor);

    EXPECT_EQ(
        frame.header().etherType,
        static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV6));
    auto rxV6 = frame.v6PayLoad();
    ASSERT_TRUE(rxV6.has_value());
    auto v6Hdr = rxV6->header();
    // The active uSID (1) has been popped; outer dst is now the next SID.
    EXPECT_EQ(v6Hdr.dstAddr, kExpectedOuterDst);
    // Hop limit decremented by 1 during midpoint forwarding.
    EXPECT_EQ(v6Hdr.hopLimit, kHopLimit - 1);
    // DSCP and ECN are preserved in the outer header.
    EXPECT_EQ(v6Hdr.trafficClass >> 2, kTc);
    EXPECT_EQ(v6Hdr.trafficClass & 0x3, ecnMarked ? 0x3 : 0);
    // Inner packet should be unchanged after midpoint uSID shift.
    auto origOuterV6 = origFrame.v6PayLoad();
    ASSERT_TRUE(origOuterV6.has_value());
    if (isV4) {
      auto expectedInnerV4 = origOuterV6->v4PayLoad();
      ASSERT_NE(expectedInnerV4, nullptr);
      auto rxInnerV4 = rxV6->v4PayLoad();
      ASSERT_NE(rxInnerV4, nullptr);
      EXPECT_EQ(*rxInnerV4, *expectedInnerV4);
    } else {
      auto expectedInnerV6 = origOuterV6->v6PayLoad();
      ASSERT_NE(expectedInnerV6, nullptr);
      auto rxInnerV6 = rxV6->v6PayLoad();
      ASSERT_NE(rxInnerV6, nullptr);
      EXPECT_EQ(*rxInnerV6, *expectedInnerV6);
    }
  }

  void verifyMidpointForwarding(
      PortID egressPort,
      bool ecnMarked,
      bool isV4,
      std::optional<PortID> injectPort = std::nullopt) {
    XLOG(DBG2) << "verifyMidpointForwarding: inner=" << (isV4 ? "v4" : "v6")
               << " ecn=" << (ecnMarked ? "marked" : "unmarked")
               << " send=" << (injectPort.has_value() ? "front-panel" : "cpu");
    assertMidpointForwarding(egressPort, ecnMarked, isV4, injectPort);
  }

  void verifyMidpointCpuAndFrontPanel(PortID egressPort) {
    auto injectPort = findInjectPort(egressPort);
    for (bool isV4 : {false, true}) {
      // ECN not marked
      verifyMidpointForwarding(egressPort, false, isV4);
      verifyMidpointForwarding(egressPort, false, isV4, injectPort);
      // ECN marked
      verifyMidpointForwarding(egressPort, true, isV4);
      verifyMidpointForwarding(egressPort, true, isV4, injectPort);
    }
  }

  void verifyMidpointDrop(
      PortID injectPort,
      PortID egressPort,
      bool isV4,
      std::optional<PortID> sendPort = std::nullopt,
      std::optional<folly::IPAddressV6> outerDst = std::nullopt) {
    XLOG(DBG2) << "verifyMidpointDrop: inner=" << (isV4 ? "v4" : "v6")
               << " send=" << (sendPort.has_value() ? "front-panel" : "cpu")
               << " outerDst="
               << (outerDst.has_value() ? outerDst->str() : "default");
    auto portStatsBefore = this->getLatestPortStats(injectPort);
    auto egressStatsBefore = this->getLatestPortStats(egressPort);

    sendMidpointPacket(false /* ecnMarked */, isV4, sendPort, outerDst);

    WITH_RETRIES({
      auto portStatsAfter = this->getLatestPortStats(injectPort);
      auto egressStatsAfter = this->getLatestPortStats(egressPort);
      EXPECT_EVENTUALLY_GT(
          *portStatsAfter.inDiscards_(), *portStatsBefore.inDiscards_());
      EXPECT_EVENTUALLY_TRUE(portStatsAfter.inSrv6MySidDiscards_().has_value());
      EXPECT_EVENTUALLY_GT(
          portStatsAfter.inSrv6MySidDiscards_().value_or(0),
          portStatsBefore.inSrv6MySidDiscards_().value_or(0));
      // Packet should not be forwarded out the egress port.
      EXPECT_EVENTUALLY_EQ(
          *egressStatsAfter.outBytes_(), *egressStatsBefore.outBytes_());
    });
  }

  void verifyMidpointDropFrontPanel(
      PortID egressPort,
      std::optional<folly::IPAddressV6> outerDst = std::nullopt) {
    auto injectPort = findInjectPort(egressPort);
    for (bool isV4 : {false, true}) {
      verifyMidpointDrop(injectPort, egressPort, isV4, injectPort, outerDst);
    }
  }

  // A SID in the locator block that no mysid entry claims is dropped, but not
  // as a mysid discard: inSrv6MySidDiscards is backed by
  // SAI_IN_DROP_REASON_SRV6_LOCAL_SID_DROP, which fires only while processing
  // a *configured* local SID, and MySidConfigUtils programs just the
  // per-function /48s with nothing covering the block. Such a packet misses
  // the route table instead, so it lands in inDstNullDiscards.
  void
  verifyUnconfiguredSidDrop(PortID injectPort, PortID egressPort, bool isV4) {
    XLOG(DBG2) << "verifyUnconfiguredSidDrop: inner=" << (isV4 ? "v4" : "v6")
               << " outerDst=" << kUnconfiguredSidOuterDst.str();
    auto portStatsBefore = this->getLatestPortStats(injectPort);
    auto egressStatsBefore = this->getLatestPortStats(egressPort);
    auto srv6DiscardsBefore =
        portStatsBefore.inSrv6MySidDiscards_().value_or(0);

    sendMidpointPacket(
        false /* ecnMarked */, isV4, injectPort, kUnconfiguredSidOuterDst);

    WITH_RETRIES({
      auto portStatsAfter = this->getLatestPortStats(injectPort);
      auto egressStatsAfter = this->getLatestPortStats(egressPort);
      EXPECT_EVENTUALLY_GT(
          *portStatsAfter.inDstNullDiscards_(),
          *portStatsBefore.inDstNullDiscards_());
    });

    // Packet should not be forwarded out the egress port.
    EXPECT_EQ(
        *this->getLatestPortStats(egressPort).outBytes_(),
        *egressStatsBefore.outBytes_());
    // Only once the drop has been counted is it meaningful to say the mysid
    // counter did not move with it. Checked after the loop rather than as an
    // EXPECT_EVENTUALLY_EQ inside it: these counters are monotonic, so an
    // increment would never retry away, and the loop would burn its whole
    // timeout before failing.
    EXPECT_EQ(
        this->getLatestPortStats(injectPort).inSrv6MySidDiscards_().value_or(0),
        srv6DiscardsBefore);
  }

  void verifyUnconfiguredSidDropFrontPanel(PortID egressPort) {
    auto injectPort = findInjectPort(egressPort);
    for (bool isV4 : {false, true}) {
      verifyUnconfiguredSidDrop(injectPort, egressPort, isV4);
    }
  }
};

TYPED_TEST_SUITE(AgentSrv6MidpointTest, Srv6MidpointPortTypes);

TYPED_TEST(AgentSrv6MidpointTest, sendPacketForUASid) {
  auto setup = [this]() { this->setupHelper(); };

  auto verify = [this]() {
    auto egressPort = this->getEgressPort(this->mySidPortDesc());
    this->verifyMidpointCpuAndFrontPanel(egressPort);
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(AgentSrv6MidpointTest, sendPacketForUASidUnresolvedDropped) {
  auto setup = [this]() {
    this->setupHelper();
    // Unresolve the same mysid neighbor so the mySid entry becomes
    // unresolved (target the exact PortDescriptor, not "first N").
    auto ecmpHelper = this->makeEcmpHelper();
    auto portDesc = this->mySidPortDesc();
    this->applyNewState(
        [&ecmpHelper, portDesc](std::shared_ptr<SwitchState> in) {
          return ecmpHelper.unresolveNextHops(
              in, {portDesc}, /*useLinkLocal=*/true);
        },
        "unresolve mysid neighbor");
    // Wait for MySidNeighborObserver to drop the resolved next hop before
    // letting verify() send packets — otherwise SAI may still hold the
    // programmed entry from setupHelper and the packet would be forwarded.
    this->waitForMySidResolveOrUnresolve(/*resolved=*/false);
  };

  auto verify = [this]() {
    auto egressPort = this->getEgressPort(this->mySidPortDesc());
    this->verifyMidpointDropFrontPanel(egressPort);
    // Same locator block, but a function id the config never programmed.
    // It drops too, yet only the unresolved entry above is a mysid discard —
    // so the mysid counter distinguishes the two, rather than catching
    // anything aimed at the block.
    this->verifyUnconfiguredSidDropFrontPanel(egressPort);
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
