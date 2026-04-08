// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/AsicUtils.h"
#include "fboss/agent/SwSwitchRouteUpdateWrapper.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/packet/Ethertype.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/state/AggregatePort.h"
#include "fboss/agent/state/RouteNextHop.h"
#include "fboss/agent/state/Srv6Tunnel.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/TrunkUtils.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/CoppTestUtils.h"
#include "fboss/agent/test/utils/OlympicTestUtils.h"
#include "fboss/agent/test/utils/PacketSnooper.h"
#include "fboss/agent/test/utils/PortTestUtils.h"
#include "fboss/agent/test/utils/QosTestUtils.h"
#include "fboss/agent/test/utils/Srv6TestUtils.h"
#include "fboss/agent/test/utils/TrapPacketUtils.h"
#include "fboss/lib/CommonUtils.h"

namespace facebook::fboss {

struct PhysicalPortSrv6 {
  static constexpr bool isTrunk = false;
};
struct AggregatePortSrv6 {
  static constexpr bool isTrunk = true;
};
using Srv6EncapPortTypes =
    ::testing::Types<PhysicalPortSrv6, AggregatePortSrv6>;

template <typename PortType>
class AgentSrv6EncapTest : public AgentHwTest {
 protected:
  static constexpr bool kIsTrunk = PortType::isTrunk;

  // All 6 uSids populated
  static inline const folly::IPAddressV6 kSid0{"3001:db8:1:2:3:4:5:6"};
  // 3 uSids populated
  static inline const folly::IPAddressV6 kSid1{"3001:db8:4:5:6::"};
  static inline const folly::IPAddressV6 kSid2{"3001:db8:7:8:9::"};

  const folly::IPAddressV6 kEncapRoutePrefix{"2800:2::"};
  static constexpr uint8_t kEncapRoutePrefixLen{64};
  const folly::IPAddressV6 kEncapRouteDstIp{"2800:2::1"};
  static constexpr int kNumNextHops{4};
  static constexpr uint8_t kECT1{1};

  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    if constexpr (kIsTrunk) {
      return {
          ProductionFeature::SRV6_ENCAP,
          ProductionFeature::L3_QOS,
          ProductionFeature::ECN,
          ProductionFeature::LAG};
    }
    return {
        ProductionFeature::SRV6_ENCAP,
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
    addSrv6TunnelConfig(cfg);
    auto asic = checkSameAndGetAsic(ensemble.getL3Asics());
    utility::addTrapPacketAcl(
        asic,
        &cfg,
        {folly::CIDRNetwork{kSid0, 128},
         folly::CIDRNetwork{kSid1, 128},
         folly::CIDRNetwork{kSid2, 128}});
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

  void resolveV4AndV6NextHops(int numNextHops) {
    auto ecmpHelper6 = makeEcmpHelper<folly::IPAddressV6>();
    this->resolveNeighbors(ecmpHelper6, numNextHops);
    auto ecmpHelper4 = makeEcmpHelper<folly::IPAddressV4>();
    this->resolveNeighbors(ecmpHelper4, numNextHops);
  }

  void unresolveV4AndV6NextHops(int numNextHops) {
    auto ecmpHelper6 = makeEcmpHelper<folly::IPAddressV6>();
    this->unresolveNeighbors(ecmpHelper6, numNextHops);
    auto ecmpHelper4 = makeEcmpHelper<folly::IPAddressV4>();
    this->unresolveNeighbors(ecmpHelper4, numNextHops);
  }
  void setupHelper(
      bool resolveNeighbors = true,
      bool programEncapRoutes = true) {
    if constexpr (kIsTrunk) {
      applyConfigAndEnableTrunks(
          this->initialConfig(*this->getAgentEnsemble()));
    }
    if (resolveNeighbors) {
      resolveV4AndV6NextHops(kNumNextHops);
    }
    if (programEncapRoutes) {
      // IPv6 encap routes (v6 next hops)
      addEncapRoute<folly::CIDRNetworkV6>(
          {kEncapRoutePrefix, kEncapRoutePrefixLen}, {{kSid0}});
      addEncapRoute<folly::CIDRNetworkV6>(
          {folly::IPAddressV6("2800:3::"), kEncapRoutePrefixLen},
          {{kSid1}, {kSid2}});
      addEncapRoute<folly::CIDRNetworkV6>(
          {folly::IPAddressV6("2800:4::"), kEncapRoutePrefixLen},
          {{kSid1}, {kSid2}});
      // IPv4 encap routes (v4 next hops)
      addEncapRoute<folly::CIDRNetworkV4>(
          {folly::IPAddressV4("100.0.0.0"), 24}, {{kSid0}});
      addEncapRoute<folly::CIDRNetworkV4>(
          {folly::IPAddressV4("200.0.0.0"), 24}, {{kSid1}, {kSid2}});
      addEncapRoute<folly::CIDRNetworkV4>(
          {folly::IPAddressV4("201.0.0.0"), 24}, {{kSid1}, {kSid2}});
    }
  }

  // Programs recursive SRv6 routes for testing SID list preservation:
  //   IGP route A (2901::/48) -> nhop(0), nhop(1) with igpSidListA
  //   IGP route B (2902::/48) -> nhop(2), nhop(3) with igpSidListB
  //   SRv6 route (routePrefix/kEncapRoutePrefixLen) -> 2901::1 (kSid0),
  //     2902::1 (kSid1), resolving recursively through IGP routes.
  // After resolution, the SRv6 route expands to 4 next hops preserving
  // the original SID lists (kSid0/kSid1), not the IGP SID lists.
  void addRecursiveSrv6Routes(const folly::IPAddressV6& routePrefix) {
    auto ecmpHelper = makeEcmpHelper();
    auto routeUpdater = this->getSw()->getRouteUpdater();

    auto makeSrv6Nhop = [](const folly::IPAddress& ip,
                           const std::vector<folly::IPAddressV6>& sidList,
                           const std::string& tunnelId) {
      return UnresolvedNextHop(
          ip,
          ECMP_WEIGHT,
          std::nullopt,
          std::nullopt,
          std::nullopt,
          std::nullopt,
          sidList,
          TunnelType::SRV6_ENCAP,
          tunnelId);
    };

    // IGP route A (2901::/48) -> nhop(0), nhop(1) with igpSidListA
    const std::vector<folly::IPAddressV6> igpSidListA{
        folly::IPAddressV6("4001:db8:a1::")};
    RouteNextHopSet igpNhopsA{
        makeSrv6Nhop(ecmpHelper.nhop(0).ip, igpSidListA, "srv6Tunnel0"),
        makeSrv6Nhop(ecmpHelper.nhop(1).ip, igpSidListA, "srv6Tunnel0")};
    routeUpdater.addRoute(
        RouterID(0),
        folly::IPAddressV6("2901::"),
        48,
        ClientID::OPENR,
        RouteNextHopEntry(igpNhopsA, AdminDistance::MAX_ADMIN_DISTANCE));

    // IGP route B (2902::/48) -> nhop(2), nhop(3) with igpSidListB
    const std::vector<folly::IPAddressV6> igpSidListB{
        folly::IPAddressV6("4001:db8:b1::")};
    RouteNextHopSet igpNhopsB{
        makeSrv6Nhop(ecmpHelper.nhop(2).ip, igpSidListB, "srv6Tunnel0"),
        makeSrv6Nhop(ecmpHelper.nhop(3).ip, igpSidListB, "srv6Tunnel0")};
    routeUpdater.addRoute(
        RouterID(0),
        folly::IPAddressV6("2902::"),
        48,
        ClientID::OPENR,
        RouteNextHopEntry(igpNhopsB, AdminDistance::MAX_ADMIN_DISTANCE));

    // SRv6 route -> 2901::1 (kSid0), 2902::1 (kSid1)
    RouteNextHopSet srv6Nhops{
        makeSrv6Nhop(folly::IPAddress("2901::1"), {kSid0}, "srv6Tunnel0"),
        makeSrv6Nhop(folly::IPAddress("2902::1"), {kSid1}, "srv6Tunnel0")};
    routeUpdater.addRoute(
        RouterID(0),
        routePrefix,
        kEncapRoutePrefixLen,
        ClientID::BGPD,
        RouteNextHopEntry(srv6Nhops, AdminDistance::EBGP));
    routeUpdater.program();
  }

  template <typename CIDRNetworkT>
  void addEncapRoute(
      const CIDRNetworkT& prefix,
      const std::vector<std::vector<folly::IPAddressV6>>& sidLists) {
    using IPAddrT = decltype(prefix.first);
    auto ecmpHelper = makeEcmpHelper<std::remove_const_t<IPAddrT>>();
    RouteNextHopSet nhops;
    for (auto i = 0; i < sidLists.size(); ++i) {
      auto nhop = ecmpHelper.nhop(i);
      nhops.insert(ResolvedNextHop(
          nhop.ip,
          nhop.intf,
          ECMP_WEIGHT,
          std::nullopt,
          std::nullopt,
          std::nullopt,
          std::nullopt,
          sidLists[i],
          TunnelType::SRV6_ENCAP,
          std::string("srv6Tunnel0")));
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

  void verifyEncapPacket(
      const std::vector<PortID>& egressPorts,
      bool ecnMarked,
      bool isV4 = false,
      const std::vector<folly::IPAddressV6>& expectedSids = {kSid0},
      std::optional<PortID> injectPort = std::nullopt,
      std::optional<folly::IPAddress> dstIp = std::nullopt) {
    const auto& sids = expectedSids;

    std::map<PortID, int64_t> bytesBefore;
    for (auto port : egressPorts) {
      bytesBefore[port] = *this->getLatestPortStats(port).outBytes_();
    }

    auto intfMac =
        utility::getMacForFirstInterfaceWithPorts(this->getProgrammedState());
    constexpr auto kTc{42};
    constexpr auto kTtl{24};
    auto tcField = ecnMarked ? static_cast<uint8_t>((kTc << 2) | 0x3)
                             : static_cast<uint8_t>(kTc << 2);
    auto srcIp =
        isV4 ? folly::IPAddress("10.0.0.1") : folly::IPAddress("1::10");
    auto pktDstIp = dstIp.has_value()
        ? dstIp.value()
        : (isV4 ? folly::IPAddress("100.0.0.1")
                : folly::IPAddress(kEncapRouteDstIp));
    auto txPacket = utility::makeUDPTxPacket(
        this->getSw(),
        this->getVlanIDForTx(),
        intfMac,
        intfMac,
        srcIp,
        pktDstIp,
        8000,
        8001,
        tcField,
        kTtl);

    auto origFrame = utility::makeEthFrame(*txPacket);

    utility::SwSwitchPacketSnooper snooper(this->getSw(), "srv6EncapSnooper");

    if (injectPort.has_value()) {
      this->getSw()->sendPacketOutOfPortAsync(
          std::move(txPacket), injectPort.value());
    } else {
      this->getSw()->sendPacketSwitchedAsync(std::move(txPacket));
    }

    auto frameRx = snooper.waitForPacket(1);
    WITH_RETRIES({
      bool anyPortGotBytes = false;
      for (auto port : egressPorts) {
        auto bytesAfter = *this->getLatestPortStats(port).outBytes_();
        if (bytesAfter > bytesBefore[port]) {
          anyPortGotBytes = true;
        }
      }
      EXPECT_EVENTUALLY_TRUE(anyPortGotBytes);
      if (!frameRx.has_value()) {
        frameRx = snooper.waitForPacket(1);
      }
      EXPECT_EVENTUALLY_TRUE(frameRx.has_value());
    });
    ASSERT_TRUE(frameRx.has_value());
    folly::io::Cursor cursor((*frameRx).get());
    utility::EthFrame frame(cursor);
    auto ethHdr = frame.header();
    // Outer header is always IPv6 (SRv6 encap)
    EXPECT_EQ(
        ethHdr.etherType, static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV6));
    auto v6Payload = frame.v6PayLoad();
    EXPECT_TRUE(v6Payload.has_value());
    auto v6Hdr = v6Payload->header();
    // Outer header dst addr should match one of the expected SIDs
    bool sidMatch = std::any_of(sids.begin(), sids.end(), [&](const auto& sid) {
      return v6Hdr.dstAddr == sid;
    });
    EXPECT_TRUE(sidMatch) << "Outer DA " << v6Hdr.dstAddr
                          << " does not match any expected SID";
    // Flow label must be non 0
    EXPECT_NE(v6Hdr.flowLabel, 0);
    EXPECT_EQ(v6Hdr.trafficClass & 0x3, ecnMarked ? 0x3 : 0);
    EXPECT_EQ(v6Hdr.trafficClass >> 2, kTc);
    // TTL is decremented
    EXPECT_EQ(v6Hdr.hopLimit, kTtl - 1);
    // Compare origPacket against inner packet
    if (isV4) {
      auto origPacket = origFrame.v4PayLoad();
      ASSERT_TRUE(origPacket.has_value());
      auto innerV4 = v6Payload->v4PayLoad();
      ASSERT_NE(innerV4, nullptr);
      // Inner packet should match origPacket. Note makeEthFrame(txPacket)
      // already does a TTL decrement by default, so we don't have to account
      // for it here.
      EXPECT_EQ(*innerV4, *origPacket);
    } else {
      auto origPacket = origFrame.v6PayLoad();
      ASSERT_TRUE(origPacket.has_value());
      // Inner packet should match origPacket. Note makeEthFrame(txPacket)
      // already does a TTL decrement by default, so we don't have to account
      // for it here.
      EXPECT_EQ(*v6Payload->v6PayLoad(), *origPacket);
    }
  }

  void verifyEncapPacketCpuAndFrontPanel(
      const std::vector<PortID>& egressPorts,
      const std::vector<folly::IPAddressV6>& expectedSids = {kSid0}) {
    auto injectPort = findInjectPort(egressPorts);
    for (bool isV4 : {false, true}) {
      // ECN not marked
      verifyEncapPacket(egressPorts, false, isV4, expectedSids);
      verifyEncapPacket(egressPorts, false, isV4, expectedSids, injectPort);
      // ECN marked
      verifyEncapPacket(egressPorts, true, isV4, expectedSids);
      verifyEncapPacket(egressPorts, true, isV4, expectedSids, injectPort);
    }
  }

  PortID findInjectPort(const std::vector<PortID>& egressPorts) {
    for (const auto& portMap :
         std::as_const(*this->getProgrammedState()->getPorts())) {
      for (const auto& [_, port] : std::as_const(*portMap.second)) {
        if (port->isPortUp() &&
            std::find(egressPorts.begin(), egressPorts.end(), port->getID()) ==
                egressPorts.end()) {
          return port->getID();
        }
      }
    }
    throw FbossError("No UP port found besides egress ports");
  }

 private:
  void addSrv6TunnelConfig(cfg::SwitchConfig& cfg) const {
    std::vector<cfg::Srv6Tunnel> tunnelList;
    tunnelList.push_back(
        utility::makeSrv6TunnelConfig(
            "srv6Tunnel0", InterfaceID(cfg.interfaces()[0].intfID().value())));
    cfg.srv6Tunnels() = tunnelList;
  }
};

TYPED_TEST_SUITE(AgentSrv6EncapTest, Srv6EncapPortTypes);

TYPED_TEST(AgentSrv6EncapTest, sendPacketToEncapRoute) {
  auto setup = [this]() { this->setupHelper(); };

  auto verify = [this]() {
    auto ecmpHelper = this->makeEcmpHelper();
    auto egressPort = this->getEgressPort(ecmpHelper.nhop(0).portDesc);
    this->verifyEncapPacketCpuAndFrontPanel({egressPort});
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(AgentSrv6EncapTest, sendPacketToEncapRouteAfterLinkFlap) {
  auto setup = [this]() {
    this->setupHelper();
    // Flap ports and re-resolve neighbors
    auto ecmpHelper = this->makeEcmpHelper();
    auto egressPort = this->getEgressPort(ecmpHelper.nhop(0).portDesc);
    this->bringDownPort(egressPort);
    this->unresolveV4AndV6NextHops(2);
    this->bringUpPort(egressPort);
    // For aggregate ports, the SAI fast-path link down handler disables
    // the LAG member (EgressDisable=true). Since LACP is not running in
    // tests, nobody updates the switch state to reflect this. We must
    // first set forwarding to DISABLED (to match SAI state), then back
    // to ENABLED to generate a state delta that triggers
    // SaiLagManager::changeLag() to re-enable the SAI LAG member.
    constexpr bool isTrunk = TestFixture::kIsTrunk;

    if constexpr (isTrunk) {
      this->applyNewState(
          [](const std::shared_ptr<SwitchState> state) {
            return utility::disableTrunkPorts(state);
          },
          "disable trunk ports to sync with SAI state");
      this->applyNewState(
          [](const std::shared_ptr<SwitchState> state) {
            return utility::enableTrunkPorts(state);
          },
          "re-enable trunk ports after link flap");
    }
    this->resolveV4AndV6NextHops(2);
  };

  auto verify = [this]() {
    auto ecmpHelper = this->makeEcmpHelper();
    auto egressPort = this->getEgressPort(ecmpHelper.nhop(0).portDesc);
    this->verifyEncapPacketCpuAndFrontPanel({egressPort});
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(AgentSrv6EncapTest, recursiveResolutionPreservesSidList) {
  auto setup = [this]() {
    // Skip programming direct encap routes to avoid colliding
    // SRv6 managed next hop keys with recursive resolution.
    this->setupHelper(true /*resolveNeighbors*/, false /*programEncapRoutes*/);
    this->addRecursiveSrv6Routes(this->kEncapRoutePrefix);
  };

  auto verify = [this]() {
    auto ecmpHelper = this->makeEcmpHelper();
    // Recursive resolution expands to kNumNextHops next hops —
    // the packet may egress on any of these ports.
    std::vector<PortID> egressPorts;
    egressPorts.reserve(this->kNumNextHops);
    for (int i = 0; i < this->kNumNextHops; ++i) {
      egressPorts.push_back(this->getEgressPort(ecmpHelper.nhop(i).portDesc));
    }
    this->verifyEncapPacket(
        egressPorts,
        false /*ecnMarked*/,
        false /*isV4*/,
        {this->kSid0, this->kSid1});
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(AgentSrv6EncapTest, resolveNeighborsAfterRouteProgram) {
  auto setup = [this]() {
    this->setupHelper(false /*resolveNeighbors*/);
    // Resolve neighbors after route programming
    this->resolveV4AndV6NextHops(2);
  };

  auto verify = [this]() {
    auto ecmpHelper = this->makeEcmpHelper();
    auto egressPort = this->getEgressPort(ecmpHelper.nhop(0).portDesc);
    this->verifyEncapPacketCpuAndFrontPanel({egressPort});
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(AgentSrv6EncapTest, multipleSidListsSameNextHop) {
  auto setup = [this]() {
    this->setupHelper(true /*resolveNeighbors*/, false /*programEncapRoutes*/);

    auto ecmpHelper = this->makeEcmpHelper();
    auto nhop0 = ecmpHelper.nhop(0);
    auto nhop1 = ecmpHelper.nhop(1);

    // Phase 1: Two routes with different SID lists both via nhop(0)
    // Route A: kEncapRoutePrefix/64 -> nhop(0) with kSid0
    // Route B: 2800:3::/64 -> nhop(0) with kSid1
    {
      auto routeUpdater = this->getSw()->getRouteUpdater();
      RouteNextHopSet nhopsA;
      nhopsA.insert(ResolvedNextHop(
          nhop0.ip,
          nhop0.intf,
          ECMP_WEIGHT,
          std::nullopt,
          std::nullopt,
          std::nullopt,
          std::nullopt,
          std::vector<folly::IPAddressV6>{this->kSid0},
          TunnelType::SRV6_ENCAP,
          std::string("srv6Tunnel0")));
      routeUpdater.addRoute(
          RouterID(0),
          this->kEncapRoutePrefix,
          this->kEncapRoutePrefixLen,
          ClientID::BGPD,
          RouteNextHopEntry(nhopsA, AdminDistance::EBGP));

      RouteNextHopSet nhopsB;
      nhopsB.insert(ResolvedNextHop(
          nhop0.ip,
          nhop0.intf,
          ECMP_WEIGHT,
          std::nullopt,
          std::nullopt,
          std::nullopt,
          std::nullopt,
          std::vector<folly::IPAddressV6>{this->kSid1},
          TunnelType::SRV6_ENCAP,
          std::string("srv6Tunnel0")));
      routeUpdater.addRoute(
          RouterID(0),
          folly::IPAddressV6("2800:3::"),
          this->kEncapRoutePrefixLen,
          ClientID::BGPD,
          RouteNextHopEntry(nhopsB, AdminDistance::EBGP));
      routeUpdater.program();
    }

    // Verify both routes encap correctly while sharing nhop(0)
    auto egressPort = this->getEgressPort(ecmpHelper.nhop(0).portDesc);
    this->verifyEncapPacket(
        {egressPort}, false /*ecnMarked*/, false /*isV4*/, {this->kSid0});
    this->verifyEncapPacket(
        {egressPort},
        false /*ecnMarked*/,
        false /*isV4*/,
        {this->kSid1},
        std::nullopt /*injectPort*/,
        folly::IPAddress("2800:3::1"));

    // Phase 2: Change route B to use nhop(1) instead of nhop(0)
    {
      auto routeUpdater = this->getSw()->getRouteUpdater();
      RouteNextHopSet nhopsBNew;
      nhopsBNew.insert(ResolvedNextHop(
          nhop1.ip,
          nhop1.intf,
          ECMP_WEIGHT,
          std::nullopt,
          std::nullopt,
          std::nullopt,
          std::nullopt,
          std::vector<folly::IPAddressV6>{this->kSid1},
          TunnelType::SRV6_ENCAP,
          std::string("srv6Tunnel0")));
      routeUpdater.addRoute(
          RouterID(0),
          folly::IPAddressV6("2800:3::"),
          this->kEncapRoutePrefixLen,
          ClientID::BGPD,
          RouteNextHopEntry(nhopsBNew, AdminDistance::EBGP));
      routeUpdater.program();
    }
  };

  // After warmboot, final state:
  //   Route A: kEncapRoutePrefix/64 -> nhop(0) with kSid0
  //   Route B: 2800:3::/64 -> nhop(1) with kSid1
  auto verify = [this]() {
    auto ecmpHelper = this->makeEcmpHelper();
    auto egressPort0 = this->getEgressPort(ecmpHelper.nhop(0).portDesc);
    auto egressPort1 = this->getEgressPort(ecmpHelper.nhop(1).portDesc);

    // Route A encaps with kSid0 via nhop(0)
    this->verifyEncapPacket(
        {egressPort0}, false /*ecnMarked*/, false /*isV4*/, {this->kSid0});

    // Route B encaps with kSid1 via nhop(1)
    this->verifyEncapPacket(
        {egressPort1},
        false /*ecnMarked*/,
        false /*isV4*/,
        {this->kSid1},
        std::nullopt /*injectPort*/,
        folly::IPAddress("2800:3::1"));
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(AgentSrv6EncapTest, verifySrv6EncapEcnMarking) {
  auto setup = [this]() {
    if constexpr (TestFixture::kIsTrunk) {
      this->applyConfigAndEnableTrunks(
          this->initialConfig(*this->getAgentEnsemble()));
    }
    this->resolveV4AndV6NextHops(2);
    this->template addEncapRoute<folly::CIDRNetworkV6>(
        {this->kEncapRoutePrefix, this->kEncapRoutePrefixLen}, {{this->kSid0}});
  };

  auto verify = [this]() {
    auto ecmpHelper = this->makeEcmpHelper();
    auto egressPort = this->getEgressPort(ecmpHelper.nhop(0).portDesc);
    auto intfMac =
        utility::getMacForFirstInterfaceWithPorts(this->getProgrammedState());

    // Send 512 SRv6 packets (DSCP 5, ECN ECT1, ~7000B payload).
    // In UNIFORM mode, outer header copies DSCP+ECN bits from inner.
    auto sendFloodPackets = [this, &intfMac]() {
      auto tcField = static_cast<uint8_t>((5 << 2) | this->kECT1);
      for (int i = 0; i < 512; ++i) {
        auto txPacket = utility::makeUDPTxPacket(
            this->getSw(),
            this->getVlanIDForTx(),
            intfMac,
            intfMac,
            folly::IPAddressV6("1::10"),
            this->kEncapRouteDstIp,
            8000,
            8001,
            tcField,
            64,
            std::vector<uint8_t>(7000, 0xff));
        this->getSw()->sendPacketSwitchedAsync(std::move(txPacket));
      }
    };

    // After encap, outer dst = SID, ECN CE = 0x3
    auto isEcnMarked = [this](
                           const folly::IPAddressV6& dstAddr, uint8_t ecnBits) {
      return dstAddr == this->kSid0 && ecnBits == 0x3;
    };

    utility::verifySrv6EcnMarking(
        this->getAgentEnsemble(), egressPort, sendFloodPackets, isEcnMarked);
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

// Verify that SRv6-encapsulated packets are placed in the correct egress
// queue based on the inner packet's DSCP. In UNIFORM mode, inner DSCP is
// copied to the outer header, so the egress queue is determined by the
// inner DSCP value.
TYPED_TEST(AgentSrv6EncapTest, VerifyDscpQueueMapping) {
  auto setup = [this]() {
    if constexpr (TestFixture::kIsTrunk) {
      this->applyConfigAndEnableTrunks(
          this->initialConfig(*this->getAgentEnsemble()));
    }
    this->resolveV4AndV6NextHops(this->kNumNextHops);
    this->template addEncapRoute<folly::CIDRNetworkV6>(
        {this->kEncapRoutePrefix, this->kEncapRoutePrefixLen}, {{this->kSid0}});
  };

  auto verify = [this]() {
    auto ecmpHelper = this->makeEcmpHelper();
    auto egressPort = this->getEgressPort(ecmpHelper.nhop(0).portDesc);
    auto injectPort = this->findInjectPort({egressPort});
    auto intfMac =
        utility::getMacForFirstInterfaceWithPorts(this->getProgrammedState());

    auto sendPacket = [this, &intfMac](int dscp, bool frontPanel, PortID port) {
      auto txPacket = utility::makeUDPTxPacket(
          this->getSw(),
          this->getVlanIDForTx(),
          intfMac,
          intfMac,
          folly::IPAddressV6("1::10"),
          this->kEncapRouteDstIp,
          8000,
          8001,
          dscp << 2,
          255);
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

} // namespace facebook::fboss
