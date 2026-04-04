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

  // MySid prefix: 3001:db8:e001:: /48
  // Locator block: 3001:db8:: (32 bits), function: e001 (16 bits)
  const folly::IPAddressV6 kMySidPrefix{"3001:db8:e001::"};
  static constexpr uint8_t kMySidPrefixLen{48};

  // Input outer dst: the SID being processed at this midpoint node.
  // uSID format: [locator 3001:db8:][active uSID e001:][next uSIDs e002::]
  const folly::IPAddressV6 kPktOuterDst{"3001:db8:e001:e002::"};

  // After the active uSID e001 is processed and shifted out,
  // the rewritten outer dst forwarded to the next hop.
  const folly::IPAddressV6 kExpectedOuterDst{"3001:db8:e002::"};

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
    cfg::SwitchConfig cfg;
    if constexpr (kIsTrunk) {
      cfg = utility::oneL3IntfTwoPortConfig(
          ensemble.getSw(),
          ensemble.masterLogicalPortIds()[0],
          ensemble.masterLogicalPortIds()[1]);
      utility::addAggPort(
          1, {static_cast<int32_t>(ensemble.masterLogicalPortIds()[0])}, &cfg);
      utility::addAggPort(
          2, {static_cast<int32_t>(ensemble.masterLogicalPortIds()[1])}, &cfg);
    } else {
      cfg = utility::onePortPerInterfaceConfig(
          ensemble.getSw(),
          ensemble.masterLogicalPortIds(),
          true /*interfaceHasSubnet*/);
    }
    // Trap packets with the rewritten outer dst so the snooper can capture
    // the forwarded (uSID-shifted) packet.
    auto asic = checkSameAndGetAsic(ensemble.getL3Asics());
    utility::addTrapPacketAcl(
        asic,
        &cfg,
        // change mask to 128 when uSid shift is working
        std::set<folly::CIDRNetwork>{
            {folly::IPAddress("3001:db8:e002::"), 32}});
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
    XLOG(INFO) << "ADDING MY SID WITH NHOP: " << nexthopIp;

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
      std::optional<PortID> injectPort = std::nullopt) {
    auto portStatsBefore = this->getLatestPortStats(egressPort);
    auto bytesBefore = *portStatsBefore.outBytes_();

    auto intfMac =
        utility::getMacForFirstInterfaceWithPorts(this->getProgrammedState());
    constexpr uint8_t kHopLimit{24};
    constexpr uint8_t kTc{42};
    auto tcField = ecnMarked ? static_cast<uint8_t>((kTc << 2) | 0x3)
                             : static_cast<uint8_t>(kTc << 2);

    // Outer IPv6 dst = kPktOuterDst triggers the ADJACENCY MySid at
    // kMySidPrefix. The ASIC pops uSID e001, rewrites dst to kExpectedOuterDst,
    // and forwards out the adjacency port.
    auto txPacket = utility::makeIpInIpTxPacket(
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
    // The active uSID (e001) has been popped; outer dst is now the next SID.
    EXPECT_EQ(v6Hdr.dstAddr, kExpectedOuterDst);
    // Hop limit decremented by 1 during midpoint forwarding.
    EXPECT_EQ(v6Hdr.hopLimit, kHopLimit - 1);
    // DSCP and ECN are preserved in the outer header.
    EXPECT_EQ(v6Hdr.trafficClass >> 2, kTc);
    EXPECT_EQ(v6Hdr.trafficClass & 0x3, ecnMarked ? 0x3 : 0);
  }

  void verifyMidpointCpuAndFrontPanel(PortID egressPort) {
    auto injectPort = findInjectPort(egressPort);
    // ECN not marked
    verifyMidpointForwarding(egressPort, false);
    verifyMidpointForwarding(egressPort, false, injectPort);
    // ECN marked
    verifyMidpointForwarding(egressPort, true);
    verifyMidpointForwarding(egressPort, true, injectPort);
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

} // namespace facebook::fboss
