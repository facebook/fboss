// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/AsicUtils.h"
#include "fboss/agent/SwSwitchRouteUpdateWrapper.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/packet/Ethertype.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/packet/Srv6Packet.h"
#include "fboss/agent/state/AggregatePort.h"
#include "fboss/agent/state/RouteNextHop.h"
#include "fboss/agent/state/Srv6Tunnel.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/TrunkUtils.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/PacketSnooper.h"
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

  const folly::IPAddressV6 kSid0{"3001:db8:1:2:3::"};
  const folly::IPAddressV6 kSid1{"3001:db8:4:5:6::"};
  const folly::IPAddressV6 kSid2{"3001:db8:7:8:9::"};

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

  utility::EcmpSetupAnyNPorts6 makeEcmpHelper() {
    return utility::EcmpSetupAnyNPorts6(
        this->getProgrammedState(), this->getSw()->needL2EntryForNeighbor());
  }

  void setupHelper(bool resolveNeighbors = true) {
    if constexpr (kIsTrunk) {
      applyConfigAndEnableTrunks(
          this->initialConfig(*this->getAgentEnsemble()));
    }
    if (resolveNeighbors) {
      auto ecmpHelper = makeEcmpHelper();
      this->resolveNeighbors(ecmpHelper, 2);
    }
  }

  void addEncapRoute(
      const folly::CIDRNetworkV6& prefix,
      const std::vector<std::vector<folly::IPAddressV6>>& sidLists) {
    auto ecmpHelper = makeEcmpHelper();
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

  void verifyEncapPacket(PortID egressPort) {
    auto portStatsBefore = this->getLatestPortStats(egressPort);
    auto bytesBefore = *portStatsBefore.outBytes_();

    auto intfMac =
        utility::getMacForFirstInterfaceWithPorts(this->getProgrammedState());
    auto txPacket = utility::makeUDPTxPacket(
        this->getSw(),
        this->getVlanIDForTx(),
        intfMac,
        intfMac,
        folly::IPAddressV6("1::10"),
        folly::IPAddressV6("2800:2::1"),
        8000,
        8001);

    auto packetComparator = [](const utility::EthFrame& /*expectedFrame*/,
                               const utility::EthFrame& receivedFrame) {
      auto ethHdr = receivedFrame.header();
      if (ethHdr.etherType !=
          static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV6)) {
        return false;
      }
      auto ioBuf = receivedFrame.toIOBuf();
      // Skip the ethernet header to get to the IPv6/SRv6 payload
      folly::io::Cursor cursor(ioBuf.get());
      cursor += ethHdr.size();
      utility::Srv6Packet srv6Pkt(cursor);
      XLOG(DBG2) << "Received SRv6 packet: " << srv6Pkt.toString();
      return true;
    };

    utility::SwSwitchPacketSnooper snooper(
        this->getSw(),
        "srv6EncapSnooper",
        std::nullopt,
        std::nullopt,
        packetComparator);

    this->getSw()->sendPacketSwitchedAsync(std::move(txPacket));

    WITH_RETRIES({
      auto frameRx = snooper.waitForPacket(1);
      EXPECT_EVENTUALLY_TRUE(frameRx.has_value());
    });

    WITH_RETRIES({
      auto portStatsAfter = this->getLatestPortStats(egressPort);
      auto bytesAfter = *portStatsAfter.outBytes_();
      EXPECT_EVENTUALLY_GT(bytesAfter, bytesBefore);
    });
  }

 private:
  cfg::Srv6Tunnel makeSrv6TunnelConfig(
      const std::string& name,
      InterfaceID interfaceId) const {
    cfg::Srv6Tunnel tunnel;
    tunnel.srv6TunnelId() = name;
    tunnel.underlayIntfID() = interfaceId;
    tunnel.tunnelType() = TunnelType::SRV6_ENCAP;
    tunnel.srcIp() = "2001:db8::1";
    tunnel.ttlMode() = cfg::TunnelMode::PIPE;
    tunnel.dscpMode() = cfg::TunnelMode::UNIFORM;
    tunnel.ecnMode() = cfg::TunnelMode::UNIFORM;
    return tunnel;
  }

  void addSrv6TunnelConfig(cfg::SwitchConfig& cfg) const {
    std::vector<cfg::Srv6Tunnel> tunnelList;
    tunnelList.push_back(makeSrv6TunnelConfig(
        "srv6Tunnel0", InterfaceID(cfg.interfaces()[0].intfID().value())));
    cfg.srv6Tunnels() = tunnelList;
  }
};

TYPED_TEST_SUITE(AgentSrv6EncapTest, Srv6EncapPortTypes);

TYPED_TEST(AgentSrv6EncapTest, CreateSrv6Tunnel) {
  auto setup = [=, this]() { this->setupHelper(); };
  auto verify = [=, this]() {
    auto tunnel =
        this->getProgrammedState()->getSrv6Tunnels()->getNodeIf("srv6Tunnel0");
    ASSERT_NE(tunnel, nullptr);
    EXPECT_EQ(tunnel->getID(), "srv6Tunnel0");
    EXPECT_EQ(tunnel->getType(), TunnelType::SRV6_ENCAP);
    EXPECT_EQ(
        tunnel->getUnderlayIntfId(),
        InterfaceID(this->initialConfig(*this->getAgentEnsemble())
                        .interfaces()[0]
                        .intfID()
                        .value()));
    EXPECT_EQ(tunnel->getSrcIP(), folly::IPAddress("2001:db8::1"));
    EXPECT_EQ(tunnel->getDstIP(), std::nullopt);
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(AgentSrv6EncapTest, sendPacketToEncapRoute) {
  auto setup = [this]() {
    this->setupHelper();
    this->addEncapRoute({folly::IPAddressV6("2800:2::"), 64}, {{this->kSid0}});
  };

  auto verify = [this]() {
    auto ecmpHelper = this->makeEcmpHelper();
    auto egressPort = this->getEgressPort(ecmpHelper.nhop(0).portDesc);
    this->verifyEncapPacket(egressPort);
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(AgentSrv6EncapTest, sendPacketToEncapRouteAfterLinkFlap) {
  auto setup = [this]() {
    this->setupHelper();
    this->addEncapRoute({folly::IPAddressV6("2800:2::"), 64}, {{this->kSid0}});
    // Flap ports and re-resolve neighbors
    auto ecmpHelper = this->makeEcmpHelper();
    auto egressPort = this->getEgressPort(ecmpHelper.nhop(0).portDesc);
    this->bringDownPort(egressPort);
    this->bringUpPort(egressPort);
    this->resolveNeighbors(ecmpHelper, 2);
  };

  auto verify = [this]() {
    auto ecmpHelper = this->makeEcmpHelper();
    auto egressPort = this->getEgressPort(ecmpHelper.nhop(0).portDesc);
    this->verifyEncapPacket(egressPort);
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(AgentSrv6EncapTest, resolveNeighborsAfterRouteProgram) {
  auto setup = [this]() {
    this->setupHelper(false /*resolveNeighbors*/);
    // Program routes before resolving neighbors
    this->addEncapRoute({folly::IPAddressV6("2800:2::"), 64}, {{this->kSid0}});
    this->addEncapRoute(
        {folly::IPAddressV6("2800:3::"), 64}, {{this->kSid1}, {this->kSid2}});
    // Resolve 1 neighbor after route programming
    this->applyNewState([this](std::shared_ptr<SwitchState> in) {
      utility::EcmpSetupAnyNPorts6 ecmpHelper(
          in, this->getSw()->needL2EntryForNeighbor());
      return ecmpHelper.resolveNextHops(in, {ecmpHelper.nhop(0).portDesc});
    });
    this->addEncapRoute(
        {folly::IPAddressV6("2800:4::"), 64}, {{this->kSid1}, {this->kSid2}});
    // Resolve second neighbor after route programming
    this->applyNewState([this](std::shared_ptr<SwitchState> in) {
      utility::EcmpSetupAnyNPorts6 ecmpHelper(
          in, this->getSw()->needL2EntryForNeighbor());
      return ecmpHelper.resolveNextHops(
          in, {ecmpHelper.nhop(0).portDesc, ecmpHelper.nhop(1).portDesc});
    });
  };

  auto verify = [this]() {
    auto ecmpHelper = this->makeEcmpHelper();
    auto egressPort = this->getEgressPort(ecmpHelper.nhop(0).portDesc);
    this->verifyEncapPacket(egressPort);
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
