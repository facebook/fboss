// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/SwSwitchMySidUpdater.h"
#include "fboss/agent/SwSwitchRouteUpdateWrapper.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
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

  // Same prefixes as encap test but used for regular forwarding after decap
  const folly::IPAddressV6 kV6RoutePrefix{"2800:2::"};
  static constexpr uint8_t kV6RoutePrefixLen{64};

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
};

TYPED_TEST_SUITE(AgentSrv6DecapTest, Srv6DecapPortTypes);

TYPED_TEST(AgentSrv6DecapTest, sendPacketForDecap) {
  auto setup = [this]() { this->setupHelper(); };

  auto verify = []() {
    // TODO: Add decap packet verification
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
