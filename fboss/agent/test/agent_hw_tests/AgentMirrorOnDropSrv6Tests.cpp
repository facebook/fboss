// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/agent_hw_tests/AgentMirrorOnDropStatelessTest.h"

#include <gtest/gtest.h>

#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/AsicUtils.h"
#include "fboss/agent/SwSwitchRouteUpdateWrapper.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/packet/PktUtil.h"
#include "fboss/agent/state/RouteNextHop.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/agent/test/utils/PacketSnooper.h"
#include "fboss/agent/test/utils/Srv6TestUtils.h"
#include "fboss/agent/test/utils/TrapPacketUtils.h"
#include "fboss/lib/CommonUtils.h"

namespace facebook::fboss {

class AgentMirrorOnDropSrv6Test : public AgentMirrorOnDropStatelessTest {
 protected:
  static inline const folly::IPAddressV6 kSrv6OuterSrcIp{"100::1"};

  static inline const folly::IPAddressV6 kMidpointMySidPrefix{"fdad:ffff:1::"};
  static constexpr uint8_t kMidpointMySidPrefixLen{48};

  static inline const folly::IPAddressV6 kDecapMySidAddr{"3001:db8:7fff::"};
  static constexpr uint8_t kDecapMySidPrefixLen{48};

  static inline const folly::IPAddressV6 kBindingSidPrefix{"fc00:100:1::"};
  static constexpr uint8_t kBindingSidPrefixLen{48};
  static inline const folly::IPAddressV6 kBindingSidSid0{
      "3001:db8:1:2:3:4:5:6"};
  static inline const folly::IPAddressV6 kBgpRoute0{"2001::1"};
  static inline const folly::IPAddressV6 kOpenrPrefix0{"fdad::1:0"};

  void setCmdLineFlagOverrides() const override {
    AgentMirrorOnDropStatelessTest::setCmdLineFlagOverrides();
    FLAGS_enable_nexthop_id_manager = true;
    FLAGS_resolve_nexthops_from_id = true;
  }

  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {
        ProductionFeature::MIRROR_ON_DROP,
        ProductionFeature::MIRROR_ON_DROP_STATELESS,
        ProductionFeature::SRV6_MIDPOINT,
        ProductionFeature::SRV6_DECAP,
        ProductionFeature::SRV6_BINDING_SID};
  }

  void sendSrv6Packet(
      const PortID& injectPort,
      const folly::IPAddressV6& outerDst) {
    auto intfMac =
        getMacForFirstInterfaceWithPortsForTesting(getProgrammedState());
    auto txPacket = utility::makeIpInIpTxPacket(
        getSw(),
        getVlanIDForTx().value(),
        intfMac,
        intfMac,
        kSrv6OuterSrcIp,
        outerDst,
        folly::IPAddressV6("2001:db8::1"),
        folly::IPAddressV6("2001:db8::2"),
        8000,
        8001,
        0,
        0,
        24,
        64);
    getSw()->sendPacketOutOfPortAsync(std::move(txPacket), injectPort);
  }

  void setupModAndCollector(
      cfg::SwitchConfig& config,
      const PortID& collectorPortId) {
    config.mirrorOnDropReports()->push_back(
        makeMirrorOnDropReport("mod-srv6-test"));
    auto* asic =
        checkSameAndGetAsicForTesting(getAgentEnsemble()->getL3Asics());
    utility::addTrapPacketAcl(
        asic,
        &config,
        folly::CIDRNetwork(folly::IPAddress(kCollectorIp_), 128),
        cfg::ToCpuAction::TRAP);
    applyNewConfig(config);
    setupEcmpTraffic(collectorPortId, kCollectorIp_, kCollectorNextHopMac_);
    waitForStateUpdates(getSw());
  }

  void sendAndVerifyModPacket(
      const PortID& injectionPortId,
      const folly::IPAddressV6& outerDst,
      const MirrorOnDropDropReasonCodes& expectedReasons) {
    utility::SwSwitchPacketSnooper snooper(
        getSw(),
        "mod-srv6-snooper",
        std::nullopt,
        std::nullopt,
        std::nullopt,
        impl()->snooperReceivePacketType());
    snooper.ignoreUnclaimedRxPkts();

    sendSrv6Packet(injectionPortId, outerDst);

    WITH_RETRIES_N(5, {
      auto frameRx = snooper.waitForPacket(1);
      EXPECT_EVENTUALLY_TRUE(frameRx.has_value());
      if (frameRx.has_value()) {
        XLOG(INFO) << "Captured MirrorOnDrop packet for SRv6 drop:\n"
                   << PktUtil::hexDump(frameRx->get());
        validateMirrorOnDropPacket(
            frameRx->get(),
            injectionPortId,
            expectedReasons,
            outerDst,
            kSrv6OuterSrcIp);
      }
    });
  }

  void programBindingSidRoutes() {
    auto ecmpHelper = utility::EcmpSetupAnyNPorts<folly::IPAddressV6>(
        getProgrammedState(), getSw()->needL2EntryForNeighbor());
    auto routeUpdater = getSw()->getRouteUpdater();

    auto getNhopIp = [&ecmpHelper](int idx) {
      auto nhop = ecmpHelper.nhop(idx);
      if (nhop.linkLocalNhopIp.has_value()) {
        return folly::IPAddress(nhop.linkLocalNhopIp.value());
      }
      return folly::IPAddress(nhop.ip);
    };

    RouteNextHopSet openrNhops0{
        ResolvedNextHop(getNhopIp(0), ecmpHelper.nhop(0).intf, ECMP_WEIGHT)};
    routeUpdater.addRoute(
        RouterID(0),
        kOpenrPrefix0,
        112,
        ClientID::OPENR,
        RouteNextHopEntry(openrNhops0, AdminDistance::OPENR));

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

    routeUpdater.program();
  }
};

// Single test exercising all SRv6 drop scenarios with MOD.
// Setup installs midpoint, decap, and binding SID MySid entries with
// neighbors resolved. Verify exercises each drop sequentially, then
// unresolves the midpoint neighbor for the final scenario.
TEST_F(AgentMirrorOnDropSrv6Test, Srv6Drops) {
  PortID mySidPortId = masterLogicalInterfacePortIds()[0];
  PortID collectorPortId = masterLogicalInterfacePortIds()[1];
  PortID injectionPortId = masterLogicalInterfacePortIds()[2];

  const folly::IPAddressV6 kDecapNonLastDst{"3001:db8:7fff:1:2::"};
  const folly::IPAddressV6 kBindingSidNonLastDst{"fc00:100:1:2::"};
  const folly::IPAddressV6 kMidpointValidDst{"fdad:ffff:1:2::"};

  auto setup = [&]() {
    auto config = getAgentEnsemble()->getCurrentConfig();

    auto portName = utility::portNameForConfig(config, mySidPortId);
    config.mySidConfig() =
        utility::makeAdjacencyMySidConfig(portName, "fdad:ffff::/32", 1);

    config.srv6Tunnels() = {utility::makeSrv6TunnelConfig(
        "srv6Tunnel0", InterfaceID(config.interfaces()[0].intfID().value()))};

    setupModAndCollector(config, collectorPortId);

    auto ecmpHelper = utility::EcmpSetupAnyNPorts<folly::IPAddressV6>(
        getProgrammedState(),
        getSw()->needL2EntryForNeighbor(),
        getLocalMacAddress());
    PortDescriptor portDesc{mySidPortId};
    applyNewState(
        [&ecmpHelper, &portDesc](std::shared_ptr<SwitchState> in) {
          return ecmpHelper.resolveNextHops(
              in, {portDesc}, /*useLinkLocal=*/true);
        },
        "resolve mysid neighbor");
    utility::waitForMySidResolveOrUnresolve(
        [this]() { return getProgrammedState(); },
        kMidpointMySidPrefix,
        kMidpointMySidPrefixLen,
        /*resolved=*/true);

    auto ecmpHelper2 = utility::EcmpSetupAnyNPorts<folly::IPAddressV6>(
        getProgrammedState(), getSw()->needL2EntryForNeighbor());
    applyNewState(
        [&ecmpHelper2, collectorPortId](std::shared_ptr<SwitchState> in) {
          return ecmpHelper2.resolveNextHops(
              in,
              {PortDescriptor{collectorPortId}},
              /*useLinkLocal=*/true);
        },
        "resolve collector neighbor");

    utility::addDecapMySidEntry(getSw(), kDecapMySidAddr, kDecapMySidPrefixLen);

    programBindingSidRoutes();
    utility::addBindingSidEntry(
        getSw(),
        kBindingSidPrefix,
        kBindingSidPrefixLen,
        {utility::makeSrv6NextHopThrift(kBgpRoute0, kBindingSidSid0)});
  };

  auto verify = [&]() {
    XLOG(INFO) << "--- Midpoint is-last-SID drop ---";
    sendAndVerifyModPacket(
        injectionPortId,
        kMidpointMySidPrefix,
        getSrv6MidpointIsLastSidDropReason());

    XLOG(INFO) << "--- Decap non-last-segment drop ---";
    sendAndVerifyModPacket(
        injectionPortId,
        kDecapNonLastDst,
        getSrv6DecapNonLastSegmentDropReasons());

    XLOG(INFO) << "--- Binding SID non-last-SID drop ---";
    sendAndVerifyModPacket(
        injectionPortId,
        kBindingSidNonLastDst,
        getSrv6BindingSidNonLastSidDropReasons());

    XLOG(INFO) << "--- Midpoint unresolved neighbor drop ---";
    auto ecmpHelper = utility::EcmpSetupAnyNPorts<folly::IPAddressV6>(
        getProgrammedState(),
        getSw()->needL2EntryForNeighbor(),
        getLocalMacAddress());
    PortDescriptor portDesc{mySidPortId};
    applyNewState(
        [&ecmpHelper, &portDesc](std::shared_ptr<SwitchState> in) {
          return ecmpHelper.unresolveNextHops(
              in, {portDesc}, /*useLinkLocal=*/true);
        },
        "unresolve mysid neighbor");
    utility::waitForMySidResolveOrUnresolve(
        [this]() { return getProgrammedState(); },
        kMidpointMySidPrefix,
        kMidpointMySidPrefixLen,
        /*resolved=*/false);

    sendAndVerifyModPacket(
        injectionPortId,
        kMidpointValidDst,
        getSrv6MidpointUnresolvedDropReasons());

    // Re-resolve so the next warmboot iteration starts from the same state
    // as setup() established.
    auto ecmpHelper2 = utility::EcmpSetupAnyNPorts<folly::IPAddressV6>(
        getProgrammedState(),
        getSw()->needL2EntryForNeighbor(),
        getLocalMacAddress());
    applyNewState(
        [&ecmpHelper2, &portDesc](std::shared_ptr<SwitchState> in) {
          return ecmpHelper2.resolveNextHops(
              in, {portDesc}, /*useLinkLocal=*/true);
        },
        "re-resolve mysid neighbor");
    utility::waitForMySidResolveOrUnresolve(
        [this]() { return getProgrammedState(); },
        kMidpointMySidPrefix,
        kMidpointMySidPrefixLen,
        /*resolved=*/true);
  };

  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
