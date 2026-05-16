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

    // OpenR route A (2901::/48) -> nhop(0), nhop(1) with link-local nexthops
    RouteNextHopSet openrNhopsA{
        ResolvedNextHop(getNhopIp(0), ecmpHelper.nhop(0).intf, ECMP_WEIGHT),
        ResolvedNextHop(getNhopIp(1), ecmpHelper.nhop(1).intf, ECMP_WEIGHT)};
    routeUpdater.addRoute(
        RouterID(0),
        folly::IPAddressV6("2901::"),
        48,
        ClientID::OPENR,
        RouteNextHopEntry(openrNhopsA, AdminDistance::OPENR));

    // OpenR route B (2902::/48) -> nhop(2), nhop(3) with link-local nexthops
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

    // Add a BINDING_MICRO_SID via ThriftHandler with 2 nexthops carrying
    // SID lists. Each nexthop resolves recursively over one of the OpenR
    // routes above.
    ThriftHandler handler(this->getSw());
    auto entries = std::make_unique<std::vector<MySidEntry>>();
    MySidEntry bindingEntry;
    bindingEntry.type() = MySidType::BINDING_MICRO_SID;
    facebook::network::thrift::IPPrefix mySidPrefix;
    mySidPrefix.prefixAddress() =
        facebook::network::toBinaryAddress(folly::IPAddressV6("fc00:100::1"));
    mySidPrefix.prefixLength() = 48;
    bindingEntry.mySid() = mySidPrefix;

    NextHopThrift nhop0;
    nhop0.address() =
        facebook::network::toBinaryAddress(folly::IPAddressV6("2901::1"));
    nhop0.srv6SegmentList() = {facebook::network::toBinaryAddress(this->kSid0)};
    nhop0.tunnelType() = TunnelType::SRV6_ENCAP;
    nhop0.tunnelId() = "srv6Tunnel0";

    NextHopThrift nhop1;
    nhop1.address() =
        facebook::network::toBinaryAddress(folly::IPAddressV6("2902::1"));
    nhop1.srv6SegmentList() = {facebook::network::toBinaryAddress(this->kSid1)};
    nhop1.tunnelType() = TunnelType::SRV6_ENCAP;
    nhop1.tunnelId() = "srv6Tunnel0";

    bindingEntry.nextHops() = {nhop0, nhop1};
    entries->push_back(bindingEntry);
    handler.addMySidEntries(std::move(entries));
  };

  auto verify = [this]() {
    auto ecmpHelper = this->makeEcmpHelper();
    std::vector<PortID> egressPorts;
    egressPorts.reserve(this->kNumNextHops);
    for (int i = 0; i < this->kNumNextHops; ++i) {
      egressPorts.push_back(this->getEgressPort(ecmpHelper.nhop(i).portDesc));
    }

    std::map<PortID, int64_t> bytesBefore;
    for (auto port : egressPorts) {
      bytesBefore[port] = *this->getLatestPortStats(port).outBytes_();
    }

    auto intfMac =
        getMacForFirstInterfaceWithPortsForTesting(this->getProgrammedState());

    // Send an IP-in-IP packet with outer dst matching the binding SID prefix.
    auto txPacket = utility::makeIpInIpTxPacket(
        this->getSw(),
        this->getVlanIDForTx().value(),
        intfMac,
        intfMac,
        folly::IPAddressV6("100::1"),
        folly::IPAddressV6("fc00:100::1"),
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
    bool sidMatch =
        v6Hdr.dstAddr == this->kSid0 || v6Hdr.dstAddr == this->kSid1;
    EXPECT_TRUE(sidMatch) << "Outer DA " << v6Hdr.dstAddr
                          << " does not match kSid0 or kSid1";
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
