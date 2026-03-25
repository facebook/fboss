// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/AsicUtils.h"
#include "fboss/agent/SwSwitchMySidUpdater.h"
#include "fboss/agent/SwSwitchRouteUpdateWrapper.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
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
  const folly::IPAddressV6 kMySidAddr{"3001:db8:e001::"};

  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    if constexpr (kIsTrunk) {
      return {ProductionFeature::SRV6_DECAP, ProductionFeature::LAG};
    }
    return {ProductionFeature::SRV6_DECAP};
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
    // Add trap ACLs for inner packet destinations so snooper can capture
    // the decapped and forwarded packets
    auto asic = checkSameAndGetAsic(ensemble.getL3Asics());
    utility::addTrapPacketAcl(
        asic,
        &cfg,
        std::set<folly::CIDRNetwork>{
            {kV6RouteDstIp, 128}, {kV4RouteDstIp, 32}});
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

  void resolveV4AndV6NextHops(int numNextHops) {
    auto ecmpHelper6 = makeEcmpHelper<folly::IPAddressV6>();
    this->resolveNeighbors(ecmpHelper6, numNextHops);
    auto ecmpHelper4 = makeEcmpHelper<folly::IPAddressV4>();
    this->resolveNeighbors(ecmpHelper4, numNextHops);
  }

  void setupHelper(bool resolveNeighbors = true) {
    if constexpr (kIsTrunk) {
      applyConfigAndEnableTrunks(
          this->initialConfig(*this->getAgentEnsemble()));
    }
    if (resolveNeighbors) {
      resolveV4AndV6NextHops(2);
    }
    // IPv6 route with regular next hops (no SID lists)
    addRoute<folly::CIDRNetworkV6>(
        {kV6RoutePrefix, kV6RoutePrefixLen}, 1 /*numNextHops*/);
    // IPv4 route with regular next hops (no SID lists)
    addRoute<folly::CIDRNetworkV4>(
        {folly::IPAddressV4("100.0.0.0"), 24}, 1 /*numNextHops*/);
    // Add a mySid entry for decapsulation
    // G200 required use of local uSid range 0xe000 - 0xffff
    // for uA, uA part of uNuA, uDT*, B6_ENCAPS_RED
    addMySidEntry("3001:db8:e001::", 48);
  }

  template <typename CIDRNetworkT>
  void addRoute(const CIDRNetworkT& prefix, int numNextHops) {
    using IPAddrT = decltype(prefix.first);
    auto ecmpHelper = makeEcmpHelper<std::remove_const_t<IPAddrT>>();
    RouteNextHopSet nhops;
    for (auto i = 0; i < numNextHops; ++i) {
      auto nhop = ecmpHelper.nhop(i);
      nhops.insert(ResolvedNextHop(nhop.ip, nhop.intf, ECMP_WEIGHT));
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

  void addMySidEntry(const std::string& addr, uint8_t prefixLen) {
    MySidEntry entry;
    entry.type() = MySidType::DECAPSULATE_AND_LOOKUP;
    facebook::network::thrift::IPPrefix prefix;
    prefix.prefixAddress() =
        facebook::network::toBinaryAddress(folly::IPAddressV6(addr));
    prefix.prefixLength() = prefixLen;
    entry.mySid() = prefix;
    auto sw = this->getSw();
    auto rib = sw->getRib();
    auto ribMySidToSwitchStateFunc =
        createRibMySidToSwitchStateFunction(std::nullopt);
    rib->update(
        sw->getScopeResolver(),
        {entry},
        {} /* toDelete */,
        "addMySidEntry",
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

  void verifyDecapPacket(
      PortID egressPort,
      bool isV4,
      std::optional<PortID> injectPort = std::nullopt) {
    auto portStatsBefore = this->getLatestPortStats(egressPort);
    auto bytesBefore = *portStatsBefore.outBytes_();

    auto intfMac =
        utility::getMacForFirstInterfaceWithPorts(this->getProgrammedState());
    constexpr uint16_t kSrcPort{8000};
    constexpr uint16_t kDstPort{8001};
    constexpr uint8_t kHopLimit{64};

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
          0 /* outerTrafficClass */,
          0 /* innerDscp */,
          kHopLimit);
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
          0 /* outerTrafficClass */,
          0 /* innerTrafficClass */,
          kHopLimit);
    }

    utility::SwSwitchPacketSnooper snooper(this->getSw(), "srv6DecapSnooper");

    if (injectPort.has_value()) {
      this->getSw()->sendPacketOutOfPortAsync(
          std::move(txPacket), injectPort.value());
    } else {
      this->getSw()->sendPacketSwitchedAsync(std::move(txPacket));
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
    if (isV4) {
      EXPECT_EQ(
          frame.header().etherType,
          static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV4));
      auto rxV4 = frame.v4PayLoad();
      ASSERT_TRUE(rxV4.has_value());
      auto v4Hdr = rxV4->header();
      EXPECT_EQ(v4Hdr.srcAddr, folly::IPAddressV4("10.0.0.1"));
      EXPECT_EQ(v4Hdr.dstAddr, kV4RouteDstIp);
      auto rxUdp = rxV4->udpPayload();
      ASSERT_TRUE(rxUdp.has_value());
      EXPECT_EQ(rxUdp->header().srcPort, kSrcPort);
      EXPECT_EQ(rxUdp->header().dstPort, kDstPort);
    } else {
      EXPECT_EQ(
          frame.header().etherType,
          static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV6));
      auto rxV6 = frame.v6PayLoad();
      ASSERT_TRUE(rxV6.has_value());
      auto v6Hdr = rxV6->header();
      EXPECT_EQ(v6Hdr.srcAddr, folly::IPAddressV6("1::10"));
      EXPECT_EQ(v6Hdr.dstAddr, kV6RouteDstIp);
      auto rxUdp = rxV6->udpPayload();
      ASSERT_TRUE(rxUdp.has_value());
      EXPECT_EQ(rxUdp->header().srcPort, kSrcPort);
      EXPECT_EQ(rxUdp->header().dstPort, kDstPort);
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
    // Verify decap with inner v6 packet
    this->verifyDecapPacket(egressPort, false /* isV4 */);
    // Verify decap with inner v4 packet
    this->verifyDecapPacket(egressPort, true /* isV4 */);
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
