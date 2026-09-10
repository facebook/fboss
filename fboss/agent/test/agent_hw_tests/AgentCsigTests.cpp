// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

// AgentCsigTest: one GTest fixture for all CSIG HW tests.
//   csigBasic*        — L3 routed CSIG (passthrough / strip / update)
//   srv6EncapCsig*    — SRv6 encap + CSIG (passthrough / strip / update)
//     passthrough: ingress CSIG copied to new outer L2 only; no CSIG in SRv6
//     payload strip: no CSIG on outer L2; no CSIG in SRv6 payload (telemetry
//     dropped) update: new egress CSIG on outer L2 only; no CSIG in SRv6
//     payload
//   srv6DecapCsig*    — SRv6 decap + CSIG (passthrough / strip / update)
//     passthrough: outer L2 CSIG copied onto decapped inner L2; tunnel
//     discarded strip: outer L2 CSIG thrown away; decapped inner exits with no
//     CSIG tags update: outer CSIG dropped; new egress CSIG injected on
//     decapped inner L2
//   srv6MidpointCsig* — SRv6 midpoint + CSIG (passthrough / strip / update)
//     passthrough: outer L2 CSIG passes through unaltered; outer IPv6 only
//     strip: outer L2 CSIG deleted; plain outer IPv6
//     update: outer L2 CSIG updated egress metrics; outer IPv6 only
//   srv6BsidCsig*     — SRv6 binding SID + CSIG (passthrough / strip / update)
//     passthrough: ingress CSIG copied into new BSid outer L2 header
//     strip: original outer L2 CSIG deleted; new BSid frame has no telemetry
//     update: new BSid outer L2 CSIG with egress metrics for BSid segment path
//   *UpdateTgenStyle — SDK test_csig_tgen_v4_v6 model (dual UPDATE, cross-port
//     flood, probe hop 64 / trap hop 63+64). Preferred over legacy UPDATE
//     tests.
// Ingress CSIG inject is always untagged: [dst|src|0x9900|CSIG|IP|...].

#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/AsicUtils.h"
#include "fboss/agent/SwSwitchRouteUpdateWrapper.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/if/gen-cpp2/common_types.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/agent/packet/EthHdr.h"
#include "fboss/agent/packet/Ethertype.h"
#include "fboss/agent/packet/IPPacket.h"
#include "fboss/agent/packet/IPProto.h"
#include "fboss/agent/packet/IPv6Hdr.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/packet/PktUtil.h"
#include "fboss/agent/rib/RoutingInformationBase.h"
#include "fboss/agent/state/AggregatePort.h"
#include "fboss/agent/state/RouteNextHop.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/agent/test/TrunkUtils.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/CoppTestUtils.h"
#include "fboss/agent/test/utils/CsigTestUtils.h"
#include "fboss/agent/test/utils/LoadBalancerTestUtils.h"
#include "fboss/agent/test/utils/MirrorTestUtils.h"
#include "fboss/agent/test/utils/OlympicTestUtils.h"
#include "fboss/agent/test/utils/PacketSnooper.h"
#include "fboss/agent/test/utils/PortTestUtils.h"
#include "fboss/agent/test/utils/QosTestUtils.h"
#include "fboss/agent/test/utils/Srv6TestUtils.h"
#include "fboss/agent/test/utils/TrapPacketUtils.h"
#include "fboss/lib/CommonUtils.h"

#include <folly/Format.h>
#include <folly/io/IOBuf.h>
#include <folly/logging/xlog.h>

namespace facebook::fboss {

class AgentCsigTest : public AgentHwTest {
 protected:
  // All 6 uSids populated
  static inline const folly::IPAddressV6 kSid0{"3001:db8:1:2:3:4:5:6"};
  // 3 uSids populated
  static inline const folly::IPAddressV6 kSid1{"3001:db8:4:5:6::"};
  static inline const folly::IPAddressV6 kSid2{"3001:db8:7:8:9::"};

  // SRv6 decap (AgentSrv6DecapTests)
  static inline const folly::IPAddressV6 kDecapMySidAddr{"3001:db8:7fff::"};
  static constexpr uint8_t kDecapMySidPrefixLen{48};
  // Inject outer IPv6 payloadLength for decap strip probes (inner v6+UDP+50B
  // pad).
  static constexpr uint16_t kDecapStripInjectOuterPayloadLen{0x62};
  static constexpr size_t kDecapStripInjectInnerIpv6Offset{0};
  static constexpr size_t kDecapStripPayloadCsigShimSize{4};

  // SRv6 midpoint (AgentSrv6MidpointTest)
  static inline const folly::IPAddressV6 kMidpointMySidPrefix{"fdad:ffff:1::"};
  static constexpr uint8_t kMidpointMySidPrefixLen{48};
  static inline const folly::IPAddressV6 kMidpointPktOuterDst{
      "fdad:ffff:1:2::"};
  static inline const folly::IPAddressV6 kMidpointExpectedOuterDst{
      "fdad:ffff:2::"};

  // SRv6 binding SID (AgentSrv6BindingSidTests)
  static inline const folly::IPAddressV6 kBindingMySidPrefix{"fc00:100:1::"};
  static constexpr uint8_t kBindingMySidPrefixLen{48};
  static inline const folly::IPAddressV6 kBgpRoute0{"2001::1"};
  static inline const folly::IPAddressV6 kOpenrPrefix0{"fdad::1:0"};
  static inline const folly::IPAddressV6 kIpInIpInnerSrc{"2001:db8::1"};
  static inline const folly::IPAddressV6 kIpInIpInnerDst{"2001:db8::2"};

  static inline const folly::IPAddressV6 kEncapRoutePrefix{"2800:2::"};
  static constexpr uint8_t kEncapRoutePrefixLen{64};
  static inline const folly::IPAddressV6 kEncapRouteDstIp{"2800:2::1"};
  static inline const folly::IPAddressV4 kRecursiveV4Prefix{"100.0.0.0"};
  static constexpr uint8_t kRecursiveV4PrefixLen{24};
  static constexpr int kNumNextHops{4};
  static constexpr uint8_t kECT1{1};
  static constexpr int kCongestionFloodPacketCount{512};
  static constexpr int kMaxMirroredPacketsToScan{64};
  static constexpr uint8_t kFloodDscp{5};
  static constexpr uint8_t kFloodTtl{64};
  static constexpr uint8_t kTgenFloodTtl{5};
  // CPU-switched probe inject hop limit (SDK sw_port / tgen probe packet).
  static constexpr uint8_t kTgenProbeHopLimit{64};
  // SDK test_csig_tgen_v4_v6 traps egress loopback copies at TTL/hop 63 after
  // one decrement from the hop-64 probe (SAI_ACL_ENTRY_ATTR_FIELD_TTL = 63).
  static constexpr uint8_t kTgenProbeTrapMatchHopLimit{63};
  static constexpr int kTgenRefloodPacketCount{32};
  static constexpr int kMaxTgenRefloodAttempts{2};
  static inline const std::string kEgressSpanMirrorName{
      "csig-srv6-egress-span"};

  // SAI routed CSIG injects untagged Ethernet: [dst|src|0x9900|CSIG|IP|...].
  // VLAN tagging on inject breaks CSIG handling on Yuba/G202X front-panel
  // ports.
  static std::optional<VlanID> csigRoutedInjectVlan() {
    return std::nullopt;
  }

  // NHG names for SRv6 encap routes (also used as counter IDs)
  // Same names used for both v4 and v6 routes
  static inline const std::string kNhgSid0{"kSid0"};
  static inline const std::string kNhgSid1OrSid2{"kSid1_or_kSid2"};

  // Production recursive SRv6: a child prefix programmed by OpenR (no SID) and
  // TE_Agent (kSid0), that a BGP parent (kEncapRoutePrefix) resolves through.
  const folly::IPAddressV6 kChildPrefix{"2901::"};
  static constexpr uint8_t kChildPrefixLen{48};
  const folly::IPAddress kChildDstIp{"2901::1234"};
  const folly::IPAddress kBgpRecursiveNhop{"2901::1"};
  static inline const std::string kChildRouteCounter{"recursiveChildSid"};
  static inline const std::string kParentRouteCounter{"recursiveParentSid"};

  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {
        ProductionFeature::SRV6_ENCAP,
        ProductionFeature::SRV6_DECAP,
        ProductionFeature::SRV6_MIDPOINT,
        ProductionFeature::SRV6_BINDING_SID,
        ProductionFeature::L3_QOS,
        ProductionFeature::ECN,
        ProductionFeature::ROUTE_COUNTERS};
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
    addSrv6TunnelConfig(cfg);
    cfg.loadBalancers() =
        utility::getEcmpFullWithFlowLabelTrunkFullWithFlowLabelHashConfig(
            ensemble.getL3Asics());
    // CSIG+SRv6 loopback-trap tests capture on egress port ingress ACL only.
    // Do not install COPY-to-CPU trap ACLs on expected egress destinations
    // (encap SID, decap inner dst, midpoint shifted outer dst) — those steal
    // the packet before it can egress and loop back.
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
    auto ecmpHelper = makeEcmpHelper<folly::IPAddressV6>();
    this->resolveNeighbors(ecmpHelper, numNextHops, true /* useLinkLocal */);
  }

  void unresolveNextHops(int numNextHops) {
    auto ecmpHelper = makeEcmpHelper<folly::IPAddressV6>();
    this->unresolveNeighbors(ecmpHelper, numNextHops, true /* useLinkLocal */);
  }
  void setupHelper(
      bool resolveNeighbors = true,
      bool programEncapRoutes = true) {
    if (resolveNeighbors) {
      resolveNextHops(kNumNextHops);
    }
    if (programEncapRoutes) {
      // IPv6 encap routes (v6 next hops)
      addEncapRoute<folly::CIDRNetworkV6>(
          {kEncapRoutePrefix, kEncapRoutePrefixLen}, {{kSid0}}, kNhgSid0);
      addEncapRoute<folly::CIDRNetworkV6>(
          {folly::IPAddressV6("2800:3::"), kEncapRoutePrefixLen},
          {{kSid1}, {kSid2}},
          kNhgSid1OrSid2);
      addEncapRoute<folly::CIDRNetworkV6>(
          {folly::IPAddressV6("2800:4::"), kEncapRoutePrefixLen},
          {{kSid1}, {kSid2}},
          kNhgSid1OrSid2);
      // IPv4 encap routes (v4 next hops) - use same NHG names as v6
      addEncapRoute<folly::CIDRNetworkV4>(
          {folly::IPAddressV4("100.0.0.0"), 24}, {{kSid0}}, kNhgSid0);
      addEncapRoute<folly::CIDRNetworkV4>(
          {folly::IPAddressV4("200.0.0.0"), 24},
          {{kSid1}, {kSid2}},
          kNhgSid1OrSid2);
      addEncapRoute<folly::CIDRNetworkV4>(
          {folly::IPAddressV4("201.0.0.0"), 24},
          {{kSid1}, {kSid2}},
          kNhgSid1OrSid2);
    }
  }

  // Programs recursive SRv6 routes for testing SID list override:
  //   OpenR route A (2901::/48) -> nhop(0), nhop(1), each carrying kSid2
  //   OpenR route B (2902::/48) -> nhop(2), nhop(3), each carrying kSid2
  //   SRv6 route (routePrefix/kEncapRoutePrefixLen) -> 2901::1 (kSid0),
  //     2902::1 (kSid1), resolving recursively through the OpenR routes.
  // After resolution, the SRv6 route expands to 4 next hops carrying the
  // outer SID lists (kSid0/kSid1), which override the inner OpenR SID list
  // (kSid2).
  void addRecursiveSrv6Routes(const folly::IPAddressV6& routePrefix) {
    auto ecmpHelper = makeEcmpHelper();
    auto routeUpdater = this->getSw()->getRouteUpdater();

    // Helper to get link-local IP for IPv6 next hops
    auto getNhopIp = [&ecmpHelper](int idx) {
      auto nhop = ecmpHelper.nhop(idx);
      if (nhop.linkLocalNhopIp.has_value()) {
        return folly::IPAddress(nhop.linkLocalNhopIp.value());
      }
      return folly::IPAddress(nhop.ip);
    };

    // Inner OpenR routes carry their own SID list (kSid2) so that recursive
    // resolution is exercised against a non-empty inner SID list; the outer
    // SRv6 route's SIDs (kSid0/kSid1) must override it.
    auto makeSidCarryingNhop = [](const folly::IPAddress& ip,
                                  InterfaceID intf) {
      return ResolvedNextHop(
          ip,
          intf,
          ECMP_WEIGHT,
          std::nullopt,
          std::nullopt,
          std::nullopt,
          std::nullopt,
          std::vector<folly::IPAddressV6>{kSid2},
          TunnelType::SRV6_ENCAP,
          std::string("srv6Tunnel0"));
    };

    // OpenR route A (2901::/48) -> nhop(0), nhop(1), link-local nexthops
    // carrying kSid2
    RouteNextHopSet openrNhopsA{
        makeSidCarryingNhop(getNhopIp(0), ecmpHelper.nhop(0).intf),
        makeSidCarryingNhop(getNhopIp(1), ecmpHelper.nhop(1).intf)};
    routeUpdater.addRoute(
        RouterID(0),
        folly::IPAddressV6("2901::"),
        48,
        ClientID::OPENR,
        RouteNextHopEntry(openrNhopsA, AdminDistance::OPENR));

    // OpenR route B (2902::/48) -> nhop(2), nhop(3), link-local nexthops
    // carrying kSid2
    RouteNextHopSet openrNhopsB{
        makeSidCarryingNhop(getNhopIp(2), ecmpHelper.nhop(0).intf),
        makeSidCarryingNhop(getNhopIp(3), ecmpHelper.nhop(1).intf)};
    routeUpdater.addRoute(
        RouterID(0),
        folly::IPAddressV6("2902::"),
        48,
        ClientID::OPENR,
        RouteNextHopEntry(openrNhopsB, AdminDistance::OPENR));

    // SRv6 route -> 2901::1 (kSid0), 2902::1 (kSid1)
    // These unresolved nexthops carry SRV6 fields and resolve recursively
    // over the OpenR routes above, whose link-local nexthops carry kSid2
    // (overridden by the outer kSid0/kSid1 after resolution).
    RouteNextHopSet srv6Nhops{
        UnresolvedNextHop(
            folly::IPAddress("2901::1"),
            ECMP_WEIGHT,
            std::nullopt,
            std::nullopt,
            std::nullopt,
            std::nullopt,
            std::vector<folly::IPAddressV6>{kSid0},
            TunnelType::SRV6_ENCAP,
            std::string("srv6Tunnel0")),
        UnresolvedNextHop(
            folly::IPAddress("2902::1"),
            ECMP_WEIGHT,
            std::nullopt,
            std::nullopt,
            std::nullopt,
            std::nullopt,
            std::vector<folly::IPAddressV6>{kSid1},
            TunnelType::SRV6_ENCAP,
            std::string("srv6Tunnel0"))};
    routeUpdater.addRoute(
        RouterID(0),
        routePrefix,
        kEncapRoutePrefixLen,
        ClientID::TE_AGENT,
        RouteNextHopEntry(srv6Nhops, AdminDistance::TE_AGENT));
    routeUpdater.program();
  }

  void programRecursiveOpenrRoutes() {
    auto ecmpHelper = makeEcmpHelper();
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
  }

  void removeRecursiveOpenrRoutes() {
    auto routeUpdater = this->getSw()->getRouteUpdater();
    routeUpdater.delRoute(
        RouterID(0), folly::IPAddressV6("2901::"), 48, ClientID::OPENR);
    routeUpdater.delRoute(
        RouterID(0), folly::IPAddressV6("2902::"), 48, ClientID::OPENR);
    routeUpdater.program();
  }

  void addRecursiveSrv6RoutesWithV4(
      const folly::IPAddressV6& v6Prefix,
      uint8_t v6PrefixLen,
      const folly::IPAddressV4& v4Prefix,
      uint8_t v4PrefixLen) {
    auto ecmpHelper = makeEcmpHelper();
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

    RouteNextHopSet srv6Nhops{
        UnresolvedNextHop(
            folly::IPAddress("2901::1"),
            ECMP_WEIGHT,
            std::nullopt,
            std::nullopt,
            std::nullopt,
            std::nullopt,
            std::vector<folly::IPAddressV6>{kSid0},
            TunnelType::SRV6_ENCAP,
            std::string("srv6Tunnel0")),
        UnresolvedNextHop(
            folly::IPAddress("2902::1"),
            ECMP_WEIGHT,
            std::nullopt,
            std::nullopt,
            std::nullopt,
            std::nullopt,
            std::vector<folly::IPAddressV6>{kSid1},
            TunnelType::SRV6_ENCAP,
            std::string("srv6Tunnel0"))};
    routeUpdater.addRoute(
        RouterID(0),
        v6Prefix,
        v6PrefixLen,
        ClientID::TE_AGENT,
        RouteNextHopEntry(srv6Nhops, AdminDistance::TE_AGENT));
    routeUpdater.addRoute(
        RouterID(0),
        v4Prefix,
        v4PrefixLen,
        ClientID::TE_AGENT,
        RouteNextHopEntry(srv6Nhops, AdminDistance::TE_AGENT));
    routeUpdater.program();
  }

  void addRecursiveSrv6RoutesSameSidListSameRif() {
    utility::EcmpSetupTargetedPorts<folly::IPAddressV6> ecmpHelper(
        this->getProgrammedState(), this->getSw()->needL2EntryForNeighbor());

    auto makeLinkLocalNhop = [](utility::EcmpNextHop<folly::IPAddressV6> nhop,
                                const folly::IPAddressV6& linkLocalIp) {
      nhop.linkLocalNhopIp = linkLocalIp;
      return nhop;
    };

    const auto rif0Nhop = ecmpHelper.getNextHops()[0];
    const folly::IPAddressV6 linkLocalIp0{"fe80:face:b11c::1"};
    const folly::IPAddressV6 linkLocalIp1{"fe80:face:b11c::2"};
    const auto rif0Ip0 = makeLinkLocalNhop(rif0Nhop, linkLocalIp0);
    const auto rif0Ip1 = makeLinkLocalNhop(rif0Nhop, linkLocalIp1);

    this->applyNewState(
        [&ecmpHelper, &rif0Ip0, &rif0Ip1](
            const std::shared_ptr<SwitchState> state) {
          auto newState = ecmpHelper.resolveNextHop(
              state, rif0Ip0, true /* useLinkLocal */);
          return ecmpHelper.resolveNextHop(
              newState, rif0Ip1, true /* useLinkLocal */);
        },
        "resolve recursive SRv6 link-local next hops");

    auto makeResolvedNhop = [](const auto& nhop) {
      return ResolvedNextHop(
          folly::IPAddress(nhop.linkLocalNhopIp.value()),
          nhop.intf,
          ECMP_WEIGHT);
    };

    auto routeUpdater = this->getSw()->getRouteUpdater();
    routeUpdater.addRoute(
        RouterID(0),
        folly::IPAddressV6("2901::"),
        48,
        ClientID::OPENR,
        RouteNextHopEntry(
            RouteNextHopSet{
                makeResolvedNhop(rif0Ip0), makeResolvedNhop(rif0Ip1)},
            AdminDistance::OPENR));
    routeUpdater.addRoute(
        RouterID(0),
        folly::IPAddressV6("2902::"),
        48,
        ClientID::OPENR,
        RouteNextHopEntry(
            RouteNextHopSet{
                makeResolvedNhop(rif0Ip0), makeResolvedNhop(rif0Ip1)},
            AdminDistance::OPENR));

    auto makeSrv6Nhop = [this](const folly::IPAddress& ip) {
      return UnresolvedNextHop(
          ip,
          ECMP_WEIGHT,
          std::nullopt,
          std::nullopt,
          std::nullopt,
          std::nullopt,
          std::vector<folly::IPAddressV6>{kSid0},
          TunnelType::SRV6_ENCAP,
          std::string("srv6Tunnel0"));
    };

    routeUpdater.addRoute(
        RouterID(0),
        kEncapRoutePrefix,
        kEncapRoutePrefixLen,
        ClientID::TE_AGENT,
        RouteNextHopEntry(
            RouteNextHopSet{makeSrv6Nhop(folly::IPAddress("2901::1"))},
            AdminDistance::TE_AGENT));
    routeUpdater.addRoute(
        RouterID(0),
        folly::IPAddressV6("2800:3::"),
        kEncapRoutePrefixLen,
        ClientID::TE_AGENT,
        RouteNextHopEntry(
            RouteNextHopSet{makeSrv6Nhop(folly::IPAddress("2902::1"))},
            AdminDistance::TE_AGENT));
    routeUpdater.program();
  }

  // Programs the SAME prefix (kEncapRoutePrefix) from two clients to exercise
  // admin-distance preference: OpenR (plain link-local nexthop, no SID list)
  // and TE_Agent (link-local nexthop carrying kSid0). TE_Agent has the lower
  // admin distance, so it wins and the route is SRv6-encapped with kSid0.
  void addTeAgentPreferredOverOpenrRoute() {
    auto ecmpHelper = makeEcmpHelper();
    auto getNhopIp = [&ecmpHelper](int idx) {
      auto nhop = ecmpHelper.nhop(idx);
      if (nhop.linkLocalNhopIp.has_value()) {
        return folly::IPAddress(nhop.linkLocalNhopIp.value());
      }
      return folly::IPAddress(nhop.ip);
    };
    auto routeUpdater = this->getSw()->getRouteUpdater();

    // OpenR route: plain link-local nexthop, no SID list (higher admin
    // distance, should lose).
    routeUpdater.addRoute(
        RouterID(0),
        kEncapRoutePrefix,
        kEncapRoutePrefixLen,
        ClientID::OPENR,
        RouteNextHopEntry(
            RouteNextHopSet{ResolvedNextHop(
                getNhopIp(0), ecmpHelper.nhop(0).intf, ECMP_WEIGHT)},
            AdminDistance::OPENR));

    // TE_Agent route: link-local nexthop carrying kSid0 (lower admin distance,
    // should win and drive the SRv6 encap).
    routeUpdater.addRoute(
        RouterID(0),
        kEncapRoutePrefix,
        kEncapRoutePrefixLen,
        ClientID::TE_AGENT,
        RouteNextHopEntry(
            RouteNextHopSet{ResolvedNextHop(
                getNhopIp(0),
                ecmpHelper.nhop(0).intf,
                ECMP_WEIGHT,
                std::nullopt,
                std::nullopt,
                std::nullopt,
                std::nullopt,
                std::vector<folly::IPAddressV6>{kSid0},
                TunnelType::SRV6_ENCAP,
                std::string("srv6Tunnel0"))},
            AdminDistance::TE_AGENT));
    routeUpdater.program();
  }

  // Production recursive SRv6 topology:
  //   OpenR    programs kChildPrefix -> kNumNextHops nhops, NO SID list.
  //   TE_Agent programs kChildPrefix -> the SAME nhops, WITH kSid0 (wins by
  //            admin distance), counted by kChildRouteCounter.
  //   BGP      programs the parent kEncapRoutePrefix -> a next hop inside
  //            kChildPrefix, resolving recursively through the child, counted
  //            by kParentRouteCounter.
  // The parent inherits the child's SID list, so both prefixes egress the same
  // SID-list next hops.
  void addProductionRecursiveSrv6Routes() {
    auto ecmpHelper = makeEcmpHelper();
    auto getNhopIp = [&ecmpHelper](int idx) {
      auto nhop = ecmpHelper.nhop(idx);
      if (nhop.linkLocalNhopIp.has_value()) {
        return folly::IPAddress(nhop.linkLocalNhopIp.value());
      }
      return folly::IPAddress(nhop.ip);
    };
    auto routeUpdater = this->getSw()->getRouteUpdater();

    // (a) OpenR child: same nhops, no SID list (loses on admin distance).
    RouteNextHopSet openrNhops;
    for (int i = 0; i < kNumNextHops; ++i) {
      openrNhops.insert(
          ResolvedNextHop(getNhopIp(i), ecmpHelper.nhop(i).intf, ECMP_WEIGHT));
    }
    routeUpdater.addRoute(
        RouterID(0),
        kChildPrefix,
        kChildPrefixLen,
        ClientID::OPENR,
        RouteNextHopEntry(openrNhops, AdminDistance::OPENR));

    // (b) TE_Agent child: the same nhops carrying kSid0 (wins on admin
    // distance).
    RouteNextHopSet teAgentNhops;
    for (int i = 0; i < kNumNextHops; ++i) {
      teAgentNhops.insert(ResolvedNextHop(
          getNhopIp(i),
          ecmpHelper.nhop(i).intf,
          ECMP_WEIGHT,
          std::nullopt,
          std::nullopt,
          std::nullopt,
          std::nullopt,
          std::vector<folly::IPAddressV6>{kSid0},
          TunnelType::SRV6_ENCAP,
          std::string("srv6Tunnel0")));
    }
    routeUpdater.addRoute(
        RouterID(0),
        kChildPrefix,
        kChildPrefixLen,
        ClientID::TE_AGENT,
        RouteNextHopEntry(
            teAgentNhops,
            AdminDistance::TE_AGENT,
            std::make_optional<RouteCounterID>(kChildRouteCounter)));

    // (c) BGP parent: next hop is inside kChildPrefix, resolving recursively
    //     through the child (and inheriting its SID list).
    routeUpdater.addRoute(
        RouterID(0),
        kEncapRoutePrefix,
        kEncapRoutePrefixLen,
        ClientID::BGPD,
        RouteNextHopEntry(
            RouteNextHopSet{UnresolvedNextHop(kBgpRecursiveNhop, ECMP_WEIGHT)},
            AdminDistance::EBGP,
            std::make_optional<RouteCounterID>(kParentRouteCounter)));

    routeUpdater.program();
  }

  template <typename CIDRNetworkT>
  void addEncapRoute(
      const CIDRNetworkT& prefix,
      const std::vector<std::vector<folly::IPAddressV6>>& sidLists,
      const std::string& nhgName) {
    RouteNextHopSet nhops;
    // Always use ipv6 link local nhops since that;s the prod
    // use case
    auto ecmpHelper = makeEcmpHelper<folly::IPAddressV6>();
    for (auto i = 0; i < sidLists.size(); ++i) {
      auto nhop = ecmpHelper.nhop(i);
      CHECK(nhop.linkLocalNhopIp.has_value());
      nhops.insert(ResolvedNextHop(
          folly::IPAddress(*nhop.linkLocalNhopIp),
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

    auto rib = this->getSw()->getRib();
    std::vector<std::pair<std::string, RouteNextHopSet>> groups;
    groups.emplace_back(nhgName, nhops);
    rib->addOrUpdateNamedNextHopGroups(
        this->getSw()->getScopeResolver(),
        groups,
        createRibToSwitchStateFunction(),
        this->getSw());

    UnicastRoute route;
    route.dest()->ip() =
        facebook::network::toBinaryAddress(folly::IPAddress(prefix.first));
    route.dest()->prefixLength() = prefix.second;
    NamedRouteDestination namedDest;
    namedDest.nextHopGroup_ref() = nhgName;
    route.namedRouteDestination() = namedDest;
    route.counterID() = nhgName;

    auto routeUpdater = this->getSw()->getRouteUpdater();
    routeUpdater.addRoute(RouterID(0), ClientID::TE_AGENT, route);
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
      std::optional<folly::IPAddress> dstIp = std::nullopt,
      const std::string& counterID = "") {
    const auto& sids = expectedSids;

    std::map<PortID, int64_t> bytesBefore;
    for (auto port : egressPorts) {
      bytesBefore[port] = *this->getLatestPortStats(port).outBytes_();
    }

    int64_t counterBytesBefore = 0;
    int64_t counterPacketsBefore = 0;
    if (!counterID.empty()) {
      auto hwSwitchStats = this->getHwSwitchStats();
      auto& routeCounters = *hwSwitchStats.counterStats()->routeCounters();
      auto it = routeCounters.find(counterID);
      if (it != routeCounters.end()) {
        counterBytesBefore = it->second.bytes().value_or(0);
        counterPacketsBefore = it->second.packets().value_or(0);
      }
    }

    auto intfMac =
        getMacForFirstInterfaceWithPortsForTesting(this->getProgrammedState());
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

    XLOG(DBG2) << " Verifying : v4: " << isV4 << " with counter ID: "
               << (counterID.size() ? counterID : "none")
               << " dst IP: " << pktDstIp
               << " from CPU: " << (injectPort ? "no" : "yes");
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
      this->sendPacketSwitchedAsync(std::move(txPacket));
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

    if (!counterID.empty()) {
      WITH_RETRIES({
        auto hwSwitchStats = this->getHwSwitchStats();
        auto& routeCounters = *hwSwitchStats.counterStats()->routeCounters();
        auto it = routeCounters.find(counterID);
        ASSERT_EVENTUALLY_TRUE(it != routeCounters.end())
            << "Route counter " << counterID << " not found";
        EXPECT_EVENTUALLY_GT(
            it->second.bytes().value_or(0), counterBytesBefore);
        EXPECT_EVENTUALLY_EQ(
            it->second.packets().value_or(0), counterPacketsBefore + 1);
      });
    }
  }

  void verifyEncapPacketCpuAndFrontPanel(
      const std::vector<PortID>& egressPorts,
      const std::vector<folly::IPAddressV6>& expectedSids = {kSid0},
      const std::string& counterID = "") {
    auto injectPort = findInjectPort(egressPorts);
    for (bool isV4 : {false, true}) {
      // ECN not marked
      verifyEncapPacket(
          egressPorts,
          false,
          isV4,
          expectedSids,
          std::nullopt,
          std::nullopt,
          counterID);
      verifyEncapPacket(
          egressPorts,
          false,
          isV4,
          expectedSids,
          injectPort,
          std::nullopt,
          counterID);
      // ECN marked
      verifyEncapPacket(
          egressPorts,
          true,
          isV4,
          expectedSids,
          std::nullopt,
          std::nullopt,
          counterID);
      verifyEncapPacket(
          egressPorts,
          true,
          isV4,
          expectedSids,
          injectPort,
          std::nullopt,
          counterID);
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

  void configureCsigOnIngressAndEgressPorts(
      PortID ingressPort,
      PortID egressPort) {
    utility::configureCsigForSrv6EncapTest(
        this->getSw()->getMonolithicHwSwitchHandler()->getHwSwitch(),
        ingressPort,
        egressPort);
  }

  void configureCsigTgenStyleOnPorts(PortID floodPort, PortID egressPort) {
    utility::configureCsigForTgenStyleTest(
        this->getSw()->getMonolithicHwSwitchHandler()->getHwSwitch(),
        floodPort,
        egressPort);
  }

  void configureCsigEgressUpdateQuietClamp(
      PortID ingressPort,
      PortID egressPort) {
    utility::configureCsigForSrv6QuietClampTest(
        this->getSw()->getMonolithicHwSwitchHandler()->getHwSwitch(),
        ingressPort,
        egressPort);
  }

  void csigSrv6EncapRoutesOnly() {
    setupHelper();
  }

  void csigDecapRoutesOnly() {
    resolveNextHops(2);
    addPlainEcmpRoute<folly::CIDRNetworkV6>(
        {kEncapRoutePrefix, kEncapRoutePrefixLen}, 1);
    utility::addDecapMySidEntry(
        this->getSw(), kDecapMySidAddr, kDecapMySidPrefixLen);
    waitForStateUpdates(getSw());
  }

  void csigMidpointRoutesOnly() {
    applyMidpointSidConfig();
    auto ecmpHelper = makeEcmpHelper();
    auto portDesc = midpointMySidPortDesc();
    this->applyNewState(
        [&ecmpHelper, portDesc](std::shared_ptr<SwitchState> in) {
          return ecmpHelper.resolveNextHops(
              in, {portDesc}, true /* useLinkLocal */);
        },
        "resolve midpoint mySid neighbor");
    utility::waitForMySidResolveOrUnresolve(
        [this]() { return this->getProgrammedState(); },
        kMidpointMySidPrefix,
        kMidpointMySidPrefixLen,
        true /* resolved */);
  }

  void csigBindingSidRoutesOnly() {
    programBindingSidRoutes();
    utility::addBindingSidEntry(
        this->getSw(),
        kBindingMySidPrefix,
        kBindingMySidPrefixLen,
        {utility::makeSrv6NextHopThrift(kBgpRoute0, kSid0)});
    waitForStateUpdates(getSw());
  }

  void skipUnlessCsigSupportedAsic() {
    auto asic =
        checkSameAndGetAsicForTesting(this->getAgentEnsemble()->getL3Asics());
    if (!utility::isCsigSupportedAsic(asic)) {
      GTEST_SKIP() << "CSIG tests require Yuba/G202X ASIC";
    }
  }

  void csigSrv6EncapSetup() {
    setupHelper();
    auto ecmpHelper = makeEcmpHelper();
    auto egressPort = getEgressPort(ecmpHelper.nhop(0).portDesc);
    auto ingressPort = findInjectPort({egressPort});
    configureCsigOnIngressAndEgressPorts(ingressPort, egressPort);
  }

  void csigSrv6EncapTgenStyleSetup() {
    setupHelper();
    auto egressPort = getDefaultCsigEgressPort();
    auto floodPort = findInjectPort({egressPort});
    configureCsigTgenStyleOnPorts(floodPort, egressPort);
  }

  void csigDecapTgenStyleSetup() {
    csigDecapRoutesOnly();
    auto egressPort = getDefaultCsigEgressPort();
    auto floodPort = findInjectPort({egressPort});
    configureCsigTgenStyleOnPorts(floodPort, egressPort);
  }

  void csigMidpointTgenStyleSetup() {
    csigMidpointRoutesOnly();
    auto egressPort = getMidpointCsigEgressPort();
    auto floodPort = findInjectPort({egressPort});
    configureCsigTgenStyleOnPorts(floodPort, egressPort);
  }

  void csigBindingSidTgenStyleSetup() {
    csigBindingSidRoutesOnly();
    auto egressPort = getDefaultCsigEgressPort();
    auto floodPort = findInjectPort({egressPort});
    configureCsigTgenStyleOnPorts(floodPort, egressPort);
  }

  PortID getDefaultCsigEgressPort() {
    auto ecmpHelper = makeEcmpHelper();
    return getEgressPort(ecmpHelper.nhop(0).portDesc);
  }

  cfg::MySidConfig makeMidpointAdjacencyMySidConfig(
      const cfg::SwitchConfig& cfg,
      const AgentEnsemble& ensemble) const {
    auto portName =
        utility::portNameForConfig(cfg, ensemble.masterLogicalPortIds()[0]);
    return utility::makeAdjacencyMySidConfig(
        portName, "fdad:ffff::/32", /*functionId=*/1);
  }

  PortDescriptor midpointMySidPortDesc() const {
    return PortDescriptor(this->masterLogicalPortIds()[0]);
  }

  template <typename CIDRNetworkT>
  void addPlainEcmpRoute(const CIDRNetworkT& prefix, int numNextHops) {
    RouteNextHopSet nhops;
    auto ecmpHelper = makeEcmpHelper<folly::IPAddressV6>();
    for (auto i = 0; i < numNextHops; ++i) {
      auto nhop = ecmpHelper.nhop(i);
      CHECK(nhop.linkLocalNhopIp.has_value());
      nhops.insert(ResolvedNextHop(
          folly::IPAddress(*nhop.linkLocalNhopIp), nhop.intf, ECMP_WEIGHT));
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

  void programBindingSidRoutes() {
    resolveNextHops(kNumNextHops);
    auto ecmpHelper = makeEcmpHelper();
    auto routeUpdater = this->getSw()->getRouteUpdater();
    auto nhop = ecmpHelper.nhop(0);
    auto nhopIp = nhop.linkLocalNhopIp.has_value()
        ? folly::IPAddress(nhop.linkLocalNhopIp.value())
        : folly::IPAddress(nhop.ip);
    routeUpdater.addRoute(
        RouterID(0),
        kOpenrPrefix0,
        112,
        ClientID::OPENR,
        RouteNextHopEntry(
            RouteNextHopSet{ResolvedNextHop(nhopIp, nhop.intf, ECMP_WEIGHT)},
            AdminDistance::OPENR));
    routeUpdater.addRoute(
        RouterID(0),
        kBgpRoute0,
        128,
        ClientID::BGPD,
        RouteNextHopEntry(
            RouteNextHopSet{
                UnresolvedNextHop(folly::IPAddress("fdad::1:1"), ECMP_WEIGHT)},
            AdminDistance::EBGP));
    routeUpdater.program();
  }

  void csigDecapSetup() {
    resolveNextHops(2);
    addPlainEcmpRoute<folly::CIDRNetworkV6>(
        {kEncapRoutePrefix, kEncapRoutePrefixLen}, 1);
    utility::addDecapMySidEntry(
        this->getSw(), kDecapMySidAddr, kDecapMySidPrefixLen);
    waitForStateUpdates(getSw());
    auto ecmpHelper = makeEcmpHelper();
    auto egressPort = getEgressPort(ecmpHelper.nhop(0).portDesc);
    auto ingressPort = findInjectPort({egressPort});
    configureCsigOnIngressAndEgressPorts(ingressPort, egressPort);
  }

  void applyMidpointSidConfig() {
    auto config = this->getAgentEnsemble()->getCurrentConfig();
    config.mySidConfig() =
        makeMidpointAdjacencyMySidConfig(config, *this->getAgentEnsemble());
    this->applyNewConfig(config);
    waitForStateUpdates(getSw());
  }

  void csigMidpointSetup() {
    applyMidpointSidConfig();
    auto ecmpHelper = makeEcmpHelper();
    auto portDesc = midpointMySidPortDesc();
    this->applyNewState(
        [&ecmpHelper, portDesc](std::shared_ptr<SwitchState> in) {
          return ecmpHelper.resolveNextHops(
              in, {portDesc}, true /* useLinkLocal */);
        },
        "resolve midpoint mySid neighbor");
    utility::waitForMySidResolveOrUnresolve(
        [this]() { return this->getProgrammedState(); },
        kMidpointMySidPrefix,
        kMidpointMySidPrefixLen,
        true /* resolved */);
    auto egressPort = getEgressPort(portDesc);
    auto ingressPort = findInjectPort({egressPort});
    configureCsigOnIngressAndEgressPorts(ingressPort, egressPort);
  }

  void csigBindingSidSetup() {
    programBindingSidRoutes();
    utility::addBindingSidEntry(
        this->getSw(),
        kBindingMySidPrefix,
        kBindingMySidPrefixLen,
        {utility::makeSrv6NextHopThrift(kBgpRoute0, kSid0)});
    waitForStateUpdates(getSw());
    auto ecmpHelper = makeEcmpHelper();
    auto egressPort = getEgressPort(ecmpHelper.nhop(0).portDesc);
    auto ingressPort = findInjectPort({egressPort});
    configureCsigOnIngressAndEgressPorts(ingressPort, egressPort);
  }

  PortID getMidpointCsigEgressPort() {
    return getEgressPort(midpointMySidPortDesc());
  }

  void resolveMidpointNeighborOnEgressPort(PortID egressPort) {
    auto ecmpHelper = makeEcmpHelper();
    auto portDesc = midpointMySidPortDesc();
    CHECK_EQ(getEgressPort(portDesc), egressPort)
        << "midpoint egress must match mySid adjacency port";
    applyNewState(
        [&ecmpHelper, portDesc](std::shared_ptr<SwitchState> in) {
          return ecmpHelper.resolveNextHops(
              in, {portDesc}, true /* useLinkLocal */);
        },
        "resolve midpoint mySid neighbor");
  }

  void installEgressPortIngressTrap(PortID egressPort) {
    auto config = this->getAgentEnsemble()->getCurrentConfig();
    auto asic =
        checkSameAndGetAsicForTesting(this->getAgentEnsemble()->getL3Asics());
    utility::configureTrapAcl(asic, config, egressPort);
    this->applyNewConfig(config);
    waitForStateUpdates(getSw());
    XLOG(INFO) << "CSIG installed ingress TRAP ACL on egress port "
               << egressPort << " for loopback punt capture";
  }

  // Tgen-style flood uses hop_limit=5 (kTgenFloodTtl); CPU-switched encap probe
  // uses hop_limit=64 and is trapped on egress loopback at hop 63 (SDK tgen).
  // Also trap hop 64 for routed UPDATE tests where TTL decrement is disabled on
  // the congestion-loop host route (loopback copy keeps hop 64).
  void installEgressPortIngressTrapForTgenProbe(PortID egressPort) {
    auto config = this->getAgentEnsemble()->getCurrentConfig();
    auto asic =
        checkSameAndGetAsicForTesting(this->getAgentEnsemble()->getL3Asics());
    utility::addTrapPacketAcl(
        asic, &config, egressPort, kTgenProbeTrapMatchHopLimit);
    utility::addTrapPacketAcl(asic, &config, egressPort, kTgenProbeHopLimit);
    this->applyNewConfig(config);
    waitForStateUpdates(getSw());
    XLOG(INFO) << "CSIG installed hop-limit TRAP ACL on egress port "
               << egressPort << " hop_limit=" << +kTgenProbeTrapMatchHopLimit
               << " and " << +kTgenProbeHopLimit
               << " (excludes tgen plain flood TTL=" << +kTgenFloodTtl << ")";
  }

  void installEgressSpanMirror(PortID egressPort, PortID mirrorDestPort) {
    CHECK_NE(egressPort, mirrorDestPort)
        << "egress SPAN source and destination must be different ports";
    auto config = this->getAgentEnsemble()->getCurrentConfig();
    bool mirrorExists = false;
    for (const auto& mirror : *config.mirrors()) {
      if (mirror.name() == kEgressSpanMirrorName) {
        mirrorExists = true;
        break;
      }
    }
    if (!mirrorExists) {
      cfg::MirrorDestination destination;
      destination.egressPort() = cfg::MirrorEgressPort();
      destination.egressPort()->logicalID() = mirrorDestPort;
      cfg::Mirror mirrorConfig;
      mirrorConfig.name() = kEgressSpanMirrorName;
      mirrorConfig.destination() = destination;
      mirrorConfig.truncate() = false;
      config.mirrors()->push_back(mirrorConfig);
    }
    auto portCfg = utility::findCfgPort(config, egressPort);
    portCfg->egressMirror() = kEgressSpanMirrorName;
    this->applyNewConfig(config);
    waitForStateUpdates(getSw());
    XLOG(INFO) << "CSIG+SRv6 encap installed egress SPAN mirror "
               << kEgressSpanMirrorName << " from port " << egressPort
               << " to port " << mirrorDestPort;
  }

  struct UndecappedSrv6TrapAnalysis {
    bool valid{false};
    uint16_t outerPayloadLength{0};
    uint8_t outerNextHeader{0};
    folly::IPAddressV6 outerDst;
    bool payloadHasIngressCsig{false};
    std::optional<size_t> ingressCsigPayloadOffset;
    std::optional<size_t> innerIpv6PayloadOffset;
  };

  struct DecapStripInjectLayout {
    bool valid{false};
    uint16_t outerPayloadLength{0};
    uint8_t outerNextHeader{0};
    bool payloadHasIngressCsig{false};
    std::optional<size_t> ingressCsigPayloadOffset;
    std::optional<size_t> innerIpv6PayloadOffset;
  };

  struct CsigTrapScanStats {
    bool foundMatch{false};
    int trappedPacketsScanned{0};
    int csigPacketsScanned{0};
    int nonCsigPacketsSkipped{0};
    int undecappedCopiesSeen{0};
    int undecappedSignaturesMatched{0};
    int undecappedSignaturesMismatched{0};
  };

  // Egress loopback trap punts may prepend metadata before the real L2 header.
  // Observed: [00 00 00 00][dst][src][00 00 00 01][ethertype|vlan|payload...]
  struct TrapL2Parse {
    folly::MacAddress dstMac;
    folly::MacAddress srcMac;
    uint16_t etherType{0};
    std::vector<VlanTag> vlanTags;
    size_t payloadOffset{0};

    folly::io::Cursor payloadCursor(const folly::IOBuf* frame) const {
      folly::io::Cursor cursor(frame);
      cursor.skip(payloadOffset);
      return cursor;
    }
  };

  static bool isTrapLeadingPuntPrefix(const uint8_t* bytes) {
    return bytes[0] == 0 && bytes[1] == 0 && bytes[2] == 0 && bytes[3] == 0;
  }

  static bool isTrapPostMacMeta(const uint8_t* bytes) {
    return bytes[0] == 0 && bytes[1] == 0 && bytes[2] == 0 && bytes[3] == 1;
  }

  std::optional<TrapL2Parse> parseTrapFrameL2(const folly::IOBuf* frame) const {
    if (frame == nullptr) {
      return std::nullopt;
    }
    TrapL2Parse result;
    size_t leadingSkip = 0;
    if (frame->length() >= 4) {
      uint8_t lead[4];
      folly::io::Cursor leadCursor(frame);
      leadCursor.pull(lead, 4);
      if (isTrapLeadingPuntPrefix(lead)) {
        leadingSkip = 4;
      }
    }

    folly::io::Cursor macCursor(frame);
    macCursor.skip(leadingSkip);
    if (macCursor.length() < 12) {
      return std::nullopt;
    }
    uint8_t macBuf[12];
    macCursor.pull(macBuf, 12);
    result.dstMac = folly::MacAddress::fromBinary(
        folly::ByteRange(&macBuf[0], folly::MacAddress::SIZE));
    result.srcMac = folly::MacAddress::fromBinary(
        folly::ByteRange(&macBuf[6], folly::MacAddress::SIZE));

    folly::io::Cursor ethCursor(frame);
    ethCursor.skip(leadingSkip + 12);
    // Post-MAC trap metadata (00 00 00 01) appears only on some punt formats
    // without the leading 4-byte prefix. When the leading prefix is present,
    // bytes 10-15 are the full src MAC (often ending in ...00:01) and must
    // not be skipped.
    if (leadingSkip == 0 && ethCursor.length() >= 4) {
      uint8_t meta[4];
      folly::io::Cursor metaCursor(frame);
      metaCursor.skip(leadingSkip + 12);
      metaCursor.pull(meta, 4);
      if (isTrapPostMacMeta(meta)) {
        ethCursor.skip(4);
      }
    }
    if (ethCursor.length() < 2) {
      return std::nullopt;
    }

    result.etherType = ethCursor.readBE<uint16_t>();
    while (
        result.etherType == static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_VLAN) ||
        result.etherType == static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_QINQ)) {
      if (ethCursor.length() < 4) {
        return std::nullopt;
      }
      uint8_t vtag[4];
      ethCursor.pull(vtag, 4);
      uint32_t tag = (static_cast<uint32_t>(result.etherType) << 16) |
          (static_cast<uint32_t>(vtag[0]) << 8) |
          static_cast<uint32_t>(vtag[1]);
      result.vlanTags.emplace_back(tag);
      result.etherType = (static_cast<uint16_t>(vtag[2]) << 8) |
          static_cast<uint16_t>(vtag[3]);
    }

    result.payloadOffset = frame->length() - ethCursor.length();
    return result;
  }

  std::optional<uint16_t> peekTrapFrameL2EtherType(
      const folly::IOBuf* frame) const {
    auto parsed = parseTrapFrameL2(frame);
    if (!parsed.has_value()) {
      return std::nullopt;
    }
    return parsed->etherType;
  }

  bool trapFrameHasEmbeddedPassthroughCsig(const folly::IOBuf* frame) const {
    auto parsed = parseTrapFrameL2(frame);
    if (!parsed.has_value() ||
        parsed->etherType != static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV6)) {
      return false;
    }
    try {
      folly::io::Cursor cursor = parsed->payloadCursor(frame);
      IPv6Hdr outerHdr(cursor);
      folly::io::Cursor payloadStart = cursor;
      return findIngressCsigPayloadOffset(payloadStart).has_value();
    } catch (const std::exception&) {
      return false;
    }
  }

  bool isCsigTaggedTrapFrame(const folly::IOBuf* frame) const {
    auto etherType = peekTrapFrameL2EtherType(frame);
    if (!etherType.has_value()) {
      return false;
    }
    if (*etherType == utility::kCsigTpid) {
      return true;
    }
    // SRv6 encap egress loopback traps deliver outer IPv6 at L2 with the
    // ingress CSIG shim embedded inside the SRv6 payload.
    if (*etherType == static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV6)) {
      return trapFrameHasEmbeddedPassthroughCsig(frame);
    }
    return false;
  }

  // SRv6 encap egress loopback traps may deliver outer IPv6 at L2 when CSIG was
  // not copied to the new outer L2 (HW bug under test).
  bool isCsigTaggedAtL2TrapFrame(const folly::IOBuf* frame) const {
    auto etherType = peekTrapFrameL2EtherType(frame);
    return etherType.has_value() && *etherType == utility::kCsigTpid;
  }

  void logTrapFrameDetails(
      const folly::IOBuf* frame,
      const std::string& label,
      int copyIndex,
      const std::string& reason) const {
    const bool isMatch = reason.find("match") != std::string::npos;
    // Congestion loopback re-traps the same probe (hop_limit=64) many times.
    if (!isMatch && copyIndex > 1) {
      return;
    }
    if (frame == nullptr) {
      XLOG(INFO) << label << " trap copy #" << copyIndex << " " << reason
                 << ": null frame";
      return;
    }
    if (!isMatch) {
      std::string csigSummary;
      try {
        auto parsed = parseTrapFrameL2(frame);
        if (parsed.has_value() && parsed->etherType == utility::kCsigTpid) {
          folly::io::Cursor payloadCursor = parsed->payloadCursor(frame);
          auto csigTag = utility::parseCsigTag(payloadCursor);
          if (csigTag.has_value()) {
            csigSummary = folly::sformat(
                " CSIG signal=0x{:02x} lm=0x{:02x} next_ethertype=0x{:04x}",
                csigTag->signal,
                csigTag->locatorMetadata,
                csigTag->nextEtherType);
          }
        }
        auto outerDst = peekTrapFrameOuterIpv6Dst(frame);
        if (outerDst.has_value()) {
          csigSummary += " outer_ipv6_dst=" + outerDst->str();
        }
      } catch (const std::exception&) {
      }
      XLOG(INFO) << label << " trap copy #" << copyIndex << " " << reason
                 << " len=" << frame->length() << csigSummary
                 << " (later trap copies suppressed)";
      return;
    }

    XLOG(INFO) << label << " trap copy #" << copyIndex << " " << reason
               << " len=" << frame->length();
    try {
      auto parsed = parseTrapFrameL2(frame);
      if (!parsed.has_value()) {
        XLOG(INFO) << label << " trap copy #" << copyIndex
                   << " trap L2 parse failed";
      } else {
        XLOG(INFO) << label << " trap copy #" << copyIndex
                   << " L2 dst=" << parsed->dstMac.toString()
                   << " src=" << parsed->srcMac.toString() << " ethertype=0x"
                   << std::hex << parsed->etherType << std::dec
                   << " vlan_tags=" << parsed->vlanTags.size();
        for (size_t i = 0; i < parsed->vlanTags.size(); ++i) {
          XLOG(INFO) << label << " trap copy #" << copyIndex << " vlan[" << i
                     << "] tpid=0x" << std::hex << parsed->vlanTags[i].tpid()
                     << " vid=" << std::dec << parsed->vlanTags[i].vid();
        }

        folly::io::Cursor payloadCursor = parsed->payloadCursor(frame);
        if (parsed->etherType == utility::kCsigTpid) {
          auto csigTag = utility::parseCsigTag(payloadCursor);
          if (csigTag.has_value()) {
            XLOG(INFO) << label << " trap copy #" << copyIndex
                       << " CSIG signal_type=" << +csigTag->signalType
                       << " signal=0x" << std::hex << +csigTag->signal
                       << " locator=0x" << +csigTag->locatorMetadata
                       << " next_ethertype=0x" << csigTag->nextEtherType
                       << std::dec;
            if (csigTag->nextEtherType ==
                static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV4)) {
              utility::IPPacket<folly::IPAddressV4> v4(payloadCursor);
              XLOG(INFO) << label << " trap copy #" << copyIndex
                         << " IPv4 dst=" << v4.header().dstAddr.str()
                         << " ttl=" << +v4.header().ttl;
            } else if (
                csigTag->nextEtherType ==
                static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV6)) {
              utility::IPPacket<folly::IPAddressV6> v6(payloadCursor);
              XLOG(INFO) << label << " trap copy #" << copyIndex
                         << " IPv6 dst=" << v6.header().dstAddr.str()
                         << " hop_limit=" << +v6.header().hopLimit;
            }
          }
        } else if (
            parsed->etherType ==
            static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV4)) {
          utility::IPPacket<folly::IPAddressV4> v4(payloadCursor);
          XLOG(INFO) << label << " trap copy #" << copyIndex
                     << " IPv4 dst=" << v4.header().dstAddr.str()
                     << " ttl=" << +v4.header().ttl;
        } else if (
            parsed->etherType ==
            static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV6)) {
          folly::io::Cursor v6Cursor = payloadCursor;
          try {
            IPv6Hdr outerHdr(v6Cursor);
            XLOG(INFO) << label << " trap copy #" << copyIndex
                       << " IPv6 dst=" << outerHdr.dstAddr.str()
                       << " hop_limit=" << +outerHdr.hopLimit
                       << " next_header=0x" << std::hex << +outerHdr.nextHeader
                       << std::dec;
            if (trapFrameHasEmbeddedPassthroughCsig(frame)) {
              XLOG(INFO) << label << " trap copy #" << copyIndex
                         << " embedded passthrough CSIG shim in SRv6 payload";
            } else if (
                outerHdr.dstAddr == kEncapRouteDstIp &&
                outerHdr.nextHeader ==
                    static_cast<uint8_t>(IP_PROTO::IP_PROTO_UDP)) {
              // Post-decap strip: plain routed IPv6 + UDP, not nested SRv6.
              XLOG(INFO) << label << " trap copy #" << copyIndex
                         << " strip-like routed IPv6 dst=" << kEncapRouteDstIp;
            } else if (
                outerHdr.nextHeader ==
                    static_cast<uint8_t>(IP_PROTO::IP_PROTO_IPV6) ||
                outerHdr.nextHeader ==
                    static_cast<uint8_t>(IP_PROTO::IP_PROTO_IPV6_ROUTE) ||
                outerHdr.nextHeader ==
                    static_cast<uint8_t>(IP_PROTO::IP_PROTO_IPV6_HOPOPT) ||
                outerHdr.nextHeader ==
                    static_cast<uint8_t>(IP_PROTO::IP_PROTO_GRE)) {
              XLOG(INFO) << label << " trap copy #" << copyIndex
                         << " SRv6-style outer IPv6 next_header=0x" << std::hex
                         << +outerHdr.nextHeader << std::dec
                         << " outer_dst=" << outerHdr.dstAddr.str();
            }
          } catch (const std::exception& ex) {
            XLOG(INFO) << label << " trap copy #" << copyIndex
                       << " outer IPv6 parse failed: " << ex.what();
          }
        }
      }
    } catch (const std::exception& ex) {
      XLOG(INFO) << label << " trap copy #" << copyIndex
                 << " header parse failed: " << ex.what();
    }
    XLOG(INFO) << label << " trap copy #" << copyIndex << " hex:\n"
               << PktUtil::hexDump(frame);
  }

  void logNonCsigTrapFrame(
      const folly::IOBuf* frame,
      const std::string& label,
      int copyIndex) const {
    if (copyIndex > 1) {
      return;
    }
    logTrapFrameDetails(frame, label, copyIndex, "non-CSIG skipped");
  }

  void logFrameEtherType(const folly::IOBuf* frame, const std::string& label)
      const {
    if (frame == nullptr) {
      return;
    }
    folly::io::Cursor cursor(frame);
    EthHdr ethHdr(cursor);
    XLOG(INFO) << label << " L2 ethertype=0x" << std::hex << ethHdr.etherType
               << std::dec << " len=" << frame->length()
               << " vlan_tags=" << ethHdr.vlanTags.size();
    for (size_t i = 0; i < ethHdr.vlanTags.size(); ++i) {
      XLOG(INFO) << label << " vlan[" << i << "] tpid=0x" << std::hex
                 << ethHdr.vlanTags[i].tpid() << " vid=" << std::dec
                 << ethHdr.vlanTags[i].vid();
    }
  }

  // Label for the next CPU-switched CSIG inject (set by each verify* step).
  std::string csigInjectLogLabel_{"CSIG inject"};

  void logCsigInjectPacketDetails(
      const std::string& label,
      const TxPacket* pkt,
      const char* injectPath) const {
    if (pkt == nullptr || pkt->buf() == nullptr) {
      XLOG(INFO) << label << " inject: null packet path=" << injectPath;
      return;
    }
    const auto* frame = pkt->buf();
    const auto configuredVlan = csigRoutedInjectVlan();
    XLOG(INFO) << label << " inject: path=" << injectPath << " configured_vlan="
               << (configuredVlan.has_value()
                       ? folly::sformat(
                             "vid={}", static_cast<uint16_t>(*configuredVlan))
                       : "none (untagged CSIG L2)")
               << " len=" << frame->length();

    try {
      folly::io::Cursor cursor(frame);
      EthHdr ethHdr(cursor);
      XLOG(INFO) << label << " inject: L2 dst=" << ethHdr.dstAddr.toString()
                 << " src=" << ethHdr.srcAddr.toString()
                 << " vlan_tags_on_wire=" << ethHdr.vlanTags.size()
                 << " ethertype=0x" << std::hex << ethHdr.etherType << std::dec;
      if (!ethHdr.vlanTags.empty()) {
        for (size_t i = 0; i < ethHdr.vlanTags.size(); ++i) {
          XLOG(ERR) << label << " inject: UNEXPECTED vlan[" << i << "] on wire "
                    << "tpid=0x" << std::hex << ethHdr.vlanTags[i].tpid()
                    << " vid=" << std::dec << ethHdr.vlanTags[i].vid()
                    << " (CSIG inject must be untagged on Yuba/G202X; "
                    << "configured_vlan="
                    << (configuredVlan.has_value() ? "set" : "none") << ")";
        }
      } else if (configuredVlan.has_value()) {
        XLOG(ERR) << label << " inject: configured_vlan vid="
                  << static_cast<uint16_t>(*configuredVlan)
                  << " but wire frame has no VLAN tag (builder mismatch)";
      }

      folly::io::Cursor payloadCursor(frame);
      payloadCursor.skip(ethHdr.size());
      if (ethHdr.etherType == utility::kCsigTpid) {
        auto csigTag = utility::parseCsigTag(payloadCursor);
        if (csigTag.has_value()) {
          XLOG(INFO) << label
                     << " inject: CSIG signal_type=" << +csigTag->signalType
                     << " signal=0x" << std::hex << +csigTag->signal << " lm=0x"
                     << +csigTag->locatorMetadata << " next_ethertype=0x"
                     << csigTag->nextEtherType << std::dec;
          if (csigTag->nextEtherType ==
              static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV6)) {
            utility::IPPacket<folly::IPAddressV6> v6(payloadCursor);
            XLOG(INFO) << label
                       << " inject: IPv6 src=" << v6.header().srcAddr.str()
                       << " dst=" << v6.header().dstAddr.str()
                       << " hop_limit=" << +v6.header().hopLimit
                       << " next_header=0x" << std::hex
                       << +v6.header().nextHeader << std::dec;
            if (v6.header().nextHeader ==
                static_cast<uint8_t>(IP_PROTO::IP_PROTO_IPV6)) {
              folly::io::Cursor innerCursor = payloadCursor;
              innerCursor.skip(IPv6Hdr::SIZE);
              try {
                IPv6Hdr innerHdr(innerCursor);
                XLOG(INFO) << label << " inject: inner-IPv6 dst="
                           << innerHdr.dstAddr.str()
                           << " hop_limit=" << +innerHdr.hopLimit;
              } catch (const std::exception&) {
              }
            }
          } else if (
              csigTag->nextEtherType ==
              static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV4)) {
            utility::IPPacket<folly::IPAddressV4> v4(payloadCursor);
            XLOG(INFO) << label
                       << " inject: IPv4 src=" << v4.header().srcAddr.str()
                       << " dst=" << v4.header().dstAddr.str()
                       << " ttl=" << +v4.header().ttl;
          }
        }
      } else if (
          ethHdr.etherType ==
          static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV6)) {
        IPv6Hdr v6Hdr(payloadCursor);
        XLOG(INFO) << label
                   << " inject: plain-IPv6-at-L2 dst=" << v6Hdr.dstAddr.str()
                   << " hop_limit=" << +v6Hdr.hopLimit;
      }
    } catch (const std::exception& ex) {
      XLOG(INFO) << label << " inject: header parse failed: " << ex.what();
    }

    XLOG(INFO) << label << " inject hex:\n" << PktUtil::hexDump(frame);
  }

  void sendCsigInjectSwitched(
      const std::string& label,
      std::unique_ptr<TxPacket> txPacket) {
    logCsigInjectPacketDetails(label, txPacket.get(), "CPU-switched");
    this->sendPacketSwitchedAsync(std::move(txPacket));
  }

  void sendCsigInjectOutOfPort(
      const std::string& label,
      std::unique_ptr<TxPacket> txPacket,
      PortID port,
      bool logDetails = true) {
    if (logDetails) {
      logCsigInjectPacketDetails(label, txPacket.get(), "out-of-port");
    }
    this->getSw()->sendPacketOutOfPortAsync(std::move(txPacket), port);
  }

  std::optional<folly::IPAddressV6> peekCsigTrapOuterIpv6Dst(
      const folly::IOBuf* frame) const {
    if (frame == nullptr) {
      return std::nullopt;
    }
    auto parsed = parseTrapFrameL2(frame);
    if (!parsed.has_value() || parsed->etherType != utility::kCsigTpid) {
      return std::nullopt;
    }
    folly::io::Cursor cursor = parsed->payloadCursor(frame);
    auto csigTag = utility::parseCsigTag(cursor);
    if (!csigTag.has_value()) {
      return std::nullopt;
    }
    if (csigTag->nextEtherType !=
        static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV6)) {
      return std::nullopt;
    }
    try {
      utility::IPPacket<folly::IPAddressV6> outerV6(cursor);
      return outerV6.header().dstAddr;
    } catch (const std::exception&) {
      return std::nullopt;
    }
  }

  // Outer SRv6 IPv6 dst from egress loopback trap: L2 CSIG (UPDATE) or bare L2
  // IPv6 (encap without outer CSIG / embedded ingress shim in payload).
  std::optional<folly::IPAddressV6> peekTrapFrameOuterIpv6Dst(
      const folly::IOBuf* frame) const {
    if (frame == nullptr) {
      return std::nullopt;
    }
    auto parsed = parseTrapFrameL2(frame);
    if (!parsed.has_value()) {
      return std::nullopt;
    }
    if (parsed->etherType == utility::kCsigTpid) {
      return peekCsigTrapOuterIpv6Dst(frame);
    }
    if (parsed->etherType != static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV6)) {
      return std::nullopt;
    }
    try {
      folly::io::Cursor cursor = parsed->payloadCursor(frame);
      IPv6Hdr outerHdr(cursor);
      return outerHdr.dstAddr;
    } catch (const std::exception&) {
      return std::nullopt;
    }
  }

  // SDK tgen background: plain routed flood (no CSIG) to the egress NH.
  bool isPlainRoutedFloodTrapFrame(
      const folly::IOBuf* frame,
      const folly::IPAddress& floodRoutedDst,
      uint8_t floodHopLimit) const {
    if (isCsigTaggedTrapFrame(frame)) {
      return false;
    }
    auto parsed = parseTrapFrameL2(frame);
    if (!parsed.has_value() ||
        parsed->etherType != static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV6)) {
      return false;
    }
    try {
      folly::io::Cursor cursor = parsed->payloadCursor(frame);
      IPv6Hdr hdr(cursor);
      return hdr.dstAddr.str() == floodRoutedDst.str() &&
          hdr.hopLimit == floodHopLimit;
    } catch (const std::exception&) {
      return false;
    }
  }

  // Encap UPDATE under congestion: flood traffic to 1::1 also hits the egress
  // trap with outer L2 UPDATE CSIG. Skip those copies so the scan budget is
  // reserved for SRv6 encap probes (outer IPv6 dst == SID).
  template <typename ValidateFn>
  CsigTrapScanStats scanCsigSrv6EncapUpdateTrapSnooper(
      utility::SwSwitchPacketSnooper& trapSnooper,
      const std::string& label,
      const folly::IPAddressV6& expectedSid,
      ValidateFn&& validateFn,
      const folly::IPAddress* skipPlainFloodRoutedDst = nullptr,
      uint8_t skipPlainFloodHopLimit = kTgenFloodTtl) const {
    CsigTrapScanStats stats;
    int floodTrapsSkipped = 0;
    int plainFloodTrapsSkipped = 0;
    bool loggedSkippedCsigSample = false;
    while (!stats.foundMatch &&
           stats.trappedPacketsScanned < kMaxMirroredPacketsToScan) {
      auto frameRx = trapSnooper.waitForPacket(1);
      if (!frameRx.has_value()) {
        break;
      }
      if (!isCsigTaggedTrapFrame(frameRx->get())) {
        if (skipPlainFloodRoutedDst != nullptr &&
            isPlainRoutedFloodTrapFrame(
                frameRx->get(),
                *skipPlainFloodRoutedDst,
                skipPlainFloodHopLimit)) {
          ++plainFloodTrapsSkipped;
          continue;
        }
        auto outerDst = peekTrapFrameOuterIpv6Dst(frameRx->get());
        if (outerDst.has_value() && *outerDst == expectedSid) {
          ++stats.trappedPacketsScanned;
          const int copyIndex = stats.trappedPacketsScanned;
          ++stats.csigPacketsScanned;
          if (validateFn(frameRx->get())) {
            stats.foundMatch = true;
            logTrapFrameDetails(
                frameRx->get(), label, copyIndex, "CSIG encap UPDATE match");
          } else {
            logTrapFrameDetails(
                frameRx->get(),
                label,
                copyIndex,
                "CSIG encap UPDATE validation failed");
          }
          continue;
        }
        ++stats.trappedPacketsScanned;
        ++stats.nonCsigPacketsSkipped;
        logNonCsigTrapFrame(frameRx->get(), label, stats.trappedPacketsScanned);
        continue;
      }
      auto outerDst = peekTrapFrameOuterIpv6Dst(frameRx->get());
      if (!outerDst.has_value() || *outerDst != expectedSid) {
        ++floodTrapsSkipped;
        if (!loggedSkippedCsigSample) {
          loggedSkippedCsigSample = true;
          const auto skipReason = !outerDst.has_value()
              ? "no outer IPv6"
              : (outerDst->str() == kEncapRouteDstIp.str()
                     ? "pre-encap probe (encap dst, not SID yet)"
                     : "congestion flood or non-probe CSIG");
          XLOG(INFO) << label << " trap snooper: sample skipped CSIG copy "
                     << "outer_ipv6_dst="
                     << (outerDst.has_value() ? outerDst->str() : "?")
                     << " expected_sid=" << expectedSid << " (" << skipReason
                     << ")";
          logTrapFrameDetails(
              frameRx->get(),
              label,
              floodTrapsSkipped,
              "skipped CSIG copy (outer dst != SID)");
        }
        continue;
      }
      ++stats.trappedPacketsScanned;
      const int copyIndex = stats.trappedPacketsScanned;
      ++stats.csigPacketsScanned;
      if (validateFn(frameRx->get())) {
        stats.foundMatch = true;
        logTrapFrameDetails(
            frameRx->get(), label, copyIndex, "CSIG encap UPDATE match");
      } else {
        logTrapFrameDetails(
            frameRx->get(),
            label,
            copyIndex,
            "CSIG encap UPDATE validation failed");
      }
    }
    if (plainFloodTrapsSkipped > 0) {
      XLOG(INFO) << label << " trap snooper: skipped " << plainFloodTrapsSkipped
                 << " plain routed congestion-flood copies (dst "
                 << (skipPlainFloodRoutedDst != nullptr
                         ? skipPlainFloodRoutedDst->str()
                         : "?")
                 << " hop_limit=" << +skipPlainFloodHopLimit
                 << ", no scan budget consumed)";
    }
    if (floodTrapsSkipped > 0) {
      XLOG(INFO) << label << " trap snooper: skipped " << floodTrapsSkipped
                 << " CSIG congestion-flood copies (outer IPv6 dst != SID "
                 << expectedSid << ")";
    }
    if (stats.csigPacketsScanned == 0) {
      XLOG(INFO) << label << " trap snooper: no encap-probe CSIG frames "
                 << "(outer dst " << expectedSid << ") among "
                 << stats.trappedPacketsScanned << " trapped copies ("
                 << stats.nonCsigPacketsSkipped << " non-CSIG skipped, "
                 << floodTrapsSkipped << " flood CSIG skipped, "
                 << plainFloodTrapsSkipped << " plain flood skipped)";
    }
    return stats;
  }

  // Decap UPDATE under congestion: flood traffic to the routed NH also hits
  // the egress trap with outer L2 UPDATE CSIG (dst != decap inner). Skip those
  // copies and undecapped SRv6 traps (outer dst == mySid) without consuming the
  // scan budget reserved for decap-probe frames (IPv6 dst == inner dst).
  template <typename ValidateFn>
  CsigTrapScanStats scanCsigDecapUpdateTrapSnooper(
      utility::SwSwitchPacketSnooper& trapSnooper,
      const std::string& label,
      const folly::IPAddress& expectedInnerDst,
      ValidateFn&& validateFn) const {
    CsigTrapScanStats stats;
    int nonProbeTrapsSkipped = 0;
    while (!stats.foundMatch &&
           stats.trappedPacketsScanned < kMaxMirroredPacketsToScan) {
      auto frameRx = trapSnooper.waitForPacket(1);
      if (!frameRx.has_value()) {
        break;
      }
      if (!isCsigTaggedTrapFrame(frameRx->get())) {
        ++stats.trappedPacketsScanned;
        ++stats.nonCsigPacketsSkipped;
        logNonCsigTrapFrame(frameRx->get(), label, stats.trappedPacketsScanned);
        continue;
      }
      auto outerDst = peekCsigTrapOuterIpv6Dst(frameRx->get());
      if (!outerDst.has_value() || outerDst->str() != expectedInnerDst.str()) {
        ++nonProbeTrapsSkipped;
        continue;
      }
      ++stats.trappedPacketsScanned;
      const int copyIndex = stats.trappedPacketsScanned;
      ++stats.csigPacketsScanned;
      if (validateFn(frameRx->get())) {
        stats.foundMatch = true;
        logTrapFrameDetails(
            frameRx->get(), label, copyIndex, "CSIG decap UPDATE match");
      } else {
        logTrapFrameDetails(
            frameRx->get(),
            label,
            copyIndex,
            "CSIG decap UPDATE validation failed");
      }
    }
    if (nonProbeTrapsSkipped > 0) {
      XLOG(INFO) << label << " trap snooper: skipped " << nonProbeTrapsSkipped
                 << " CSIG non-decap-probe copies (IPv6 dst != inner dst "
                 << expectedInnerDst << ", includes congestion flood and "
                 << "undecapped mySid " << kDecapMySidAddr << ")";
    }
    if (stats.csigPacketsScanned == 0) {
      XLOG(INFO) << label << " trap snooper: no decap-probe CSIG frames "
                 << "(IPv6 dst " << expectedInnerDst << ") among "
                 << stats.trappedPacketsScanned << " trapped copies ("
                 << stats.nonCsigPacketsSkipped << " non-CSIG skipped, "
                 << nonProbeTrapsSkipped << " non-probe CSIG skipped)";
    }
    return stats;
  }

  template <typename ValidateFn>
  CsigTrapScanStats scanCsigTrapSnooper(
      utility::SwSwitchPacketSnooper& trapSnooper,
      const std::string& label,
      ValidateFn&& validateFn) const {
    CsigTrapScanStats stats;
    while (!stats.foundMatch &&
           stats.trappedPacketsScanned < kMaxMirroredPacketsToScan) {
      auto frameRx = trapSnooper.waitForPacket(1);
      if (!frameRx.has_value()) {
        break;
      }
      ++stats.trappedPacketsScanned;
      const int copyIndex = stats.trappedPacketsScanned;
      if (!isCsigTaggedTrapFrame(frameRx->get())) {
        ++stats.nonCsigPacketsSkipped;
        logNonCsigTrapFrame(frameRx->get(), label, copyIndex);
        continue;
      }
      ++stats.csigPacketsScanned;
      if (validateFn(frameRx->get())) {
        stats.foundMatch = true;
        logTrapFrameDetails(frameRx->get(), label, copyIndex, "CSIG match");
      } else {
        logTrapFrameDetails(
            frameRx->get(), label, copyIndex, "CSIG validation failed");
      }
    }
    if (stats.csigPacketsScanned == 0) {
      XLOG(INFO) << label << " trap snooper: no CSIG-tagged (0x9900) frames "
                 << "among " << stats.trappedPacketsScanned
                 << " trapped copies (" << stats.nonCsigPacketsSkipped
                 << " non-CSIG skipped)";
    }
    return stats;
  }

  // STRIP egress removes CSIG before the wire; validate every trapped copy.
  template <typename ValidateFn>
  CsigTrapScanStats scanStripTrapSnooper(
      utility::SwSwitchPacketSnooper& trapSnooper,
      const std::string& label,
      ValidateFn&& validateFn) const {
    CsigTrapScanStats stats;
    while (!stats.foundMatch &&
           stats.trappedPacketsScanned < kMaxMirroredPacketsToScan) {
      auto frameRx = trapSnooper.waitForPacket(1);
      if (!frameRx.has_value()) {
        break;
      }
      ++stats.trappedPacketsScanned;
      const int copyIndex = stats.trappedPacketsScanned;
      if (isCsigTaggedAtL2TrapFrame(frameRx->get())) {
        ++stats.csigPacketsScanned;
        logTrapFrameDetails(
            frameRx->get(), label, copyIndex, "L2 CSIG still present");
        continue;
      }
      if (trapFrameHasEmbeddedPassthroughCsig(frameRx->get())) {
        logTrapFrameDetails(
            frameRx->get(),
            label,
            copyIndex,
            "passthrough-like embedded CSIG (not strip)");
      }
      if (validateFn(frameRx->get())) {
        stats.foundMatch = true;
        logTrapFrameDetails(frameRx->get(), label, copyIndex, "strip match");
      } else {
        logTrapFrameDetails(
            frameRx->get(), label, copyIndex, "strip validation failed");
      }
    }
    if (!stats.foundMatch) {
      if (stats.trappedPacketsScanned == 0) {
        XLOG(INFO) << label << " trap snooper: no trapped copies";
      } else {
        XLOG(INFO) << label << " trap snooper: no strip match among "
                   << stats.trappedPacketsScanned << " trapped copies ("
                   << stats.csigPacketsScanned << " still L2 CSIG-tagged)";
      }
    }
    return stats;
  }

  template <typename ValidateFn>
  CsigTrapScanStats scanDecapStripTrapSnooper(
      utility::SwSwitchPacketSnooper& trapSnooper,
      const std::string& label,
      ValidateFn&& validateFn) const {
    CsigTrapScanStats stats;
    while (!stats.foundMatch &&
           stats.trappedPacketsScanned < kMaxMirroredPacketsToScan) {
      auto frameRx = trapSnooper.waitForPacket(1);
      if (!frameRx.has_value()) {
        break;
      }
      ++stats.trappedPacketsScanned;
      const int copyIndex = stats.trappedPacketsScanned;
      if (isCsigTaggedAtL2TrapFrame(frameRx->get())) {
        ++stats.csigPacketsScanned;
        logTrapFrameDetails(
            frameRx->get(), label, copyIndex, "L2 CSIG still present");
        continue;
      }
      if (trapFrameHasUndecappedSrv6Outer(frameRx->get())) {
        ++stats.undecappedCopiesSeen;
        auto analysis = analyzeUndecappedSrv6TrapFrame(frameRx->get());
        if (analysis.has_value()) {
          const bool signatureMatched =
              undecappedSignatureMatchesOuterL2OnlyInject(*analysis);
          if (signatureMatched) {
            ++stats.undecappedSignaturesMatched;
          } else {
            ++stats.undecappedSignaturesMismatched;
          }
          logUndecappedSrv6TrapAnalysis(
              label, copyIndex, *analysis, signatureMatched);
        }
        logTrapFrameDetails(
            frameRx->get(),
            label,
            copyIndex,
            "undecapped SRv6 outer (pre-strip)");
      } else if (trapFrameHasEmbeddedPassthroughCsig(frameRx->get())) {
        logTrapFrameDetails(
            frameRx->get(),
            label,
            copyIndex,
            "passthrough-like embedded CSIG (not strip)");
      }
      if (validateFn(frameRx->get())) {
        stats.foundMatch = true;
        logTrapFrameDetails(frameRx->get(), label, copyIndex, "strip match");
      } else {
        logTrapFrameDetails(
            frameRx->get(), label, copyIndex, "strip validation failed");
      }
    }
    if (!stats.foundMatch) {
      if (stats.trappedPacketsScanned == 0) {
        XLOG(INFO) << label << " trap snooper: no trapped copies";
      } else {
        XLOG(INFO) << label << " trap snooper: no strip match among "
                   << stats.trappedPacketsScanned << " trapped copies ("
                   << stats.csigPacketsScanned << " still L2 CSIG-tagged, "
                   << stats.undecappedCopiesSeen << " undecapped SRv6)";
      }
    }
    return stats;
  }

  void logCsigTrapScanSummary(
      const std::string& label,
      const CsigTrapScanStats& stats,
      bool foundMatch) const {
    XLOG(INFO) << label << " trap snooper summary: egress_trap_csig="
               << (foundMatch ? "yes" : "no")
               << " trapped_frames_scanned=" << stats.trappedPacketsScanned
               << " csig_frames_scanned=" << stats.csigPacketsScanned
               << " non_csig_skipped=" << stats.nonCsigPacketsSkipped
               << " undecapped_copies=" << stats.undecappedCopiesSeen
               << " undecapped_step_signature_match="
               << stats.undecappedSignaturesMatched
               << " undecapped_step_signature_mismatch="
               << stats.undecappedSignaturesMismatched;
  }

  // Dynamic CSIG ABWC comes from egress port TX-rate instrumentation (ARC
  // poll_csig), not TM queue depth. Keep TX enabled and saturate egress arate.
  int csigCongestionFloodPacketCount(PortID egressPort) const {
    const auto minPktsForLineRate =
        getAgentEnsemble()->getMinPktsForLineRate(egressPort);
    return std::max(
        kCongestionFloodPacketCount, static_cast<int>(minPktsForLineRate));
  }

  void waitForCsigEgressLineRate(PortID egressPort) {
    XLOG(INFO) << "CSIG waiting for egress line rate on port " << egressPort;
    getAgentEnsemble()->waitForLineRateOnPort(egressPort);
  }

  // ECMP helper with router MAC as next-hop MAC so MAC-loopback recycled
  // packets keep hitting the L3 pipeline (same pattern as AgentCoppTests).
  utility::EcmpSetupAnyNPorts6 makeCsigEgressEcmpHelper() const {
    auto intfMac =
        getMacForFirstInterfaceWithPortsForTesting(this->getProgrammedState());
    return utility::EcmpSetupAnyNPorts6(
        this->getProgrammedState(),
        this->getSw()->needL2EntryForNeighbor(),
        intfMac);
  }

  boost::container::flat_set<PortDescriptor> egressPortDescriptors(
      PortID egressPort) const {
    auto ecmpHelper = makeCsigEgressEcmpHelper();
    boost::container::flat_set<PortDescriptor> portDescs;
    for (int i = 0; i < kNumNextHops; ++i) {
      if (getEgressPort(ecmpHelper.nhop(i).portDesc) == egressPort) {
        portDescs.insert(ecmpHelper.nhop(i).portDesc);
        break;
      }
    }
    return portDescs;
  }

  // Routed L3 dataplane loop on the egress NH port (TTL decrement disabled).
  // Dynamic CSIG ABWC tracks egress TX rate; a recycling routed flood sustains
  // line rate on MAC loopback where a one-shot CPU burst cannot.
  void setupCsigEgressDataplaneLoop(PortID egressPort) {
    resolveGlobalNeighborOnEgressPort(egressPort);

    auto ecmpHelper = makeCsigEgressEcmpHelper();
    auto portDescs = egressPortDescriptors(egressPort);
    CHECK(!portDescs.empty()) << "No ECMP nhop for egress port " << egressPort;

    auto routedDstIp = folly::IPAddress(ecmpHelper.ip(*portDescs.begin()));
    typename utility::EcmpSetupAnyNPorts6::RouteT hostPrefix(
        routedDstIp.asV6(), 128);
    auto routeUpdater = this->getSw()->getRouteUpdater();
    ecmpHelper.programRoutes(
        &routeUpdater,
        portDescs,
        {hostPrefix},
        {},
        true /* disableTTLDecrement */);

    for (int i = 0; i < kNumNextHops; ++i) {
      if (getEgressPort(ecmpHelper.nhop(i).portDesc) == egressPort) {
        utility::disablePortTTLDecrementIfSupported(
            getAgentEnsemble(), ecmpHelper.getRouterId(), ecmpHelper.nhop(i));
        break;
      }
    }
  }

  void buildCsigEgressLineRateCongestion(PortID egressPort) {
    setupCsigEgressDataplaneLoop(egressPort);
    auto routedDstIp = getRoutedDstIpForEgressPort(egressPort);
    auto intfMac =
        getMacForFirstInterfaceWithPortsForTesting(this->getProgrammedState());
    auto tcField = static_cast<uint8_t>((kFloodDscp << 2) | kECT1);

    const auto minPktsForLineRate = csigCongestionFloodPacketCount(egressPort);
    XLOG(INFO) << "CSIG building egress dataplane-loop congestion on port "
               << egressPort << " routed_dst=" << routedDstIp
               << " pkts=" << (minPktsForLineRate * 4)
               << " (dual-flow, out-of-port inject on MAC loopback)";
    bool loggedFloodSample = false;
    for (int i = 0; i < minPktsForLineRate * 2; ++i) {
      for (int j = 0; j < 2; ++j) {
        auto txPacket = makeCsigRoutedTrafficPacket(
            intfMac,
            routedDstIp,
            tcField,
            std::vector<uint8_t>(7000, 0xff),
            kTgenFloodTtl);
        if (!loggedFloodSample) {
          sendCsigInjectOutOfPort(
              "CSIG egress dataplane-loop congestion flood (sample)",
              std::move(txPacket),
              egressPort);
          loggedFloodSample = true;
        } else {
          this->getSw()->sendPacketOutOfPortAsync(
              std::move(txPacket), egressPort);
        }
      }
    }
    waitForCsigEgressLineRate(egressPort);
  }

  void sendTgenStyleCrossPortFloodBurst(
      PortID floodPort,
      PortID egressPort,
      int packetCount) {
    auto routedDstIp = getRoutedDstIpForEgressPort(egressPort);
    auto intfMac =
        getMacForFirstInterfaceWithPortsForTesting(this->getProgrammedState());
    auto tcField = static_cast<uint8_t>((kFloodDscp << 2) | kECT1);

    XLOG(INFO) << "CSIG tgen-style cross-port flood on port " << floodPort
               << " routed_dst=" << routedDstIp << " pkts=" << packetCount
               << " (no CSIG shim, TTL=" << +kTgenFloodTtl << ", egress port "
               << egressPort << ")";
    for (int i = 0; i < packetCount; ++i) {
      auto txPacket = makePlainRoutedTrafficPacket(
          intfMac,
          routedDstIp,
          tcField,
          kTgenFloodTtl,
          std::vector<uint8_t>(7000, 0xff));
      if (i == 0) {
        sendCsigInjectOutOfPort(
            "CSIG tgen-style cross-port flood (sample)",
            std::move(txPacket),
            floodPort);
      } else {
        this->getSw()->sendPacketOutOfPortAsync(std::move(txPacket), floodPort);
      }
    }
  }

  int tgenStyleInitialFloodPacketCount(PortID egressPort) const {
    return csigCongestionFloodPacketCount(egressPort) * 4;
  }

  void buildTgenStyleCrossPortCongestion(PortID floodPort, PortID egressPort) {
    setupCsigEgressDataplaneLoop(egressPort);
    sendTgenStyleCrossPortFloodBurst(
        floodPort, egressPort, tgenStyleInitialFloodPacketCount(egressPort));
    waitForCsigEgressLineRate(egressPort);
  }

  // Re-flood top-up: small burst only. Do not call waitForLineRateOnPort here —
  // between WITH_RETRIES iterations egress counters are often flat and that
  // wait throws. Large refloods also punt excess traffic to the CPU (port 1)
  // and spam IPv6Handler logs.
  void refloodTgenStyleCrossPortCongestion(
      PortID floodPort,
      PortID egressPort) {
    const auto bytesBefore = *this->getLatestPortStats(egressPort).outBytes_();
    sendTgenStyleCrossPortFloodBurst(
        floodPort, egressPort, kTgenRefloodPacketCount);
    const auto bytesAfter = *this->getLatestPortStats(egressPort).outBytes_();
    XLOG(INFO) << "CSIG tgen-style re-flood done: egress port " << egressPort
               << " outBytes " << bytesBefore << " -> " << bytesAfter << " ("
               << kTgenRefloodPacketCount << " pkts)";
  }

  folly::IPAddress getRoutedDstIpForEgressPort(PortID egressPort) {
    auto ecmpHelper = makeCsigEgressEcmpHelper();
    for (int i = 0; i < kNumNextHops; ++i) {
      if (getEgressPort(ecmpHelper.nhop(i).portDesc) == egressPort) {
        return folly::IPAddress(ecmpHelper.nhop(i).ip);
      }
    }
    throw FbossError("No ECMP nhop for egress port ", egressPort);
  }

  // CSIG egress tagging applies to L3 routed packets only (not bridged).
  // Resolve the global on-link neighbor used for plain routed forwarding.
  void resolveGlobalNeighborOnEgressPort(PortID egressPort) {
    auto ecmpHelper = makeCsigEgressEcmpHelper();
    auto portDescs = egressPortDescriptors(egressPort);
    CHECK(!portDescs.empty()) << "No ECMP nhop for egress port " << egressPort;
    applyNewState([&ecmpHelper, portDescs](std::shared_ptr<SwitchState> in) {
      return ecmpHelper.resolveNextHops(in, portDescs, false /*useLinkLocal*/);
    });
  }

  void logValidatedCsigFrame(const folly::IOBuf* frame) const {
    XLOG(INFO) << "CSIG+SRv6 encap trapped packet (" << frame->length()
               << " bytes):\n"
               << PktUtil::hexDump(frame);
    auto parsed = parseTrapFrameL2(frame);
    if (!parsed.has_value()) {
      XLOG(INFO) << "CSIG+SRv6 encap trapped L2 parse failed";
      return;
    }
    XLOG(INFO) << "CSIG+SRv6 encap trapped L2 ethertype=0x" << std::hex
               << parsed->etherType << std::dec;
    if (parsed->etherType == utility::kCsigTpid) {
      folly::io::Cursor cursor = parsed->payloadCursor(frame);
      auto csigTag = utility::parseCsigTag(cursor);
      if (csigTag.has_value()) {
        XLOG(INFO) << "CSIG+SRv6 encap parsed tag: signal_type="
                   << +csigTag->signalType << " signal=0x" << std::hex
                   << +csigTag->signal << std::dec << " locator=0x" << std::hex
                   << +csigTag->locatorMetadata << std::dec
                   << " next_ethertype=0x" << std::hex << csigTag->nextEtherType
                   << std::dec;
      }
    } else if (
        parsed->etherType == static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV6)) {
      XLOG(INFO) << "CSIG+SRv6 encap trapped outer IPv6 with embedded CSIG";
    }
  }

  bool validateCsigPassthroughFrame(
      const folly::IOBuf* frame,
      const folly::IPAddress& expectedDstIp) const {
    if (frame == nullptr) {
      return false;
    }
    auto parsed = parseTrapFrameL2(frame);
    if (!parsed.has_value() || parsed->etherType != utility::kCsigTpid) {
      return false;
    }
    folly::io::Cursor cursor = parsed->payloadCursor(frame);

    auto csigTag = utility::parseCsigTag(cursor);
    if (!csigTag.has_value()) {
      return false;
    }
    if (csigTag->signalType != utility::kCsigAbwcSignalType) {
      return false;
    }
    // PASSTHROUGH: egress CSIG shim matches ingress (not egress link locator).
    if (csigTag->locatorMetadata != utility::kCsigIngressPktLinkLocator) {
      return false;
    }
    if (csigTag->signal != utility::kCsigIngressAbwcSignal) {
      return false;
    }

    if (expectedDstIp.isV4()) {
      if (csigTag->nextEtherType !=
          static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV4)) {
        return false;
      }
      utility::IPPacket<folly::IPAddressV4> v4Packet(cursor);
      auto v4Hdr = v4Packet.header();
      return v4Hdr.dstAddr == expectedDstIp.asV4() &&
          v4Hdr.ttl == kFloodTtl - 1 && v4Hdr.dscp == kFloodDscp;
    }

    if (csigTag->nextEtherType !=
        static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV6)) {
      return false;
    }
    utility::IPPacket<folly::IPAddressV6> v6Packet(cursor);
    auto v6Hdr = v6Packet.header();
    return v6Hdr.dstAddr == expectedDstIp.asV6() &&
        v6Hdr.hopLimit == kFloodTtl - 1 &&
        (v6Hdr.trafficClass >> 2) == kFloodDscp;
  }

  bool validateCsigRoutedFrame(
      const folly::IOBuf* frame,
      const folly::IPAddress& expectedDstIp) const {
    if (frame == nullptr) {
      return false;
    }
    auto parsed = parseTrapFrameL2(frame);
    if (!parsed.has_value() || parsed->etherType != utility::kCsigTpid) {
      return false;
    }
    folly::io::Cursor cursor = parsed->payloadCursor(frame);

    auto csigTag = utility::parseCsigTag(cursor);
    if (!csigTag.has_value()) {
      return false;
    }
    if (csigTag->signalType != utility::kCsigAbwcSignalType) {
      return false;
    }
    if (csigTag->locatorMetadata != utility::kDefaultCsigLinkLocator) {
      return false;
    }
    if (!utility::isEgressUpdatedAbwcSignal(csigTag->signal)) {
      return false;
    }

    if (expectedDstIp.isV4()) {
      if (csigTag->nextEtherType !=
          static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV4)) {
        return false;
      }
      utility::IPPacket<folly::IPAddressV4> v4Packet(cursor);
      auto v4Hdr = v4Packet.header();
      return v4Hdr.dstAddr == expectedDstIp.asV4() &&
          egressTrapHopLimitOk(v4Hdr.ttl) && v4Hdr.dscp == kFloodDscp;
    }

    if (csigTag->nextEtherType !=
        static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV6)) {
      return false;
    }
    utility::IPPacket<folly::IPAddressV6> v6Packet(cursor);
    auto v6Hdr = v6Packet.header();
    return v6Hdr.dstAddr == expectedDstIp.asV6() &&
        egressTrapHopLimitOk(v6Hdr.hopLimit) &&
        (v6Hdr.trafficClass >> 2) == kFloodDscp;
  }

  // SDK quiet-clamp UPDATE (csig_base / srv6_fpp_base): ingress wire 2b85
  // (logical signal=7)
  // -> egress wire 28xx (signal 0x10) with lm = programmed egress port locator
  // metadata.
  bool validateCsigQuietClampRoutedFrame(
      const folly::IOBuf* frame,
      const folly::IPAddress& expectedDstIp) const {
    if (frame == nullptr) {
      return false;
    }
    auto parsed = parseTrapFrameL2(frame);
    if (!parsed.has_value() || parsed->etherType != utility::kCsigTpid) {
      return false;
    }
    folly::io::Cursor cursor = parsed->payloadCursor(frame);

    auto csigTag = utility::parseCsigTag(cursor);
    if (!csigTag.has_value()) {
      return false;
    }
    if (csigTag->signalType != utility::kCsigAbwcSignalType) {
      return false;
    }
    if (csigTag->locatorMetadata != utility::kCsigQuietClampEgressLinkLocator) {
      return false;
    }
    if (!utility::isQuietClampEgressCsigSignal(csigTag->signal)) {
      return false;
    }

    if (expectedDstIp.isV4()) {
      if (csigTag->nextEtherType !=
          static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV4)) {
        return false;
      }
      utility::IPPacket<folly::IPAddressV4> v4Packet(cursor);
      auto v4Hdr = v4Packet.header();
      return v4Hdr.dstAddr == expectedDstIp.asV4() &&
          egressTrapHopLimitOk(v4Hdr.ttl) && v4Hdr.dscp == kFloodDscp;
    }

    if (csigTag->nextEtherType !=
        static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV6)) {
      return false;
    }
    utility::IPPacket<folly::IPAddressV6> v6Packet(cursor);
    auto v6Hdr = v6Packet.header();
    return v6Hdr.dstAddr == expectedDstIp.asV6() &&
        egressTrapHopLimitOk(v6Hdr.hopLimit) &&
        (v6Hdr.trafficClass >> 2) == kFloodDscp;
  }

  bool validateCsigStripRoutedFrame(
      const folly::IOBuf* frame,
      const folly::IPAddress& expectedDstIp) const {
    if (frame == nullptr) {
      return false;
    }
    auto parsed = parseTrapFrameL2(frame);
    if (!parsed.has_value()) {
      return false;
    }
    if (parsed->etherType == utility::kCsigTpid) {
      return false;
    }

    folly::io::Cursor cursor = parsed->payloadCursor(frame);
    if (parsed->etherType == static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV4)) {
      utility::IPPacket<folly::IPAddressV4> v4Packet(cursor);
      auto v4Hdr = v4Packet.header();
      return expectedDstIp.isV4() && v4Hdr.dstAddr == expectedDstIp.asV4() &&
          v4Hdr.ttl == kFloodTtl - 1 && v4Hdr.dscp == kFloodDscp;
    }
    if (parsed->etherType == static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV6)) {
      utility::IPPacket<folly::IPAddressV6> v6Packet(cursor);
      auto v6Hdr = v6Packet.header();
      return !expectedDstIp.isV4() && v6Hdr.dstAddr == expectedDstIp.asV6() &&
          v6Hdr.hopLimit == kFloodTtl - 1 &&
          (v6Hdr.trafficClass >> 2) == kFloodDscp;
    }
    return false;
  }

  bool trapFrameHasUndecappedSrv6Outer(const folly::IOBuf* frame) const {
    auto parsed = parseTrapFrameL2(frame);
    if (!parsed.has_value() ||
        parsed->etherType != static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV6)) {
      return false;
    }
    try {
      folly::io::Cursor cursor = parsed->payloadCursor(frame);
      IPv6Hdr outerHdr(cursor);
      return outerHdr.dstAddr == kDecapMySidAddr;
    } catch (const std::exception&) {
      return false;
    }
  }

  // DECAP PASSTHROUGH: accumulated outer L2 CSIG copied onto the decapped
  // inner packet's L2 header; SRv6 tunnel discarded.
  bool validateCsigDecapPassthroughFrame(
      const folly::IOBuf* frame,
      const folly::IPAddress& expectedInnerDst) const {
    if (!validateCsigPassthroughFrame(frame, expectedInnerDst)) {
      return false;
    }
    return !trapFrameHasUndecappedSrv6Outer(frame);
  }

  // DECAP STRIP: outer CSIG thrown away; decapped inner exits without CSIG.
  bool validateCsigDecapStripFrame(
      const folly::IOBuf* frame,
      const folly::IPAddress& expectedInnerDst) const {
    if (!validateCsigStripRoutedFrame(frame, expectedInnerDst)) {
      return false;
    }
    return !trapFrameHasUndecappedSrv6Outer(frame);
  }

  // DECAP UPDATE: outer CSIG dropped; new egress CSIG on decapped inner L2.
  bool validateCsigDecapUpdateFrame(
      const folly::IOBuf* frame,
      const folly::IPAddress& expectedInnerDst) const {
    if (!validateCsigRoutedFrame(frame, expectedInnerDst)) {
      return false;
    }
    return !trapFrameHasUndecappedSrv6Outer(frame);
  }

  bool validateCsigDecapUpdateQuietClampFrame(
      const folly::IOBuf* frame,
      const folly::IPAddress& expectedInnerDst) const {
    if (!validateCsigQuietClampRoutedFrame(frame, expectedInnerDst)) {
      return false;
    }
    return !trapFrameHasUndecappedSrv6Outer(frame);
  }

  bool outerPayloadHasIngressPassthroughCsig(
      folly::io::Cursor& outerPayloadCursor) const {
    return findIngressCsigPayloadOffset(outerPayloadCursor).has_value();
  }

  bool isIngressCsigShimTag(const utility::CsigTag& tag) const {
    if (tag.signalType != utility::kCsigAbwcSignalType ||
        tag.locatorMetadata != utility::kCsigIngressPktLinkLocator ||
        tag.nextEtherType != static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV6)) {
      return false;
    }
    return tag.signal == utility::kCsigIngressAbwcSignal ||
        tag.signal == utility::kCsigQuietClampIngressSignal ||
        tag.signal == utility::kCsigSdkScapyQuietClampIngressSignal;
  }

  std::optional<utility::CsigTag> tryParseIngressCsigShimAtOffset(
      const folly::io::Cursor& outerPayloadCursor,
      size_t offset) const {
    folly::io::Cursor scan = outerPayloadCursor;
    scan.skip(offset);
    if (scan.length() < kDecapStripPayloadCsigShimSize) {
      return std::nullopt;
    }
    // Bare 4-byte shim wrongly placed in SRv6 payload (invalid wire format).
    {
      folly::io::Cursor bare = scan;
      auto tag = utility::parseCsigTag(bare);
      if (tag.has_value() && isIngressCsigShimTag(*tag)) {
        return tag;
      }
    }
    // L2-style TPID prefix inside payload (some HW trap mirrors): 99 00 2b 85
    // 86 dd.
    if (scan.length() >= kDecapStripPayloadCsigShimSize + 2) {
      folly::io::Cursor l2style = scan;
      if (l2style.readBE<uint16_t>() == utility::kCsigTpid) {
        auto tag = utility::parseCsigTag(l2style);
        if (tag.has_value() && isIngressCsigShimTag(*tag)) {
          return tag;
        }
      }
    }
    return std::nullopt;
  }

  std::optional<size_t> findIngressCsigPayloadOffset(
      const folly::io::Cursor& outerPayloadCursor) const {
    const size_t payloadLen = outerPayloadCursor.length();
    for (size_t offset = 0;
         offset + kDecapStripPayloadCsigShimSize <= payloadLen;
         ++offset) {
      if (tryParseIngressCsigShimAtOffset(outerPayloadCursor, offset)
              .has_value()) {
        return offset;
      }
    }
    return std::nullopt;
  }

  std::optional<size_t> findInnerIpv6DstPayloadOffset(
      const folly::io::Cursor& outerPayloadCursor,
      const folly::IPAddressV6& expectedInnerDst) const {
    const size_t payloadLen = outerPayloadCursor.length();
    if (payloadLen < IPv6Hdr::SIZE) {
      return std::nullopt;
    }
    for (size_t offset = 0; offset + IPv6Hdr::SIZE <= payloadLen; ++offset) {
      if (tryMatchInnerIpv6DstAtOffset(
              outerPayloadCursor, offset, expectedInnerDst)) {
        return offset;
      }
    }
    return std::nullopt;
  }

  std::optional<UndecappedSrv6TrapAnalysis> analyzeUndecappedSrv6TrapFrame(
      const folly::IOBuf* frame) const {
    if (frame == nullptr) {
      return std::nullopt;
    }
    auto parsed = parseTrapFrameL2(frame);
    if (!parsed.has_value() ||
        parsed->etherType != static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV6)) {
      return std::nullopt;
    }
    try {
      folly::io::Cursor cursor = parsed->payloadCursor(frame);
      IPv6Hdr outerHdr(cursor);
      if (outerHdr.dstAddr != kDecapMySidAddr) {
        return std::nullopt;
      }
      UndecappedSrv6TrapAnalysis analysis;
      analysis.valid = true;
      analysis.outerPayloadLength = outerHdr.payloadLength;
      analysis.outerNextHeader = outerHdr.nextHeader;
      analysis.outerDst = outerHdr.dstAddr;

      folly::io::Cursor payloadStart = cursor;
      analysis.ingressCsigPayloadOffset =
          findIngressCsigPayloadOffset(payloadStart);
      analysis.payloadHasIngressCsig =
          analysis.ingressCsigPayloadOffset.has_value();
      analysis.innerIpv6PayloadOffset =
          findInnerIpv6DstPayloadOffset(payloadStart, kEncapRouteDstIp);
      return analysis;
    } catch (const std::exception&) {
      return std::nullopt;
    }
  }

  bool undecappedSignatureMatchesOuterL2OnlyInject(
      const UndecappedSrv6TrapAnalysis& analysis) const {
    if (!analysis.valid) {
      return false;
    }
    return !analysis.payloadHasIngressCsig;
  }

  std::optional<DecapStripInjectLayout> parseDecapStripInjectLayout(
      const folly::IOBuf* injectFrame) const {
    if (injectFrame == nullptr) {
      return std::nullopt;
    }
    try {
      folly::io::Cursor cursor(injectFrame);
      EthHdr ethHdr(cursor);
      if (ethHdr.etherType != utility::kCsigTpid) {
        return std::nullopt;
      }
      auto outerCsig = utility::parseCsigTag(cursor);
      if (!outerCsig.has_value() ||
          outerCsig->nextEtherType !=
              static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV6)) {
        return std::nullopt;
      }
      IPv6Hdr outerHdr(cursor);
      folly::io::Cursor payloadStart = cursor;
      DecapStripInjectLayout layout;
      layout.valid = true;
      layout.outerPayloadLength = outerHdr.payloadLength;
      layout.outerNextHeader = outerHdr.nextHeader;
      layout.ingressCsigPayloadOffset =
          findIngressCsigPayloadOffset(payloadStart);
      layout.payloadHasIngressCsig =
          layout.ingressCsigPayloadOffset.has_value();
      layout.innerIpv6PayloadOffset =
          findInnerIpv6DstPayloadOffset(payloadStart, kEncapRouteDstIp);
      return layout;
    } catch (const std::exception&) {
      return std::nullopt;
    }
  }

  bool validateDecapStripInjectLayout(
      const DecapStripInjectLayout& layout) const {
    if (!layout.valid) {
      return false;
    }
    if (layout.outerPayloadLength != kDecapStripInjectOuterPayloadLen) {
      return false;
    }
    if (!layout.innerIpv6PayloadOffset.has_value() ||
        *layout.innerIpv6PayloadOffset != kDecapStripInjectInnerIpv6Offset) {
      return false;
    }
    return !layout.payloadHasIngressCsig;
  }

  void logUndecappedSrv6TrapAnalysis(
      const std::string& label,
      int copyIndex,
      const UndecappedSrv6TrapAnalysis& analysis,
      bool signatureMatched) const {
    XLOG(INFO) << label << " trap copy #" << copyIndex
               << " undecapped SRv6 outer_dst=" << analysis.outerDst.str()
               << " outer_payload_len=0x" << std::hex
               << analysis.outerPayloadLength << std::dec
               << " (inject expect 0x" << std::hex
               << kDecapStripInjectOuterPayloadLen << std::dec << ")"
               << " next_header=0x" << std::hex << +analysis.outerNextHeader
               << std::dec << " payload_ingress_csig="
               << (analysis.payloadHasIngressCsig ? "yes" : "no")
               << " (inject expect no)"
               << " ingress_csig_offset="
               << (analysis.ingressCsigPayloadOffset.has_value()
                       ? std::to_string(*analysis.ingressCsigPayloadOffset)
                       : "none")
               << " inner_ipv6_offset="
               << (analysis.innerIpv6PayloadOffset.has_value()
                       ? std::to_string(*analysis.innerIpv6PayloadOffset)
                       : "none")
               << " step_signature="
               << (signatureMatched ? "match" : "MISMATCH");
  }

  void logDecapStripInjectLayout(
      const DecapStripInjectLayout& layout,
      const std::string& stepLabel) const {
    if (!layout.valid) {
      XLOG(INFO) << stepLabel << " inject layout: parse failed";
      return;
    }
    const bool layoutValid = validateDecapStripInjectLayout(layout);
    XLOG(INFO) << stepLabel << " inject layout: outer_payload_len=0x"
               << std::hex << layout.outerPayloadLength << std::dec
               << " (expect 0x" << std::hex << kDecapStripInjectOuterPayloadLen
               << std::dec << ") outer_next_header=0x" << std::hex
               << +layout.outerNextHeader << std::dec
               << " payload_ingress_csig="
               << (layout.payloadHasIngressCsig ? "yes" : "no")
               << " (inject expect no)"
               << " ingress_csig_offset="
               << (layout.ingressCsigPayloadOffset.has_value()
                       ? std::to_string(*layout.ingressCsigPayloadOffset)
                       : "none")
               << " inner_ipv6_offset="
               << (layout.innerIpv6PayloadOffset.has_value()
                       ? std::to_string(*layout.innerIpv6PayloadOffset)
                       : "none")
               << " (expect " << kDecapStripInjectInnerIpv6Offset << ")"
               << " inject_layout_signature="
               << (layoutValid ? "match" : "MISMATCH");
  }

  bool tryMatchInnerIpv6DstAtOffset(
      const folly::io::Cursor& outerPayloadCursor,
      size_t offset,
      const folly::IPAddressV6& expectedInnerDst) const {
    folly::io::Cursor versionCursor = outerPayloadCursor;
    versionCursor.skip(offset);
    if (versionCursor.length() < IPv6Hdr::SIZE) {
      return false;
    }
    try {
      if ((versionCursor.read<uint8_t>() >> 4) != IPV6_VERSION) {
        return false;
      }
      folly::io::Cursor dstCursor = outerPayloadCursor;
      dstCursor.skip(offset + 24);
      if (dstCursor.length() < folly::IPAddressV6::byteCount()) {
        return false;
      }
      return PktUtil::readIPv6(&dstCursor) == expectedInnerDst;
    } catch (const std::exception&) {
      return false;
    }
  }

  bool scanStripSrv6InnerIpv6Dst(
      folly::io::Cursor& outerPayloadCursor,
      const folly::IPAddressV6& expectedInnerDst) const {
    const size_t payloadLen = outerPayloadCursor.length();
    if (payloadLen < IPv6Hdr::SIZE) {
      return false;
    }
    for (size_t offset = 0; offset + IPv6Hdr::SIZE <= payloadLen; ++offset) {
      if (tryMatchInnerIpv6DstAtOffset(
              outerPayloadCursor, offset, expectedInnerDst)) {
        return true;
      }
    }
    return false;
  }

  // SRv6 encap STRIP: no CSIG on outer L2 and ingress CSIG stripped from the
  // encapsulated inner packet (SRv6 tunnel drops telemetry history).
  bool validateCsigStripInOuterIpv6(
      folly::io::Cursor& cursorAfterOuterIpv6,
      const folly::IPAddressV6& expectedSid,
      const folly::IPAddressV6& expectedInnerDst,
      bool requireNonZeroOuterFlowLabel = true) const {
    IPv6Hdr outerHdr(cursorAfterOuterIpv6);
    if (outerHdr.dstAddr != expectedSid) {
      return false;
    }
    if (requireNonZeroOuterFlowLabel && outerHdr.flowLabel == 0) {
      return false;
    }
    if ((outerHdr.trafficClass >> 2) != kFloodDscp) {
      return false;
    }
    if (outerHdr.hopLimit != kFloodTtl - 1) {
      return false;
    }

    folly::io::Cursor payloadStart = cursorAfterOuterIpv6;
    if (outerPayloadHasIngressPassthroughCsig(payloadStart)) {
      return false;
    }

    if (outerHdr.nextHeader == static_cast<uint8_t>(IP_PROTO::IP_PROTO_IPV6)) {
      if (tryMatchInnerIpv6DstAtOffset(payloadStart, 0, expectedInnerDst)) {
        return true;
      }
    }

    return scanStripSrv6InnerIpv6Dst(payloadStart, expectedInnerDst);
  }

  bool validateCsigStripSrv6Frame(
      const folly::IOBuf* frame,
      const folly::IPAddressV6& expectedSid,
      const folly::IPAddressV6& expectedInnerDst = kEncapRouteDstIp,
      bool requireNonZeroOuterFlowLabel = true) const {
    if (frame == nullptr) {
      return false;
    }
    auto parsed = parseTrapFrameL2(frame);
    if (!parsed.has_value()) {
      return false;
    }
    if (parsed->etherType == utility::kCsigTpid) {
      return false;
    }
    if (parsed->etherType != static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV6)) {
      return false;
    }

    folly::io::Cursor cursor = parsed->payloadCursor(frame);
    return validateCsigStripInOuterIpv6(
        cursor, expectedSid, expectedInnerDst, requireNonZeroOuterFlowLabel);
  }

  std::unique_ptr<TxPacket> makeCsigRoutedTrafficPacket(
      folly::MacAddress intfMac,
      const folly::IPAddress& routedDstIp,
      uint8_t tcField,
      std::optional<std::vector<uint8_t>> payload = std::nullopt,
      uint8_t hopLimit = kFloodTtl) const {
    if (routedDstIp.isV4()) {
      return utility::makeCsigUdpTxPacket(
          this->getSw(),
          csigRoutedInjectVlan(),
          intfMac,
          intfMac,
          folly::IPAddress("10.0.0.1").asV4(),
          routedDstIp.asV4(),
          8000,
          8001,
          tcField,
          hopLimit,
          payload);
    }
    return utility::makeCsigUdpTxPacket(
        this->getSw(),
        csigRoutedInjectVlan(),
        intfMac,
        intfMac,
        folly::IPAddress("1::10").asV6(),
        routedDstIp.asV6(),
        8000,
        8001,
        tcField,
        hopLimit,
        payload);
  }

  std::unique_ptr<TxPacket> makeCsigRoutedProbePacket(
      folly::MacAddress intfMac,
      const folly::IPAddress& routedDstIp,
      uint8_t tcField) const {
    return makeCsigRoutedTrafficPacket(intfMac, routedDstIp, tcField);
  }

  std::unique_ptr<TxPacket> makeCsigEncapTrafficPacket(
      folly::MacAddress intfMac,
      uint8_t tcField,
      std::optional<std::vector<uint8_t>> payload = std::nullopt) const {
    return utility::makeCsigUdpTxPacket(
        this->getSw(),
        csigRoutedInjectVlan(),
        intfMac,
        intfMac,
        folly::IPAddress("1::10").asV6(),
        folly::IPAddress(kEncapRouteDstIp).asV6(),
        8000,
        8001,
        tcField,
        kFloodTtl,
        payload);
  }

  // SDK tgen probe: CSIG-tagged encap trigger with hop-limit 64; egress trap
  // ACL matches hop 63 on loopback (kTgenProbeTrapMatchHopLimit).
  std::unique_ptr<TxPacket> makeCsigEncapProbePacket(
      folly::MacAddress intfMac,
      uint8_t tcField,
      std::optional<std::vector<uint8_t>> payload = std::nullopt) const {
    return utility::makeCsigUdpTxPacket(
        this->getSw(),
        csigRoutedInjectVlan(),
        intfMac,
        intfMac,
        folly::IPAddress("1::10").asV6(),
        folly::IPAddress(kEncapRouteDstIp).asV6(),
        8000,
        8001,
        tcField,
        kTgenProbeHopLimit,
        payload);
  }

  // Plain routed flood (no CSIG shim), SDK tgen background traffic on flood
  // port.
  std::unique_ptr<TxPacket> makePlainRoutedTrafficPacket(
      folly::MacAddress intfMac,
      const folly::IPAddress& routedDstIp,
      uint8_t tcField,
      uint8_t ttlOrHopLimit,
      std::optional<std::vector<uint8_t>> payload = std::nullopt) const {
    if (routedDstIp.isV4()) {
      return utility::makeUDPTxPacket(
          this->getSw(),
          csigRoutedInjectVlan(),
          intfMac,
          intfMac,
          folly::IPAddress("10.0.0.1").asV4(),
          routedDstIp.asV4(),
          8000,
          8001,
          tcField,
          ttlOrHopLimit,
          payload);
    }
    return utility::makeUDPTxPacket(
        this->getSw(),
        csigRoutedInjectVlan(),
        intfMac,
        intfMac,
        folly::IPAddress("1::10").asV6(),
        routedDstIp.asV6(),
        8000,
        8001,
        tcField,
        ttlOrHopLimit,
        payload);
  }

  // Untagged L2 inject for CSIG+SRv6 passthrough (same as routed CSIG inject).
  std::unique_ptr<TxPacket> makeCsigEncapPassthroughPacket(
      folly::MacAddress intfMac,
      uint8_t tcField,
      std::optional<std::vector<uint8_t>> payload = std::nullopt) const {
    return utility::makeCsigUdpTxPacket(
        this->getSw(),
        csigRoutedInjectVlan(),
        intfMac,
        intfMac,
        folly::IPAddress("1::10").asV6(),
        folly::IPAddress(kEncapRouteDstIp).asV6(),
        8000,
        8001,
        tcField,
        kFloodTtl,
        payload);
  }

  // SAI test_csig_passthrough: CSIG-tagged routed pkt forwarded with the same
  // ingress shim (no egress UPDATE, no congestion flood).
  void verifyRoutedCsigPassthrough(PortID egressPort) {
    auto ingressPort = this->findInjectPort({egressPort});
    resolveGlobalNeighborOnEgressPort(egressPort);
    auto routedDstIp = getRoutedDstIpForEgressPort(egressPort);
    auto hw = this->getSw()->getMonolithicHwSwitchHandler()->getHwSwitch();
    utility::configureCsigPassthroughPorts(hw, ingressPort, egressPort);
    XLOG(INFO) << "CSIG routed passthrough: L3 dst " << routedDstIp
               << " via egress port " << egressPort
               << " (ingress shim preserved, egress loopback trap)";

    auto intfMac =
        getMacForFirstInterfaceWithPortsForTesting(this->getProgrammedState());
    auto tcField = static_cast<uint8_t>((kFloodDscp << 2) | kECT1);

    installEgressPortIngressTrap(egressPort);

    utility::SwSwitchPacketSnooper trapSnooper(
        this->getSw(), "csigRoutedPassthroughTrapSnooper");
    trapSnooper.ignoreUnclaimedRxPkts();

    const auto bytesBefore = *this->getLatestPortStats(egressPort).outBytes_();
    auto sendProbe = [&]() {
      auto txPacket = makeCsigRoutedProbePacket(intfMac, routedDstIp, tcField);
      sendCsigInjectSwitched(csigInjectLogLabel_, std::move(txPacket));
    };
    sendProbe();

    bool foundCsigPassthrough = false;
    CsigTrapScanStats scanStats;
    WITH_RETRIES({
      scanStats = scanCsigTrapSnooper(
          trapSnooper,
          "CSIG routed passthrough",
          [&](const folly::IOBuf* frame) {
            return validateCsigPassthroughFrame(frame, routedDstIp);
          });
      if (!scanStats.foundMatch && scanStats.trappedPacketsScanned == 0) {
        const auto bytesAfter =
            *this->getLatestPortStats(egressPort).outBytes_();
        XLOG(INFO) << "CSIG routed passthrough: no trapped copies yet; "
                   << "egress_port=" << egressPort << " outBytes "
                   << bytesBefore << " -> " << bytesAfter
                   << "; resending probe";
        sendProbe();
      }
      foundCsigPassthrough = scanStats.foundMatch;
      EXPECT_EVENTUALLY_TRUE(foundCsigPassthrough);
    });
    logCsigTrapScanSummary(
        "CSIG routed passthrough", scanStats, foundCsigPassthrough);
    XLOG(INFO) << "CSIG routed passthrough routed_dst=" << routedDstIp;
    ASSERT_TRUE(foundCsigPassthrough)
        << "No egress loopback-trapped frame with unchanged CSIG shim "
        << "(locator 0x" << std::hex << +utility::kCsigIngressPktLinkLocator
        << ", signal 0x" << +utility::kCsigIngressAbwcSignal << std::dec
        << ") for L3 dst " << routedDstIp;
  }

  // SAI test_csig_strip: CSIG-tagged routed pkt forwarded without egress CSIG.
  void verifyRoutedCsigStrip(PortID egressPort) {
    auto ingressPort = this->findInjectPort({egressPort});
    resolveGlobalNeighborOnEgressPort(egressPort);
    auto routedDstIp = getRoutedDstIpForEgressPort(egressPort);
    auto hw = this->getSw()->getMonolithicHwSwitchHandler()->getHwSwitch();
    utility::configureCsigStripPorts(hw, ingressPort, egressPort);
    XLOG(INFO) << "CSIG routed strip: L3 dst " << routedDstIp
               << " via egress port " << egressPort
               << " (egress frame has no CSIG TPID)";

    auto intfMac =
        getMacForFirstInterfaceWithPortsForTesting(this->getProgrammedState());
    auto tcField = static_cast<uint8_t>((kFloodDscp << 2) | kECT1);

    installEgressPortIngressTrap(egressPort);

    utility::SwSwitchPacketSnooper trapSnooper(
        this->getSw(), "csigRoutedStripTrapSnooper");
    trapSnooper.ignoreUnclaimedRxPkts();

    const auto bytesBefore = *this->getLatestPortStats(egressPort).outBytes_();
    auto sendProbe = [&]() {
      auto txPacket = makeCsigRoutedProbePacket(intfMac, routedDstIp, tcField);
      sendCsigInjectSwitched(csigInjectLogLabel_, std::move(txPacket));
    };
    sendProbe();

    bool foundStripFrame = false;
    CsigTrapScanStats scanStats;
    WITH_RETRIES({
      scanStats = scanStripTrapSnooper(
          trapSnooper, "CSIG routed strip", [&](const folly::IOBuf* frame) {
            return validateCsigStripRoutedFrame(frame, routedDstIp);
          });
      if (!scanStats.foundMatch && scanStats.trappedPacketsScanned == 0) {
        const auto bytesAfter =
            *this->getLatestPortStats(egressPort).outBytes_();
        XLOG(INFO) << "CSIG routed strip: no trapped copies yet; "
                   << "egress_port=" << egressPort << " outBytes "
                   << bytesBefore << " -> " << bytesAfter
                   << "; resending probe";
        sendProbe();
      }
      foundStripFrame = scanStats.foundMatch;
      EXPECT_EVENTUALLY_TRUE(foundStripFrame);
    });
    logCsigTrapScanSummary("CSIG routed strip", scanStats, foundStripFrame);
    ASSERT_TRUE(foundStripFrame)
        << "No egress loopback-trapped frame without CSIG TPID for L3 dst "
        << routedDstIp;
  }

  bool validateCsigSrv6PassthroughFrame(
      const folly::IOBuf* frame,
      const folly::IPAddressV6& expectedSid,
      const folly::IPAddressV6& expectedInnerDst = kEncapRouteDstIp,
      bool requireNonZeroOuterFlowLabel = true) const {
    if (frame == nullptr) {
      return false;
    }
    auto parsed = parseTrapFrameL2(frame);
    if (!parsed.has_value()) {
      return false;
    }

    // PASSTHROUGH: ingress CSIG is copied to the new outer L2 header.
    if (parsed->etherType != utility::kCsigTpid) {
      return false;
    }

    folly::io::Cursor cursor = parsed->payloadCursor(frame);
    auto csigTag = utility::parseCsigTag(cursor);
    if (!csigTag.has_value()) {
      return false;
    }
    if (csigTag->signalType != utility::kCsigAbwcSignalType) {
      return false;
    }
    if (csigTag->locatorMetadata != utility::kCsigIngressPktLinkLocator) {
      return false;
    }
    if (csigTag->signal != utility::kCsigIngressAbwcSignal) {
      return false;
    }
    // PASSTHROUGH: ingress CSIG copied to new outer L2 only; no CSIG in
    // payload.
    if (csigTag->nextEtherType !=
        static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV6)) {
      return false;
    }

    return validateCsigStripInOuterIpv6(
        cursor, expectedSid, expectedInnerDst, requireNonZeroOuterFlowLabel);
  }

  // SRv6 encap PASSTHROUGH: ingress CSIG on new outer L2 only; plain inner in
  // SRv6 payload (no duplicate CSIG in payload).
  void verifySrv6CsigPassthrough(
      PortID egressPort,
      const folly::IPAddressV6& expectedSid = kSid0) {
    auto ingressPort = this->findInjectPort({egressPort});
    resolveGlobalNeighborOnEgressPort(egressPort);
    auto hw = this->getSw()->getMonolithicHwSwitchHandler()->getHwSwitch();
    utility::configureCsigPassthroughPorts(hw, ingressPort, egressPort);
    XLOG(INFO) << "CSIG+SRv6 passthrough: encap dst " << kEncapRouteDstIp
               << " SID " << expectedSid << " via egress port " << egressPort
               << " (ingress CSIG copied to outer L2 only, no payload CSIG, "
               << "egress loopback trap)";

    auto intfMac =
        getMacForFirstInterfaceWithPortsForTesting(this->getProgrammedState());
    auto tcField = static_cast<uint8_t>((kFloodDscp << 2) | kECT1);

    installEgressPortIngressTrap(egressPort);

    utility::SwSwitchPacketSnooper trapSnooper(
        this->getSw(), "csigSrv6PassthroughTrapSnooper");
    trapSnooper.ignoreUnclaimedRxPkts();

    const auto bytesBefore = *this->getLatestPortStats(egressPort).outBytes_();
    auto sendProbe = [&]() {
      auto txPacket = makeCsigEncapPassthroughPacket(intfMac, tcField);
      sendCsigInjectSwitched(csigInjectLogLabel_, std::move(txPacket));
    };
    sendProbe();

    bool foundCsigPassthrough = false;
    CsigTrapScanStats scanStats;
    WITH_RETRIES({
      scanStats = scanCsigTrapSnooper(
          trapSnooper, "CSIG+SRv6 passthrough", [&](const folly::IOBuf* frame) {
            return validateCsigSrv6PassthroughFrame(frame, expectedSid);
          });
      if (!scanStats.foundMatch && scanStats.trappedPacketsScanned == 0) {
        const auto bytesAfter =
            *this->getLatestPortStats(egressPort).outBytes_();
        XLOG(INFO) << "CSIG+SRv6 passthrough: no trapped copies yet; "
                   << "egress_port=" << egressPort << " outBytes "
                   << bytesBefore << " -> " << bytesAfter
                   << "; resending probe";
        sendProbe();
      } else if (!scanStats.foundMatch && scanStats.csigPacketsScanned > 0) {
        XLOG(INFO) << "CSIG+SRv6 passthrough: scanned "
                   << scanStats.csigPacketsScanned
                   << " CSIG trap copies without match; resending probe";
        sendProbe();
      }
      foundCsigPassthrough = scanStats.foundMatch;
      EXPECT_EVENTUALLY_TRUE(foundCsigPassthrough);
    });
    logCsigTrapScanSummary(
        "CSIG+SRv6 passthrough", scanStats, foundCsigPassthrough);
    XLOG(INFO) << "CSIG+SRv6 passthrough encap_dst=" << kEncapRouteDstIp
               << " sid=" << expectedSid;
    ASSERT_TRUE(foundCsigPassthrough)
        << "No egress loopback-trapped SRv6 encap frame with ingress CSIG on "
        << "outer L2 only (no payload CSIG; locator 0x" << std::hex
        << +utility::kCsigIngressPktLinkLocator << ", signal 0x"
        << +utility::kCsigIngressAbwcSignal << std::dec << "), SID "
        << expectedSid << ", inner dst " << kEncapRouteDstIp;
  }

  // SAI test_csig_strip on SRv6 encap: CSIG removed from inner packet and not
  // placed on the new outer L2 header.
  void verifySrv6CsigStrip(
      PortID egressPort,
      const folly::IPAddressV6& expectedSid = kSid0) {
    auto ingressPort = this->findInjectPort({egressPort});
    resolveGlobalNeighborOnEgressPort(egressPort);
    auto hw = this->getSw()->getMonolithicHwSwitchHandler()->getHwSwitch();
    utility::configureCsigStripPorts(hw, ingressPort, egressPort);
    XLOG(INFO) << "CSIG+SRv6 strip: encap dst " << kEncapRouteDstIp << " SID "
               << expectedSid << " via egress port " << egressPort
               << " (no outer L2 CSIG, no CSIG in SRv6 payload)";

    auto intfMac =
        getMacForFirstInterfaceWithPortsForTesting(this->getProgrammedState());
    auto tcField = static_cast<uint8_t>((kFloodDscp << 2) | kECT1);

    installEgressPortIngressTrap(egressPort);

    utility::SwSwitchPacketSnooper trapSnooper(
        this->getSw(), "csigSrv6StripTrapSnooper");
    trapSnooper.ignoreUnclaimedRxPkts();

    const auto bytesBefore = *this->getLatestPortStats(egressPort).outBytes_();
    auto sendProbe = [&]() {
      auto txPacket = makeCsigEncapPassthroughPacket(intfMac, tcField);
      sendCsigInjectSwitched(csigInjectLogLabel_, std::move(txPacket));
    };
    sendProbe();

    bool foundStripFrame = false;
    CsigTrapScanStats scanStats;
    WITH_RETRIES({
      scanStats = scanStripTrapSnooper(
          trapSnooper, "CSIG+SRv6 strip", [&](const folly::IOBuf* frame) {
            return validateCsigStripSrv6Frame(frame, expectedSid);
          });
      if (!scanStats.foundMatch && scanStats.trappedPacketsScanned == 0) {
        const auto bytesAfter =
            *this->getLatestPortStats(egressPort).outBytes_();
        XLOG(INFO) << "CSIG+SRv6 strip: no trapped copies yet; egress_port="
                   << egressPort << " outBytes " << bytesBefore << " -> "
                   << bytesAfter << "; resending probe";
        sendProbe();
      } else if (!scanStats.foundMatch && scanStats.trappedPacketsScanned > 0) {
        XLOG(INFO) << "CSIG+SRv6 strip: scanned "
                   << scanStats.trappedPacketsScanned
                   << " trapped copies without strip match; resending probe";
        sendProbe();
      }
      foundStripFrame = scanStats.foundMatch;
      EXPECT_EVENTUALLY_TRUE(foundStripFrame);
    });
    logCsigTrapScanSummary("CSIG+SRv6 strip", scanStats, foundStripFrame);
    ASSERT_TRUE(foundStripFrame)
        << "No egress loopback-trapped SRv6 encap strip frame: outer L2 must "
        << "have no CSIG TPID, SRv6 payload must not contain ingress CSIG "
        << "shim, SID " << expectedSid << ", inner dst " << kEncapRouteDstIp;
  }

  // Baseline: CSIG-tagged L3 routed egress under congestion (no SRv6 tunnel).
  // All inject traffic carries the ingress CSIG shim; egress UPDATE rewrites
  // the ABWC signal from the programmed histogram when the port is congested.
  void verifyRoutedCsigTagUnderCongestion(PortID egressPort) {
    resolveGlobalNeighborOnEgressPort(egressPort);
    auto routedDstIp = getRoutedDstIpForEgressPort(egressPort);
    XLOG(INFO) << "CSIG routed baseline: L3 dst " << routedDstIp
               << " via egress port " << egressPort
               << " (CSIG-tagged inject, hop-limit trap, congestion flood TTL="
               << +kTgenFloodTtl << ")";

    std::map<PortID, int64_t> bytesBefore;
    bytesBefore[egressPort] = *this->getLatestPortStats(egressPort).outBytes_();

    auto intfMac =
        getMacForFirstInterfaceWithPortsForTesting(this->getProgrammedState());
    auto tcField = static_cast<uint8_t>((kFloodDscp << 2) | kECT1);

    buildCsigEgressLineRateCongestion(egressPort);

    // Flood uses hop_limit=5; CPU-switched probe uses hop_limit=64. Port-wide
    // trap punts every looped CSIG flood copy and spams PacketSnooper.
    installEgressPortIngressTrapForTgenProbe(egressPort);

    utility::SwSwitchPacketSnooper trapSnooper(
        this->getSw(), "csigRoutedEgressTrapSnooper");
    trapSnooper.ignoreUnclaimedRxPkts();

    auto txPacket = makeCsigRoutedProbePacket(intfMac, routedDstIp, tcField);
    sendCsigInjectSwitched(csigInjectLogLabel_, std::move(txPacket));

    bool foundCsigOnEgressTrap = false;
    CsigTrapScanStats scanStats;
    WITH_RETRIES({
      scanStats = scanCsigTrapSnooper(
          trapSnooper,
          "CSIG routed egress-trapped",
          [&](const folly::IOBuf* frame) {
            return validateCsigRoutedFrame(frame, routedDstIp);
          });
      foundCsigOnEgressTrap = scanStats.foundMatch;

      auto bytesAfter = *this->getLatestPortStats(egressPort).outBytes_();
      EXPECT_EVENTUALLY_TRUE(bytesAfter > bytesBefore[egressPort]);
      EXPECT_EVENTUALLY_TRUE(foundCsigOnEgressTrap);
    });
    logCsigTrapScanSummary(
        "CSIG routed baseline capture", scanStats, foundCsigOnEgressTrap);
    XLOG(INFO) << "CSIG routed baseline routed_dst=" << routedDstIp;
    ASSERT_TRUE(foundCsigOnEgressTrap)
        << "No egress loopback-trapped routed frame with CSIG TPID (0x9900) and "
        << "egress-updated ABWC signal for L3 dst " << routedDstIp
        << " after scanning " << scanStats.trappedPacketsScanned
        << " trapped copies (" << scanStats.csigPacketsScanned << " CSIG, "
        << scanStats.nonCsigPacketsSkipped << " non-CSIG skipped).";
  }

  // SRv6 encap UPDATE: outer L2 carries new egress CSIG; inner ingress CSIG
  // must not remain in the SRv6 payload.
  bool validateCsigSrv6EncapFrame(
      const folly::IOBuf* frame,
      const folly::IPAddressV6& expectedSid,
      const folly::IPAddressV6& expectedInnerDst = kEncapRouteDstIp,
      bool requireNonZeroOuterFlowLabel = true) const {
    if (frame == nullptr) {
      return false;
    }
    auto parsed = parseTrapFrameL2(frame);
    if (!parsed.has_value() || parsed->etherType != utility::kCsigTpid) {
      return false;
    }
    folly::io::Cursor cursor = parsed->payloadCursor(frame);

    auto csigTag = utility::parseCsigTag(cursor);
    if (!csigTag.has_value()) {
      return false;
    }
    if (csigTag->signalType != utility::kCsigAbwcSignalType) {
      return false;
    }
    if (csigTag->locatorMetadata != utility::kDefaultCsigLinkLocator) {
      return false;
    }
    if (csigTag->nextEtherType !=
        static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV6)) {
      return false;
    }
    if (!utility::isEgressUpdatedAbwcSignal(csigTag->signal)) {
      return false;
    }

    utility::IPPacket<folly::IPAddressV6> outerV6(cursor);
    auto v6Hdr = outerV6.header();
    if (v6Hdr.dstAddr != expectedSid) {
      return false;
    }
    if (requireNonZeroOuterFlowLabel && v6Hdr.flowLabel == 0) {
      return false;
    }
    if ((v6Hdr.trafficClass >> 2) != kFloodDscp) {
      return false;
    }
    if (!egressTrapHopLimitOk(v6Hdr.hopLimit)) {
      return false;
    }

    // UPDATE: ingress CSIG on the inner packet must be wiped/ignored.
    folly::io::Cursor payloadStart = cursor;
    if (outerPayloadHasIngressPassthroughCsig(payloadStart)) {
      return false;
    }

    auto innerV6 = outerV6.v6PayLoad();
    if (innerV6 == nullptr) {
      return false;
    }
    auto innerHdr = innerV6->header();
    return innerHdr.dstAddr == expectedInnerDst;
  }

  bool validateCsigSrv6EncapQuietClampFrame(
      const folly::IOBuf* frame,
      const folly::IPAddressV6& expectedSid,
      const folly::IPAddressV6& expectedInnerDst = kEncapRouteDstIp,
      bool requireNonZeroOuterFlowLabel = false) const {
    if (frame == nullptr) {
      return false;
    }
    auto parsed = parseTrapFrameL2(frame);
    if (!parsed.has_value() || parsed->etherType != utility::kCsigTpid) {
      return false;
    }
    folly::io::Cursor cursor = parsed->payloadCursor(frame);

    auto csigTag = utility::parseCsigTag(cursor);
    if (!csigTag.has_value()) {
      return false;
    }
    if (csigTag->signalType != utility::kCsigAbwcSignalType) {
      return false;
    }
    if (csigTag->locatorMetadata != utility::kCsigQuietClampEgressLinkLocator) {
      return false;
    }
    if (csigTag->nextEtherType !=
        static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV6)) {
      return false;
    }
    if (!utility::isQuietClampEgressCsigSignal(csigTag->signal)) {
      return false;
    }

    utility::IPPacket<folly::IPAddressV6> outerV6(cursor);
    auto v6Hdr = outerV6.header();
    if (v6Hdr.dstAddr != expectedSid) {
      return false;
    }
    if (requireNonZeroOuterFlowLabel && v6Hdr.flowLabel == 0) {
      return false;
    }
    if ((v6Hdr.trafficClass >> 2) != kFloodDscp) {
      return false;
    }
    if (!egressTrapHopLimitOk(v6Hdr.hopLimit)) {
      return false;
    }

    folly::io::Cursor payloadStart = cursor;
    if (outerPayloadHasIngressPassthroughCsig(payloadStart)) {
      return false;
    }

    auto innerV6 = outerV6.v6PayLoad();
    if (innerV6 == nullptr) {
      return false;
    }
    auto innerHdr = innerV6->header();
    return innerHdr.dstAddr == expectedInnerDst;
  }

  std::unique_ptr<TxPacket> makeCsigMidpointProbePacket(
      folly::MacAddress intfMac,
      const folly::IPAddressV6& outerDst,
      const folly::IPAddressV6& innerDst,
      uint8_t tcField) const {
    // Inject: [dst|src|0x9900|outer L2 CSIG|outer IPv6|inner IPv6|UDP] (Layout
    // B3).
    return utility::makeCsigIpInIpTxPacket(
        this->getSw(),
        csigRoutedInjectVlan(),
        intfMac,
        intfMac,
        folly::IPAddressV6("100::1"),
        outerDst,
        kIpInIpInnerSrc,
        innerDst,
        8000,
        8001,
        tcField,
        0,
        kFloodTtl,
        kFloodTtl);
  }

  std::unique_ptr<TxPacket> makeCsigIpInIpProbePacket(
      folly::MacAddress intfMac,
      const folly::IPAddressV6& outerDst,
      const folly::IPAddressV6& innerDst,
      uint8_t tcField) const {
    return utility::makeCsigIpInIpTxPacket(
        this->getSw(),
        csigRoutedInjectVlan(),
        intfMac,
        intfMac,
        folly::IPAddressV6("100::1"),
        outerDst,
        kIpInIpInnerSrc,
        innerDst,
        8000,
        8001,
        tcField,
        0,
        kFloodTtl,
        kFloodTtl);
  }

  std::unique_ptr<TxPacket> makeCsigDecapProbePacket(
      folly::MacAddress intfMac,
      uint8_t tcField) const {
    // Inject: [dst|src|0x9900|outer L2 CSIG|outer IPv6 dst=mySid|inner
    // IPv6|UDP]. Decap tests validate the decapped egress inner packet's L2
    // header.
    return utility::makeCsigIpInIpTxPacket(
        this->getSw(),
        csigRoutedInjectVlan(),
        intfMac,
        intfMac,
        folly::IPAddressV6("1::1"),
        kDecapMySidAddr,
        folly::IPAddressV6("1::10"),
        kEncapRouteDstIp,
        8000,
        8001,
        tcField,
        tcField,
        kFloodTtl,
        kFloodTtl);
  }

  std::unique_ptr<TxPacket> makeCsigDecapTgenProbePacket(
      folly::MacAddress intfMac,
      uint8_t tcField) const {
    return utility::makeCsigIpInIpTxPacket(
        this->getSw(),
        csigRoutedInjectVlan(),
        intfMac,
        intfMac,
        folly::IPAddressV6("1::1"),
        kDecapMySidAddr,
        folly::IPAddressV6("1::10"),
        kEncapRouteDstIp,
        8000,
        8001,
        tcField,
        tcField,
        kTgenProbeHopLimit,
        kTgenProbeHopLimit);
  }

  std::unique_ptr<TxPacket> makeCsigMidpointTgenProbePacket(
      folly::MacAddress intfMac,
      const folly::IPAddressV6& outerDst,
      const folly::IPAddressV6& innerDst,
      uint8_t tcField) const {
    return utility::makeCsigIpInIpTxPacket(
        this->getSw(),
        csigRoutedInjectVlan(),
        intfMac,
        intfMac,
        folly::IPAddressV6("100::1"),
        outerDst,
        kIpInIpInnerSrc,
        innerDst,
        8000,
        8001,
        tcField,
        0,
        kTgenProbeHopLimit,
        kTgenProbeHopLimit);
  }

  std::unique_ptr<TxPacket> makeCsigIpInIpTgenProbePacket(
      folly::MacAddress intfMac,
      const folly::IPAddressV6& outerDst,
      const folly::IPAddressV6& innerDst,
      uint8_t tcField) const {
    return utility::makeCsigIpInIpTxPacket(
        this->getSw(),
        csigRoutedInjectVlan(),
        intfMac,
        intfMac,
        folly::IPAddressV6("100::1"),
        outerDst,
        kIpInIpInnerSrc,
        innerDst,
        8000,
        8001,
        tcField,
        0,
        kTgenProbeHopLimit,
        kTgenProbeHopLimit);
  }

  // SDK srv6_fpp_base quiet-clamp inject: scapy CSIG(signal=7) default
  // signal_buf=0x2 encodes on wire as 99 00 2b 85 (5-bit signal 0x17), not
  // FBOSS literal 0x07/0x2385.
  std::unique_ptr<TxPacket> makeCsigEncapQuietClampProbePacket(
      folly::MacAddress intfMac,
      uint8_t tcField,
      std::optional<std::vector<uint8_t>> payload = std::nullopt) const {
    return utility::makeCsigUdpTxPacket(
        this->getSw(),
        csigRoutedInjectVlan(),
        intfMac,
        intfMac,
        folly::IPAddress("1::10").asV6(),
        folly::IPAddress(kEncapRouteDstIp).asV6(),
        8000,
        8001,
        tcField,
        kFloodTtl,
        payload,
        utility::kCsigAbwcSignalType,
        utility::kCsigSdkScapyQuietClampIngressSignal,
        utility::kCsigIngressPktLinkLocator);
  }

  std::unique_ptr<TxPacket> makeCsigDecapQuietClampProbePacket(
      folly::MacAddress intfMac,
      uint8_t tcField) const {
    return utility::makeCsigIpInIpTxPacket(
        this->getSw(),
        csigRoutedInjectVlan(),
        intfMac,
        intfMac,
        folly::IPAddressV6("1::1"),
        kDecapMySidAddr,
        folly::IPAddressV6("1::10"),
        kEncapRouteDstIp,
        8000,
        8001,
        tcField,
        tcField,
        kFloodTtl,
        kFloodTtl,
        0,
        std::nullopt,
        utility::kCsigAbwcSignalType,
        utility::kCsigSdkScapyQuietClampIngressSignal,
        utility::kCsigIngressPktLinkLocator);
  }

  std::unique_ptr<TxPacket> makeCsigMidpointQuietClampProbePacket(
      folly::MacAddress intfMac,
      const folly::IPAddressV6& outerDst,
      const folly::IPAddressV6& innerDst,
      uint8_t tcField) const {
    return utility::makeCsigIpInIpTxPacket(
        this->getSw(),
        csigRoutedInjectVlan(),
        intfMac,
        intfMac,
        folly::IPAddressV6("100::1"),
        outerDst,
        kIpInIpInnerSrc,
        innerDst,
        8000,
        8001,
        tcField,
        0,
        kFloodTtl,
        kFloodTtl,
        0,
        std::nullopt,
        utility::kCsigAbwcSignalType,
        utility::kCsigSdkScapyQuietClampIngressSignal,
        utility::kCsigIngressPktLinkLocator);
  }

  std::unique_ptr<TxPacket> makeCsigBsidQuietClampProbePacket(
      folly::MacAddress intfMac,
      const folly::IPAddressV6& outerDst,
      const folly::IPAddressV6& innerDst,
      uint8_t tcField) const {
    return utility::makeCsigIpInIpTxPacket(
        this->getSw(),
        csigRoutedInjectVlan(),
        intfMac,
        intfMac,
        folly::IPAddressV6("100::1"),
        outerDst,
        kIpInIpInnerSrc,
        innerDst,
        8000,
        8001,
        tcField,
        0,
        kFloodTtl,
        kFloodTtl,
        0,
        std::nullopt,
        utility::kCsigAbwcSignalType,
        utility::kCsigSdkScapyQuietClampIngressSignal,
        utility::kCsigIngressPktLinkLocator);
  }

  template <typename MakeProbeFn>
  void runDecapCsigStripProbe(
      PortID egressPort,
      const std::string& stepLabel,
      MakeProbeFn&& makeProbe) {
    utility::SwSwitchPacketSnooper trapSnooper(
        this->getSw(), stepLabel + "TrapSnooper");
    trapSnooper.ignoreUnclaimedRxPkts();
    const auto bytesBefore = *this->getLatestPortStats(egressPort).outBytes_();
    auto sendProbe = [&]() {
      auto txPacket = makeProbe();
      auto injectLayout = parseDecapStripInjectLayout(txPacket->buf());
      ASSERT_TRUE(injectLayout.has_value())
          << stepLabel << ": failed to parse inject packet layout";
      logDecapStripInjectLayout(*injectLayout, stepLabel);
      ASSERT_TRUE(validateDecapStripInjectLayout(*injectLayout))
          << stepLabel << ": inject packet does not match outer-L2-only layout "
          << "(outer_payload_len=0x62, no payload CSIG, inner at 0)";
      sendCsigInjectSwitched(csigInjectLogLabel_, std::move(txPacket));
    };
    sendProbe();
    bool found = false;
    CsigTrapScanStats scanStats;
    WITH_RETRIES({
      scanStats = scanDecapStripTrapSnooper(
          trapSnooper, stepLabel, [&](const folly::IOBuf* frame) {
            return validateCsigDecapStripFrame(
                frame, folly::IPAddress(kEncapRouteDstIp));
          });
      if (!scanStats.foundMatch && scanStats.trappedPacketsScanned == 0) {
        const auto bytesAfter =
            *this->getLatestPortStats(egressPort).outBytes_();
        XLOG(INFO) << stepLabel
                   << ": no trapped copies yet; egress_port=" << egressPort
                   << " outBytes " << bytesBefore << " -> " << bytesAfter
                   << "; resending probe";
        sendProbe();
      } else if (!scanStats.foundMatch && scanStats.trappedPacketsScanned > 0) {
        XLOG(INFO) << stepLabel << ": scanned "
                   << scanStats.trappedPacketsScanned
                   << " trapped copies without strip match; resending probe";
        sendProbe();
      }
      found = scanStats.foundMatch;
      EXPECT_EVENTUALLY_TRUE(found);
    });
    logCsigTrapScanSummary(stepLabel, scanStats, found);
    if (scanStats.undecappedCopiesSeen > 0) {
      ASSERT_GT(scanStats.undecappedSignaturesMatched, 0)
          << stepLabel
          << ": undecapped trap copies did not match outer-L2-only inject "
          << "(expect no payload ingress CSIG). Check inject/undecapped logs.";
      ASSERT_EQ(scanStats.undecappedSignaturesMismatched, 0)
          << stepLabel
          << ": at least one undecapped trap copy contradicted inject layout.";
    } else {
      XLOG(INFO) << stepLabel
                 << ": no undecapped SRv6 trap copies seen; inject layout "
                 << "was validated at send — strip match confirms decapped "
                 << "inner only.";
    }
    ASSERT_TRUE(found) << stepLabel
                       << ": no egress loopback-trapped decap STRIP frame; "
                       << "ingress CSIG must be discarded and decapped inner "
                       << "must have no CSIG TPID, inner dst "
                       << kEncapRouteDstIp;
  }

  void verifyDecapCsigPassthrough(PortID egressPort) {
    auto ingressPort = this->findInjectPort({egressPort});
    resolveGlobalNeighborOnEgressPort(egressPort);
    auto hw = this->getSw()->getMonolithicHwSwitchHandler()->getHwSwitch();
    utility::configureCsigPassthroughPorts(hw, ingressPort, egressPort);
    XLOG(INFO) << "CSIG+SRv6 decap passthrough: mySid " << kDecapMySidAddr
               << " inner dst " << kEncapRouteDstIp << " via egress port "
               << egressPort
               << " (outer L2 CSIG copied to decapped inner L2, tunnel "
               << "discarded, egress loopback trap)";
    auto intfMac =
        getMacForFirstInterfaceWithPortsForTesting(this->getProgrammedState());
    auto tcField = static_cast<uint8_t>((kFloodDscp << 2) | kECT1);
    installEgressPortIngressTrap(egressPort);
    utility::SwSwitchPacketSnooper trapSnooper(
        this->getSw(), "csigDecapPassthroughTrapSnooper");
    trapSnooper.ignoreUnclaimedRxPkts();
    const auto bytesBefore = *this->getLatestPortStats(egressPort).outBytes_();
    auto sendProbe = [&]() {
      auto txPacket = makeCsigDecapProbePacket(intfMac, tcField);
      sendCsigInjectSwitched(csigInjectLogLabel_, std::move(txPacket));
    };
    sendProbe();
    bool found = false;
    CsigTrapScanStats scanStats;
    WITH_RETRIES({
      scanStats = scanCsigTrapSnooper(
          trapSnooper,
          "CSIG+SRv6 decap passthrough",
          [&](const folly::IOBuf* frame) {
            return validateCsigDecapPassthroughFrame(
                frame, folly::IPAddress(kEncapRouteDstIp));
          });
      if (!scanStats.foundMatch && scanStats.trappedPacketsScanned == 0) {
        const auto bytesAfter =
            *this->getLatestPortStats(egressPort).outBytes_();
        XLOG(INFO) << "CSIG+SRv6 decap passthrough: no trapped copies yet; "
                   << "egress_port=" << egressPort << " outBytes "
                   << bytesBefore << " -> " << bytesAfter
                   << "; resending probe";
        sendProbe();
      } else if (!scanStats.foundMatch && scanStats.csigPacketsScanned > 0) {
        XLOG(INFO) << "CSIG+SRv6 decap passthrough: scanned "
                   << scanStats.csigPacketsScanned
                   << " CSIG trap copies without match; resending probe";
        sendProbe();
      }
      found = scanStats.foundMatch;
      EXPECT_EVENTUALLY_TRUE(found);
    });
    logCsigTrapScanSummary("CSIG+SRv6 decap passthrough", scanStats, found);
    XLOG(INFO) << "CSIG+SRv6 decap passthrough mySid=" << kDecapMySidAddr
               << " inner_dst=" << kEncapRouteDstIp;
    ASSERT_TRUE(found)
        << "No egress loopback-trapped decap PASSTHROUGH frame: outer L2 CSIG "
        << "must be copied onto decapped inner L2 (locator 0x" << std::hex
        << +utility::kCsigIngressPktLinkLocator << ", signal 0x"
        << +utility::kCsigIngressAbwcSignal << std::dec
        << "), SRv6 tunnel discarded, inner dst " << kEncapRouteDstIp;
  }

  void verifyDecapCsigStrip(PortID egressPort) {
    auto ingressPort = this->findInjectPort({egressPort});
    resolveGlobalNeighborOnEgressPort(egressPort);
    auto hw = this->getSw()->getMonolithicHwSwitchHandler()->getHwSwitch();
    utility::configureCsigStripPorts(hw, ingressPort, egressPort);
    XLOG(INFO) << "CSIG+SRv6 decap strip: mySid " << kDecapMySidAddr
               << " inner dst " << kEncapRouteDstIp << " via egress port "
               << egressPort
               << " (outer L2 CSIG only inject; decapped inner has no CSIG)";
    auto intfMac =
        getMacForFirstInterfaceWithPortsForTesting(this->getProgrammedState());
    auto tcField = static_cast<uint8_t>((kFloodDscp << 2) | kECT1);
    installEgressPortIngressTrap(egressPort);

    runDecapCsigStripProbe(egressPort, "CSIG+SRv6 decap strip", [&]() {
      return makeCsigDecapProbePacket(intfMac, tcField);
    });

    XLOG(INFO) << "CSIG+SRv6 decap strip passed mySid=" << kDecapMySidAddr
               << " inner_dst=" << kEncapRouteDstIp;
  }

  void verifyDecapCsigUpdate(PortID egressPort) {
    auto ingressPort = this->findInjectPort({egressPort});
    resolveGlobalNeighborOnEgressPort(egressPort);
    configureCsigOnIngressAndEgressPorts(ingressPort, egressPort);
    XLOG(INFO) << "CSIG+SRv6 decap UPDATE: mySid " << kDecapMySidAddr
               << " inner dst " << kEncapRouteDstIp << " via egress port "
               << egressPort
               << " (outer CSIG dropped, new egress CSIG on decapped inner L2)";

    std::map<PortID, int64_t> bytesBefore;
    bytesBefore[egressPort] = *this->getLatestPortStats(egressPort).outBytes_();

    auto intfMac =
        getMacForFirstInterfaceWithPortsForTesting(this->getProgrammedState());
    auto tcField = static_cast<uint8_t>((kFloodDscp << 2) | kECT1);
    buildCsigEgressLineRateCongestion(egressPort);
    installEgressPortIngressTrapForTgenProbe(egressPort);
    utility::SwSwitchPacketSnooper trapSnooper(
        this->getSw(), "csigDecapUpdateTrapSnooper");
    trapSnooper.ignoreUnclaimedRxPkts();

    auto sendProbe = [&]() {
      auto txPacket = makeCsigDecapProbePacket(intfMac, tcField);
      sendCsigInjectSwitched(csigInjectLogLabel_, std::move(txPacket));
    };
    sendProbe();

    bool found = false;
    CsigTrapScanStats scanStats;
    const folly::IPAddress expectedInnerDst(kEncapRouteDstIp);
    WITH_RETRIES({
      scanStats = scanCsigDecapUpdateTrapSnooper(
          trapSnooper,
          "CSIG+SRv6 decap update",
          expectedInnerDst,
          [&](const folly::IOBuf* frame) {
            return validateCsigDecapUpdateFrame(frame, expectedInnerDst);
          });
      found = scanStats.foundMatch;

      auto bytesAfter = *this->getLatestPortStats(egressPort).outBytes_();
      EXPECT_EVENTUALLY_TRUE(bytesAfter > bytesBefore[egressPort]);
      if (!scanStats.foundMatch && scanStats.trappedPacketsScanned == 0) {
        XLOG(INFO) << "CSIG+SRv6 decap update: no decap-probe trapped copies "
                   << "yet; egress_port=" << egressPort << " outBytes "
                   << bytesBefore[egressPort] << " -> " << bytesAfter
                   << "; resending probe";
        sendProbe();
      } else if (!scanStats.foundMatch && scanStats.csigPacketsScanned > 0) {
        XLOG(INFO) << "CSIG+SRv6 decap update: scanned "
                   << scanStats.csigPacketsScanned
                   << " decap-probe CSIG trap copies without match; resending "
                   << "probe";
        sendProbe();
      }
      EXPECT_EVENTUALLY_TRUE(found);
    });
    logCsigTrapScanSummary("CSIG+SRv6 decap update", scanStats, found);
    XLOG(INFO) << "CSIG+SRv6 decap update mySid=" << kDecapMySidAddr
               << " inner_dst=" << kEncapRouteDstIp;
    if (!found && scanStats.csigPacketsScanned > 0) {
      XLOG(ERR) << "CSIG+SRv6 decap update: trapped copies never matched "
                << "decapped inner L2 UPDATE CSIG (0x9900 loc 0x" << std::hex
                << +utility::kDefaultCsigLinkLocator << std::dec
                << ", egress-updated ABWC, inner dst " << kEncapRouteDstIp
                << "). If traps show ingress loc 0x" << std::hex
                << +utility::kCsigIngressPktLinkLocator << " signal 0x"
                << +utility::kCsigIngressAbwcSignal << std::dec
                << " with outer IPv6 dst still " << kDecapMySidAddr
                << ", HW left SRv6 undecapped and did not apply CSIG UPDATE "
                << "on the decapped packet (csigBasicUpdate routed path can "
                << "still pass).";
    }
    ASSERT_TRUE(found)
        << "No egress loopback-trapped decap UPDATE frame: outer CSIG must be "
        << "dropped, decapped inner L2 must carry new egress CSIG (locator 0x"
        << std::hex << +utility::kDefaultCsigLinkLocator << std::dec
        << ", congested ABWC), inner dst " << kEncapRouteDstIp
        << " after scanning " << scanStats.trappedPacketsScanned
        << " trapped copies (" << scanStats.csigPacketsScanned << " CSIG, "
        << scanStats.nonCsigPacketsSkipped << " non-CSIG skipped).";
  }

  // Midpoint nodes apply CSIG only to the outer SRv6 header; inner ip-in-ip is
  // not processed. Trap validation checks outer L2 CSIG + outer IPv6 only.
  static folly::io::Cursor skipDuplicateCsigTpidInPayload(
      folly::io::Cursor cursor) {
    if (cursor.length() >= 2) {
      folly::io::Cursor peek = cursor;
      if (peek.readBE<uint16_t>() == utility::kCsigTpid) {
        return peek;
      }
    }
    return cursor;
  }

  // Congestion-loop routes disable TTL decrement; trapped egress may keep the
  // injected hop limit (kFloodTtl) or show one decrement (kFloodTtl - 1).
  bool egressTrapHopLimitOk(uint8_t hopLimit) const {
    return hopLimit == kFloodTtl - 1 || hopLimit == kFloodTtl;
  }

  bool midpointOuterHopLimitOk(uint8_t hopLimit) const {
    return egressTrapHopLimitOk(hopLimit);
  }

  bool validateCsigMidpointOuterIpv6(
      folly::io::Cursor& cursorAtOuterIpv6,
      const folly::IPAddressV6& expectedOuterDst,
      bool requireNonZeroOuterFlowLabel = false) const {
    try {
      IPv6Hdr outerHdr(cursorAtOuterIpv6);
      if (outerHdr.dstAddr != expectedOuterDst) {
        return false;
      }
      if (requireNonZeroOuterFlowLabel && outerHdr.flowLabel == 0) {
        return false;
      }
      if ((outerHdr.trafficClass >> 2) != kFloodDscp) {
        return false;
      }
      return midpointOuterHopLimitOk(outerHdr.hopLimit);
    } catch (const std::exception&) {
      return false;
    }
  }

  // MIDPOINT PASSTHROUGH: ingress outer L2 CSIG unchanged; outer IPv6 SID
  // shift.
  bool validateCsigMidpointPassthroughFrame(
      const folly::IOBuf* frame,
      const folly::IPAddressV6& expectedOuterDst,
      const folly::IPAddressV6& /*expectedInnerDst*/ = kIpInIpInnerDst,
      bool requireNonZeroOuterFlowLabel = false) const {
    if (frame == nullptr) {
      return false;
    }
    auto parsed = parseTrapFrameL2(frame);
    if (!parsed.has_value() || parsed->etherType != utility::kCsigTpid) {
      return false;
    }
    folly::io::Cursor cursor =
        skipDuplicateCsigTpidInPayload(parsed->payloadCursor(frame));

    auto csigTag = utility::parseCsigTag(cursor);
    if (!csigTag.has_value()) {
      return false;
    }
    if (csigTag->signalType != utility::kCsigAbwcSignalType) {
      return false;
    }
    if (csigTag->locatorMetadata != utility::kCsigIngressPktLinkLocator) {
      return false;
    }
    if (csigTag->signal != utility::kCsigIngressAbwcSignal) {
      return false;
    }
    if (csigTag->nextEtherType !=
        static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV6)) {
      return false;
    }

    return validateCsigMidpointOuterIpv6(
        cursor, expectedOuterDst, requireNonZeroOuterFlowLabel);
  }

  // MIDPOINT STRIP: ingress outer L2 CSIG (0x9900) removed on egress.
  bool validateCsigMidpointStripInOuterIpv6(
      folly::io::Cursor& cursorAtOuterIpv6,
      const folly::IPAddressV6& expectedOuterDst,
      const folly::IPAddressV6& /*expectedInnerDst*/,
      bool requireNonZeroOuterFlowLabel = false) const {
    return validateCsigMidpointOuterIpv6(
        cursorAtOuterIpv6, expectedOuterDst, requireNonZeroOuterFlowLabel);
  }

  bool validateCsigMidpointStripFrame(
      const folly::IOBuf* frame,
      const folly::IPAddressV6& expectedOuterDst,
      const folly::IPAddressV6& expectedInnerDst = kIpInIpInnerDst,
      bool requireNonZeroOuterFlowLabel = false) const {
    if (frame == nullptr) {
      return false;
    }
    auto parsed = parseTrapFrameL2(frame);
    if (!parsed.has_value()) {
      return false;
    }
    if (parsed->etherType == utility::kCsigTpid) {
      return false;
    }
    if (parsed->etherType != static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV6)) {
      return false;
    }

    folly::io::Cursor cursor = parsed->payloadCursor(frame);
    return validateCsigMidpointStripInOuterIpv6(
        cursor,
        expectedOuterDst,
        expectedInnerDst,
        requireNonZeroOuterFlowLabel);
  }

  // MIDPOINT UPDATE: egress outer L2 CSIG metrics; outer IPv6 SID shift.
  bool trapFrameHasMidpointIngressCsigPassthrough(
      const folly::IOBuf* frame,
      const folly::IPAddressV6& expectedOuterDst) const {
    if (frame == nullptr) {
      return false;
    }
    auto parsed = parseTrapFrameL2(frame);
    if (!parsed.has_value() || parsed->etherType != utility::kCsigTpid) {
      return false;
    }
    folly::io::Cursor cursor =
        skipDuplicateCsigTpidInPayload(parsed->payloadCursor(frame));
    auto csigTag = utility::parseCsigTag(cursor);
    if (!csigTag.has_value()) {
      return false;
    }
    if (csigTag->signalType != utility::kCsigAbwcSignalType) {
      return false;
    }
    if (csigTag->locatorMetadata != utility::kCsigIngressPktLinkLocator ||
        csigTag->signal != utility::kCsigIngressAbwcSignal) {
      return false;
    }
    if (csigTag->nextEtherType !=
        static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV6)) {
      return false;
    }
    return validateCsigMidpointOuterIpv6(
        cursor, expectedOuterDst, /*requireNonZeroOuterFlowLabel=*/false);
  }

  bool validateCsigMidpointUpdateFrame(
      const folly::IOBuf* frame,
      const folly::IPAddressV6& expectedOuterDst,
      const folly::IPAddressV6& /*expectedInnerDst*/ = kIpInIpInnerDst,
      bool requireNonZeroOuterFlowLabel = false) const {
    if (frame == nullptr) {
      return false;
    }
    auto parsed = parseTrapFrameL2(frame);
    if (!parsed.has_value() || parsed->etherType != utility::kCsigTpid) {
      return false;
    }
    folly::io::Cursor cursor =
        skipDuplicateCsigTpidInPayload(parsed->payloadCursor(frame));

    auto csigTag = utility::parseCsigTag(cursor);
    if (!csigTag.has_value()) {
      return false;
    }
    if (csigTag->signalType != utility::kCsigAbwcSignalType) {
      return false;
    }
    if (csigTag->locatorMetadata != utility::kDefaultCsigLinkLocator) {
      return false;
    }
    if (csigTag->nextEtherType !=
        static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV6)) {
      return false;
    }
    if (!utility::isEgressUpdatedAbwcSignal(csigTag->signal)) {
      return false;
    }

    return validateCsigMidpointOuterIpv6(
        cursor, expectedOuterDst, requireNonZeroOuterFlowLabel);
  }

  bool validateCsigMidpointUpdateQuietClampFrame(
      const folly::IOBuf* frame,
      const folly::IPAddressV6& expectedOuterDst,
      const folly::IPAddressV6& /*expectedInnerDst*/ = kIpInIpInnerDst,
      bool requireNonZeroOuterFlowLabel = false) const {
    if (frame == nullptr) {
      return false;
    }
    auto parsed = parseTrapFrameL2(frame);
    if (!parsed.has_value() || parsed->etherType != utility::kCsigTpid) {
      return false;
    }
    folly::io::Cursor cursor =
        skipDuplicateCsigTpidInPayload(parsed->payloadCursor(frame));

    auto csigTag = utility::parseCsigTag(cursor);
    if (!csigTag.has_value()) {
      return false;
    }
    if (csigTag->signalType != utility::kCsigAbwcSignalType) {
      return false;
    }
    if (csigTag->locatorMetadata != utility::kCsigQuietClampEgressLinkLocator) {
      return false;
    }
    if (csigTag->nextEtherType !=
        static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV6)) {
      return false;
    }
    if (!utility::isQuietClampEgressCsigSignal(csigTag->signal)) {
      return false;
    }

    return validateCsigMidpointOuterIpv6(
        cursor, expectedOuterDst, requireNonZeroOuterFlowLabel);
  }

  void verifyMidpointCsigPassthrough(PortID egressPort) {
    auto ingressPort = this->findInjectPort({egressPort});
    resolveMidpointNeighborOnEgressPort(egressPort);
    auto hw = this->getSw()->getMonolithicHwSwitchHandler()->getHwSwitch();
    utility::configureCsigPassthroughPorts(hw, ingressPort, egressPort);
    XLOG(INFO) << "CSIG+SRv6 midpoint passthrough: outer dst "
               << kMidpointPktOuterDst << " -> expected "
               << kMidpointExpectedOuterDst << " via egress port " << egressPort
               << " (outer L2 CSIG passes through unaltered, outer IPv6 only, "
               << "egress loopback trap)";
    auto intfMac =
        getMacForFirstInterfaceWithPortsForTesting(this->getProgrammedState());
    auto tcField = static_cast<uint8_t>((kFloodDscp << 2) | kECT1);
    installEgressPortIngressTrap(egressPort);
    utility::SwSwitchPacketSnooper trapSnooper(
        this->getSw(), "csigMidpointPassthroughTrapSnooper");
    trapSnooper.ignoreUnclaimedRxPkts();
    const auto bytesBefore = *this->getLatestPortStats(egressPort).outBytes_();
    auto sendProbe = [&]() {
      auto txPacket = makeCsigMidpointProbePacket(
          intfMac, kMidpointPktOuterDst, kIpInIpInnerDst, tcField);
      sendCsigInjectSwitched(csigInjectLogLabel_, std::move(txPacket));
    };
    sendProbe();
    bool found = false;
    CsigTrapScanStats scanStats;
    WITH_RETRIES({
      scanStats = scanCsigTrapSnooper(
          trapSnooper,
          "CSIG+SRv6 midpoint passthrough",
          [&](const folly::IOBuf* frame) {
            return validateCsigMidpointPassthroughFrame(
                frame,
                kMidpointExpectedOuterDst,
                kIpInIpInnerDst,
                /*requireNonZeroOuterFlowLabel=*/false);
          });
      if (!scanStats.foundMatch && scanStats.trappedPacketsScanned == 0) {
        const auto bytesAfter =
            *this->getLatestPortStats(egressPort).outBytes_();
        XLOG(INFO) << "CSIG+SRv6 midpoint passthrough: no trapped copies yet; "
                   << "egress_port=" << egressPort << " outBytes "
                   << bytesBefore << " -> " << bytesAfter
                   << "; resending probe";
        sendProbe();
      } else if (!scanStats.foundMatch && scanStats.csigPacketsScanned > 0) {
        XLOG(INFO) << "CSIG+SRv6 midpoint passthrough: scanned "
                   << scanStats.csigPacketsScanned
                   << " CSIG trap copies without match; resending probe";
        sendProbe();
      }
      found = scanStats.foundMatch;
      EXPECT_EVENTUALLY_TRUE(found);
    });
    logCsigTrapScanSummary("CSIG+SRv6 midpoint passthrough", scanStats, found);
    XLOG(INFO) << "CSIG+SRv6 midpoint passthrough outer_dst="
               << kMidpointExpectedOuterDst << " inner_dst=" << kIpInIpInnerDst;
    ASSERT_TRUE(found)
        << "No egress loopback-trapped midpoint PASSTHROUGH frame: outer L2 CSIG "
        << "must pass through unaltered (locator 0x" << std::hex
        << +utility::kCsigIngressPktLinkLocator << ", signal 0x"
        << +utility::kCsigIngressAbwcSignal << std::dec << "), outer IPv6 dst "
        << kMidpointExpectedOuterDst;
  }

  void verifyMidpointCsigStrip(PortID egressPort) {
    auto ingressPort = this->findInjectPort({egressPort});
    resolveMidpointNeighborOnEgressPort(egressPort);
    auto hw = this->getSw()->getMonolithicHwSwitchHandler()->getHwSwitch();
    utility::configureCsigStripPorts(hw, ingressPort, egressPort);
    XLOG(INFO) << "CSIG+SRv6 midpoint strip: outer dst " << kMidpointPktOuterDst
               << " -> expected " << kMidpointExpectedOuterDst
               << " via egress port " << egressPort
               << " (ingress outer L2 CSIG 0x9900 deleted on egress; outer "
               << "IPv6 only)";
    auto intfMac =
        getMacForFirstInterfaceWithPortsForTesting(this->getProgrammedState());
    auto tcField = static_cast<uint8_t>((kFloodDscp << 2) | kECT1);
    installEgressPortIngressTrap(egressPort);
    utility::SwSwitchPacketSnooper trapSnooper(
        this->getSw(), "csigMidpointStripTrapSnooper");
    trapSnooper.ignoreUnclaimedRxPkts();
    const auto bytesBefore = *this->getLatestPortStats(egressPort).outBytes_();
    auto sendProbe = [&]() {
      auto txPacket = makeCsigMidpointProbePacket(
          intfMac, kMidpointPktOuterDst, kIpInIpInnerDst, tcField);
      sendCsigInjectSwitched(csigInjectLogLabel_, std::move(txPacket));
    };
    sendProbe();
    bool found = false;
    CsigTrapScanStats scanStats;
    WITH_RETRIES({
      scanStats = scanStripTrapSnooper(
          trapSnooper,
          "CSIG+SRv6 midpoint strip",
          [&](const folly::IOBuf* frame) {
            return validateCsigMidpointStripFrame(
                frame,
                kMidpointExpectedOuterDst,
                kIpInIpInnerDst,
                /*requireNonZeroOuterFlowLabel=*/false);
          });
      if (!scanStats.foundMatch && scanStats.trappedPacketsScanned == 0) {
        const auto bytesAfter =
            *this->getLatestPortStats(egressPort).outBytes_();
        XLOG(INFO) << "CSIG+SRv6 midpoint strip: no trapped copies yet; "
                   << "egress_port=" << egressPort << " outBytes "
                   << bytesBefore << " -> " << bytesAfter
                   << "; resending probe";
        sendProbe();
      } else if (!scanStats.foundMatch && scanStats.trappedPacketsScanned > 0) {
        XLOG(INFO) << "CSIG+SRv6 midpoint strip: scanned "
                   << scanStats.trappedPacketsScanned
                   << " trapped copies without strip match; resending probe";
        sendProbe();
      }
      found = scanStats.foundMatch;
      EXPECT_EVENTUALLY_TRUE(found);
    });
    logCsigTrapScanSummary("CSIG+SRv6 midpoint strip", scanStats, found);
    XLOG(INFO) << "CSIG+SRv6 midpoint strip outer_dst="
               << kMidpointExpectedOuterDst << " inner_dst=" << kIpInIpInnerDst;
    ASSERT_TRUE(found)
        << "No egress loopback-trapped midpoint STRIP frame: ingress outer "
        << "L2 CSIG (0x9900) must be deleted, plain outer IPv6 dst "
        << kMidpointExpectedOuterDst;
  }

  void verifyMidpointCsigUpdate(
      PortID egressPort,
      const folly::IPAddressV6& expectedOuterDst = kMidpointExpectedOuterDst) {
    auto ingressPort = this->findInjectPort({egressPort});
    resolveMidpointNeighborOnEgressPort(egressPort);
    resolveGlobalNeighborOnEgressPort(egressPort);
    configureCsigOnIngressAndEgressPorts(ingressPort, egressPort);
    XLOG(INFO) << "CSIG+SRv6 midpoint UPDATE: outer dst "
               << kMidpointPktOuterDst << " -> expected " << expectedOuterDst
               << " via egress port " << egressPort
               << " (outer L2 CSIG metrics updated; outer IPv6 only)";

    std::map<PortID, int64_t> bytesBefore;
    bytesBefore[egressPort] = *this->getLatestPortStats(egressPort).outBytes_();

    auto intfMac =
        getMacForFirstInterfaceWithPortsForTesting(this->getProgrammedState());
    auto tcField = static_cast<uint8_t>((kFloodDscp << 2) | kECT1);
    buildCsigEgressLineRateCongestion(egressPort);
    installEgressPortIngressTrapForTgenProbe(egressPort);
    utility::SwSwitchPacketSnooper trapSnooper(
        this->getSw(), "csigMidpointUpdateTrapSnooper");
    trapSnooper.ignoreUnclaimedRxPkts();

    auto sendProbe = [&]() {
      auto txPacket = makeCsigMidpointProbePacket(
          intfMac, kMidpointPktOuterDst, kIpInIpInnerDst, tcField);
      sendCsigInjectSwitched(csigInjectLogLabel_, std::move(txPacket));
    };
    sendProbe();

    bool found = false;
    CsigTrapScanStats scanStats;
    WITH_RETRIES({
      scanStats = scanCsigTrapSnooper(
          trapSnooper,
          "CSIG+SRv6 midpoint update",
          [&](const folly::IOBuf* frame) {
            return validateCsigMidpointUpdateFrame(
                frame,
                expectedOuterDst,
                kIpInIpInnerDst,
                /*requireNonZeroOuterFlowLabel=*/false);
          });
      found = scanStats.foundMatch;

      auto bytesAfter = *this->getLatestPortStats(egressPort).outBytes_();
      EXPECT_EVENTUALLY_TRUE(bytesAfter > bytesBefore[egressPort]);
      if (!scanStats.foundMatch && scanStats.trappedPacketsScanned == 0) {
        XLOG(INFO) << "CSIG+SRv6 midpoint update: no trapped copies yet; "
                   << "egress_port=" << egressPort << " outBytes "
                   << bytesBefore[egressPort] << " -> " << bytesAfter
                   << "; resending probe";
        sendProbe();
      } else if (!scanStats.foundMatch && scanStats.csigPacketsScanned > 0) {
        XLOG(INFO) << "CSIG+SRv6 midpoint update: scanned "
                   << scanStats.csigPacketsScanned
                   << " CSIG trap copies without match; resending probe";
        sendProbe();
      }
      EXPECT_EVENTUALLY_TRUE(found);
    });
    logCsigTrapScanSummary("CSIG+SRv6 midpoint update", scanStats, found);
    XLOG(INFO) << "CSIG+SRv6 midpoint update outer_dst=" << expectedOuterDst
               << " inner_dst=" << kIpInIpInnerDst;
    if (!found && scanStats.csigPacketsScanned > 0) {
      XLOG(ERR)
          << "CSIG+SRv6 midpoint update: trapped copies had outer L2 CSIG "
          << "but none matched egress UPDATE (loc 0x" << std::hex
          << +utility::kDefaultCsigLinkLocator << std::dec
          << "). If traps show ingress loc 0x" << std::hex
          << +utility::kCsigIngressPktLinkLocator << " signal 0x"
          << +utility::kCsigIngressAbwcSignal << std::dec << " with outer dst "
          << expectedOuterDst
          << ", HW applied midpoint SID shift without CSIG UPDATE.";
    }
    ASSERT_TRUE(found)
        << "No egress loopback-trapped midpoint UPDATE frame: outer L2 CSIG "
        << "must carry updated egress metrics (locator 0x" << std::hex
        << +utility::kDefaultCsigLinkLocator << std::dec
        << ", egress-updated ABWC), outer IPv6 dst " << expectedOuterDst
        << " after scanning " << scanStats.trappedPacketsScanned
        << " trapped copies (" << scanStats.csigPacketsScanned << " CSIG, "
        << scanStats.nonCsigPacketsSkipped << " non-CSIG skipped).";
  }

  bool validateCsigBsidPassthroughInOuterIpv6(
      folly::io::Cursor& cursorAtOuterIpv6,
      const folly::IPAddressV6& expectedSid,
      const folly::IPAddressV6& expectedInnerDst,
      bool requireNonZeroOuterFlowLabel = false) const {
    IPv6Hdr outerHdr(cursorAtOuterIpv6);
    if (outerHdr.dstAddr != expectedSid) {
      return false;
    }
    if (requireNonZeroOuterFlowLabel && outerHdr.flowLabel == 0) {
      return false;
    }
    if ((outerHdr.trafficClass >> 2) != kFloodDscp) {
      return false;
    }
    if (outerHdr.hopLimit != kFloodTtl - 1) {
      return false;
    }

    folly::io::Cursor payloadStart = cursorAtOuterIpv6;
    if (outerPayloadHasIngressPassthroughCsig(payloadStart)) {
      return false;
    }
    if (outerHdr.nextHeader == static_cast<uint8_t>(IP_PROTO::IP_PROTO_IPV6)) {
      if (tryMatchInnerIpv6DstAtOffset(payloadStart, 0, expectedInnerDst)) {
        return true;
      }
    }
    return scanStripSrv6InnerIpv6Dst(payloadStart, expectedInnerDst);
  }

  // BSID PASSTHROUGH: ingress CSIG from original outer L2 copied to new BSid
  // outer L2; SRv6 payload carries ip-in-ip inner without payload CSIG.
  bool validateCsigBsidPassthroughFrame(
      const folly::IOBuf* frame,
      const folly::IPAddressV6& expectedSid,
      const folly::IPAddressV6& expectedInnerDst = kIpInIpInnerDst,
      bool requireNonZeroOuterFlowLabel = false) const {
    if (frame == nullptr) {
      return false;
    }
    auto parsed = parseTrapFrameL2(frame);
    if (!parsed.has_value() || parsed->etherType != utility::kCsigTpid) {
      return false;
    }
    folly::io::Cursor cursor = parsed->payloadCursor(frame);

    auto csigTag = utility::parseCsigTag(cursor);
    if (!csigTag.has_value()) {
      return false;
    }
    if (csigTag->signalType != utility::kCsigAbwcSignalType) {
      return false;
    }
    if (csigTag->locatorMetadata != utility::kCsigIngressPktLinkLocator) {
      return false;
    }
    if (csigTag->signal != utility::kCsigIngressAbwcSignal) {
      return false;
    }
    if (csigTag->nextEtherType !=
        static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV6)) {
      return false;
    }

    return validateCsigBsidPassthroughInOuterIpv6(
        cursor, expectedSid, expectedInnerDst, requireNonZeroOuterFlowLabel);
  }

  // BSID STRIP: original outer L2 CSIG deleted; new BSid outer L2 has no CSIG.
  bool validateCsigBsidStripFrame(
      const folly::IOBuf* frame,
      const folly::IPAddressV6& expectedSid,
      const folly::IPAddressV6& expectedInnerDst = kIpInIpInnerDst,
      bool requireNonZeroOuterFlowLabel = false) const {
    return validateCsigStripSrv6Frame(
        frame, expectedSid, expectedInnerDst, requireNonZeroOuterFlowLabel);
  }

  bool validateCsigBsidUpdateInOuterIpv6(
      folly::io::Cursor& cursorAtOuterIpv6,
      const folly::IPAddressV6& expectedSid,
      const folly::IPAddressV6& expectedInnerDst,
      bool requireNonZeroOuterFlowLabel = false) const {
    IPv6Hdr outerHdr(cursorAtOuterIpv6);
    if (outerHdr.dstAddr != expectedSid) {
      return false;
    }
    if (requireNonZeroOuterFlowLabel && outerHdr.flowLabel == 0) {
      return false;
    }
    if ((outerHdr.trafficClass >> 2) != kFloodDscp) {
      return false;
    }
    if (!egressTrapHopLimitOk(outerHdr.hopLimit)) {
      return false;
    }

    folly::io::Cursor payloadStart = cursorAtOuterIpv6;
    if (outerPayloadHasIngressPassthroughCsig(payloadStart)) {
      return false;
    }
    return scanStripSrv6InnerIpv6Dst(payloadStart, expectedInnerDst);
  }

  // BSID UPDATE: new BSid outer L2 carries egress CSIG; original ingress CSIG
  // wiped; metrics reflect the new BSid segment path.
  bool validateCsigBsidUpdateFrame(
      const folly::IOBuf* frame,
      const folly::IPAddressV6& expectedSid,
      const folly::IPAddressV6& expectedInnerDst = kIpInIpInnerDst,
      bool requireNonZeroOuterFlowLabel = false) const {
    if (frame == nullptr) {
      return false;
    }
    auto parsed = parseTrapFrameL2(frame);
    if (!parsed.has_value() || parsed->etherType != utility::kCsigTpid) {
      return false;
    }
    folly::io::Cursor cursor = parsed->payloadCursor(frame);

    auto csigTag = utility::parseCsigTag(cursor);
    if (!csigTag.has_value()) {
      return false;
    }
    if (csigTag->signalType != utility::kCsigAbwcSignalType) {
      return false;
    }
    if (csigTag->locatorMetadata != utility::kDefaultCsigLinkLocator) {
      return false;
    }
    if (csigTag->nextEtherType !=
        static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV6)) {
      return false;
    }
    if (!utility::isEgressUpdatedAbwcSignal(csigTag->signal)) {
      return false;
    }

    return validateCsigBsidUpdateInOuterIpv6(
        cursor, expectedSid, expectedInnerDst, requireNonZeroOuterFlowLabel);
  }

  bool validateCsigBsidUpdateQuietClampFrame(
      const folly::IOBuf* frame,
      const folly::IPAddressV6& expectedSid,
      const folly::IPAddressV6& expectedInnerDst = kIpInIpInnerDst,
      bool requireNonZeroOuterFlowLabel = false) const {
    if (frame == nullptr) {
      return false;
    }
    auto parsed = parseTrapFrameL2(frame);
    if (!parsed.has_value() || parsed->etherType != utility::kCsigTpid) {
      return false;
    }
    folly::io::Cursor cursor = parsed->payloadCursor(frame);

    auto csigTag = utility::parseCsigTag(cursor);
    if (!csigTag.has_value()) {
      return false;
    }
    if (csigTag->signalType != utility::kCsigAbwcSignalType) {
      return false;
    }
    if (csigTag->locatorMetadata != utility::kCsigQuietClampEgressLinkLocator) {
      return false;
    }
    if (csigTag->nextEtherType !=
        static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV6)) {
      return false;
    }
    if (!utility::isQuietClampEgressCsigSignal(csigTag->signal)) {
      return false;
    }

    return validateCsigBsidUpdateInOuterIpv6(
        cursor, expectedSid, expectedInnerDst, requireNonZeroOuterFlowLabel);
  }

  void verifyBindingSidCsigPassthrough(PortID egressPort) {
    auto ingressPort = this->findInjectPort({egressPort});
    resolveGlobalNeighborOnEgressPort(egressPort);
    auto hw = this->getSw()->getMonolithicHwSwitchHandler()->getHwSwitch();
    utility::configureCsigPassthroughPorts(hw, ingressPort, egressPort);
    XLOG(INFO) << "CSIG+SRv6 binding SID passthrough: mySid "
               << kBindingMySidPrefix << " -> SID " << kSid0
               << " via egress port " << egressPort
               << " (ingress CSIG copied to new BSid outer L2, egress loopback "
               << "trap)";
    auto intfMac =
        getMacForFirstInterfaceWithPortsForTesting(this->getProgrammedState());
    auto tcField = static_cast<uint8_t>((kFloodDscp << 2) | kECT1);
    installEgressPortIngressTrap(egressPort);
    utility::SwSwitchPacketSnooper trapSnooper(
        this->getSw(), "csigBsidPassthroughTrapSnooper");
    trapSnooper.ignoreUnclaimedRxPkts();
    const auto bytesBefore = *this->getLatestPortStats(egressPort).outBytes_();
    auto sendProbe = [&]() {
      auto txPacket = makeCsigIpInIpProbePacket(
          intfMac, kBindingMySidPrefix, kIpInIpInnerDst, tcField);
      sendCsigInjectSwitched(csigInjectLogLabel_, std::move(txPacket));
    };
    sendProbe();
    bool found = false;
    CsigTrapScanStats scanStats;
    WITH_RETRIES({
      scanStats = scanCsigTrapSnooper(
          trapSnooper,
          "CSIG+SRv6 binding SID passthrough",
          [&](const folly::IOBuf* frame) {
            return validateCsigBsidPassthroughFrame(
                frame,
                kSid0,
                kIpInIpInnerDst,
                /*requireNonZeroOuterFlowLabel=*/false);
          });
      if (!scanStats.foundMatch && scanStats.trappedPacketsScanned == 0) {
        const auto bytesAfter =
            *this->getLatestPortStats(egressPort).outBytes_();
        XLOG(INFO) << "CSIG+SRv6 binding SID passthrough: no trapped copies "
                   << "yet; egress_port=" << egressPort << " outBytes "
                   << bytesBefore << " -> " << bytesAfter
                   << "; resending probe";
        sendProbe();
      } else if (!scanStats.foundMatch && scanStats.csigPacketsScanned > 0) {
        XLOG(INFO) << "CSIG+SRv6 binding SID passthrough: scanned "
                   << scanStats.csigPacketsScanned
                   << " CSIG trap copies without match; resending probe";
        sendProbe();
      }
      found = scanStats.foundMatch;
      EXPECT_EVENTUALLY_TRUE(found);
    });
    logCsigTrapScanSummary(
        "CSIG+SRv6 binding SID passthrough", scanStats, found);
    XLOG(INFO) << "CSIG+SRv6 binding SID passthrough sid=" << kSid0
               << " inner_dst=" << kIpInIpInnerDst;
    ASSERT_TRUE(found)
        << "No egress loopback-trapped BSID PASSTHROUGH frame: ingress CSIG "
        << "must be copied to new BSid outer L2 (locator 0x" << std::hex
        << +utility::kCsigIngressPktLinkLocator << ", signal 0x"
        << +utility::kCsigIngressAbwcSignal << std::dec << "), SID " << kSid0
        << ", inner dst " << kIpInIpInnerDst;
  }

  void verifyBindingSidCsigStrip(PortID egressPort) {
    auto ingressPort = this->findInjectPort({egressPort});
    resolveGlobalNeighborOnEgressPort(egressPort);
    auto hw = this->getSw()->getMonolithicHwSwitchHandler()->getHwSwitch();
    utility::configureCsigStripPorts(hw, ingressPort, egressPort);
    XLOG(INFO) << "CSIG+SRv6 binding SID strip: mySid " << kBindingMySidPrefix
               << " -> SID " << kSid0 << " via egress port " << egressPort
               << " (original outer L2 CSIG deleted; new BSid frame plain)";
    auto intfMac =
        getMacForFirstInterfaceWithPortsForTesting(this->getProgrammedState());
    auto tcField = static_cast<uint8_t>((kFloodDscp << 2) | kECT1);
    installEgressPortIngressTrap(egressPort);
    utility::SwSwitchPacketSnooper trapSnooper(
        this->getSw(), "csigBsidStripTrapSnooper");
    trapSnooper.ignoreUnclaimedRxPkts();
    const auto bytesBefore = *this->getLatestPortStats(egressPort).outBytes_();
    auto sendProbe = [&]() {
      auto txPacket = makeCsigIpInIpProbePacket(
          intfMac, kBindingMySidPrefix, kIpInIpInnerDst, tcField);
      sendCsigInjectSwitched(csigInjectLogLabel_, std::move(txPacket));
    };
    sendProbe();
    bool found = false;
    CsigTrapScanStats scanStats;
    WITH_RETRIES({
      scanStats = scanStripTrapSnooper(
          trapSnooper,
          "CSIG+SRv6 binding SID strip",
          [&](const folly::IOBuf* frame) {
            return validateCsigBsidStripFrame(
                frame,
                kSid0,
                kIpInIpInnerDst,
                /*requireNonZeroOuterFlowLabel=*/false);
          });
      if (!scanStats.foundMatch && scanStats.trappedPacketsScanned == 0) {
        const auto bytesAfter =
            *this->getLatestPortStats(egressPort).outBytes_();
        XLOG(INFO) << "CSIG+SRv6 binding SID strip: no trapped copies yet; "
                   << "egress_port=" << egressPort << " outBytes "
                   << bytesBefore << " -> " << bytesAfter
                   << "; resending probe";
        sendProbe();
      } else if (!scanStats.foundMatch && scanStats.trappedPacketsScanned > 0) {
        XLOG(INFO) << "CSIG+SRv6 binding SID strip: scanned "
                   << scanStats.trappedPacketsScanned
                   << " trapped copies without strip match; resending probe";
        sendProbe();
      }
      found = scanStats.foundMatch;
      EXPECT_EVENTUALLY_TRUE(found);
    });
    logCsigTrapScanSummary("CSIG+SRv6 binding SID strip", scanStats, found);
    XLOG(INFO) << "CSIG+SRv6 binding SID strip sid=" << kSid0
               << " inner_dst=" << kIpInIpInnerDst;
    ASSERT_TRUE(found)
        << "No egress loopback-trapped BSID STRIP frame: new BSid outer L2 "
        << "must have no CSIG telemetry, SID " << kSid0 << ", inner dst "
        << kIpInIpInnerDst;
  }

  void verifyBindingSidCsigUpdate(PortID egressPort) {
    auto ingressPort = this->findInjectPort({egressPort});
    resolveGlobalNeighborOnEgressPort(egressPort);
    configureCsigOnIngressAndEgressPorts(ingressPort, egressPort);
    XLOG(INFO) << "CSIG+SRv6 binding SID update: mySid " << kBindingMySidPrefix
               << " -> SID " << kSid0 << " via egress port " << egressPort
               << " (new BSid outer L2 egress CSIG; original ingress wiped)";
    std::map<PortID, int64_t> bytesBefore;
    bytesBefore[egressPort] = *this->getLatestPortStats(egressPort).outBytes_();
    auto intfMac =
        getMacForFirstInterfaceWithPortsForTesting(this->getProgrammedState());
    auto tcField = static_cast<uint8_t>((kFloodDscp << 2) | kECT1);
    buildCsigEgressLineRateCongestion(egressPort);
    installEgressPortIngressTrapForTgenProbe(egressPort);
    utility::SwSwitchPacketSnooper trapSnooper(
        this->getSw(), "csigBsidUpdateTrapSnooper");
    trapSnooper.ignoreUnclaimedRxPkts();
    auto sendProbe = [&]() {
      auto txPacket = makeCsigIpInIpProbePacket(
          intfMac, kBindingMySidPrefix, kIpInIpInnerDst, tcField);
      sendCsigInjectSwitched(csigInjectLogLabel_, std::move(txPacket));
    };
    sendProbe();
    bool found = false;
    CsigTrapScanStats scanStats;
    WITH_RETRIES({
      scanStats = scanCsigTrapSnooper(
          trapSnooper,
          "CSIG+SRv6 binding SID update",
          [&](const folly::IOBuf* frame) {
            return validateCsigBsidUpdateFrame(
                frame,
                kSid0,
                kIpInIpInnerDst,
                /*requireNonZeroOuterFlowLabel=*/false);
          });
      found = scanStats.foundMatch;

      auto bytesAfter = *this->getLatestPortStats(egressPort).outBytes_();
      EXPECT_EVENTUALLY_TRUE(bytesAfter > bytesBefore[egressPort]);
      if (!scanStats.foundMatch && scanStats.trappedPacketsScanned == 0) {
        XLOG(INFO) << "CSIG+SRv6 binding SID update: no trapped copies yet; "
                   << "egress_port=" << egressPort << " outBytes "
                   << bytesBefore[egressPort] << " -> " << bytesAfter
                   << "; resending probe";
        sendProbe();
      } else if (!scanStats.foundMatch && scanStats.csigPacketsScanned > 0) {
        XLOG(INFO) << "CSIG+SRv6 binding SID update: scanned "
                   << scanStats.csigPacketsScanned
                   << " CSIG trap copies without match; resending probe";
        sendProbe();
      }
      EXPECT_EVENTUALLY_TRUE(found);
    });
    logCsigTrapScanSummary("CSIG+SRv6 binding SID update", scanStats, found);
    if (!found && scanStats.csigPacketsScanned > 0) {
      XLOG(ERR) << "CSIG+SRv6 binding SID update: trapped copies never had "
                << "new BSid outer L2 UPDATE CSIG (0x9900 loc 0x" << std::hex
                << +utility::kDefaultCsigLinkLocator << std::dec
                << ", egress-updated ABWC). If traps show L2 ethertype 0x86dd "
                << "outer SID " << kSid0
                << " with ingress shim 99 00 2b 85 embedded in SRv6 payload, "
                << "HW applied BSID encapsulation without CSIG UPDATE on the "
                << "new outer L2 (same class as encap STRIP/UPDATE HW gap).";
    }
    ASSERT_TRUE(found)
        << "No egress loopback-trapped BSID UPDATE frame: new BSid outer L2 "
        << "must carry egress CSIG (locator 0x" << std::hex
        << +utility::kDefaultCsigLinkLocator << std::dec
        << ", congested ABWC), original ingress CSIG wiped, SID " << kSid0
        << ", inner dst " << kIpInIpInnerDst << " after scanning "
        << scanStats.trappedPacketsScanned << " trapped copies ("
        << scanStats.csigPacketsScanned << " CSIG, "
        << scanStats.nonCsigPacketsSkipped << " non-CSIG skipped).";
  }

  void verifySrv6EncapCsigUpdateQuietClamp(
      PortID egressPort,
      const folly::IPAddressV6& expectedSid) {
    csigInjectLogLabel_ = "CSIG+SRv6 encap quiet-clamp UPDATE probe";
    auto ingressPort = findInjectPort({egressPort});
    configureCsigEgressUpdateQuietClamp(ingressPort, egressPort);
    XLOG(INFO) << "CSIG+SRv6 encap quiet-clamp UPDATE: encap dst "
               << kEncapRouteDstIp << " SID " << expectedSid << " ingress "
               << ingressPort << " egress " << egressPort
               << " (SDK: ingress PASSTHROUGH, egress PASS->UPDATE; "
               << "inject wire 99 00 2b 85 signal=0x17 lm=5 -> egress 99 00 28 "
               << std::hex << +utility::kCsigQuietClampEgressLinkLocator
               << std::dec << " (lm=port programmed))";

    auto intfMac =
        getMacForFirstInterfaceWithPortsForTesting(this->getProgrammedState());
    auto tcField = static_cast<uint8_t>((kFloodDscp << 2) | kECT1);
    installEgressPortIngressTrap(egressPort);
    utility::SwSwitchPacketSnooper trapSnooper(
        this->getSw(), "csigSrv6EncapQuietClampUpdateTrapSnooper");
    trapSnooper.ignoreUnclaimedRxPkts();

    auto sendProbe = [&]() {
      auto txPacket = makeCsigEncapQuietClampProbePacket(intfMac, tcField);
      sendCsigInjectSwitched(csigInjectLogLabel_, std::move(txPacket));
    };
    sendProbe();

    bool found = false;
    CsigTrapScanStats scanStats;
    WITH_RETRIES({
      scanStats = scanCsigSrv6EncapUpdateTrapSnooper(
          trapSnooper,
          "CSIG+SRv6 encap quiet-clamp update",
          expectedSid,
          [&](const folly::IOBuf* frame) {
            return validateCsigSrv6EncapQuietClampFrame(frame, expectedSid);
          });
      found = scanStats.foundMatch;
      if (!scanStats.foundMatch && scanStats.trappedPacketsScanned == 0) {
        sendProbe();
      } else if (!scanStats.foundMatch && scanStats.csigPacketsScanned > 0) {
        sendProbe();
      }
      EXPECT_EVENTUALLY_TRUE(found);
    });
    logCsigTrapScanSummary(
        "CSIG+SRv6 encap quiet-clamp update", scanStats, found);
    ASSERT_TRUE(found)
        << "No egress loopback-trapped SRv6 encap quiet-clamp UPDATE frame: "
        << "outer L2 CSIG shim 99 00 28 " << std::hex
        << +utility::kCsigQuietClampEgressLinkLocator << std::dec
        << " (wire signal=0x10 lm=port programmed), "
        << "no ingress CSIG in SRv6 payload, SID " << expectedSid;
  }

  void verifyDecapCsigUpdateQuietClamp(PortID egressPort) {
    csigInjectLogLabel_ = "CSIG+SRv6 decap quiet-clamp UPDATE probe";
    resolveGlobalNeighborOnEgressPort(egressPort);
    auto ingressPort = findInjectPort({egressPort});
    configureCsigEgressUpdateQuietClamp(ingressPort, egressPort);
    XLOG(INFO) << "CSIG+SRv6 decap quiet-clamp UPDATE: mySid "
               << kDecapMySidAddr << " inner dst " << kEncapRouteDstIp
               << " ingress " << ingressPort << " egress " << egressPort
               << " (SDK: ingress PASSTHROUGH, egress PASS->UPDATE; "
               << "inject wire 99 00 2b 85 -> egress 99 00 28 " << std::hex
               << +utility::kCsigQuietClampEgressLinkLocator << std::dec << ")";

    auto intfMac =
        getMacForFirstInterfaceWithPortsForTesting(this->getProgrammedState());
    auto tcField = static_cast<uint8_t>((kFloodDscp << 2) | kECT1);
    installEgressPortIngressTrap(egressPort);
    utility::SwSwitchPacketSnooper trapSnooper(
        this->getSw(), "csigDecapQuietClampUpdateTrapSnooper");
    trapSnooper.ignoreUnclaimedRxPkts();

    auto sendProbe = [&]() {
      auto txPacket = makeCsigDecapQuietClampProbePacket(intfMac, tcField);
      sendCsigInjectSwitched(csigInjectLogLabel_, std::move(txPacket));
    };
    sendProbe();

    bool found = false;
    CsigTrapScanStats scanStats;
    const folly::IPAddress expectedInnerDst(kEncapRouteDstIp);
    WITH_RETRIES({
      scanStats = scanCsigDecapUpdateTrapSnooper(
          trapSnooper,
          "CSIG+SRv6 decap quiet-clamp update",
          expectedInnerDst,
          [&](const folly::IOBuf* frame) {
            return validateCsigDecapUpdateQuietClampFrame(
                frame, expectedInnerDst);
          });
      found = scanStats.foundMatch;
      if (!scanStats.foundMatch && scanStats.trappedPacketsScanned == 0) {
        sendProbe();
      } else if (!scanStats.foundMatch && scanStats.csigPacketsScanned > 0) {
        sendProbe();
      }
      EXPECT_EVENTUALLY_TRUE(found);
    });
    logCsigTrapScanSummary(
        "CSIG+SRv6 decap quiet-clamp update", scanStats, found);
    ASSERT_TRUE(found)
        << "No egress loopback-trapped decap quiet-clamp UPDATE frame: "
        << "decapped inner L2 CSIG shim 99 00 28 " << std::hex
        << +utility::kCsigQuietClampEgressLinkLocator << std::dec
        << " (wire signal=0x10 lm=port programmed), "
        << "inner dst " << kEncapRouteDstIp;
  }

  void verifyMidpointCsigUpdateQuietClamp(
      PortID egressPort,
      const folly::IPAddressV6& expectedOuterDst = kMidpointExpectedOuterDst) {
    csigInjectLogLabel_ = "CSIG+SRv6 midpoint quiet-clamp UPDATE probe";
    resolveMidpointNeighborOnEgressPort(egressPort);
    auto ingressPort = findInjectPort({egressPort});
    configureCsigEgressUpdateQuietClamp(ingressPort, egressPort);
    XLOG(INFO) << "CSIG+SRv6 midpoint quiet-clamp UPDATE: outer dst "
               << kMidpointPktOuterDst << " -> expected " << expectedOuterDst
               << " ingress " << ingressPort << " egress " << egressPort
               << " (SDK: ingress PASSTHROUGH, egress PASS->UPDATE; "
               << "inject wire 99 00 2b 85 -> egress 99 00 28 " << std::hex
               << +utility::kCsigQuietClampEgressLinkLocator << std::dec << ")";

    auto intfMac =
        getMacForFirstInterfaceWithPortsForTesting(this->getProgrammedState());
    auto tcField = static_cast<uint8_t>((kFloodDscp << 2) | kECT1);
    installEgressPortIngressTrap(egressPort);
    utility::SwSwitchPacketSnooper trapSnooper(
        this->getSw(), "csigMidpointQuietClampUpdateTrapSnooper");
    trapSnooper.ignoreUnclaimedRxPkts();

    auto sendProbe = [&]() {
      auto txPacket = makeCsigMidpointQuietClampProbePacket(
          intfMac, kMidpointPktOuterDst, kIpInIpInnerDst, tcField);
      sendCsigInjectSwitched(csigInjectLogLabel_, std::move(txPacket));
    };
    sendProbe();

    bool found = false;
    CsigTrapScanStats scanStats;
    WITH_RETRIES({
      scanStats = scanCsigTrapSnooper(
          trapSnooper,
          "CSIG+SRv6 midpoint quiet-clamp update",
          [&](const folly::IOBuf* frame) {
            return validateCsigMidpointUpdateQuietClampFrame(
                frame,
                expectedOuterDst,
                kIpInIpInnerDst,
                /*requireNonZeroOuterFlowLabel=*/false);
          });
      found = scanStats.foundMatch;
      if (!scanStats.foundMatch && scanStats.trappedPacketsScanned == 0) {
        sendProbe();
      } else if (!scanStats.foundMatch && scanStats.csigPacketsScanned > 0) {
        sendProbe();
      }
      EXPECT_EVENTUALLY_TRUE(found);
    });
    logCsigTrapScanSummary(
        "CSIG+SRv6 midpoint quiet-clamp update", scanStats, found);
    ASSERT_TRUE(found)
        << "No egress loopback-trapped midpoint quiet-clamp UPDATE frame: "
        << "outer L2 CSIG shim 99 00 28 " << std::hex
        << +utility::kCsigQuietClampEgressLinkLocator << std::dec
        << " (wire signal=0x10 lm=port programmed), "
        << "outer IPv6 dst " << expectedOuterDst;
  }

  void verifyBindingSidCsigUpdateQuietClamp(PortID egressPort) {
    csigInjectLogLabel_ = "CSIG+SRv6 binding SID quiet-clamp UPDATE probe";
    resolveGlobalNeighborOnEgressPort(egressPort);
    auto ingressPort = findInjectPort({egressPort});
    configureCsigEgressUpdateQuietClamp(ingressPort, egressPort);
    XLOG(INFO) << "CSIG+SRv6 binding SID quiet-clamp UPDATE: mySid "
               << kBindingMySidPrefix << " -> SID " << kSid0 << " ingress "
               << ingressPort << " egress " << egressPort
               << " (SDK: ingress PASSTHROUGH, egress PASS->UPDATE; "
               << "inject wire 99 00 2b 85 -> egress 99 00 28 " << std::hex
               << +utility::kCsigQuietClampEgressLinkLocator << std::dec << ")";

    auto intfMac =
        getMacForFirstInterfaceWithPortsForTesting(this->getProgrammedState());
    auto tcField = static_cast<uint8_t>((kFloodDscp << 2) | kECT1);
    installEgressPortIngressTrap(egressPort);
    utility::SwSwitchPacketSnooper trapSnooper(
        this->getSw(), "csigBsidQuietClampUpdateTrapSnooper");
    trapSnooper.ignoreUnclaimedRxPkts();

    auto sendProbe = [&]() {
      auto txPacket = makeCsigBsidQuietClampProbePacket(
          intfMac, kBindingMySidPrefix, kIpInIpInnerDst, tcField);
      sendCsigInjectSwitched(csigInjectLogLabel_, std::move(txPacket));
    };
    sendProbe();

    bool found = false;
    CsigTrapScanStats scanStats;
    WITH_RETRIES({
      scanStats = scanCsigTrapSnooper(
          trapSnooper,
          "CSIG+SRv6 binding SID quiet-clamp update",
          [&](const folly::IOBuf* frame) {
            return validateCsigBsidUpdateQuietClampFrame(
                frame,
                kSid0,
                kIpInIpInnerDst,
                /*requireNonZeroOuterFlowLabel=*/false);
          });
      found = scanStats.foundMatch;
      if (!scanStats.foundMatch && scanStats.trappedPacketsScanned == 0) {
        sendProbe();
      } else if (!scanStats.foundMatch && scanStats.csigPacketsScanned > 0) {
        sendProbe();
      }
      EXPECT_EVENTUALLY_TRUE(found);
    });
    logCsigTrapScanSummary(
        "CSIG+SRv6 binding SID quiet-clamp update", scanStats, found);
    ASSERT_TRUE(found)
        << "No egress loopback-trapped BSID quiet-clamp UPDATE frame: "
        << "new BSid outer L2 CSIG shim 99 00 28 " << std::hex
        << +utility::kCsigQuietClampEgressLinkLocator << std::dec
        << " (wire signal=0x10 lm=port programmed), "
        << "SID " << kSid0 << ", inner dst " << kIpInIpInnerDst;
  }

  // CSIG dynamic ABWC uses egress port TX-rate (SDK ARC poll_csig), not TM
  // queue depth. Flood at line rate with TX enabled; validate CSIG on egress
  // trap.
  void verifySrv6EncapWithCsigTagUnderCongestion(
      PortID egressPort,
      const folly::IPAddressV6& expectedSid) {
    auto ingressPort = this->findInjectPort({egressPort});
    configureCsigOnIngressAndEgressPorts(ingressPort, egressPort);
    XLOG(INFO) << "CSIG+SRv6 encap UPDATE: encap dst " << kEncapRouteDstIp
               << " SID " << expectedSid << " via egress port " << egressPort
               << " (new outer L2 CSIG only; no CSIG in SRv6 payload)";

    std::map<PortID, int64_t> bytesBefore;
    bytesBefore[egressPort] = *this->getLatestPortStats(egressPort).outBytes_();

    auto intfMac =
        getMacForFirstInterfaceWithPortsForTesting(this->getProgrammedState());
    auto tcField = static_cast<uint8_t>((kFloodDscp << 2) | kECT1);

    // Saturate egress NH with a routed dataplane loop; probe uses SRv6 encap.
    buildCsigEgressLineRateCongestion(egressPort);
    installEgressPortIngressTrapForTgenProbe(egressPort);

    utility::SwSwitchPacketSnooper trapSnooper(
        this->getSw(), "csigSrv6EncapUpdateTrapSnooper");
    trapSnooper.ignoreUnclaimedRxPkts();

    auto sendProbe = [&]() {
      auto txPacket = makeCsigEncapTrafficPacket(intfMac, tcField);
      sendCsigInjectSwitched(csigInjectLogLabel_, std::move(txPacket));
    };
    sendProbe();

    bool foundCsigOnEgressTrap = false;
    CsigTrapScanStats scanStats;
    WITH_RETRIES({
      scanStats = scanCsigSrv6EncapUpdateTrapSnooper(
          trapSnooper,
          "CSIG+SRv6 encap update",
          expectedSid,
          [&](const folly::IOBuf* frame) {
            return validateCsigSrv6EncapFrame(frame, expectedSid);
          });
      foundCsigOnEgressTrap = scanStats.foundMatch;

      auto bytesAfter = *this->getLatestPortStats(egressPort).outBytes_();
      EXPECT_EVENTUALLY_TRUE(bytesAfter > bytesBefore[egressPort]);
      if (!scanStats.foundMatch && scanStats.trappedPacketsScanned == 0) {
        XLOG(INFO) << "CSIG+SRv6 encap update: no encap-probe trapped copies "
                   << "yet; egress_port=" << egressPort << " outBytes "
                   << bytesBefore[egressPort] << " -> " << bytesAfter
                   << "; resending probe";
        sendProbe();
      } else if (!scanStats.foundMatch && scanStats.csigPacketsScanned > 0) {
        XLOG(INFO) << "CSIG+SRv6 encap update: scanned "
                   << scanStats.csigPacketsScanned
                   << " encap-probe CSIG trap copies without match; resending "
                   << "probe";
        sendProbe();
      }
      EXPECT_EVENTUALLY_TRUE(foundCsigOnEgressTrap);
    });
    logCsigTrapScanSummary(
        "CSIG+SRv6 encap update", scanStats, foundCsigOnEgressTrap);
    if (!foundCsigOnEgressTrap && scanStats.csigPacketsScanned > 0) {
      XLOG(ERR)
          << "CSIG+SRv6 encap update: trapped copies never had outer L2 "
          << "UPDATE CSIG (0x9900 loc 0x" << std::hex
          << +utility::kDefaultCsigLinkLocator << std::dec
          << ", egress-updated ABWC). If traps show L2 ethertype 0x86dd "
          << "outer SID " << expectedSid
          << " with ingress shim 99 00 2b 85 embedded in the SRv6 "
          << "payload, HW performed encap without CSIG UPDATE (same class "
          << "as encap STRIP: ingress CSIG copied into payload, not "
          << "rewritten on new outer L2). Routed csigBasicUpdate can "
          << "still pass while SRv6 encap UPDATE does not.";
    }
    ASSERT_TRUE(foundCsigOnEgressTrap)
        << "No egress loopback-trapped SRv6 encap UPDATE frame: outer L2 CSIG "
        << "with congested ABWC, no ingress CSIG in SRv6 payload, SID "
        << expectedSid << " after scanning " << scanStats.trappedPacketsScanned
        << " trapped copies (" << scanStats.csigPacketsScanned << " CSIG, "
        << scanStats.nonCsigPacketsSkipped << " non-CSIG skipped).";
  }

  // SDK test_csig_tgen_v4_v6 model: dual-port UPDATE + cross-port flood. FBOSS
  // has no tgen; probe uses CPU switched inject (untagged) because front-panel
  // OOP inject adds VLAN 2000 and breaks CSIG on Yuba/G202X (see
  // csigRoutedInjectVlan).
  void verifySrv6EncapWithCsigTagUnderCongestionTgenStyle(
      PortID egressPort,
      const folly::IPAddressV6& expectedSid) {
    auto floodPort = findInjectPort({egressPort});
    configureCsigTgenStyleOnPorts(floodPort, egressPort);

    const std::string stepLabel = "CSIG+SRv6 encap update (tgen-style)";
    csigInjectLogLabel_ = stepLabel;
    XLOG(INFO) << stepLabel << ": encap dst " << kEncapRouteDstIp << " SID "
               << expectedSid << " egress=" << egressPort
               << " flood=" << floodPort
               << " probe=CPU-switched (untagged CSIG, SDK sw_port equivalent)"
               << " (dual UPDATE, cross-port flood)";

    std::map<PortID, int64_t> bytesBefore;
    bytesBefore[egressPort] = *this->getLatestPortStats(egressPort).outBytes_();

    auto intfMac =
        getMacForFirstInterfaceWithPortsForTesting(this->getProgrammedState());
    auto tcField = static_cast<uint8_t>((kFloodDscp << 2) | kECT1);

    buildTgenStyleCrossPortCongestion(floodPort, egressPort);
    installEgressPortIngressTrapForTgenProbe(egressPort);

    utility::SwSwitchPacketSnooper trapSnooper(
        this->getSw(), "csigSrv6EncapUpdateTgenStyleTrapSnooper");
    trapSnooper.ignoreUnclaimedRxPkts();

    const auto floodRoutedDst = getRoutedDstIpForEgressPort(egressPort);

    auto sendProbe = [&]() {
      auto txPacket = makeCsigEncapProbePacket(intfMac, tcField);
      sendCsigInjectSwitched(csigInjectLogLabel_, std::move(txPacket));
    };
    int refloodAttempts = 0;
    auto refloodAndProbe = [&]() {
      if (refloodAttempts >= kMaxTgenRefloodAttempts) {
        XLOG(INFO) << stepLabel << ": max re-flood attempts ("
                   << kMaxTgenRefloodAttempts
                   << ") reached; resending CPU-switched probe only";
        sendProbe();
        return;
      }
      ++refloodAttempts;
      XLOG(INFO) << stepLabel << ": re-flood attempt " << refloodAttempts << "/"
                 << kMaxTgenRefloodAttempts << " on flood_port=" << floodPort
                 << " egress=" << egressPort;
      refloodTgenStyleCrossPortCongestion(floodPort, egressPort);
      sendProbe();
    };
    sendProbe();

    bool foundCsigOnEgressTrap = false;
    CsigTrapScanStats scanStats;
    WITH_RETRIES({
      scanStats = scanCsigSrv6EncapUpdateTrapSnooper(
          trapSnooper,
          stepLabel,
          expectedSid,
          [&](const folly::IOBuf* frame) {
            return validateCsigSrv6EncapFrame(frame, expectedSid);
          },
          &floodRoutedDst);
      foundCsigOnEgressTrap = scanStats.foundMatch;

      auto bytesAfter = *this->getLatestPortStats(egressPort).outBytes_();
      EXPECT_EVENTUALLY_TRUE(bytesAfter > bytesBefore[egressPort]);
      if (!scanStats.foundMatch && scanStats.trappedPacketsScanned == 0) {
        XLOG(INFO) << stepLabel << ": no encap-probe trapped copies "
                   << "yet; egress_port=" << egressPort << " outBytes "
                   << bytesBefore[egressPort] << " -> " << bytesAfter
                   << "; re-flood and resend CPU-switched probe";
        refloodAndProbe();
      } else if (!scanStats.foundMatch && scanStats.csigPacketsScanned > 0) {
        XLOG(INFO) << stepLabel << ": scanned " << scanStats.csigPacketsScanned
                   << " encap-probe CSIG trap copies without match (ingress "
                   << "shim 99 00 2b 85 likely); re-flood and resend probe";
        refloodAndProbe();
      } else if (
          !scanStats.foundMatch && scanStats.csigPacketsScanned == 0 &&
          scanStats.nonCsigPacketsSkipped > 0) {
        XLOG(INFO) << stepLabel << ": only non-probe non-CSIG traps ("
                   << scanStats.nonCsigPacketsSkipped
                   << ") seen so far; re-flood and resend CPU-switched probe";
        refloodAndProbe();
      }
      EXPECT_EVENTUALLY_TRUE(foundCsigOnEgressTrap);
    });
    logCsigTrapScanSummary(stepLabel, scanStats, foundCsigOnEgressTrap);
    if (!foundCsigOnEgressTrap && scanStats.csigPacketsScanned > 0) {
      XLOG(ERR) << stepLabel << ": trapped copies never had outer L2 "
                << "UPDATE CSIG (0x9900 loc 0x" << std::hex
                << +utility::kDefaultCsigLinkLocator << std::dec
                << ", egress-updated ABWC). Ingress shim 99 00 2b 85 (signal "
                << "0x17, lm 0x05) with valid SID " << expectedSid
                << " — egress ARC did not stamp UPDATE before probe encap "
                << "(congestion/line-rate timing even with cross-port flood). "
                << "Compare with srv6EncapCsigUpdate (egress-local flood) and "
                << "csigBasicUpdate (plain routed).";
    }
    ASSERT_TRUE(foundCsigOnEgressTrap)
        << stepLabel
        << ": no egress loopback-trapped SRv6 encap UPDATE frame after "
        << "tgen-style setup: flood_port=" << floodPort
        << " probe=CPU-switched SID " << expectedSid << " after scanning "
        << scanStats.trappedPacketsScanned << " trapped copies ("
        << scanStats.csigPacketsScanned << " CSIG, "
        << scanStats.nonCsigPacketsSkipped << " non-CSIG skipped).";
  }

  void verifyDecapCsigUpdateTgenStyle(PortID egressPort) {
    auto floodPort = findInjectPort({egressPort});
    configureCsigTgenStyleOnPorts(floodPort, egressPort);
    resolveGlobalNeighborOnEgressPort(egressPort);

    const std::string stepLabel = "CSIG+SRv6 decap update (tgen-style)";
    csigInjectLogLabel_ = stepLabel;
    XLOG(INFO) << stepLabel << ": mySid " << kDecapMySidAddr << " inner dst "
               << kEncapRouteDstIp << " egress=" << egressPort
               << " flood=" << floodPort
               << " probe=CPU-switched (untagged CSIG, SDK sw_port equivalent)"
               << " (dual UPDATE, cross-port flood)";

    std::map<PortID, int64_t> bytesBefore;
    bytesBefore[egressPort] = *this->getLatestPortStats(egressPort).outBytes_();

    auto intfMac =
        getMacForFirstInterfaceWithPortsForTesting(this->getProgrammedState());
    auto tcField = static_cast<uint8_t>((kFloodDscp << 2) | kECT1);

    buildTgenStyleCrossPortCongestion(floodPort, egressPort);
    installEgressPortIngressTrapForTgenProbe(egressPort);

    utility::SwSwitchPacketSnooper trapSnooper(
        this->getSw(), "csigDecapUpdateTgenStyleTrapSnooper");
    trapSnooper.ignoreUnclaimedRxPkts();

    const folly::IPAddress expectedInnerDst(kEncapRouteDstIp);

    auto sendProbe = [&]() {
      auto txPacket = makeCsigDecapTgenProbePacket(intfMac, tcField);
      sendCsigInjectSwitched(csigInjectLogLabel_, std::move(txPacket));
    };
    int refloodAttempts = 0;
    auto refloodAndProbe = [&]() {
      if (refloodAttempts >= kMaxTgenRefloodAttempts) {
        XLOG(INFO) << stepLabel << ": max re-flood attempts ("
                   << kMaxTgenRefloodAttempts
                   << ") reached; resending CPU-switched probe only";
        sendProbe();
        return;
      }
      ++refloodAttempts;
      XLOG(INFO) << stepLabel << ": re-flood attempt " << refloodAttempts << "/"
                 << kMaxTgenRefloodAttempts << " on flood_port=" << floodPort
                 << " egress=" << egressPort;
      refloodTgenStyleCrossPortCongestion(floodPort, egressPort);
      sendProbe();
    };
    sendProbe();

    bool found = false;
    CsigTrapScanStats scanStats;
    WITH_RETRIES({
      scanStats = scanCsigDecapUpdateTrapSnooper(
          trapSnooper,
          stepLabel,
          expectedInnerDst,
          [&](const folly::IOBuf* frame) {
            return validateCsigDecapUpdateFrame(frame, expectedInnerDst);
          });
      found = scanStats.foundMatch;

      auto bytesAfter = *this->getLatestPortStats(egressPort).outBytes_();
      EXPECT_EVENTUALLY_TRUE(bytesAfter > bytesBefore[egressPort]);
      if (!scanStats.foundMatch && scanStats.trappedPacketsScanned == 0) {
        XLOG(INFO) << stepLabel << ": no decap-probe trapped copies yet; "
                   << "egress_port=" << egressPort << " outBytes "
                   << bytesBefore[egressPort] << " -> " << bytesAfter
                   << "; re-flood and resend CPU-switched probe";
        refloodAndProbe();
      } else if (!scanStats.foundMatch && scanStats.csigPacketsScanned > 0) {
        XLOG(INFO) << stepLabel << ": scanned " << scanStats.csigPacketsScanned
                   << " decap-probe CSIG trap copies without match (ingress "
                   << "shim 99 00 2b 85 likely); re-flood and resend probe";
        refloodAndProbe();
      } else if (
          !scanStats.foundMatch && scanStats.csigPacketsScanned == 0 &&
          scanStats.nonCsigPacketsSkipped > 0) {
        XLOG(INFO) << stepLabel << ": only non-probe non-CSIG traps ("
                   << scanStats.nonCsigPacketsSkipped
                   << ") seen so far; re-flood and resend CPU-switched probe";
        refloodAndProbe();
      }
      EXPECT_EVENTUALLY_TRUE(found);
    });
    logCsigTrapScanSummary(stepLabel, scanStats, found);
    if (!found && scanStats.csigPacketsScanned > 0) {
      XLOG(ERR) << stepLabel << ": trapped copies never matched decapped inner "
                << "L2 UPDATE CSIG (0x9900 loc 0x" << std::hex
                << +utility::kDefaultCsigLinkLocator << std::dec
                << ", egress-updated ABWC, inner dst " << kEncapRouteDstIp
                << "). Compare with srv6DecapCsigUpdate (egress-local flood).";
    }
    ASSERT_TRUE(found)
        << stepLabel
        << ": no egress loopback-trapped decap UPDATE frame after tgen-style "
        << "setup: flood_port=" << floodPort << " mySid=" << kDecapMySidAddr
        << " inner_dst=" << kEncapRouteDstIp << " after scanning "
        << scanStats.trappedPacketsScanned << " trapped copies ("
        << scanStats.csigPacketsScanned << " CSIG, "
        << scanStats.nonCsigPacketsSkipped << " non-CSIG skipped).";
  }

  void verifyMidpointCsigUpdateTgenStyle(
      PortID egressPort,
      const folly::IPAddressV6& expectedOuterDst = kMidpointExpectedOuterDst) {
    auto floodPort = findInjectPort({egressPort});
    configureCsigTgenStyleOnPorts(floodPort, egressPort);
    resolveMidpointNeighborOnEgressPort(egressPort);
    resolveGlobalNeighborOnEgressPort(egressPort);

    const std::string stepLabel = "CSIG+SRv6 midpoint update (tgen-style)";
    csigInjectLogLabel_ = stepLabel;
    XLOG(INFO) << stepLabel << ": outer dst " << kMidpointPktOuterDst
               << " -> expected " << expectedOuterDst
               << " egress=" << egressPort << " flood=" << floodPort
               << " probe=CPU-switched (dual UPDATE, cross-port flood)";

    std::map<PortID, int64_t> bytesBefore;
    bytesBefore[egressPort] = *this->getLatestPortStats(egressPort).outBytes_();

    auto intfMac =
        getMacForFirstInterfaceWithPortsForTesting(this->getProgrammedState());
    auto tcField = static_cast<uint8_t>((kFloodDscp << 2) | kECT1);

    buildTgenStyleCrossPortCongestion(floodPort, egressPort);
    installEgressPortIngressTrapForTgenProbe(egressPort);

    utility::SwSwitchPacketSnooper trapSnooper(
        this->getSw(), "csigMidpointUpdateTgenStyleTrapSnooper");
    trapSnooper.ignoreUnclaimedRxPkts();

    auto sendProbe = [&]() {
      auto txPacket = makeCsigMidpointTgenProbePacket(
          intfMac, kMidpointPktOuterDst, kIpInIpInnerDst, tcField);
      sendCsigInjectSwitched(csigInjectLogLabel_, std::move(txPacket));
    };
    int refloodAttempts = 0;
    auto refloodAndProbe = [&]() {
      if (refloodAttempts >= kMaxTgenRefloodAttempts) {
        XLOG(INFO) << stepLabel << ": max re-flood attempts ("
                   << kMaxTgenRefloodAttempts
                   << ") reached; resending CPU-switched probe only";
        sendProbe();
        return;
      }
      ++refloodAttempts;
      XLOG(INFO) << stepLabel << ": re-flood attempt " << refloodAttempts << "/"
                 << kMaxTgenRefloodAttempts << " on flood_port=" << floodPort
                 << " egress=" << egressPort;
      refloodTgenStyleCrossPortCongestion(floodPort, egressPort);
      sendProbe();
    };
    sendProbe();

    bool found = false;
    CsigTrapScanStats scanStats;
    WITH_RETRIES({
      scanStats = scanCsigTrapSnooper(
          trapSnooper, stepLabel, [&](const folly::IOBuf* frame) {
            return validateCsigMidpointUpdateFrame(
                frame,
                expectedOuterDst,
                kIpInIpInnerDst,
                /*requireNonZeroOuterFlowLabel=*/false);
          });
      found = scanStats.foundMatch;

      auto bytesAfter = *this->getLatestPortStats(egressPort).outBytes_();
      EXPECT_EVENTUALLY_TRUE(bytesAfter > bytesBefore[egressPort]);
      if (!scanStats.foundMatch && scanStats.trappedPacketsScanned == 0) {
        XLOG(INFO) << stepLabel
                   << ": no trapped copies yet; egress_port=" << egressPort
                   << " outBytes " << bytesBefore[egressPort] << " -> "
                   << bytesAfter << "; re-flood and resend CPU-switched probe";
        refloodAndProbe();
      } else if (!scanStats.foundMatch && scanStats.csigPacketsScanned > 0) {
        XLOG(INFO) << stepLabel << ": scanned " << scanStats.csigPacketsScanned
                   << " CSIG trap copies without match; re-flood and resend "
                   << "probe";
        refloodAndProbe();
      } else if (
          !scanStats.foundMatch && scanStats.csigPacketsScanned == 0 &&
          scanStats.nonCsigPacketsSkipped > 0) {
        XLOG(INFO) << stepLabel << ": only non-probe non-CSIG traps ("
                   << scanStats.nonCsigPacketsSkipped
                   << ") seen so far; re-flood and resend CPU-switched probe";
        refloodAndProbe();
      }
      EXPECT_EVENTUALLY_TRUE(found);
    });
    logCsigTrapScanSummary(stepLabel, scanStats, found);
    if (!found && scanStats.csigPacketsScanned > 0) {
      XLOG(ERR)
          << stepLabel << ": trapped copies had outer L2 CSIG but none "
          << "matched egress UPDATE (loc 0x" << std::hex
          << +utility::kDefaultCsigLinkLocator << std::dec
          << "). Compare with srv6MidpointCsigUpdate (egress-local flood).";
    }
    ASSERT_TRUE(found)
        << stepLabel
        << ": no egress loopback-trapped midpoint UPDATE frame after "
        << "tgen-style setup: flood_port=" << floodPort
        << " outer_dst=" << expectedOuterDst << " after scanning "
        << scanStats.trappedPacketsScanned << " trapped copies ("
        << scanStats.csigPacketsScanned << " CSIG, "
        << scanStats.nonCsigPacketsSkipped << " non-CSIG skipped).";
  }

  void verifyBindingSidCsigUpdateTgenStyle(PortID egressPort) {
    auto floodPort = findInjectPort({egressPort});
    configureCsigTgenStyleOnPorts(floodPort, egressPort);
    resolveGlobalNeighborOnEgressPort(egressPort);

    const std::string stepLabel = "CSIG+SRv6 binding SID update (tgen-style)";
    csigInjectLogLabel_ = stepLabel;
    XLOG(INFO) << stepLabel << ": mySid " << kBindingMySidPrefix << " -> SID "
               << kSid0 << " egress=" << egressPort << " flood=" << floodPort
               << " probe=CPU-switched (dual UPDATE, cross-port flood)";

    std::map<PortID, int64_t> bytesBefore;
    bytesBefore[egressPort] = *this->getLatestPortStats(egressPort).outBytes_();

    auto intfMac =
        getMacForFirstInterfaceWithPortsForTesting(this->getProgrammedState());
    auto tcField = static_cast<uint8_t>((kFloodDscp << 2) | kECT1);

    buildTgenStyleCrossPortCongestion(floodPort, egressPort);
    installEgressPortIngressTrapForTgenProbe(egressPort);

    utility::SwSwitchPacketSnooper trapSnooper(
        this->getSw(), "csigBsidUpdateTgenStyleTrapSnooper");
    trapSnooper.ignoreUnclaimedRxPkts();

    auto sendProbe = [&]() {
      auto txPacket = makeCsigIpInIpTgenProbePacket(
          intfMac, kBindingMySidPrefix, kIpInIpInnerDst, tcField);
      sendCsigInjectSwitched(csigInjectLogLabel_, std::move(txPacket));
    };
    int refloodAttempts = 0;
    auto refloodAndProbe = [&]() {
      if (refloodAttempts >= kMaxTgenRefloodAttempts) {
        XLOG(INFO) << stepLabel << ": max re-flood attempts ("
                   << kMaxTgenRefloodAttempts
                   << ") reached; resending CPU-switched probe only";
        sendProbe();
        return;
      }
      ++refloodAttempts;
      XLOG(INFO) << stepLabel << ": re-flood attempt " << refloodAttempts << "/"
                 << kMaxTgenRefloodAttempts << " on flood_port=" << floodPort
                 << " egress=" << egressPort;
      refloodTgenStyleCrossPortCongestion(floodPort, egressPort);
      sendProbe();
    };
    sendProbe();

    bool found = false;
    CsigTrapScanStats scanStats;
    WITH_RETRIES({
      scanStats = scanCsigTrapSnooper(
          trapSnooper, stepLabel, [&](const folly::IOBuf* frame) {
            return validateCsigBsidUpdateFrame(
                frame,
                kSid0,
                kIpInIpInnerDst,
                /*requireNonZeroOuterFlowLabel=*/false);
          });
      found = scanStats.foundMatch;

      auto bytesAfter = *this->getLatestPortStats(egressPort).outBytes_();
      EXPECT_EVENTUALLY_TRUE(bytesAfter > bytesBefore[egressPort]);
      if (!scanStats.foundMatch && scanStats.trappedPacketsScanned == 0) {
        XLOG(INFO) << stepLabel
                   << ": no trapped copies yet; egress_port=" << egressPort
                   << " outBytes " << bytesBefore[egressPort] << " -> "
                   << bytesAfter << "; re-flood and resend CPU-switched probe";
        refloodAndProbe();
      } else if (!scanStats.foundMatch && scanStats.csigPacketsScanned > 0) {
        XLOG(INFO) << stepLabel << ": scanned " << scanStats.csigPacketsScanned
                   << " CSIG trap copies without match; re-flood and resend "
                   << "probe";
        refloodAndProbe();
      } else if (
          !scanStats.foundMatch && scanStats.csigPacketsScanned == 0 &&
          scanStats.nonCsigPacketsSkipped > 0) {
        XLOG(INFO) << stepLabel << ": only non-probe non-CSIG traps ("
                   << scanStats.nonCsigPacketsSkipped
                   << ") seen so far; re-flood and resend CPU-switched probe";
        refloodAndProbe();
      }
      EXPECT_EVENTUALLY_TRUE(found);
    });
    logCsigTrapScanSummary(stepLabel, scanStats, found);
    if (!found && scanStats.csigPacketsScanned > 0) {
      XLOG(ERR) << stepLabel << ": trapped copies never had new BSid outer L2 "
                << "UPDATE CSIG (loc 0x" << std::hex
                << +utility::kDefaultCsigLinkLocator << std::dec
                << "). Compare with srv6BsidCsigUpdate (egress-local flood).";
    }
    ASSERT_TRUE(found)
        << stepLabel
        << ": no egress loopback-trapped BSID UPDATE frame after tgen-style "
        << "setup: flood_port=" << floodPort << " SID " << kSid0
        << " inner_dst=" << kIpInIpInnerDst << " after scanning "
        << scanStats.trappedPacketsScanned << " trapped copies ("
        << scanStats.csigPacketsScanned << " CSIG, "
        << scanStats.nonCsigPacketsSkipped << " non-CSIG skipped).";
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

// --- Basic CSIG (L3 routed, no SRv6) ---

TEST_F(AgentCsigTest, csigBasicPassthrough) {
  skipUnlessCsigSupportedAsic();
  auto setup = [this]() { this->setupHelper(); };
  auto verify = [this]() {
    this->verifyRoutedCsigPassthrough(this->getDefaultCsigEgressPort());
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentCsigTest, csigBasicStrip) {
  skipUnlessCsigSupportedAsic();
  auto setup = [this]() { this->setupHelper(); };
  auto verify = [this]() {
    this->verifyRoutedCsigStrip(this->getDefaultCsigEgressPort());
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentCsigTest, csigBasicUpdate) {
  skipUnlessCsigSupportedAsic();
  auto setup = [this]() { this->csigSrv6EncapSetup(); };
  auto verify = [this]() {
    this->verifyRoutedCsigTagUnderCongestion(this->getDefaultCsigEgressPort());
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

// --- CSIG + SRv6 encap ---

TEST_F(AgentCsigTest, srv6EncapCsigPassthrough) {
  skipUnlessCsigSupportedAsic();
  auto setup = [this]() { this->csigSrv6EncapSetup(); };
  auto verify = [this]() {
    this->verifySrv6CsigPassthrough(this->getDefaultCsigEgressPort(), kSid0);
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentCsigTest, srv6EncapCsigStrip) {
  skipUnlessCsigSupportedAsic();
  auto setup = [this]() { this->csigSrv6EncapSetup(); };
  auto verify = [this]() {
    this->verifySrv6CsigStrip(this->getDefaultCsigEgressPort(), kSid0);
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentCsigTest, srv6EncapCsigUpdate) {
  skipUnlessCsigSupportedAsic();
  auto setup = [this]() { this->csigSrv6EncapSetup(); };
  auto verify = [this]() {
    this->verifySrv6EncapWithCsigTagUnderCongestion(
        this->getDefaultCsigEgressPort(), kSid0);
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentCsigTest, srv6EncapCsigUpdateTgenStyle) {
  skipUnlessCsigSupportedAsic();
  auto setup = [this]() { this->csigSrv6EncapTgenStyleSetup(); };
  auto verify = [this]() {
    this->verifySrv6EncapWithCsigTagUnderCongestionTgenStyle(
        this->getDefaultCsigEgressPort(), kSid0);
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentCsigTest, srv6EncapCsigUpdateQuietClamp) {
  skipUnlessCsigSupportedAsic();
  auto setup = [this]() { this->csigSrv6EncapRoutesOnly(); };
  auto verify = [this]() {
    this->verifySrv6EncapCsigUpdateQuietClamp(
        this->getDefaultCsigEgressPort(), kSid0);
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

// --- CSIG + SRv6 decap ---

TEST_F(AgentCsigTest, srv6DecapCsigPassthrough) {
  skipUnlessCsigSupportedAsic();
  auto setup = [this]() { this->csigDecapSetup(); };
  auto verify = [this]() {
    this->verifyDecapCsigPassthrough(this->getDefaultCsigEgressPort());
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentCsigTest, srv6DecapCsigStrip) {
  skipUnlessCsigSupportedAsic();
  auto setup = [this]() { this->csigDecapSetup(); };
  auto verify = [this]() {
    this->verifyDecapCsigStrip(this->getDefaultCsigEgressPort());
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentCsigTest, srv6DecapCsigUpdate) {
  skipUnlessCsigSupportedAsic();
  auto setup = [this]() { this->csigDecapSetup(); };
  auto verify = [this]() {
    this->verifyDecapCsigUpdate(this->getDefaultCsigEgressPort());
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentCsigTest, srv6DecapCsigUpdateTgenStyle) {
  skipUnlessCsigSupportedAsic();
  auto setup = [this]() { this->csigDecapTgenStyleSetup(); };
  auto verify = [this]() {
    this->verifyDecapCsigUpdateTgenStyle(this->getDefaultCsigEgressPort());
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentCsigTest, srv6DecapCsigUpdateQuietClamp) {
  skipUnlessCsigSupportedAsic();
  auto setup = [this]() { this->csigDecapRoutesOnly(); };
  auto verify = [this]() {
    this->verifyDecapCsigUpdateQuietClamp(this->getDefaultCsigEgressPort());
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

// --- CSIG + SRv6 midpoint ---

TEST_F(AgentCsigTest, srv6MidpointCsigPassthrough) {
  skipUnlessCsigSupportedAsic();
  auto setup = [this]() { this->csigMidpointSetup(); };
  auto verify = [this]() {
    this->verifyMidpointCsigPassthrough(this->getMidpointCsigEgressPort());
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentCsigTest, srv6MidpointCsigStrip) {
  skipUnlessCsigSupportedAsic();
  auto setup = [this]() { this->csigMidpointSetup(); };
  auto verify = [this]() {
    this->verifyMidpointCsigStrip(this->getMidpointCsigEgressPort());
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentCsigTest, srv6MidpointCsigUpdate) {
  skipUnlessCsigSupportedAsic();
  auto setup = [this]() { this->csigMidpointSetup(); };
  auto verify = [this]() {
    this->verifyMidpointCsigUpdate(this->getMidpointCsigEgressPort());
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentCsigTest, srv6MidpointCsigUpdateTgenStyle) {
  skipUnlessCsigSupportedAsic();
  auto setup = [this]() { this->csigMidpointTgenStyleSetup(); };
  auto verify = [this]() {
    this->verifyMidpointCsigUpdateTgenStyle(this->getMidpointCsigEgressPort());
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentCsigTest, srv6MidpointCsigUpdateQuietClamp) {
  skipUnlessCsigSupportedAsic();
  auto setup = [this]() { this->csigMidpointRoutesOnly(); };
  auto verify = [this]() {
    this->verifyMidpointCsigUpdateQuietClamp(this->getMidpointCsigEgressPort());
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

// --- CSIG + SRv6 binding SID ---

TEST_F(AgentCsigTest, srv6BsidCsigPassthrough) {
  skipUnlessCsigSupportedAsic();
  auto setup = [this]() { this->csigBindingSidSetup(); };
  auto verify = [this]() {
    this->verifyBindingSidCsigPassthrough(this->getDefaultCsigEgressPort());
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentCsigTest, srv6BsidCsigStrip) {
  skipUnlessCsigSupportedAsic();
  auto setup = [this]() { this->csigBindingSidSetup(); };
  auto verify = [this]() {
    this->verifyBindingSidCsigStrip(this->getDefaultCsigEgressPort());
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentCsigTest, srv6BsidCsigUpdate) {
  skipUnlessCsigSupportedAsic();
  auto setup = [this]() { this->csigBindingSidSetup(); };
  auto verify = [this]() {
    this->verifyBindingSidCsigUpdate(this->getDefaultCsigEgressPort());
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentCsigTest, srv6BsidCsigUpdateTgenStyle) {
  skipUnlessCsigSupportedAsic();
  auto setup = [this]() { this->csigBindingSidTgenStyleSetup(); };
  auto verify = [this]() {
    this->verifyBindingSidCsigUpdateTgenStyle(this->getDefaultCsigEgressPort());
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentCsigTest, srv6BsidCsigUpdateQuietClamp) {
  skipUnlessCsigSupportedAsic();
  auto setup = [this]() { this->csigBindingSidRoutesOnly(); };
  auto verify = [this]() {
    this->verifyBindingSidCsigUpdateQuietClamp(
        this->getDefaultCsigEgressPort());
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
