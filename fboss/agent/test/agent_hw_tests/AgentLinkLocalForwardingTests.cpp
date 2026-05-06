// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <folly/IPAddress.h>

#include "fboss/agent/TxPacket.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/state/RouteNextHop.h"
#include "fboss/agent/state/RouteNextHopEntry.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/agent/test/gen-cpp2/production_features_types.h"
#include "fboss/agent/test/utils/CoppTestUtils.h"
#include "fboss/agent/test/utils/LoadBalancerTestUtils.h"
#include "fboss/agent/test/utils/OlympicTestUtils.h"
#include "fboss/agent/test/utils/QueueTestUtils.h"

namespace facebook::fboss {

class AgentLinkLocalForwardingTest : public AgentHwTest {
 public:
  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {ProductionFeature::L3_FORWARDING};
  }

 protected:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto cfg = AgentHwTest::initialConfig(ensemble);
    utility::addOlympicQosMaps(cfg, ensemble.getL3Asics());
    utility::setDefaultCpuTrafficPolicyConfig(
        cfg, ensemble.getL3Asics(), ensemble.isSai());
    utility::addCpuQueueConfig(cfg, ensemble.getL3Asics(), ensemble.isSai());
    cfg.loadBalancers()->push_back(
        utility::getEcmpFullHashConfig(ensemble.getL3Asics()));
    return cfg;
  }

  std::unique_ptr<TxPacket> makeTxPacket(
      const folly::IPAddress& dstIp,
      uint16_t l4DstPort) const {
    auto vlanId = getVlanIDForTx();
    auto intfMac =
        getMacForFirstInterfaceWithPortsForTesting(getProgrammedState());
    auto srcMac = utility::MacAddressGenerator().get(intfMac.u64HBO() + 1);
    auto srcIp = folly::IPAddress("100::1");
    return utility::makeUDPTxPacket(
        getSw(),
        vlanId,
        srcMac,
        intfMac,
        srcIp,
        dstIp,
        8000 /* l4 src port */,
        l4DstPort);
  }
};

// Resolve a link-local NDP entry on a single port and verify that a packet
// with dst = the link-local neighbor address is forwarded out (not trapped to
// CPU). Uses a non-OpenR L4 port so the recently-added
// cpuPolicing-high-openr-linklocal-acl (which matches fe80::/10 + UDP/16818)
// cannot match.
TEST_F(AgentLinkLocalForwardingTest, SingleLinkLocalNeighbor) {
  auto setup = [this]() {
    auto portIds = masterLogicalInterfacePortIds();
    CHECK(!portIds.empty());
    utility::EcmpSetupTargetedPorts<folly::IPAddressV6> ecmpHelper(
        getProgrammedState(), getSw()->needL2EntryForNeighbor());
    boost::container::flat_set<PortDescriptor> ports{
        PortDescriptor(portIds[0])};
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      return ecmpHelper.resolveNextHops(in, ports, /*useLinkLocal=*/true);
    });
  };
  auto verify = [this]() {
    auto portIds = masterLogicalInterfacePortIds();
    utility::EcmpSetupTargetedPorts<folly::IPAddressV6> ecmpHelper(
        getProgrammedState(), getSw()->needL2EntryForNeighbor());
    // Read the dst from the same nhop the setup resolved against — guarantees
    // we send to the IP the helper installed in the NDP table, no matter what
    // convention the helper uses internally for link-local addresses.
    auto linkLocalDst =
        *ecmpHelper.nhop(PortDescriptor(portIds[0])).linkLocalNhopIp;
    getAgentEnsemble()->ensureSendPacketSwitched(
        makeTxPacket(folly::IPAddress(linkLocalDst), 8001 /* l4 dst port */));
  };
  verifyAcrossWarmBoots(setup, verify);
}

// Resolve link-local NDP entries on 4 ports, program an ECMP route to a
// global v6 prefix (2001::/64) whose 4 nexthops are the link-local
// neighbor address (one per port-interface), then pump randomized 5-tuple
// traffic destined to 2001::* and assert load is balanced across the 4
// ports within tolerance.
TEST_F(AgentLinkLocalForwardingTest, EcmpLinkLocalNexthopsLoadBalanced) {
  constexpr int kEcmpWidth = 4;
  constexpr int kMaxDeviationPct = 25;
  auto setup = [this]() {
    auto portIds = masterLogicalInterfacePortIds();
    CHECK_GE(portIds.size(), kEcmpWidth);
    utility::EcmpSetupTargetedPorts<folly::IPAddressV6> ecmpHelper(
        getProgrammedState(), getSw()->needL2EntryForNeighbor());
    boost::container::flat_set<PortDescriptor> ports;
    for (int i = 0; i < kEcmpWidth; ++i) {
      ports.insert(PortDescriptor(portIds[i]));
    }
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      return ecmpHelper.resolveNextHops(in, ports, /*useLinkLocal=*/true);
    });
    RouteNextHopEntry::NextHopSet nhops;
    for (const auto& portDesc : ports) {
      auto nhop = ecmpHelper.nhop(portDesc);
      nhops.insert(
          ResolvedNextHop(*nhop.linkLocalNhopIp, nhop.intf, ECMP_WEIGHT));
    }
    auto updater = getSw()->getRouteUpdater();
    updater.addRoute(
        RouterID(0),
        folly::IPAddressV6("2001::"),
        64,
        ClientID::BGPD,
        RouteNextHopEntry(nhops, AdminDistance::EBGP));
    updater.program();
  };
  auto verify = [this]() {
    std::vector<PortDescriptor> ecmpPorts;
    ecmpPorts.reserve(kEcmpWidth);
    auto portIds = masterLogicalInterfacePortIds();
    for (int i = 0; i < kEcmpWidth; ++i) {
      ecmpPorts.emplace_back(portIds[i]);
    }
    std::function<std::map<PortID, HwPortStats>(const std::vector<PortID>&)>
        getPortStatsFn = [this](const std::vector<PortID>& ids) {
          return getSw()->getHwPortStats(ids);
        };
    utility::pumpTrafficAndVerifyLoadBalanced(
        [this]() {
          utility::pumpTraffic(
              true /* isV6 */,
              utility::getAllocatePktFn(getSw()),
              utility::getSendPktFunc(getSw()),
              getMacForFirstInterfaceWithPortsForTesting(getProgrammedState()),
              getVlanIDForTx());
        },
        [this, &ecmpPorts]() {
          auto ports = std::make_unique<std::vector<int32_t>>();
          ports->reserve(ecmpPorts.size());
          for (const auto& p : ecmpPorts) {
            ports->push_back(static_cast<int32_t>(p.phyPortID()));
          }
          getSw()->clearPortStats(ports);
        },
        [&]() {
          return utility::isLoadBalanced(
              ecmpPorts,
              std::vector<NextHopWeight>(kEcmpWidth, 1),
              getPortStatsFn,
              kMaxDeviationPct);
        });
  };
  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
