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

  const folly::IPAddressV6 kMySidPrefix{"fdad:ffff:1::"};
  static constexpr uint8_t kMySidPrefixLen{48};
  static constexpr int kNumNextHops{4};

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

  void sendBindingSidPacketAndVerify(
      const folly::IPAddressV6& mySidDst,
      const std::vector<PortID>& egressPorts,
      const std::vector<folly::IPAddressV6>& expectedSids) {
    std::map<PortID, int64_t> bytesBefore;
    for (auto port : egressPorts) {
      bytesBefore[port] = *this->getLatestPortStats(port).outBytes_();
    }

    auto intfMac =
        getMacForFirstInterfaceWithPortsForTesting(this->getProgrammedState());

    auto txPacket = utility::makeIpInIpTxPacket(
        this->getSw(),
        this->getVlanIDForTx().value(),
        intfMac,
        intfMac,
        folly::IPAddressV6("100::1"),
        mySidDst,
        folly::IPAddressV6("2001:db8::1"),
        folly::IPAddressV6("2001:db8::2"),
        8000,
        8001,
        0,
        0,
        64,
        64);

    utility::SwSwitchPacketSnooper snooper(this->getSw(), "bindingSidSnooper");
    this->sendPacketSwitchedAsync(std::move(txPacket));

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

TYPED_TEST(AgentSrv6BindingSidTest, recursiveResolutionPreservesSidList) {
  auto setup = [this]() {
    this->setupHelper();

    auto ecmpHelper = this->makeEcmpHelper();
    auto routeUpdater = this->getSw()->getRouteUpdater();

    auto getNhopIp = [&ecmpHelper](int idx) {
      auto nhop = ecmpHelper.nhop(idx);
      if (nhop.linkLocalNhopIp.has_value()) {
        return folly::IPAddress(nhop.linkLocalNhopIp.value());
      }
      return folly::IPAddress(nhop.ip);
    };

    RouteNextHopSet openrNhopsA{
        ResolvedNextHop(getNhopIp(0), ecmpHelper.nhop(0).intf, ECMP_WEIGHT),
        ResolvedNextHop(getNhopIp(1), ecmpHelper.nhop(1).intf, ECMP_WEIGHT)};
    routeUpdater.addRoute(
        RouterID(0),
        folly::IPAddressV6("2901::"),
        48,
        ClientID::OPENR,
        RouteNextHopEntry(openrNhopsA, AdminDistance::OPENR));

    RouteNextHopSet openrNhopsB{
        ResolvedNextHop(getNhopIp(2), ecmpHelper.nhop(2).intf, ECMP_WEIGHT),
        ResolvedNextHop(getNhopIp(3), ecmpHelper.nhop(3).intf, ECMP_WEIGHT)};
    routeUpdater.addRoute(
        RouterID(0),
        folly::IPAddressV6("2902::"),
        48,
        ClientID::OPENR,
        RouteNextHopEntry(openrNhopsB, AdminDistance::OPENR));
    routeUpdater.program();

    this->addBindingSidEntry(
        folly::IPAddressV6("fc00:100:1::"),
        {this->makeSrv6NextHopThrift(
             folly::IPAddressV6("2901::1"), this->kSid0),
         this->makeSrv6NextHopThrift(
             folly::IPAddressV6("2902::1"), this->kSid1)});
  };

  auto verify = [this]() {
    auto ecmpHelper = this->makeEcmpHelper();
    std::vector<PortID> egressPorts;
    egressPorts.reserve(this->kNumNextHops);
    for (int i = 0; i < this->kNumNextHops; ++i) {
      egressPorts.push_back(this->getEgressPort(ecmpHelper.nhop(i).portDesc));
    }
    this->sendBindingSidPacketAndVerify(
        folly::IPAddressV6("fc00:100:1::"),
        egressPorts,
        {this->kSid0, this->kSid1});
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(AgentSrv6BindingSidTest, singleLinkLocalSrv6NextHop) {
  auto setup = [this]() {
    this->setupHelper();

    auto ecmpHelper = this->makeEcmpHelper();
    auto nhop = ecmpHelper.nhop(0);
    auto nhopIp = nhop.linkLocalNhopIp.has_value()
        ? folly::IPAddressV6(nhop.linkLocalNhopIp.value().str())
        : nhop.ip;

    auto srv6Nhop = this->makeSrv6NextHopThrift(nhopIp, this->kSid0);
    srv6Nhop.address()->ifName() =
        folly::to<std::string>("fboss", static_cast<int>(nhop.intf));
    this->addBindingSidEntry(folly::IPAddressV6("fc00:100:1::"), {srv6Nhop});
  };

  auto verify = [this]() {
    auto ecmpHelper = this->makeEcmpHelper();
    std::vector<PortID> egressPorts{
        this->getEgressPort(ecmpHelper.nhop(0).portDesc)};
    this->sendBindingSidPacketAndVerify(
        folly::IPAddressV6("fc00:100:1::"), egressPorts, {this->kSid0});
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
