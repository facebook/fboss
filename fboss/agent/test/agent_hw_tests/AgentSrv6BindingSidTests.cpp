// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/AsicUtils.h"
#include "fboss/agent/SwSwitchRouteUpdateWrapper.h"
#include "fboss/agent/ThriftHandler.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/if/gen-cpp2/common_types.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/agent/packet/Ethertype.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/rib/RoutingInformationBase.h"
#include "fboss/agent/state/AggregatePort.h"
#include "fboss/agent/state/RouteNextHop.h"
#include "fboss/agent/state/Srv6Tunnel.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/agent/test/TrunkUtils.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/OlympicTestUtils.h"
#include "fboss/agent/test/utils/PacketSnooper.h"
#include "fboss/agent/test/utils/Srv6TestUtils.h"
#include "fboss/agent/test/utils/TrapPacketUtils.h"
#include "fboss/lib/CommonUtils.h"

namespace facebook::fboss {

struct PhysicalPortBindingSid {
  static constexpr bool isTrunk = false;
};
struct AggregatePortBindingSid {
  static constexpr bool isTrunk = true;
};
using Srv6BindingSidPortTypes =
    ::testing::Types<PhysicalPortBindingSid, AggregatePortBindingSid>;

template <typename PortType>
class AgentSrv6BindingSidTest : public AgentHwTest {
 protected:
  static constexpr bool kIsTrunk = PortType::isTrunk;

  static inline const folly::IPAddressV6 kSid0{"3001:db8:1:2:3:4:5:6"};
  static inline const folly::IPAddressV6 kSid1{"3001:db8:4:5:6::"};

  static inline const folly::IPAddressV6 kBgpRoute0{"2001::1"};
  static inline const folly::IPAddressV6 kBgpRoute1{"3001::1"};
  static inline const folly::IPAddressV6 kOpenrPrefix0{"fdad::1:0"};
  static inline const folly::IPAddressV6 kOpenrPrefix1{"fdad::2:0"};

  static inline const folly::IPAddressV6 kMySidPrefix{"fc00:100:1::"};
  static constexpr uint8_t kMySidPrefixLen{48};
  static constexpr int kNumNextHops{2};

  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    if constexpr (kIsTrunk) {
      return {ProductionFeature::SRV6_BINDING_SID, ProductionFeature::LAG};
    }
    return {ProductionFeature::SRV6_BINDING_SID};
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
    auto asic = checkSameAndGetAsicForTesting(ensemble.getL3Asics());
    std::set<folly::CIDRNetwork> trapPrefixes{
        folly::CIDRNetwork{kSid0, 128}, folly::CIDRNetwork{kSid1, 128}};
    utility::addTrapPacketAcl(asic, &cfg, trapPrefixes);
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

  void setupHelper(bool resolveNeighbors = true) {
    if constexpr (kIsTrunk) {
      applyConfigAndEnableTrunks(
          this->initialConfig(*this->getAgentEnsemble()));
    }
    if (resolveNeighbors) {
      resolveNextHops(kNumNextHops);
    }
    programRoutes();
  }

  void programRoutes() {
    auto ecmpHelper = makeEcmpHelper();
    auto routeUpdater = this->getSw()->getRouteUpdater();

    auto getNhopIp = [&ecmpHelper](int idx) {
      auto nhop = ecmpHelper.nhop(idx);
      if (nhop.linkLocalNhopIp.has_value()) {
        return folly::IPAddress(nhop.linkLocalNhopIp.value());
      }
      return folly::IPAddress(nhop.ip);
    };

    // OpenR route 0 (fdad::1:0/112) -> nhop(0) link-local
    RouteNextHopSet openrNhops0{
        ResolvedNextHop(getNhopIp(0), ecmpHelper.nhop(0).intf, ECMP_WEIGHT)};
    routeUpdater.addRoute(
        RouterID(0),
        kOpenrPrefix0,
        112,
        ClientID::OPENR,
        RouteNextHopEntry(openrNhops0, AdminDistance::OPENR));

    // OpenR route 1 (fdad::2:0/112) -> nhop(1) link-local
    RouteNextHopSet openrNhops1{
        ResolvedNextHop(getNhopIp(1), ecmpHelper.nhop(1).intf, ECMP_WEIGHT)};
    routeUpdater.addRoute(
        RouterID(0),
        kOpenrPrefix1,
        112,
        ClientID::OPENR,
        RouteNextHopEntry(openrNhops1, AdminDistance::OPENR));

    // BGP route 0 (2001::1/128) -> fdad::1:1 (resolves via OpenR route 0)
    routeUpdater.addRoute(
        RouterID(0),
        kBgpRoute0,
        128,
        ClientID::BGPD,
        RouteNextHopEntry(
            RouteNextHopSet{UnresolvedNextHop(
                folly::IPAddress(folly::IPAddressV6("fdad::1:1")),
                ECMP_WEIGHT)},
            AdminDistance::EBGP));

    // BGP route 1 (3001::1/128) -> fdad::2:1 (resolves via OpenR route 1)
    routeUpdater.addRoute(
        RouterID(0),
        kBgpRoute1,
        128,
        ClientID::BGPD,
        RouteNextHopEntry(
            RouteNextHopSet{UnresolvedNextHop(
                folly::IPAddress(folly::IPAddressV6("fdad::2:1")),
                ECMP_WEIGHT)},
            AdminDistance::EBGP));

    routeUpdater.program();
  }

  PortID getEgressPort(PortDescriptor portDesc) {
    if (portDesc.isPhysicalPort()) {
      return portDesc.phyPortID();
    }
    auto aggPort = this->getProgrammedState()->getAggregatePorts()->getNode(
        portDesc.aggPortID());
    return aggPort->sortedSubports().begin()->portID;
  }

  void addBindingSidEntry(
      const folly::IPAddressV6& mySidAddr,
      const std::vector<NextHopThrift>& nhops) {
    ThriftHandler handler(this->getSw());
    auto entries = std::make_unique<std::vector<MySidEntry>>();
    MySidEntry bindingEntry;
    bindingEntry.type() = MySidType::BINDING_MICRO_SID;
    facebook::network::thrift::IPPrefix mySidPrefix;
    mySidPrefix.prefixAddress() = facebook::network::toBinaryAddress(mySidAddr);
    mySidPrefix.prefixLength() = kMySidPrefixLen;
    bindingEntry.mySid() = mySidPrefix;
    bindingEntry.nextHops() = nhops;
    entries->push_back(bindingEntry);
    handler.addMySidEntries(std::move(entries));
  }

  NextHopThrift makeSrv6NextHopThrift(
      const folly::IPAddressV6& nhopAddr,
      const folly::IPAddressV6& sid) {
    NextHopThrift nhop;
    nhop.address() = facebook::network::toBinaryAddress(nhopAddr);
    nhop.srv6SegmentList() = {facebook::network::toBinaryAddress(sid)};
    nhop.tunnelType() = TunnelType::SRV6_ENCAP;
    nhop.tunnelId() = "srv6Tunnel0";
    return nhop;
  }

  PortID findInjectPort(const std::vector<PortID>& egressPorts) {
    for (const auto& portMap :
         std::as_const(*this->getProgrammedState()->getPorts())) {
      for (const auto& [_, port] : std::as_const(*portMap.second)) {
        if (std::find(egressPorts.begin(), egressPorts.end(), port->getID()) ==
                egressPorts.end() &&
            port->isPortUp()) {
          return port->getID();
        }
      }
    }
    throw FbossError("No UP port found besides egress ports");
  }

  static constexpr uint8_t kDscp{42};
  static constexpr uint8_t kHopLimit{24};

  utility::EthFrame sendBindingSidPacket(
      bool ecnMarked,
      bool isV4,
      std::optional<PortID> injectPort = std::nullopt) {
    auto intfMac =
        getMacForFirstInterfaceWithPortsForTesting(this->getProgrammedState());

    auto outerTc = ecnMarked ? static_cast<uint8_t>((kDscp << 2) | 0x3)
                             : static_cast<uint8_t>(kDscp << 2);

    std::unique_ptr<TxPacket> txPacket;
    if (isV4) {
      txPacket = utility::makeIpInIpTxPacket(
          this->getSw(),
          this->getVlanIDForTx().value(),
          intfMac,
          intfMac,
          folly::IPAddressV6("100::1"),
          kMySidPrefix,
          folly::IPAddressV4("10.0.0.1"),
          folly::IPAddressV4("10.0.0.2"),
          8000,
          8001,
          outerTc,
          0,
          kHopLimit,
          64);
    } else {
      txPacket = utility::makeIpInIpTxPacket(
          this->getSw(),
          this->getVlanIDForTx().value(),
          intfMac,
          intfMac,
          folly::IPAddressV6("100::1"),
          kMySidPrefix,
          folly::IPAddressV6("2001:db8::1"),
          folly::IPAddressV6("2001:db8::2"),
          8000,
          8001,
          outerTc,
          0,
          kHopLimit,
          64);
    }

    auto originalFrame = utility::makeEthFrame(*txPacket);
    if (injectPort.has_value()) {
      XLOG(DBG2) << "sendBindingSidPacket: inner=" << (isV4 ? "v4" : "v6")
                 << " ecn=" << (ecnMarked ? "marked" : "unmarked")
                 << " front-panel inject on port " << injectPort.value();
      this->getSw()->sendPacketOutOfPortAsync(
          std::move(txPacket), injectPort.value());
    } else {
      XLOG(DBG2) << "sendBindingSidPacket: inner=" << (isV4 ? "v4" : "v6")
                 << " ecn=" << (ecnMarked ? "marked" : "unmarked")
                 << " CPU inject";
      this->sendPacketSwitchedAsync(std::move(txPacket));
    }
    return originalFrame;
  }

  void assertBindingSidForwarding(
      const std::vector<PortID>& egressPorts,
      const std::vector<folly::IPAddressV6>& expectedSids,
      bool ecnMarked,
      bool isV4,
      std::optional<PortID> injectPort = std::nullopt) {
    std::map<PortID, int64_t> bytesBefore;
    for (auto port : egressPorts) {
      bytesBefore[port] = *this->getLatestPortStats(port).outBytes_();
    }

    utility::SwSwitchPacketSnooper snooper(this->getSw(), "bindingSidSnooper");
    auto originalFrame = sendBindingSidPacket(ecnMarked, isV4, injectPort);

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
    auto v6Payload = frame.v6PayLoad();
    ASSERT_TRUE(v6Payload.has_value());
    auto v6Hdr = v6Payload->header();

    bool sidMatch = false;
    for (const auto& sid : expectedSids) {
      if (v6Hdr.dstAddr == sid) {
        sidMatch = true;
        break;
      }
    }
    EXPECT_TRUE(sidMatch) << "Outer DA " << v6Hdr.dstAddr
                          << " does not match any expected SID";

    EXPECT_EQ(v6Hdr.hopLimit, kHopLimit - 1);
    EXPECT_EQ(v6Hdr.trafficClass >> 2, kDscp);
    EXPECT_EQ(v6Hdr.trafficClass & 0x3, ecnMarked ? 0x3 : 0);

    auto origOuterV6 = originalFrame.v6PayLoad();
    ASSERT_TRUE(origOuterV6.has_value());
    if (isV4) {
      auto expectedInnerV4 = origOuterV6->v4PayLoad();
      auto rxInnerV4 = v6Payload->v4PayLoad();
      ASSERT_NE(expectedInnerV4, nullptr);
      ASSERT_NE(rxInnerV4, nullptr);
      // Uncomment after vendor bug for inner TC is fixed
      // EXPECT_EQ(*rxInnerV4, *expectedInnerV4);
    } else {
      auto expectedInnerV6 = origOuterV6->v6PayLoad();
      auto rxInnerV6 = v6Payload->v6PayLoad();
      ASSERT_NE(expectedInnerV6, nullptr);
      ASSERT_NE(rxInnerV6, nullptr);
      // Uncomment after vendor bug for inner TC is fixed
      // EXPECT_EQ(*rxInnerV6, *expectedInnerV6);
    }
  }

  void verifyBindingSidCpuAndFrontPanel(
      const std::vector<PortID>& egressPorts,
      const std::vector<folly::IPAddressV6>& expectedSids) {
    auto injectPort = findInjectPort(egressPorts);
    for (bool isV4 : {false, true}) {
      // ECN not marked
      assertBindingSidForwarding(egressPorts, expectedSids, false, isV4);
      assertBindingSidForwarding(
          egressPorts, expectedSids, false, isV4, injectPort);
      // ECN marked
      assertBindingSidForwarding(egressPorts, expectedSids, true, isV4);
      assertBindingSidForwarding(
          egressPorts, expectedSids, true, isV4, injectPort);
    }
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

TYPED_TEST_SUITE(AgentSrv6BindingSidTest, Srv6BindingSidPortTypes);

TYPED_TEST(AgentSrv6BindingSidTest, multipleNextHops) {
  auto setup = [this]() {
    this->setupHelper();
    this->addBindingSidEntry(
        this->kMySidPrefix,
        {this->makeSrv6NextHopThrift(this->kBgpRoute0, this->kSid0),
         this->makeSrv6NextHopThrift(this->kBgpRoute1, this->kSid1)});
  };

  auto verify = [this]() {
    auto ecmpHelper = this->makeEcmpHelper();
    std::vector<PortID> egressPorts{
        this->getEgressPort(ecmpHelper.nhop(0).portDesc),
        this->getEgressPort(ecmpHelper.nhop(1).portDesc)};
    this->verifyBindingSidCpuAndFrontPanel(
        egressPorts, {this->kSid0, this->kSid1});
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(AgentSrv6BindingSidTest, singleNextHop) {
  auto setup = [this]() {
    this->setupHelper();
    this->addBindingSidEntry(
        this->kMySidPrefix,
        {this->makeSrv6NextHopThrift(this->kBgpRoute0, this->kSid0)});
  };

  auto verify = [this]() {
    auto ecmpHelper = this->makeEcmpHelper();
    std::vector<PortID> egressPorts{
        this->getEgressPort(ecmpHelper.nhop(0).portDesc)};
    this->verifyBindingSidCpuAndFrontPanel(egressPorts, {this->kSid0});
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
