// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

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
#include "fboss/agent/test/utils/PacketSnooper.h"
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

  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    if constexpr (kIsTrunk) {
      return {ProductionFeature::SRV6_ENCAP, ProductionFeature::LAG};
    }
    return {ProductionFeature::SRV6_ENCAP};
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
    addSrv6TunnelConfig(cfg);
    auto asic = checkSameAndGetAsic(ensemble.getL3Asics());
    utility::addTrapPacketAcl(
        asic,
        &cfg,
        {folly::CIDRNetwork{kSid0, 128},
         folly::CIDRNetwork{kSid1, 128},
         folly::CIDRNetwork{kSid2, 128}});
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
  void setupHelper(bool resolveNeighbors = true) {
    if constexpr (kIsTrunk) {
      applyConfigAndEnableTrunks(
          this->initialConfig(*this->getAgentEnsemble()));
    }
    if (resolveNeighbors) {
      resolveV4AndV6NextHops(2);
    }
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
      PortID egressPort,
      bool ecnMarked,
      bool isV4 = false,
      const std::vector<folly::IPAddressV6>& expectedSids = {kSid0},
      std::optional<PortID> injectPort = std::nullopt) {
    const auto& sids = expectedSids;

    auto portStatsBefore = this->getLatestPortStats(egressPort);
    auto bytesBefore = *portStatsBefore.outBytes_();

    auto intfMac =
        utility::getMacForFirstInterfaceWithPorts(this->getProgrammedState());
    constexpr auto kTc{42};
    constexpr auto kTtl{24};
    auto tcField = ecnMarked ? static_cast<uint8_t>((kTc << 2) | 0x3)
                             : static_cast<uint8_t>(kTc << 2);
    auto srcIp =
        isV4 ? folly::IPAddress("10.0.0.1") : folly::IPAddress("1::10");
    auto dstIp = isV4 ? folly::IPAddress("100.0.0.1") : kEncapRouteDstIp;
    auto txPacket = utility::makeUDPTxPacket(
        this->getSw(),
        this->getVlanIDForTx(),
        intfMac,
        intfMac,
        srcIp,
        dstIp,
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
      PortID egressPort,
      const std::vector<folly::IPAddressV6>& expectedSids = {kSid0}) {
    auto injectPort = findInjectPort(egressPort);
    for (bool isV4 : {false, true}) {
      // ECN not marked
      verifyEncapPacket(egressPort, false, isV4, expectedSids);
      verifyEncapPacket(egressPort, false, isV4, expectedSids, injectPort);
      // ECN marked
      verifyEncapPacket(egressPort, true, isV4, expectedSids);
      verifyEncapPacket(egressPort, true, isV4, expectedSids, injectPort);
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
    this->verifyEncapPacketCpuAndFrontPanel(egressPort);
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
    this->resolveV4AndV6NextHops(2);
  };

  auto verify = [this]() {
    auto ecmpHelper = this->makeEcmpHelper();
    auto egressPort = this->getEgressPort(ecmpHelper.nhop(0).portDesc);
    this->verifyEncapPacketCpuAndFrontPanel(egressPort);
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
    this->verifyEncapPacketCpuAndFrontPanel(egressPort);
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
