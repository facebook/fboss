// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <fmt/format.h>

#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/agent/test/utils/AclTestUtils.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/PacketTestUtils.h"

DECLARE_bool(enable_acl_table_group);

namespace facebook::fboss {

class AgentPortBoundIngressAclTest : public AgentHwTest {
 public:
  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {
        ProductionFeature::L3_FORWARDING,
        ProductionFeature::PORT_BOUND_INGRESS_ACL};
  }

  std::optional<size_t> maxRequiredInterfacePorts() const override {
    return 4;
  }

  void setCmdLineFlagOverrides() const override {
    AgentHwTest::setCmdLineFlagOverrides();
    FLAGS_enable_acl_table_group = true;
  }

 protected:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    return utility::onePortPerInterfaceConfig(
        ensemble.getSw(),
        ensemble.masterLogicalPortIds(),
        true /* interfaceHasSubnet */);
  }

  void setupL3Forwarding() {
    utility::EcmpSetupAnyNPorts6 ecmpHelper(
        getProgrammedState(), getSw()->needL2EntryForNeighbor());
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      return ecmpHelper.resolveNextHops(in, 1);
    });
    auto updater = getSw()->getRouteUpdater();
    ecmpHelper.programRoutes(&updater, 1);
  }

  void addRestrictAclTable(cfg::AclTableGroup* aclTableGroup) {
    // TODO: Add the remaining production restrict-* ACL entries to this table.
    cfg::AclEntry permitAcl;
    permitAcl.name() = kRestrictPermitAclName;
    permitAcl.actionType() = cfg::AclActionType::PERMIT;
    permitAcl.l4DstPort() = kRestrictPermitL4DstPort;

    cfg::AclEntry denyAcl;
    denyAcl.name() = kRestrictDenyAclName;
    denyAcl.actionType() = cfg::AclActionType::DENY;

    cfg::AclTable aclTable;
    aclTable.name() = kRestrictAclTableName;
    aclTable.priority() = 1;
    aclTable.actionTypes() = {
        cfg::AclTableActionType::PACKET_ACTION,
        cfg::AclTableActionType::COUNTER};
    aclTable.qualifiers() = {cfg::AclTableQualifier::L4_DST_PORT};
    aclTable.aclEntries() = {std::move(permitAcl), std::move(denyAcl)};
    aclTableGroup->aclTables()->push_back(std::move(aclTable));
  }

  void addBlockAclTable(cfg::AclTableGroup* aclTableGroup) {
    // TODO: Add the remaining production block-* ACL entries to this table.
    cfg::AclEntry permitAcl;
    permitAcl.name() = kBlockPermitAclName;
    permitAcl.actionType() = cfg::AclActionType::PERMIT;
    permitAcl.proto() = 17;
    permitAcl.l4DstPort() = kBlockPermitL4DstPort;

    cfg::AclEntry denyAcl;
    denyAcl.name() = kBlockDenyAclName;
    denyAcl.actionType() = cfg::AclActionType::DENY;

    cfg::AclTable aclTable;
    aclTable.name() = kBlockAclTableName;
    aclTable.priority() = 2;
    aclTable.actionTypes() = {
        cfg::AclTableActionType::PACKET_ACTION,
        cfg::AclTableActionType::COUNTER};
    aclTable.qualifiers() = {
        cfg::AclTableQualifier::IP_PROTOCOL_NUMBER,
        cfg::AclTableQualifier::L4_DST_PORT};
    aclTable.aclEntries() = {std::move(permitAcl), std::move(denyAcl)};
    aclTableGroup->aclTables()->push_back(std::move(aclTable));
  }

  void addPortBoundAclTableGroup(cfg::SwitchConfig* config) {
    cfg::AclTableGroup aclTableGroup;
    aclTableGroup.name() = kAclTableGroupName;
    aclTableGroup.stage() = cfg::AclStage::INGRESS;
    aclTableGroup.bindPoint() = cfg::AclTableGroupBindPoint::PORT;
    addRestrictAclTable(&aclTableGroup);
    addBlockAclTable(&aclTableGroup);
    config->aclTableGroups()->push_back(std::move(aclTableGroup));

    for (const auto& [aclName, counterName] :
         std::initializer_list<std::pair<const char*, const char*>>{
             {kRestrictPermitAclName, kRestrictPermitCounterName},
             {kRestrictDenyAclName, kRestrictDenyCounterName},
             {kBlockPermitAclName, kBlockPermitCounterName},
             {kBlockDenyAclName, kBlockDenyCounterName}}) {
      utility::addAclStat(
          config,
          aclName,
          counterName,
          utility::getAclCounterTypes(getAgentEnsemble()->getL3Asics()));
    }
  }

  void bindPortToAclTable(
      cfg::SwitchConfig* config,
      PortID port,
      folly::StringPiece aclTableName) {
    utility::findCfgPort(*config, port)->ingressAclTableName() =
        aclTableName.str();
  }

  void configurePortBoundAcl(PortID restrictPort, PortID blockPort) {
    auto config = initialConfig(*getAgentEnsemble());
    addPortBoundAclTableGroup(&config);
    bindPortToAclTable(&config, restrictPort, kRestrictAclTableName);
    bindPortToAclTable(&config, blockPort, kBlockAclTableName);

    XLOG(INFO) << "Configuring port-bound ingress ACL group "
               << kAclTableGroupName << ": port " << restrictPort
               << " -> table " << kRestrictAclTableName << ", port "
               << blockPort << " -> table " << kBlockAclTableName;
    applyNewConfig(config);
  }

  void sendPacket(PortID ingressPort, uint16_t l4DstPort) {
    auto vlanId = getVlanIDForTx();
    auto intfMac =
        getMacForFirstInterfaceWithPortsForTesting(getProgrammedState());
    auto srcMac = utility::MacAddressGenerator().get(intfMac.u64HBO() + 1);
    auto packet = utility::makeUDPTxPacket(
        getSw(),
        vlanId,
        srcMac,
        intfMac,
        folly::IPAddressV6(kSrcIp),
        folly::IPAddressV6(kDstIp),
        kL4SrcPort,
        l4DstPort,
        0,
        255);
    getSw()->sendPacketOutOfPortAsync(std::move(packet), ingressPort);
  }

  int64_t getPortCounter(PortID portId, folly::StringPiece counterName) const {
    auto port = getProgrammedState()->getPorts()->getNode(portId);
    auto key = fmt::format("{}.{}.sum", port->getName(), counterName);
    return getAgentEnsemble()->getFb303Counter(
        key, scopeResolver().scope(portId).switchId());
  }

  void verifyAclPacket(
      folly::StringPiece caseName,
      PortID ingressPort,
      uint16_t l4DstPort,
      const std::string& permitCounterName,
      const std::string& denyCounterName,
      bool expectPermit) {
    const auto egressPort = masterLogicalPortIds()[0];
    const auto permitCounterBefore =
        utility::getAclInOutPackets(getSw(), permitCounterName);
    const auto denyCounterBefore =
        utility::getAclInOutPackets(getSw(), denyCounterName);
    const auto egressPacketsBefore =
        getPortCounter(egressPort, kOutUnicastPktsCounterName);

    XLOG(INFO) << "[PortBoundIngressAcl][" << caseName
               << "] Sending IPv6 UDP packet " << kSrcIp << ":" << kL4SrcPort
               << " -> " << kDstIp << ":" << l4DstPort << " on ingress port "
               << ingressPort << "; expected "
               << (expectPermit ? "PERMIT" : "DROP");
    sendPacket(ingressPort, l4DstPort);
    WITH_RETRIES({
      if (expectPermit) {
        EXPECT_EVENTUALLY_GE(
            utility::getAclInOutPackets(getSw(), permitCounterName),
            permitCounterBefore + 1);
        EXPECT_EVENTUALLY_EQ(
            utility::getAclInOutPackets(getSw(), denyCounterName),
            denyCounterBefore);
        EXPECT_EVENTUALLY_GT(
            getPortCounter(egressPort, kOutUnicastPktsCounterName),
            egressPacketsBefore);
      } else {
        EXPECT_EVENTUALLY_EQ(
            utility::getAclInOutPackets(getSw(), permitCounterName),
            permitCounterBefore);
        EXPECT_EVENTUALLY_GE(
            utility::getAclInOutPackets(getSw(), denyCounterName),
            denyCounterBefore + 1);
        EXPECT_EVENTUALLY_EQ(
            getPortCounter(egressPort, kOutUnicastPktsCounterName),
            egressPacketsBefore);
      }
    });

    const auto permitCounterAfter =
        utility::getAclInOutPackets(getSw(), permitCounterName);
    const auto denyCounterAfter =
        utility::getAclInOutPackets(getSw(), denyCounterName);
    const auto egressPacketsAfter =
        getPortCounter(egressPort, kOutUnicastPktsCounterName);
    XLOG(INFO) << "[PortBoundIngressAcl][" << caseName << "] Observed "
               << (egressPacketsAfter > egressPacketsBefore ? "PERMITTED"
                                                            : "DROPPED")
               << "; permit counter " << permitCounterBefore << " -> "
               << permitCounterAfter << "; deny counter " << denyCounterBefore
               << " -> " << denyCounterAfter << "; egress packets "
               << egressPacketsBefore << " -> " << egressPacketsAfter;
  }

  void verifyUnboundPacket(PortID ingressPort, uint16_t l4DstPort) {
    const auto egressPort = masterLogicalPortIds()[0];
    const auto restrictDenyCounterBefore =
        utility::getAclInOutPackets(getSw(), kRestrictDenyCounterName);
    const auto blockDenyCounterBefore =
        utility::getAclInOutPackets(getSw(), kBlockDenyCounterName);
    const auto egressPacketsBefore =
        getPortCounter(egressPort, kOutUnicastPktsCounterName);

    XLOG(INFO) << "[PortBoundIngressAcl][Unbound] Sending IPv6 UDP packet "
               << kSrcIp << ":" << kL4SrcPort << " -> " << kDstIp << ":"
               << l4DstPort << " on unbound ingress port " << ingressPort
               << "; expected PERMIT without matching either table";
    sendPacket(ingressPort, l4DstPort);
    WITH_RETRIES({
      EXPECT_EVENTUALLY_EQ(
          utility::getAclInOutPackets(getSw(), kRestrictDenyCounterName),
          restrictDenyCounterBefore);
      EXPECT_EVENTUALLY_EQ(
          utility::getAclInOutPackets(getSw(), kBlockDenyCounterName),
          blockDenyCounterBefore);
      EXPECT_EVENTUALLY_GT(
          getPortCounter(egressPort, kOutUnicastPktsCounterName),
          egressPacketsBefore);
    });
  }

  void verifyPortBoundAcl() {
    const auto& ports = masterLogicalPortIds();
    const auto restrictPort = ports[1];
    const auto blockPort = ports[2];
    const auto unboundPort = ports[3];

    verifyAclPacket(
        "Restrict permit",
        restrictPort,
        kRestrictPermitL4DstPort,
        kRestrictPermitCounterName,
        kRestrictDenyCounterName,
        true);
    verifyAclPacket(
        "Restrict deny",
        restrictPort,
        kDeniedL4DstPort,
        kRestrictPermitCounterName,
        kRestrictDenyCounterName,
        false);
    verifyAclPacket(
        "Block permit",
        blockPort,
        kBlockPermitL4DstPort,
        kBlockPermitCounterName,
        kBlockDenyCounterName,
        true);
    verifyAclPacket(
        "Block deny",
        blockPort,
        kRestrictPermitL4DstPort,
        kBlockPermitCounterName,
        kBlockDenyCounterName,
        false);
    verifyUnboundPacket(unboundPort, kDeniedL4DstPort);
  }

  static constexpr auto kAclTableGroupName = "port-ingress-acl-group";
  static constexpr auto kRestrictAclTableName =
      "port-ingress-restrict-acl-table";
  static constexpr auto kBlockAclTableName = "port-ingress-block-acl-table";
  static constexpr auto kRestrictPermitAclName = "restrict-permit-53";
  static constexpr auto kRestrictDenyAclName = "restrict-deny";
  static constexpr auto kBlockPermitAclName = "block-permit-dhcp-67";
  static constexpr auto kBlockDenyAclName = "block-deny";
  static constexpr auto kRestrictPermitCounterName = "restrict-permit-53";
  static constexpr auto kRestrictDenyCounterName = "restrict-deny";
  static constexpr auto kBlockPermitCounterName = "block-permit-dhcp-67";
  static constexpr auto kBlockDenyCounterName = "block-deny";
  static constexpr auto kOutUnicastPktsCounterName = "out_unicast_pkts";
  static constexpr auto kSrcIp = "2620:0:1cfe:face:b00c::1";
  static constexpr auto kDstIp = "2620:0:1cfe:face:b00c::10";
  static constexpr uint16_t kL4SrcPort = 8000;
  static constexpr uint16_t kRestrictPermitL4DstPort = 53;
  static constexpr uint16_t kBlockPermitL4DstPort = 67;
  static constexpr uint16_t kDeniedL4DstPort = 54;
};

// Two ports select different tables from the same port-bound ingress group.
// The restrict table permits UDP/53, the block table permits UDP/67, and both
// tables deny unmatched traffic. A third, unbound port remains unrestricted.
TEST_F(AgentPortBoundIngressAclTest, VerifyPortBoundAclTraffic) {
  auto setup = [this]() {
    setupL3Forwarding();
    configurePortBoundAcl(masterLogicalPortIds()[1], masterLogicalPortIds()[2]);
  };
  auto verify = [this]() { verifyPortBoundAcl(); };

  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
