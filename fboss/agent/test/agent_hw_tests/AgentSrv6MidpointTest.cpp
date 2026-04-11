// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/AsicUtils.h"
#include "fboss/agent/SwSwitchMySidUpdater.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/if/gen-cpp2/common_types.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/agent/packet/Ethertype.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/state/AggregatePort.h"
#include "fboss/agent/state/RouteNextHop.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/agent/test/TrunkUtils.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/PacketSnooper.h"
#include "fboss/agent/test/utils/TrapPacketUtils.h"
#include "fboss/lib/CommonUtils.h"

#include "fboss/agent/AddressUtil.h"

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

  // MySid prefix: 3001:db8:1:: /48
  // Locator block: 3001:db8:: (32 bits), function: 1 (16 bits)
  const folly::IPAddressV6 kMySidPrefix{"3001:db8:1::"};
  static constexpr uint8_t kMySidPrefixLen{48};

  // Input outer dst: the SID being processed at this midpoint node.
  // uSID format: [locator 3001:db8:][active uSID 1:][next uSIDs e002::]
  const folly::IPAddressV6 kPktOuterDst{"3001:db8:1:2::"};

  // After the active uSID 1 is processed and shifted out,
  // the rewritten outer dst forwarded to the next hop.
  const folly::IPAddressV6 kExpectedOuterDst{"3001:db8:2::"};

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
    // Trap packets with the rewritten outer dst so the snooper can capture
    // the forwarded (uSID-shifted) packet.
    auto asic = checkSameAndGetAsic(ensemble.getL3Asics());
    utility::addTrapPacketAcl(
        asic,
        &cfg,
        // change mask to 128 when uSid shift is working
        std::set<folly::CIDRNetwork>{{folly::IPAddress("3001:db8:2::"), 32}});
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

  utility::EcmpSetupAnyNPorts<folly::IPAddressV6> makeEcmpHelper() {
    return utility::EcmpSetupAnyNPorts<folly::IPAddressV6>(
        this->getProgrammedState(),
        this->getSw()->needL2EntryForNeighbor(),
        getLocalMacAddress());
  }

  void setupHelper() {
    if constexpr (kIsTrunk) {
      applyConfigAndEnableTrunks(
          this->initialConfig(*this->getAgentEnsemble()));
    }
    auto ecmpHelper = makeEcmpHelper();
    this->resolveNeighbors(ecmpHelper, 1);
    addAdjacencyMySidEntry(ecmpHelper.nhop(0).ip);
  }

  void addAdjacencyMySidEntry(const folly::IPAddress& nexthopIp) {
    MySidEntry entry;
    entry.type() = MySidType::ADJACENCY_MICRO_SID;
    facebook::network::thrift::IPPrefix prefix;
    prefix.prefixAddress() =
        facebook::network::toBinaryAddress(folly::IPAddress(kMySidPrefix));
    prefix.prefixLength() = kMySidPrefixLen;
    entry.mySid() = prefix;

    NextHopThrift nhop;
    nhop.address() = facebook::network::toBinaryAddress(nexthopIp);
    entry.nextHops()->push_back(nhop);

    auto sw = this->getSw();
    auto rib = sw->getRib();
    auto ribMySidToSwitchStateFunc =
        createRibMySidToSwitchStateFunction(std::nullopt);
    rib->update(
        sw->getScopeResolver(),
        {entry},
        {} /* toDelete */,
        "addAdjacencyMySidEntry",
        ribMySidToSwitchStateFunc,
        sw);
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

  void verifyMidpointForwarding(
      PortID egressPort,
      bool ecnMarked,
      bool isV4,
      std::optional<PortID> injectPort = std::nullopt) {
    auto portStatsBefore = this->getLatestPortStats(egressPort);
    auto bytesBefore = *portStatsBefore.outBytes_();

    auto intfMac =
        getMacForFirstInterfaceWithPortsForTesting(this->getProgrammedState());
    constexpr uint8_t kHopLimit{24};
    constexpr uint8_t kTc{42};
    auto tcField = ecnMarked ? static_cast<uint8_t>((kTc << 2) | 0x3)
                             : static_cast<uint8_t>(kTc << 2);

    // Outer IPv6 dst = kPktOuterDst triggers the ADJACENCY MySid at
    // kMySidPrefix. The ASIC pops uSID 1, rewrites dst to kExpectedOuterDst,
    // and forwards out the adjacency port.
    std::unique_ptr<TxPacket> txPacket;
    if (isV4) {
      txPacket = utility::makeIpInIpTxPacket(
          this->getSw(),
          this->getVlanIDForTx().value(),
          intfMac,
          intfMac,
          folly::IPAddressV6("100::1") /* outerSrc */,
          kPktOuterDst /* outerDst */,
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
          kPktOuterDst /* outerDst */,
          folly::IPAddressV6("2001:db8::1") /* innerSrc */,
          folly::IPAddressV6("2001:db8::2") /* innerDst */,
          8000 /* srcPort */,
          8001 /* dstPort */,
          tcField /* outerTrafficClass */,
          0 /* innerTrafficClass */,
          kHopLimit,
          64 /* innerHopLimit */);
    }

    // Capture original frame before moving txPacket, for inner packet
    // comparison.
    auto origFrame = utility::makeEthFrame(*txPacket);

    utility::SwSwitchPacketSnooper snooper(
        this->getSw(), "srv6MidpointSnooper");

    if (injectPort.has_value()) {
      this->getSw()->sendPacketOutOfPortAsync(
          std::move(txPacket), injectPort.value());
    } else {
      this->getSw()->sendPacketSwitchedAsync(std::move(txPacket));
    }

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
};

TYPED_TEST_SUITE(AgentSrv6MidpointTest, Srv6MidpointPortTypes);

TYPED_TEST(AgentSrv6MidpointTest, sendPacketForUASid) {
  auto setup = [this]() { this->setupHelper(); };

  auto verify = [this]() {
    auto ecmpHelper = this->makeEcmpHelper();
    auto egressPort = this->getEgressPort(ecmpHelper.nhop(0).portDesc);
    this->verifyMidpointCpuAndFrontPanel(egressPort);
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(AgentSrv6MidpointTest, sendPacketForUASidUnresolvedDropped) {
  auto setup = [this]() {
    this->setupHelper();
    // Unresolve the neighbor so the mySid entry becomes unresolved.
    auto ecmpHelper = this->makeEcmpHelper();
    this->unresolveNeighbors(ecmpHelper, 1);
  };

  auto verify = [this]() {
    auto ecmpHelper = this->makeEcmpHelper();
    auto egressPort = this->getEgressPort(ecmpHelper.nhop(0).portDesc);
    auto injectPort = this->findInjectPort(egressPort);

    auto portStatsBefore = this->getLatestPortStats(injectPort);

    auto intfMac =
        utility::getMacForFirstInterfaceWithPorts(this->getProgrammedState());

    auto txPacket = utility::makeIpInIpTxPacket(
        this->getSw(),
        this->getVlanIDForTx().value(),
        intfMac,
        intfMac,
        folly::IPAddressV6("100::1") /* outerSrc */,
        this->kPktOuterDst /* outerDst */,
        folly::IPAddressV6("2001:db8::1") /* innerSrc */,
        folly::IPAddressV6("2001:db8::2") /* innerDst */,
        8000 /* srcPort */,
        8001 /* dstPort */,
        0 /* outerTrafficClass */,
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

} // namespace facebook::fboss
