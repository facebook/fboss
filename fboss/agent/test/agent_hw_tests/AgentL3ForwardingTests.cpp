// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/ThriftHandler.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/state/StateUtils.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/lib/CommonUtils.h"

#include "fboss/agent/test/gen-cpp2/production_features_types.h"

DECLARE_bool(intf_nbr_tables);
namespace facebook::fboss {

class AgentL3ForwardingTest : public AgentHwTest {
 public:
  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    return {production_features::ProductionFeature::L3_FORWARDING};
  }

 protected:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto config = AgentHwTest::initialConfig(ensemble);
    return addCoppConfig(ensemble, config);
  }
  std::optional<VlanID> kVlanID() const {
    return utility::firstVlanID(getProgrammedState());
  }
  InterfaceID kIntfID() const {
    return utility::firstInterfaceID(getProgrammedState());
  }

  folly::MacAddress kNeighborMac() const {
    // Random mac address
    return folly::MacAddress("0c:42:a1:66:1d:9e");
  }
  template <typename IPAddrT>
  auto getNeighborTable(const std::shared_ptr<SwitchState>& in) {
    auto state = in;
    ;
    if (FLAGS_intf_nbr_tables) {
      return state->getInterfaces()
          ->getNode(kIntfID())
          ->template getNeighborEntryTable<IPAddrT>()
          ->modify(kIntfID(), &state);
    } else {
      return state->getVlans()
          ->getNode(*kVlanID())
          ->template getNeighborEntryTable<IPAddrT>()
          ->modify(*kVlanID(), &state);
    }
  }
  template <typename IPAddrT>
  std::shared_ptr<SwitchState> addResolvedNeighbor(
      const std::shared_ptr<SwitchState>& in,
      const IPAddrT& ip) {
    auto outState{in->clone()};
    auto neighborTable = getNeighborTable<IPAddrT>(outState);
    neighborTable->addPendingEntry(ip, kIntfID());
    auto ports = getPortsForInterface(kIntfID(), outState);
    CHECK(ports.size());
    neighborTable->updateEntry(
        ip,
        kNeighborMac(),
        PortDescriptor(*ports.begin()),
        kIntfID(),
        NeighborState::REACHABLE);
    return outState;
  }
  template <typename IPAddrT>
  std::shared_ptr<SwitchState> removeNeighbor(
      const std::shared_ptr<SwitchState>& inState,
      const IPAddrT& ip) {
    auto outState{inState->clone()};
    auto neighborTable = getNeighborTable<IPAddrT>(outState);
    neighborTable->removeEntry(ip);
    return outState;
  }

  void verifyHwAgentConnectionState(ThriftHandler& handler) {
    std::map<int16_t, HwAgentEventSyncStatus> statusMap;
    if (!getSw()->isRunModeMultiSwitch()) {
      return;
    }
    handler.getHwAgentConnectionStatus(statusMap);
    WITH_RETRIES({
      statusMap.clear();
      handler.getHwAgentConnectionStatus(statusMap);
      EXPECT_EVENTUALLY_GE(statusMap.size(), 1);
      EXPECT_EVENTUALLY_EQ(statusMap[0].statsEventSyncActive().value(), 1);
      EXPECT_EVENTUALLY_EQ(statusMap[0].fdbEventSyncActive().value(), 1);
      EXPECT_EVENTUALLY_EQ(statusMap[0].linkEventSyncActive().value(), 1);
      EXPECT_EVENTUALLY_EQ(statusMap[0].rxPktEventSyncActive().value(), 1);
      EXPECT_EVENTUALLY_EQ(statusMap[0].txPktEventSyncActive().value(), 1);
      EXPECT_EVENTUALLY_EQ(
          statusMap[0].switchReachabilityChangeEventSyncActive().value(), 1);
    });
  }
};

TEST_F(AgentL3ForwardingTest, linkLocalNeighborAndNextHop) {
  auto setup = [=, this]() {
    // Random LL IPs
    // linkLocalNhop - used as both LL nbr and nhop
    // linkLocalNbr  - used as both LL nbr and
    folly::IPAddress linkLocalNhop("fe80::e42:a1ff:fe66:1d9e");
    folly::IPAddress linkLocalNbr("fe80::e43:a1ff:fe66:1d9e");
    applyNewState(
        [&](const std::shared_ptr<SwitchState>& in) {
          return addResolvedNeighbor(in, linkLocalNhop.asV6());
        },
        "add link local nbr, nhop");
    applyNewState(
        [&](const std::shared_ptr<SwitchState>& in) {
          return addResolvedNeighbor(in, linkLocalNbr.asV6());
        },
        "add link local nbr, nhop");
    RouteNextHopSet nhops;
    nhops.emplace(ResolvedNextHop(linkLocalNhop, kIntfID(), ECMP_WEIGHT));
    auto routeUpdater = getSw()->getRouteUpdater();
    routeUpdater.addRoute(
        RouterID(0),
        folly::IPAddressV6("2803:1::"),
        64,
        ClientID::BGPD,
        RouteNextHopEntry(nhops, AdminDistance::EBGP));
    routeUpdater.program();
  };
  // No verify here. We are just testing safe warm boots
  // with LL neighbors and next hops
  verifyAcrossWarmBoots(setup, []() {});
}

TEST_F(AgentL3ForwardingTest, ttl255) {
  utility::EcmpSetupAnyNPorts6 ecmpHelper6(getSw()->getState());
  utility::EcmpSetupAnyNPorts4 ecmpHelper4(getSw()->getState());
  auto setup = [=, this]() {
    auto programDefaultRoutes = [this](auto& ecmpHelper) {
      auto wrapper = getSw()->getRouteUpdater();
      ecmpHelper.programRoutes(&wrapper, 1);
    };
    programDefaultRoutes(ecmpHelper6);
    programDefaultRoutes(ecmpHelper4);

    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      return ecmpHelper6.resolveNextHops(in, {ecmpHelper6.nhop(0).portDesc});
    });
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      return ecmpHelper4.resolveNextHops(in, {ecmpHelper4.nhop(0).portDesc});
    });
    CHECK_EQ(ecmpHelper4.nhop(0).portDesc, ecmpHelper6.nhop(0).portDesc);
  };
  auto verify = [=, this]() {
    ThriftHandler handler(getSw());
    verifyHwAgentConnectionState(handler);
    auto pumpTraffic = [=, this]() {
      for (auto isV6 : {true, false}) {
        auto vlanId = utility::firstVlanID(getProgrammedState());
        auto intfMac = utility::getFirstInterfaceMac(getProgrammedState());
        auto srcIp = folly::IPAddress(isV6 ? "1001::1" : "10.0.0.1");
        auto dstIp =
            folly::IPAddress(isV6 ? "100:100:100::1" : "100.100.100.1");
        constexpr uint8_t kTtl = 255;
        auto pkt = utility::makeUDPTxPacket(
            getSw(),
            vlanId,
            intfMac,
            intfMac,
            srcIp,
            dstIp,
            10000,
            10001,
            0,
            kTtl);
        getSw()->sendPacketOutOfPortAsync(
            std::move(pkt), ecmpHelper6.nhop(1).portDesc.phyPortID());
      }
    };
    auto port = ecmpHelper6.nhop(0).portDesc.phyPortID();
    auto portStatsBefore = getLatestPortStats(port);
    pumpTraffic();
    WITH_RETRIES({
      auto portStatsAfter = getLatestPortStats(port);
      XLOG(INFO) << " Out pkts, before:" << *portStatsBefore.outUnicastPkts_()
                 << " after:" << *portStatsAfter.outUnicastPkts_() << std::endl;
      EXPECT_EVENTUALLY_EQ(
          *portStatsAfter.outUnicastPkts_(),
          *portStatsBefore.outUnicastPkts_() + 2);
    });
    handler.clearAllPortStats();
    WITH_RETRIES({
      auto portStatsAfter = getLatestPortStats(port);
      XLOG(INFO) << " Out pkts, after:" << *portStatsAfter.outUnicastPkts_()
                 << std::endl;
      EXPECT_EVENTUALLY_EQ(*portStatsAfter.outUnicastPkts_(), 0);
    });
  };
  verifyAcrossWarmBoots(setup, verify);
}
} // namespace facebook::fboss
