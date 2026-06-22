//
// AgentSrv6PbrCounterTest
// -----------------------
// Exercises programmable packet+byte counters on SRv6 PBR ACEs that redirect
// to tiered SRv6 next-hop groups (gold / silver / bronze) on Cisco Silicon One.
//
// Mirrors sai/test/python/srv6/test_srv6_svi_pbr.py:
//   * L3 route points at the gold NHG only (FIELD_ROUTE_DST always gold).
//   * DSCP-to-TC qos map selects the ACE (10->TC1, 20->TC2, 30->TC3).
//   * Each ACE redirects to its tier NHG with a distinct uSID path.
//   * Per-tier ACL counters must increment independently on CPU and front-panel
//     inject (both paths validated before rebase).
//   * Egress SRv6 packets are trapped to CPU and verified (outer uSID).
//
// Packet path:
//   CPU inject: sendPacketSwitchedAsync -> pipeline lookup -> encap + counter.
//   Front-panel: sendPacketOutOfPort on inject port (loopback) -> re-ingress ->
//     ingress PBR ACL -> SRv6 encap -> underlay egress -> trap + counter bump.
//

#include <thrift/lib/cpp/util/EnumUtils.h>

#include <algorithm>
#include <array>
#include <map>
#include <optional>
#include <set>

#include "fboss/agent/AddressUtil.h"
#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/AsicUtils.h"
#include "fboss/agent/SwSwitchRouteUpdateWrapper.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/hw/test/HwTestPacketUtils.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/agent/packet/EthFrame.h"
#include "fboss/agent/packet/Ethertype.h"
#include "fboss/agent/rib/RoutingInformationBase.h"
#include "fboss/agent/state/AggregatePort.h"
#include "fboss/agent/state/RouteNextHop.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/TrunkUtils.h"
#include "fboss/agent/test/utils/AclTestUtils.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/LoadBalancerTestUtils.h"
#include "fboss/agent/test/utils/PacketSnooper.h"
#include "fboss/agent/test/utils/TrapPacketUtils.h"

DECLARE_bool(enable_acl_table_group);

namespace facebook::fboss {

namespace {

struct Srv6PbrTier {
  const char* name;
  uint8_t dscp;
  uint8_t tc;
  const char* sid;
  const char* nhgName;
  const char* aclName;
  const char* counterName;
};

constexpr Srv6PbrTier kGoldTier{
    "gold",
    10,
    1,
    "bbbb:b1b1:51:52:53:54:55:56",
    "goldSrv6Nhg",
    "acl_srv6_pbr_encap_gold",
    "acl_srv6_pbr_encap_stat_gold"};
constexpr Srv6PbrTier kSilverTier{
    "silver",
    20,
    2,
    "bbbb:b1b1:61:62:63:64:65:66",
    "silverSrv6Nhg",
    "acl_srv6_pbr_encap_silver",
    "acl_srv6_pbr_encap_stat_silver"};
constexpr Srv6PbrTier kBronzeTier{
    "bronze",
    30,
    3,
    "bbbb:b1b1:71:72:73:74:75:76",
    "bronzeSrv6Nhg",
    "acl_srv6_pbr_encap_bronze",
    "acl_srv6_pbr_encap_stat_bronze"};

constexpr std::array<const Srv6PbrTier*, 3> kSrv6PbrTiers{
    &kGoldTier,
    &kSilverTier,
    &kBronzeTier};

} // namespace

struct PhysicalPortSrv6Pbr {
  static constexpr bool isTrunk = false;
};
struct AggregatePortSrv6Pbr {
  static constexpr bool isTrunk = true;
};
using Srv6PbrPortTypes =
    ::testing::Types<PhysicalPortSrv6Pbr, AggregatePortSrv6Pbr>;

template <typename PortType>
class AgentSrv6PbrCounterTest : public AgentHwTest {
 public:
  static constexpr bool kIsTrunk = PortType::isTrunk;

  void setCmdLineFlagOverrides() const override {
    AgentHwTest::setCmdLineFlagOverrides();
    FLAGS_enable_acl_table_group = true;
    FLAGS_enable_nexthop_id_manager = true;
    FLAGS_resolve_nexthops_from_id = true;
  }

 protected:
  const std::string kTunnelId{"srv6PbrTunnel0"};
  const std::string kEncapSrcIp{"2401:db00::1"};
  folly::IPAddress underlayNhIp_{folly::IPAddress("fe80:face:b11c::1")};
  const std::string kGoldRouteV6Dst{"2001:db8:61::11"};
  static constexpr uint8_t kGoldRouteV6PrefixLen{128};
  const std::string kQosPolicyName{"srv6_pbr_qos"};
  const std::string kPbrAclTableName{"Srv6PbrAclTable"};

  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    std::vector<ProductionFeature> features{
        ProductionFeature::SRV6_ENCAP,
        ProductionFeature::ACL_COUNTER,
    };
    if constexpr (kIsTrunk) {
      features.push_back(ProductionFeature::LAG);
    }
    return features;
  }

  cfg::SwitchConfig initialConfig(
  // TH6/BCM pass the byte assertion for the wrong reason.
  bool isCiscoSiliconOneAsic(cfg::AsicType type) const {
    switch (type) {
      case cfg::AsicType::ASIC_TYPE_YUBA: // GR2 (Graphene2)
      case cfg::AsicType::ASIC_TYPE_G202X: // G204 / G202X family
        // NOTE: GR3 / G2LL not yet enumerated in this thrift tree; add
        // their AsicType enum values here as soon as they land. See
        // switch_config.thrift `enum AsicType`.
        ensemble.getSw()->getPlatformSupportsAddRemovePort(),
        asic->desiredLoopbackModes());
        addSrv6EncapTunnelConfig(cfg);
        addDscpToTcQosMap(cfg);
        addSrv6SidTrapAcls(cfg, asic);
        if constexpr (kIsTrunk) {
          utility::addAggPort(
              1,
              {static_cast<int32_t>(ensemble.masterLogicalPortIds()[0])},
              &cfg);
          cfg.loadBalancers() =
              utility::getEcmpFullWithFlowLabelTrunkFullWithFlowLabelHashConfig(
                  ensemble.getL3Asics());
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

    bool isCiscoSiliconOneAsic(cfg::AsicType type) const {
      switch (type) {
        case cfg::AsicType::ASIC_TYPE_YUBA:
        case cfg::AsicType::ASIC_TYPE_G202X:
          return true;
        default:
          return false;
      }

      void setupHelper() {
        if constexpr (kIsTrunk) {
          applyConfigAndEnableTrunks(
              this->initialConfig(*this->getAgentEnsemble()));
        }

        auto hwAsic =
            checkSameAndGetAsic(this->getAgentEnsemble()->getL3Asics());
        if (!isCiscoSiliconOneAsic(hwAsic->getAsicType())) {
          GTEST_SKIP()
              << "AgentSrv6PbrCounterTest only validates the Cisco "
              << "Silicon One SRv6 PBR byte-counter code path. Current "
              << "ASIC type = "
              << apache::thrift::util::enumNameSafe(hwAsic->getAsicType())
              << " is not in {YUBA, G202X}; skipping.";
        }

        utility::EcmpSetupAnyNPorts6 ecmpHelper(
            this->getProgrammedState(),
            this->getSw()->needL2EntryForNeighbor(),
            getLocalMacAddress());
        const auto& nhop = ecmpHelper.nhop(0);
        CHECK(nhop.linkLocalNhopIp.has_value());
        underlayNhIp_ = folly::IPAddress(*nhop.linkLocalNhopIp);
        this->resolveNeighbors(ecmpHelper, 1, true /* useLinkLocal */);
        auto routeUpdater = this->getSw()->getRouteUpdater();
        ecmpHelper.programRoutes(&routeUpdater, 1);
        programTierNhgsAndGoldRoute();
        programSrv6PbrAclWithCounters();
      }

      std::unique_ptr<TxPacket> makeMatchingPacket(uint8_t dscp) {
        auto vlanId = getVlanIDForTx();
        auto intfMac = getMacForFirstInterfaceWithPorts(getProgrammedState());
        auto dstMac = intfMac;
        auto srcMac = folly::MacAddress::fromHBO(dstMac.u64HBO() + 1);
        const folly::IPAddressV6 srcIp("1::10");
        const folly::IPAddressV6 dstIp(kGoldRouteV6Dst);
        return utility::makeUDPTxPacket(
            getSw(),
            vlanId,
            srcMac,
            dstMac,
            srcIp,
            dstIp,
            10000,
            443,
            static_cast<uint8_t>(dscp << 2));
      }

      uint64_t readAclCounter(const std::string& statName, bool bytes = false)
          const {
        getSw()->updateStats();
        return utility::getAclInOutPackets(getSw(), statName, bytes);
      }

      std::map<std::string, std::pair<uint64_t, uint64_t>>
      snapshotAllTierCounters() const {
        std::map<std::string, std::pair<uint64_t, uint64_t>> snapshot;
        for (const auto* t : kSrv6PbrTiers) {
          snapshot[t->counterName] = {
              readAclCounter(t->counterName),
              readAclCounter(t->counterName, true)};
        }
        return snapshot;
      }

      PortID getEgressPort(const PortDescriptor& portDesc) const {
        if (portDesc.isPhysicalPort()) {
          return portDesc.phyPortID();
        }
        auto aggPort = getProgrammedState()->getAggregatePorts()->getNodeIf(
            portDesc.aggPortID());
        return aggPort->sortedSubports().front().portID;
      }

      PortID findInjectPort(const std::vector<PortID>& egressPorts) {
        for (const auto& portMap :
             std::as_const(*getProgrammedState()->getPorts())) {
          for (const auto& [_, port] : std::as_const(*portMap.second)) {
            if (port->isPortUp() &&
                std::find(
                    egressPorts.begin(), egressPorts.end(), port->getID()) ==
                    egressPorts.end()) {
              return port->getID();
            }
          }
        }
        return masterLogicalPortIds()[1];
      }

      void verifyTierSrv6PbrEncapAndCounter(
          const Srv6PbrTier& tier,
          const std::map<std::string, std::pair<uint64_t, uint64_t>>&
              countersBefore,
          std::optional<PortID> injectPort = std::nullopt,
          bool verifyCounters = true) {
        const auto pktCountBefore = countersBefore.at(tier.counterName).first;
        const auto bytesCountBefore =
            countersBefore.at(tier.counterName).second;

        auto txPacket = makeMatchingPacket(tier.dscp);
        const size_t sizeOfPacketSent = txPacket->buf()->length();
        auto origFrame = utility::makeEthFrame(*txPacket);

        utility::SwSwitchPacketSnooper snooper(
            getSw(), std::string("srv6PbrSnooper-") + tier.name);

        const bool fromCpu = !injectPort.has_value();
        XLOG(INFO) << "AgentSrv6PbrCounterTest: sending 1 " << tier.name
                   << " UDP pkt (" << sizeOfPacketSent
                   << " bytes) dscp=" << static_cast<int>(tier.dscp)
                   << " tc=" << static_cast<int>(tier.tc)
                   << " inject=" << (fromCpu ? "cpu" : "front-panel")
                   << "; expect SRv6 encap uSID=" << tier.sid
                   << " and counter '" << tier.counterName << "' bump";

        if (injectPort.has_value()) {
          this->getAgentEnsemble()->ensureSendPacketOutOfPort(
              std::move(txPacket), injectPort.value());
        } else {
          this->sendPacketSwitchedAsync(std::move(txPacket));
        }

        auto hwAsic = checkSameAndGetAsic(getAgentEnsemble()->getL3Asics());
        const uint64_t extraBytes =
            (hwAsic->getAsicType() == cfg::AsicType::ASIC_TYPE_TOMAHAWK6) ? 4
                                                                          : 0;
        const folly::IPAddressV6 expectedSid(tier.sid);

        std::optional<std::unique_ptr<folly::IOBuf>> frameRx;
        WITH_RETRIES({
          if (!frameRx.has_value()) {
            frameRx = snooper.waitForPacket(1);
          }
          EXPECT_EVENTUALLY_TRUE(frameRx.has_value())
              << tier.name << ": no trapped SRv6 egress packet captured";
        });
        ASSERT_TRUE(frameRx.has_value());

        folly::io::Cursor cursor(frameRx->get());
        utility::EthFrame frame(cursor);
        auto ethHdr = frame.header();
        EXPECT_EQ(
            ethHdr.etherType, static_cast<uint16_t>(ETHERTYPE::ETHERTYPE_IPV6));
        auto v6Payload = frame.v6PayLoad();
        ASSERT_TRUE(v6Payload.has_value());
        auto v6Hdr = v6Payload->header();
        EXPECT_EQ(v6Hdr.dstAddr, expectedSid)
            << tier.name << ": outer SRv6 destination does not match tier uSID";
        EXPECT_NE(v6Hdr.flowLabel, 0)
            << tier.name << ": expected non-zero outer IPv6 flow label";
        EXPECT_EQ(v6Hdr.trafficClass >> 2, tier.dscp)
            << tier.name
            << ": outer IPv6 DSCP should propagate from inner packet";

        auto origInner = origFrame.v6PayLoad();
        ASSERT_TRUE(origInner.has_value());
        auto capturedInner = v6Payload->v6PayLoad();
        ASSERT_NE(capturedInner, nullptr);
        EXPECT_EQ(*capturedInner, *origInner)
            << tier.name << ": inner IPv6 payload mismatch after SRv6 encap";

        if (!verifyCounters) {
          return;
        }

        WITH_RETRIES({
          const auto pktCountAfter = readAclCounter(tier.counterName);
          const auto bytesCountAfter =
              readAclCounter(tier.counterName, true /* bytes */);

          XLOG(INFO) << "SRv6 PBR ACE counter '" << tier.counterName << "' ("
                     << tier.name << "): pkts " << pktCountBefore << " -> "
                     << pktCountAfter << ", bytes " << bytesCountBefore
                     << " -> " << bytesCountAfter;

          EXPECT_EVENTUALLY_GE(pktCountAfter, pktCountBefore + 1)
              << tier.name << ": PBR ACE packet counter did not increment";
          EXPECT_EVENTUALLY_LE(pktCountAfter, pktCountBefore + 2)
              << tier.name << ": PBR ACE packet counter over-counted";

          EXPECT_EVENTUALLY_GE(
              bytesCountAfter + extraBytes, bytesCountBefore + sizeOfPacketSent)
              << tier.name << ": PBR ACE byte counter did not increment";
          EXPECT_EVENTUALLY_LE(
              bytesCountAfter, bytesCountBefore + (2 * sizeOfPacketSent) + 4)
              << tier.name << ": PBR ACE byte counter over-counted";

          for (const auto* other : kSrv6PbrTiers) {
            if (other == &tier) {
              continue;
            }
            const auto otherPktCount = readAclCounter(other->counterName);
            const auto otherBytesCount =
                readAclCounter(other->counterName, true /* bytes */);
            EXPECT_EQ(
                otherPktCount, countersBefore.at(other->counterName).first)
                << tier.name << " traffic bumped " << other->name
                << " packet counter";
            EXPECT_EQ(
                otherBytesCount, countersBefore.at(other->counterName).second)
                << tier.name << " traffic bumped " << other->name
                << " byte counter";
          }
        });
      }

      void verifyTierSrv6PbrCpuAndFrontPanel(
          const Srv6PbrTier& tier,
          const std::map<std::string, std::pair<uint64_t, uint64_t>>&
              countersBefore) {
        utility::EcmpSetupAnyNPorts6 ecmpHelper(
            getProgrammedState(),
            getSw()->needL2EntryForNeighbor(),
            getLocalMacAddress());
        const auto egressPort = getEgressPort(ecmpHelper.nhop(0).portDesc);
        const auto injectPort = findInjectPort({egressPort});

        verifyTierSrv6PbrEncapAndCounter(tier, countersBefore, std::nullopt);
        verifyTierSrv6PbrEncapAndCounter(
            tier, snapshotAllTierCounters(), injectPort);
      }

     private:
      void addSrv6EncapTunnelConfig(cfg::SwitchConfig & cfg) const {
        cfg::Srv6Tunnel tunnel;
        tunnel.srv6TunnelId() = kTunnelId;
        tunnel.underlayIntfID() = *cfg.interfaces()[0].intfID();
        tunnel.tunnelType() = TunnelType::SRV6_ENCAP;
        tunnel.srcIp() = kEncapSrcIp;
        "  4) SDK PR #81423 (PACKET_AND_BYTE_COUNTER API) not in "
        "tree.\n"
        "  5) SDK PR #81561 (ACE_SRV6 counter bank / " cfg.srv6Tunnels() = {
            tunnel};
      }

      void addSrv6SidTrapAcls(cfg::SwitchConfig & cfg, const HwAsic* asic)
          const {
        std::set<folly::CIDRNetwork> sidPrefixes;
        for (const auto* tier : kSrv6PbrTiers) {
          sidPrefixes.insert({folly::IPAddressV6(tier->sid), 128});
        }
        utility::addTrapPacketAcl(asic, &cfg, sidPrefixes);
      }

      void addSrv6PbrAclTable(cfg::SwitchConfig & cfg) const {
        if (!FLAGS_enable_acl_table_group) {
          return;
        }
        if (utility::getAclTable(
                cfg, cfg::AclStage::INGRESS, kPbrAclTableName)) {
          return;
        }
        utility::addAclTable(
            &cfg,
            cfg::AclStage::INGRESS,
            kPbrAclTableName,
            1 /* priority */,
            {
                cfg::AclTableActionType::COUNTER,
                cfg::AclTableActionType::REDIRECT,
            },
            {
                cfg::AclTableQualifier::TC,
                cfg::AclTableQualifier::ROUTE_DST,
            },
            {});
      }

      void addDscpToTcQosMap(cfg::SwitchConfig & cfg) const {
        cfg::QosMap qosMap;
        qosMap.dscpMaps()->resize(3);
        for (size_t i = 0; i < kSrv6PbrTiers.size(); ++i) {
          const auto* tier = kSrv6PbrTiers[i];
          qosMap.dscpMaps()[i].internalTrafficClass() = tier->tc;
          qosMap.dscpMaps()[i].fromDscpToTrafficClass()->push_back(tier->dscp);
          qosMap.trafficClassToQueueId()->emplace(tier->tc, 0);
        }

        cfg.qosPolicies()->resize(1);
        cfg.qosPolicies()[0].name() = kQosPolicyName;
        cfg.qosPolicies()[0].qosMap() = qosMap;

        if (!cfg.dataPlaneTrafficPolicy().has_value()) {
          cfg.dataPlaneTrafficPolicy() = cfg::TrafficPolicyConfig();
        }
        cfg.dataPlaneTrafficPolicy()->defaultQosPolicy() = kQosPolicyName;
      }

      void programSrv6PbrAclWithCounters() {
        auto cfg = getSw()->getConfig();
        addSrv6PbrAclWithCounters(cfg);
        applyNewConfig(cfg);
      }

      cfg::RedirectNextHop makeSrv6RedirectNh(
          const cfg::SwitchConfig& cfg, const std::string& sid) const {
        cfg::RedirectNextHop nh;
        nh.ip() = underlayNhIp_.str();
        nh.intfID() = *cfg.interfaces()[0].intfID();
        nh.tunnelType() = TunnelType::SRV6_ENCAP;
        nh.tunnelId() = kTunnelId;
        nh.srv6SegmentList() = {sid};
        return nh;
      }

      void addSrv6PbrAclWithCounters(cfg::SwitchConfig & cfg) const {
        addSrv6PbrAclTable(cfg);

        const auto goldRouteDstNh = makeSrv6RedirectNh(cfg, kGoldTier.sid);

        for (const auto* tier : kSrv6PbrTiers) {
          cfg::AclEntry acl;
          acl.name() = tier->aclName;
          acl.tc() = tier->tc;
          acl.routeDst() = goldRouteDstNh;
          utility::addAclEntry(
              &cfg, acl, kPbrAclTableName, cfg::AclStage::INGRESS);

          std::vector<cfg::CounterType> counterTypes{
              cfg::CounterType::PACKETS, cfg::CounterType::BYTES};
          utility::addTrafficCounter(&cfg, tier->counterName, counterTypes);

          cfg::RedirectToNextHopAction redirectAction;
          redirectAction.redirectNextHops() = {
              makeSrv6RedirectNh(cfg, tier->sid)};

          cfg::MatchAction action;
          action.redirectToNextHop() = redirectAction;
          action.counter() = tier->counterName;

          cfg::MatchToAction mta;
          mta.matcher() = tier->aclName;
          mta.action() = action;
          if (!cfg.dataPlaneTrafficPolicy().has_value()) {
            cfg.dataPlaneTrafficPolicy() = cfg::TrafficPolicyConfig();
          }
          cfg.dataPlaneTrafficPolicy()->matchToAction()->push_back(mta);
        }
      }

      RouteNextHopSet makeSrv6NhopSet(const std::string& sid) const {
        utility::EcmpSetupAnyNPorts6 ecmpHelper(
            getProgrammedState(),
            getSw()->needL2EntryForNeighbor(),
            getLocalMacAddress());
        const auto nhop = ecmpHelper.nhop(0);
        CHECK(nhop.linkLocalNhopIp.has_value());
        const folly::IPAddressV6 srv6Sid(sid);

        RouteNextHopSet nhops;
        nhops.insert(ResolvedNextHop(
            folly::IPAddress(*nhop.linkLocalNhopIp),
            nhop.intf,
            ECMP_WEIGHT,
            std::nullopt,
            std::nullopt,
            std::nullopt,
            std::nullopt,
            std::vector<folly::IPAddressV6>{srv6Sid},
            TunnelType::SRV6_ENCAP,
            kTunnelId));
        return nhops;
      }

      void programTierNhgsAndGoldRoute() {
        std::vector<std::pair<std::string, RouteNextHopSet>> groups;
        for (const auto* tier : kSrv6PbrTiers) {
          groups.emplace_back(tier->nhgName, makeSrv6NhopSet(tier->sid));
        }

        auto rib = getSw()->getRib();
        rib->addOrUpdateNamedNextHopGroups(
            getSw()->getScopeResolver(),
            groups,
            createRibToSwitchStateFunction(),
            getSw());

        UnicastRoute route;
        route.dest()->ip() = facebook::network::toBinaryAddress(
            folly::IPAddress(kGoldRouteV6Dst));
        route.dest()->prefixLength() = kGoldRouteV6PrefixLen;
        NamedRouteDestination namedDest;
        namedDest.nextHopGroup_ref() = kGoldTier.nhgName;
        route.namedRouteDestination() = namedDest;
        route.counterID() = kGoldTier.nhgName;

        auto routeUpdater = getSw()->getRouteUpdater();
        routeUpdater.addRoute(RouterID(0), ClientID::TE_AGENT, route);
        routeUpdater.program();
      }
    };

    TYPED_TEST_SUITE(AgentSrv6PbrCounterTest, Srv6PbrPortTypes);

    TYPED_TEST(
        AgentSrv6PbrCounterTest, RedirectToSrv6BumpsPacketAndByteCounter) {
      auto setup = [this]() { this->setupHelper(); };

      auto verify = [this]() {
        auto counters = this->snapshotAllTierCounters();
        for (const auto* tier : kSrv6PbrTiers) {
          this->verifyTierSrv6PbrCpuAndFrontPanel(*tier, counters);
          counters = this->snapshotAllTierCounters();
        }
      };

      this->verifyAcrossWarmBoots(setup, verify);
    }

} // namespace facebook::fboss
