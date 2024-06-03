// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/AgentHwTest.h"

#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/utils/AclTestUtils.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/lib/CommonUtils.h"

#include <folly/IPAddress.h>
#include "fboss/agent/TxPacket.h"

#include "fboss/agent/test/gen-cpp2/production_features_types.h"

#include <boost/range/combine.hpp>
#include <folly/logging/xlog.h>

namespace {
template <typename AddrT>
struct TestParams {
  AddrT senderIp;
  AddrT receiverIp;
  AddrT mirrorDestinationIp;

  TestParams(
      const AddrT& _senderIp,
      const AddrT& _receiverIp,
      const AddrT& _mirrorDestinationIp)
      : senderIp(_senderIp),
        receiverIp(_receiverIp),
        mirrorDestinationIp(_mirrorDestinationIp) {}
};

template <typename AddrT>
TestParams<AddrT> getTestParams();

template <>
TestParams<folly::IPAddressV4> getTestParams<folly::IPAddressV4>() {
  return TestParams<folly::IPAddressV4>(
      folly::IPAddressV4("101.0.0.10"), // sender
      folly::IPAddressV4("201.0.0.10"), // receiver
      folly::IPAddressV4("101.0.0.11")); // erspan destination
}

template <>
TestParams<folly::IPAddressV6> getTestParams<folly::IPAddressV6>() {
  return TestParams<folly::IPAddressV6>(
      folly::IPAddressV6("101::10"), // sender
      folly::IPAddressV6("201::10"), // receiver
      folly::IPAddressV6("101::11")); // erspan destination
}

using TestTypes = ::testing::Types<folly::IPAddressV4, folly::IPAddressV6>;
const std::string kIngressSpan = "ingress_span";
const std::string kIngressErspan = "ingress_erspan";
const std::string kEgressSpan = "egress_span";
const std::string kEgressErspan = "egress_erspan";

// Port 0 is used for traffic and port 1 is used for mirroring.
const uint8_t kTrafficPortIndex = 0;
const uint8_t kMirrorToPortIndex = 1;

} // namespace

namespace facebook::fboss {

template <typename AddrT>
class AgentMirroringTest : public AgentHwTest {
 public:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto config = utility::onePortPerInterfaceConfig(
        ensemble.getSw(),
        ensemble.masterLogicalPortIds(),
        true /*interfaceHasSubnet*/);
    return config;
  }

  /*
   * This configures a local/erspan mirror session.
   * Adds a tunnel config if the mirrorname is erspan.
   */
  void addMirrorConfig(
      cfg::SwitchConfig* cfg,
      const AgentEnsemble& ensemble,
      const std::string& mirrorName,
      bool truncate) const {
    auto mirrorToPort = ensemble.masterLogicalPortIds(
        {cfg::PortType::INTERFACE_PORT})[kMirrorToPortIndex];
    auto params = getTestParams<AddrT>();
    cfg::MirrorDestination destination;
    destination.egressPort() = cfg::MirrorEgressPort();
    destination.egressPort()->logicalID_ref() = mirrorToPort;
    if (mirrorName == kIngressErspan || mirrorName == kEgressErspan) {
      cfg::MirrorTunnel tunnel;
      cfg::GreTunnel greTunnel;
      greTunnel.ip() = params.mirrorDestinationIp.str();
      tunnel.greTunnel() = greTunnel;
      destination.tunnel() = tunnel;
    }
    cfg::Mirror mirrorConfig;
    mirrorConfig.name() = mirrorName;
    mirrorConfig.destination() = destination;
    mirrorConfig.truncate() = truncate;
    cfg->mirrors()->push_back(mirrorConfig);
  }

  void addPortMirrorConfig(
      cfg::SwitchConfig* cfg,
      const AgentEnsemble& ensemble,
      const std::string& mirrorName) const {
    auto trafficPort = ensemble.masterLogicalPortIds(
        {cfg::PortType::INTERFACE_PORT})[kTrafficPortIndex];
    auto portConfig = utility::findCfgPort(*cfg, trafficPort);
    if (mirrorName == kIngressSpan || mirrorName == kIngressErspan) {
      portConfig->ingressMirror() = mirrorName;
    } else if (mirrorName == kEgressSpan || mirrorName == kEgressErspan) {
      portConfig->egressMirror() = mirrorName;
    } else {
      throw FbossError("Invalid mirror name ", mirrorName);
    }
  }

  void sendPackets(int count, size_t payloadSize = 1) {
    auto params = getTestParams<AddrT>();
    auto vlanId = utility::firstVlanID(getProgrammedState());
    auto intfMac = utility::getFirstInterfaceMac(getProgrammedState());
    std::vector<uint8_t> payload(payloadSize, 0xff);
    auto trafficPort = getAgentEnsemble()->masterLogicalPortIds(
        {cfg::PortType::INTERFACE_PORT})[kTrafficPortIndex];
    auto oldPacketStats = getLatestPortStats(trafficPort);
    auto oldOutPkts = *oldPacketStats.outUnicastPkts_();
    auto i = 0;
    while (i < count) {
      auto pkt = utility::makeUDPTxPacket(
          getSw(),
          vlanId,
          intfMac,
          intfMac,
          params.senderIp,
          params.receiverIp,
          srcL4Port_,
          dstL4Port_,
          0,
          255,
          payload);
      getSw()->sendPacketSwitchedAsync(std::move(pkt));
      i++;
    }
    WITH_RETRIES({
      auto portStats = getLatestPortStats(trafficPort);
      auto newOutPkt = *portStats.outUnicastPkts_() +
          *portStats.outMulticastPkts_() + *portStats.outBroadcastPkts_();
      EXPECT_EVENTUALLY_GE(newOutPkt, oldOutPkts);
      EXPECT_EVENTUALLY_EQ(newOutPkt - oldOutPkts, count);
    });
  }

  RoutePrefix<AddrT> getMirrorRoutePrefix(const folly::IPAddress dip) const {
    if constexpr (std::is_same_v<AddrT, folly::IPAddressV4>) {
      return RoutePrefix<AddrT>{
          folly::IPAddressV4{dip.str()}, static_cast<uint8_t>(dip.bitCount())};
    } else {
      return RoutePrefix<AddrT>{
          folly::IPAddressV6{dip.str()}, static_cast<uint8_t>(dip.bitCount())};
    }
  }

  template <typename T = AddrT>
  void resolveMirror(const std::string& mirrorName) {
    utility::EcmpSetupAnyNPorts<AddrT> ecmpHelper(getProgrammedState());
    auto trafficPort = getAgentEnsemble()->masterLogicalPortIds(
        {cfg::PortType::INTERFACE_PORT})[kTrafficPortIndex];
    auto mirrorToPort = getAgentEnsemble()->masterLogicalPortIds(
        {cfg::PortType::INTERFACE_PORT})[kMirrorToPortIndex];
    EXPECT_EQ(trafficPort, ecmpHelper.nhop(0).portDesc.phyPortID());
    EXPECT_EQ(mirrorToPort, ecmpHelper.nhop(1).portDesc.phyPortID());
    resolveNeigborAndProgramRoutes(ecmpHelper, 1);
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      boost::container::flat_set<PortDescriptor> nhopPorts{
          PortDescriptor(mirrorToPort)};
      return utility::EcmpSetupAnyNPorts<AddrT>(in).resolveNextHops(
          in, nhopPorts);
    });
    getSw()->getUpdateEvb()->runInEventBaseThreadAndWait([] {});
    auto mirror = getSw()->getState()->getMirrors()->getNodeIf(mirrorName);
    auto dip = mirror->getDestinationIp();
    if (dip.has_value()) {
      auto prefix = getMirrorRoutePrefix(dip.value());
      boost::container::flat_set<PortDescriptor> nhopPorts{
          PortDescriptor(mirrorToPort)};
      auto wrapper = getSw()->getRouteUpdater();
      ecmpHelper.programRoutes(&wrapper, nhopPorts, {prefix});
    }
  }

  void addAclMirrorConfig(
      cfg::SwitchConfig* cfg,
      const AgentEnsemble& ensemble,
      const std::optional<const std::string>& ingressMirrorName = std::nullopt,
      const std::optional<const std::string>& egressMirrorName =
          std::nullopt) const {
    auto trafficPort = ensemble.masterLogicalPortIds(
        {cfg::PortType::INTERFACE_PORT})[kTrafficPortIndex];
    auto aclEntryName = "mirror_acl";
    utility::addAclTableGroup(
        cfg, cfg::AclStage::INGRESS, utility::getAclTableGroupName());
    utility::addDefaultAclTable(*cfg);
    auto aclEntry = cfg::AclEntry();
    aclEntry.name() = aclEntryName;
    aclEntry.actionType() = cfg::AclActionType::PERMIT;
    aclEntry.l4SrcPort() = srcL4Port_;
    aclEntry.l4DstPort() = dstL4Port_;
    aclEntry.dstPort() = trafficPort;
    aclEntry.proto() = 17;
    /*
     * The number of packets mirrorred through ACL is different in Native BCM
     * and BCM-SAI (CS00012203195). The number of packets mirrorred in Native
     * BCM is 2 whereas number of packets mirrored in BCM-SAI is 4.
     *
     * 1) Send packet from CPU and perform hw lookup in the ASIC
     * 2) Packet is routed and forwarded to Port 0
     * 3) Packet is mirrored (ingress and egress) to port 1
     * 4) For native BCM, the packet routed out of port 0 is looped back
     *    and dropped.
     *    For BCM-SAI, the packet routed out of port 0 is looped back
     *    but L2 forwarded, ACL is looked up which mirrors again (2
     *    more packets) and dropped due to same port check.
     *
     *  The reason for the difference is the packet in native BCM is
     *  treated as unknown unicast and the dst port lookup is not performed
     *  in IFP for unknown unicast packets. Whereas in BCM-SAI, we
     *  install a FDB entry for every neighbor entry. The FDB entry is hit
     *  which makes the packet to be treated as known unicast and the IFP
     *  is hit and the packets are mirrored again.
     *
     * In order to avoid the ACL lookup, we can add additional qualifiers.
     * - SRC PORT: can be used once TAJO supports it.
     * - DMAC: DMAC can be used since the routed packet will have a different
     *   DMAC. Not all platforms support DMAC as a qualifier (For eg, Tajo)
     * - DROP Bit: WB + Fitting implications
     * - TTL: Use TTL 255 to ensure this entry is hit only for the first packet.
     *
     * For now, using TTL qualifier to ensure the routed packet is dropped. We
     * can update the test to include source port qualifier with CPU port
     * and enable it on TAJO platform.
     */
    cfg::Ttl ttl;
    ttl.value() = 255;
    ttl.mask() = 0xFF;
    aclEntry.ttl() = ttl;
    cfg->acls()->push_back(aclEntry);

    cfg::MatchAction matchAction = cfg::MatchAction();
    if (ingressMirrorName.has_value()) {
      matchAction.ingressMirror() = ingressMirrorName.value();
    }
    if (egressMirrorName.has_value()) {
      matchAction.egressMirror() = egressMirrorName.value();
    }
    cfg::MatchToAction matchToAction = cfg::MatchToAction();
    matchToAction.matcher() = aclEntryName;
    matchToAction.action() = matchAction;
    cfg->dataPlaneTrafficPolicy() = cfg::TrafficPolicyConfig();
    cfg->dataPlaneTrafficPolicy()->matchToAction()->push_back(matchToAction);
  }

  void verify(
      const std::optional<std::string>& ingressMirrorName = std::nullopt,
      const std::optional<std::string>& egressMirrorName = std::nullopt,
      int payloadSize = 500) {
    auto trafficPort = getAgentEnsemble()->masterLogicalPortIds(
        {cfg::PortType::INTERFACE_PORT})[kTrafficPortIndex];
    auto mirrorToPort = getAgentEnsemble()->masterLogicalPortIds(
        {cfg::PortType::INTERFACE_PORT})[kMirrorToPortIndex];
    auto ingressMirror = this->getProgrammedState()->getMirrors()->getNodeIf(
        ingressMirrorName.value());
    ASSERT_NE(ingressMirror, nullptr);
    EXPECT_EQ(ingressMirror->isResolved(), true);

    if (egressMirrorName.has_value()) {
      auto egressMirror = this->getProgrammedState()->getMirrors()->getNodeIf(
          egressMirrorName.value());
      ASSERT_NE(egressMirror, nullptr);
      EXPECT_EQ(egressMirror->isResolved(), true);
    }

    auto trafficPortPktStatsBefore = getLatestPortStats(trafficPort);
    auto mirrorPortPktStatsBefore = getLatestPortStats(mirrorToPort);

    auto trafficPortPktsBefore = *trafficPortPktStatsBefore.outUnicastPkts_();
    auto mirroredPortPktsBefore = *trafficPortPktStatsBefore.outUnicastPkts_();

    this->sendPackets(1, payloadSize);

    WITH_RETRIES({
      auto trafficPortPktStatsAfter = getLatestPortStats(trafficPort);
      auto trafficPortPktsAfter = *trafficPortPktStatsAfter.outUnicastPkts_();
      EXPECT_EVENTUALLY_EQ(1, trafficPortPktsAfter - trafficPortPktsBefore);
    });

    /*
     * Port mirror :
     * 2 packets are mirrored one egressing  and other ingressing the port
     * because of loopback mode
     * Acl mirror:
     * 2 packets are mirrored one ingressing IFP and other egressing IFP
     */
    auto expectedMirrorPackets = 1;
    if (egressMirrorName.has_value()) {
      expectedMirrorPackets += 1;
    }

    WITH_RETRIES({
      auto mirrorPortPktStatsAfter = getLatestPortStats(mirrorToPort);
      auto mirroredPortPktsAfter = *mirrorPortPktStatsAfter.outUnicastPkts_();
      EXPECT_EVENTUALLY_EQ(
          mirroredPortPktsAfter - mirroredPortPktsBefore,
          expectedMirrorPackets);
    });
  }

  void testPortMirror(
      const std::optional<std::string>& ingressMirrorName = std::nullopt,
      const std::optional<std::string>& egressMirrorName = std::nullopt) {
    auto setup = [=, this]() {
      this->resolveMirror(ingressMirrorName.value());
    };
    auto verify = [=, this]() {
      this->verify(ingressMirrorName, egressMirrorName);
    };
    this->verifyAcrossWarmBoots(setup, verify);
  }

  void testAclMirror(
      const std::optional<std::string>& ingressMirrorName = std::nullopt,
      const std::optional<std::string>& egressMirrorName = std::nullopt) {
    auto setup = [=, this]() {
      this->resolveMirror(ingressMirrorName.value());
    };
    auto verify = [=, this]() {
      this->verify(ingressMirrorName, egressMirrorName);
    };
    this->verifyAcrossWarmBoots(setup, verify);
  }

  void testPortMirrorWithLargePacket(
      const std::optional<std::string>& ingressMirrorName = std::nullopt,
      const std::optional<std::string>& egressMirrorName = std::nullopt) {
    auto setup = [=, this]() {
      this->resolveMirror(ingressMirrorName.value());
    };
    auto verify = [=, this]() {
      auto mirrorToPort = getAgentEnsemble()->masterLogicalPortIds(
          {cfg::PortType::INTERFACE_PORT})[kMirrorToPortIndex];
      auto statsBefore = getLatestPortStats(mirrorToPort);
      this->verify(ingressMirrorName, egressMirrorName, 8000);
      WITH_RETRIES({
        auto statsAfter = getLatestPortStats(mirrorToPort);

        auto outBytes = (*statsAfter.outBytes_() - *statsBefore.outBytes_());
        // mirror is on both ingress and egress, packet loops back and gets
        // mirrored twice
        if (egressMirrorName.has_value()) {
          outBytes = outBytes / 2;
        }
        // TODO: on TH3 for v6 packets, 254 bytes are mirrored which is a single
        // MMU cell. but for v4 packets, 234 bytes are mirrored. need to
        // investigate this behavior.
        EXPECT_EVENTUALLY_LE(
            outBytes, 1500 /* payload is truncated to ethernet MTU of 1500 */);
      });
    };
    this->verifyAcrossWarmBoots(setup, verify);
  }

  template <typename T = AddrT>
  bool skipTest() const {
    return std::is_same<T, folly::IPAddressV6>::value &&
        !isSupportedOnAllAsics(HwAsic::Feature::ERSPANv6);
  }

  const RouterID kRid{0};
  const uint16_t srcL4Port_{1234};
  const uint16_t dstL4Port_{4321};
};

template <typename AddrT>
class AgentIngressPortSpanMirroringTest : public AgentMirroringTest<AddrT> {
 public:
 public:
  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    return {production_features::ProductionFeature::INGRESS_MIRRORING};
  }

  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto cfg = AgentMirroringTest<AddrT>::initialConfig(ensemble);
    this->addMirrorConfig(&cfg, ensemble, kIngressSpan, false /* truncate */);
    this->addPortMirrorConfig(&cfg, ensemble, kIngressSpan);
    return cfg;
  }
};

template <typename AddrT>
class AgentIngressPortErspanMirroringTest : public AgentMirroringTest<AddrT> {
 public:
  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    return {production_features::ProductionFeature::INGRESS_MIRRORING};
  }

  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto cfg = AgentMirroringTest<AddrT>::initialConfig(ensemble);
    this->addMirrorConfig(&cfg, ensemble, kIngressErspan, false /* truncate */);
    this->addPortMirrorConfig(&cfg, ensemble, kIngressErspan);
    return cfg;
  }
};

template <typename AddrT>
class AgentIngressPortErspanMirroringTruncateTest
    : public AgentMirroringTest<AddrT> {
 public:
  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    return {
        production_features::ProductionFeature::INGRESS_MIRRORING,
        production_features::ProductionFeature::MIRROR_PACKET_TRUNCATION};
  }

  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto cfg = AgentMirroringTest<AddrT>::initialConfig(ensemble);
    this->addMirrorConfig(&cfg, ensemble, kIngressErspan, true /* truncate */);
    this->addPortMirrorConfig(&cfg, ensemble, kIngressErspan);
    return cfg;
  }
};

template <typename AddrT>
class AgentIngressAclSpanMirroringTest : public AgentMirroringTest<AddrT> {
 public:
  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    return {production_features::ProductionFeature::INGRESS_MIRRORING};
  }

  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto cfg = AgentMirroringTest<AddrT>::initialConfig(ensemble);
    this->addMirrorConfig(&cfg, ensemble, kIngressSpan, false /* truncate */);
    this->addAclMirrorConfig(&cfg, ensemble, kIngressSpan);
    return cfg;
  }
};

template <typename AddrT>
class AgentIngressAclErspanMirroringTest : public AgentMirroringTest<AddrT> {
 public:
  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    return {production_features::ProductionFeature::INGRESS_MIRRORING};
  }

  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto cfg = AgentMirroringTest<AddrT>::initialConfig(ensemble);
    this->addMirrorConfig(&cfg, ensemble, kIngressErspan, false /* truncate */);
    this->addAclMirrorConfig(&cfg, ensemble, kIngressErspan);
    return cfg;
  }
};

template <typename AddrT>
class AgentIngressEgressPortSpanMirroringTest
    : public AgentMirroringTest<AddrT> {
 public:
 public:
  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    return {
        production_features::ProductionFeature::INGRESS_MIRRORING,
        production_features::ProductionFeature::EGRESS_MIRRORING};
  }

  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto cfg = AgentMirroringTest<AddrT>::initialConfig(ensemble);
    this->addMirrorConfig(&cfg, ensemble, kIngressSpan, false /* truncate */);
    this->addPortMirrorConfig(&cfg, ensemble, kIngressSpan);
    this->addMirrorConfig(&cfg, ensemble, kEgressSpan, false /* truncate */);
    this->addPortMirrorConfig(&cfg, ensemble, kEgressSpan);
    return cfg;
  }
};

template <typename AddrT>
class AgentIngressEgressPortErspanMirroringTest
    : public AgentMirroringTest<AddrT> {
 public:
  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    return {
        production_features::ProductionFeature::INGRESS_MIRRORING,
        production_features::ProductionFeature::EGRESS_MIRRORING};
  }

  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto cfg = AgentMirroringTest<AddrT>::initialConfig(ensemble);
    this->addMirrorConfig(&cfg, ensemble, kIngressErspan, false /* truncate */);
    this->addPortMirrorConfig(&cfg, ensemble, kIngressErspan);
    this->addMirrorConfig(&cfg, ensemble, kEgressErspan, false /* truncate */);
    this->addPortMirrorConfig(&cfg, ensemble, kEgressErspan);
    return cfg;
  }
};

template <typename AddrT>
class AgentIngressEgressPortErspanMirroringTruncateTest
    : public AgentMirroringTest<AddrT> {
 public:
  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    return {
        production_features::ProductionFeature::INGRESS_MIRRORING,
        production_features::ProductionFeature::MIRROR_PACKET_TRUNCATION,
        production_features::ProductionFeature::EGRESS_MIRRORING};
  }

  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto cfg = AgentMirroringTest<AddrT>::initialConfig(ensemble);
    this->addMirrorConfig(&cfg, ensemble, kIngressErspan, true /* truncate */);
    this->addPortMirrorConfig(&cfg, ensemble, kIngressErspan);
    this->addMirrorConfig(&cfg, ensemble, kEgressErspan, true /* truncate */);
    this->addPortMirrorConfig(&cfg, ensemble, kEgressErspan);
    return cfg;
  }
};

template <typename AddrT>
class AgentIngressEgressAclSpanMirroringTest
    : public AgentMirroringTest<AddrT> {
 public:
  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    return {
        production_features::ProductionFeature::INGRESS_MIRRORING,
        production_features::ProductionFeature::EGRESS_MIRRORING};
  }

  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto cfg = AgentMirroringTest<AddrT>::initialConfig(ensemble);
    this->addMirrorConfig(&cfg, ensemble, kIngressSpan, false /* truncate */);
    this->addAclMirrorConfig(&cfg, ensemble, kIngressSpan);
    this->addMirrorConfig(&cfg, ensemble, kEgressSpan, false /* truncate */);
    this->addAclMirrorConfig(&cfg, ensemble, kEgressSpan);
    return cfg;
  }
};

template <typename AddrT>
class AgentIngressEgressAclErspanMirroringTest
    : public AgentMirroringTest<AddrT> {
 public:
  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    return {
        production_features::ProductionFeature::INGRESS_MIRRORING,
        production_features::ProductionFeature::EGRESS_MIRRORING};
  }

  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto cfg = AgentMirroringTest<AddrT>::initialConfig(ensemble);
    this->addMirrorConfig(&cfg, ensemble, kIngressErspan, false /* truncate */);
    this->addAclMirrorConfig(&cfg, ensemble, kIngressErspan);
    this->addMirrorConfig(&cfg, ensemble, kEgressErspan, false /* truncate */);
    this->addAclMirrorConfig(&cfg, ensemble, kEgressErspan);
    return cfg;
  }
};

TYPED_TEST_SUITE(AgentIngressPortSpanMirroringTest, TestTypes);
TYPED_TEST_SUITE(AgentIngressPortErspanMirroringTest, TestTypes);
TYPED_TEST_SUITE(AgentIngressPortErspanMirroringTruncateTest, TestTypes);
TYPED_TEST_SUITE(AgentIngressAclSpanMirroringTest, TestTypes);
TYPED_TEST_SUITE(AgentIngressAclErspanMirroringTest, TestTypes);
TYPED_TEST_SUITE(AgentIngressEgressPortSpanMirroringTest, TestTypes);
TYPED_TEST_SUITE(AgentIngressEgressPortErspanMirroringTest, TestTypes);
TYPED_TEST_SUITE(AgentIngressEgressPortErspanMirroringTruncateTest, TestTypes);
TYPED_TEST_SUITE(AgentIngressEgressAclSpanMirroringTest, TestTypes);
TYPED_TEST_SUITE(AgentIngressEgressAclErspanMirroringTest, TestTypes);

TYPED_TEST(AgentIngressPortSpanMirroringTest, SpanPortMirror) {
  this->testPortMirror(kIngressSpan);
}

TYPED_TEST(AgentIngressPortErspanMirroringTest, ErspanPortMirror) {
  this->testPortMirror(kIngressErspan);
}

TYPED_TEST(AgentIngressAclSpanMirroringTest, SpanAclMirror) {
  this->testAclMirror(kIngressSpan);
}

TYPED_TEST(AgentIngressAclErspanMirroringTest, ErspanAclMirror) {
  this->testAclMirror(kIngressErspan);
}

TYPED_TEST(
    AgentIngressPortErspanMirroringTruncateTest,
    TrucatePortErspanMirror) {
  this->testPortMirrorWithLargePacket(kIngressErspan);
}

TYPED_TEST(AgentIngressEgressPortSpanMirroringTest, SpanPortMirror) {
  this->testPortMirror(kIngressSpan, kEgressSpan);
}

TYPED_TEST(AgentIngressEgressPortErspanMirroringTest, ErspanPortMirror) {
  this->testPortMirror(kIngressErspan, kEgressErspan);
}

TYPED_TEST(AgentIngressEgressAclSpanMirroringTest, SpanAclMirror) {
  this->testAclMirror(kIngressSpan, kEgressSpan);
}

TYPED_TEST(AgentIngressEgressAclErspanMirroringTest, ErspanAclMirror) {
  this->testAclMirror(kIngressErspan, kEgressErspan);
}

TYPED_TEST(
    AgentIngressEgressPortErspanMirroringTruncateTest,
    TrucatePortErspanMirror) {
  this->testPortMirrorWithLargePacket(kIngressErspan, kEgressErspan);
}
} // namespace facebook::fboss
