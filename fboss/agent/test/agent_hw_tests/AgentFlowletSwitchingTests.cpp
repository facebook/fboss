/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/AsicUtils.h"
#include "fboss/agent/FibHelpers.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/utils/AclTestUtils.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/LoadBalancerTestUtils.h"
#include "fboss/agent/test/utils/MirrorTestUtils.h"
#include "fboss/agent/test/utils/ScaleTestUtils.h"
#include "fboss/agent/test/utils/UdfTestUtils.h"
#include "fboss/lib/CommonUtils.h"

DECLARE_bool(flowletSwitchingEnable);

namespace {
enum AclType {
  UDF_ACK, // match on bth_opcode
  UDF_NAK, // match on bth_opcode + aeth_syndrome
  UDF_ACK_WITH_NAK,
  UDF_WR_IMM_ZERO, // match on bth_opcode + reth_dma_length
  FLOWLET,
  FLOWLET_WITH_UDF_ACK,
  FLOWLET_WITH_UDF_NAK,
  UDF_FLOWLET, // match on bth_reserved
  UDF_FLOWLET_WITH_UDF_ACK,
  UDF_FLOWLET_WITH_UDF_NAK,
};
}

const std::string kSflowMirrorName = "sflow_mirror";
const std::string sflowDestinationVIP = "2001::101";
const std::string aclMirror = "acl_mirror";
const std::string aclDestinationVIP = "2002::101";

namespace facebook::fboss {

class AgentAclCounterTestBase : public AgentHwTest {
 public:
  cfg::AclActionType aclActionType_ = cfg::AclActionType::PERMIT;

  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    return {
        production_features::ProductionFeature::ACL_COUNTER,
        production_features::ProductionFeature::SINGLE_ACL_TABLE};
  }

 protected:
  void SetUp() override {
    AgentHwTest::SetUp();
    if (IsSkipped()) {
      return;
    }
    helper_ = std::make_unique<utility::EcmpSetupTargetedPorts6>(
        getProgrammedState(), getSw()->needL2EntryForNeighbor(), RouterID(0));
  }
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto cfg = utility::onePortPerInterfaceConfig(
        ensemble.getSw(),
        ensemble.masterLogicalPortIds(),
        true /*interfaceHasSubnet*/);
    return cfg;
  }

  std::string getAclName(AclType aclType) const {
    std::string aclName{};
    switch (aclType) {
      case AclType::UDF_ACK:
        aclName = "test-udf-acl";
        break;
      case AclType::UDF_NAK:
      case AclType::UDF_ACK_WITH_NAK:
        aclName = "test-udf-nak-acl";
        break;
      case AclType::UDF_WR_IMM_ZERO:
        aclName = "test-wr-imm-zero-acl";
        break;
      case AclType::FLOWLET:
      case AclType::FLOWLET_WITH_UDF_ACK:
      case AclType::FLOWLET_WITH_UDF_NAK:
        aclName = "test-flowlet-acl";
        break;
      case AclType::UDF_FLOWLET:
      case AclType::UDF_FLOWLET_WITH_UDF_ACK:
      case AclType::UDF_FLOWLET_WITH_UDF_NAK:
        aclName = utility::kFlowletAclName;
        break;
      default:
        break;
    }
    return aclName;
  }

  std::string getCounterName(AclType aclType) const {
    return getAclName(aclType) + "-stats";
  }

  void setEcmpMemberStatus(const TestEnsembleIf* ensemble) {
    // BCM native does not require this
    if (!ensemble->isSai()) {
      return;
    }
    auto asic = checkSameAndGetAsic(ensemble->getL3Asics());
    if (asic->getAsicVendor() != HwAsic::AsicVendor::ASIC_VENDOR_BCM) {
      return;
    }
    // Remove the ecmp ethertype config after BRCM fix
    constexpr auto kSetEcmpMemberStatus = R"(
  cint_reset();
  int ecmp_dlb_ethtypes[2];
  ecmp_dlb_ethtypes[0] = 0x0800;
  ecmp_dlb_ethtypes[1] = 0x86DD;
  bcm_l3_egress_ecmp_ethertype_set(0, 0, 2, ecmp_dlb_ethtypes);
  bcm_l3_egress_ecmp_member_status_set(0, 100003, BCM_L3_ECMP_DYNAMIC_MEMBER_FORCE_UP);
  bcm_l3_egress_ecmp_member_status_set(0, 100004, BCM_L3_ECMP_DYNAMIC_MEMBER_FORCE_UP);
  bcm_l3_egress_ecmp_member_status_set(0, 100005, BCM_L3_ECMP_DYNAMIC_MEMBER_FORCE_UP);
  bcm_l3_egress_ecmp_member_status_set(0, 100006, BCM_L3_ECMP_DYNAMIC_MEMBER_FORCE_UP);
  )";
    utility::runCintScript(
        const_cast<TestEnsembleIf*>(ensemble), kSetEcmpMemberStatus);
  }

  void setup(int ecmpWidth = 1) {
    std::vector<PortID> portIds = masterLogicalInterfacePortIds();
    flat_set<PortDescriptor> portDescs;
    std::vector<PortDescriptor> tempPortDescs;
    for (size_t w = 0; w < ecmpWidth; ++w) {
      tempPortDescs.emplace_back(portIds[w]);
    }
    portDescs.insert(
        std::make_move_iterator(tempPortDescs.begin()),
        std::make_move_iterator(tempPortDescs.end()));

    applyNewState([&portDescs, this](const std::shared_ptr<SwitchState>& in) {
      return helper_->resolveNextHops(in, portDescs);
    });
    auto wrapper = getSw()->getRouteUpdater();
    helper_->programRoutes(&wrapper, portDescs);

    XLOG(DBG3) << "setting ECMP Member Status: ";
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      auto out = in->clone();
      for (const auto& [_, switchSetting] :
           std::as_const(*out->getSwitchSettings())) {
        auto newSwitchSettings = switchSetting->modify(&out);
        newSwitchSettings->setForceEcmpDynamicMemberUp(true);
      }
      return out;
    });
  }

  RoutePrefixV6 getMirrorDestRoutePrefix(const folly::IPAddress dip) const {
    return RoutePrefixV6{
        folly::IPAddressV6{dip.str()}, static_cast<uint8_t>(dip.bitCount())};
  }

  void addSamplingConfig(cfg::SwitchConfig& config) {
    auto trafficPort = getAgentEnsemble()->masterLogicalPortIds(
        {cfg::PortType::INTERFACE_PORT})[utility::kTrafficPortIndex];
    std::vector<PortID> samplePorts = {trafficPort};
    utility::configureSflowSampling(config, kSflowMirrorName, samplePorts, 1);
  }

  void addAclTableConfig(
      cfg::SwitchConfig& config,
      std::vector<std::string>& udfGroups) const {
    if (FLAGS_enable_acl_table_group) {
      std::vector<cfg::AclTableQualifier> qualifiers = {};
      std::vector<cfg::AclTableActionType> actions = {};
      utility::addAclTableGroup(
          &config,
          cfg::AclStage::INGRESS,
          utility::kDefaultAclTableGroupName());
      utility::addDefaultAclTable(config, udfGroups);
    }
  }

  void resolveMirror(const std::string& mirrorName, uint8_t dstPort) {
    auto destinationPort = getAgentEnsemble()->masterLogicalPortIds(
        {cfg::PortType::INTERFACE_PORT})[dstPort];
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      boost::container::flat_set<PortDescriptor> nhopPorts{
          PortDescriptor(destinationPort)};
      return helper_->resolveNextHops(in, nhopPorts);
    });
    getSw()->getUpdateEvb()->runInFbossEventBaseThreadAndWait([] {});
    auto mirror = getSw()->getState()->getMirrors()->getNodeIf(mirrorName);
    auto dip = mirror->getDestinationIp();
    if (dip.has_value()) {
      auto prefix = getMirrorDestRoutePrefix(dip.value());
      boost::container::flat_set<PortDescriptor> nhopPorts{
          PortDescriptor(destinationPort)};
      auto wrapper = getSw()->getRouteUpdater();
      helper_->programRoutes(&wrapper, nhopPorts, {prefix});
    }
  }

  void generateApplyConfig(AclType aclType) {
    auto newCfg{initialConfig(*getAgentEnsemble())};
    std::vector<std::string> udfGroups = getUdfGroupsForAcl(aclType);
    addAclTableConfig(newCfg, udfGroups);
    addAclAndStat(&newCfg, aclType, getAgentEnsemble()->isSai());
    applyNewConfig(newCfg);
  }

  void flowletSwitchingAclHitHelper(AclType aclTypePre, AclType aclTypePost) {
    auto setup = [this, aclTypePre]() {
      this->setup();
      generateApplyConfig(aclTypePre);
    };

    auto verify = [this, aclTypePre]() { verifyAcl(aclTypePre); };

    auto setupPostWarmboot = [this, aclTypePost]() {
      generateApplyConfig(aclTypePost);
    };

    auto verifyPostWarmboot = [this, aclTypePost]() { verifyAcl(aclTypePost); };

    verifyAcrossWarmBoots(setup, verify, setupPostWarmboot, verifyPostWarmboot);
  }

  void verifyUdfAddDelete(AclType aclTypePre, AclType aclTypePost) {
    auto setup = [this, aclTypePre]() {
      this->setup();
      generateApplyConfig(aclTypePre);
      verifyAcl(aclTypePre);
    };

    auto verify = [this, aclTypePost]() {
      generateApplyConfig(aclTypePost);
      verifyAcl(aclTypePost);
    };

    verifyAcrossWarmBoots(setup, verify);
  }

  // roce packet - udpport=4791 + opcode=* + reserved=*
  // roce ack packet - udpport=4791 + opcode=17 + reserved=*
  // roce write-immediate - udpport=4791 + opcode=11 + reserved=1
  size_t sendRoceTraffic(
      const PortID& frontPanelEgrPort,
      int roceOpcode = utility::kUdfRoceOpcodeAck,
      const std::optional<std::vector<uint8_t>>& nxtHdr =
          std::optional<std::vector<uint8_t>>(),
      int packetCount = 1,
      int destPort = utility::kUdfL4DstPort) {
    auto vlanId = getVlanIDForTx();
    auto intfMac =
        utility::getMacForFirstInterfaceWithPorts(getProgrammedState());
    return utility::pumpRoCETraffic(
        true,
        utility::getAllocatePktFn(getAgentEnsemble()),
        utility::getSendPktFunc(getAgentEnsemble()),
        intfMac,
        vlanId,
        frontPanelEgrPort,
        destPort,
        255,
        std::nullopt,
        packetCount,
        roceOpcode,
        utility::kRoceReserved,
        nxtHdr);
  }

  auto verifyAclType(bool bumpOnHit, AclType aclType) {
    auto egressPort = helper_->ecmpPortDescriptorAt(0).phyPortID();
    auto pktsBefore =
        *getNextUpdatedPortStats(egressPort).outUnicastPkts__ref();
    auto aclPktCountBefore =
        utility::getAclInOutPackets(getSw(), getCounterName(aclType));
    auto aclBytesCountBefore = utility::getAclInOutPackets(
        getSw(), getCounterName(aclType), true /* bytes */);
    size_t sizeOfPacketSent = 0;

    std::vector<uint8_t> aethHdr = {0x0a, 0x11, 0x22, 0x33};
    // RDMAeth header with DMA length 0
    std::vector<uint8_t> rethHdr(16);

    switch (aclType) {
      case AclType::UDF_ACK:
        sizeOfPacketSent =
            sendRoceTraffic(egressPort, utility::kUdfRoceOpcodeAck, aethHdr);
        break;
      case AclType::UDF_NAK:
        aethHdr[0] = 0x6a; // MSB bits 2 and 3 indicate NAK
        sizeOfPacketSent =
            sendRoceTraffic(egressPort, utility::kUdfRoceOpcodeAck, aethHdr);
        break;
      case AclType::FLOWLET:
      case AclType::UDF_FLOWLET:
        rethHdr[15] = 0xFF; // non-zero sized packet
        sizeOfPacketSent = sendRoceTraffic(
            egressPort, utility::kUdfRoceOpcodeWriteImmediate, rethHdr);
        break;
      case AclType::UDF_WR_IMM_ZERO:
        sizeOfPacketSent = sendRoceTraffic(
            egressPort, utility::kUdfRoceOpcodeWriteImmediate, rethHdr);
        break;
      default:
        break;
    }

    WITH_RETRIES({
      auto aclPktCountAfter =
          utility::getAclInOutPackets(getSw(), getCounterName(aclType));

      auto aclBytesCountAfter = utility::getAclInOutPackets(
          getSw(), getCounterName(aclType), true /* bytes */);

      auto pktsAfter =
          *getNextUpdatedPortStats(egressPort).outUnicastPkts__ref();
      XLOG(DBG2) << "\n"
                 << "PacketCounter: " << pktsBefore << " -> " << pktsAfter
                 << "\n"
                 << "aclPacketCounter(" << getCounterName(aclType)
                 << "): " << aclPktCountBefore << " -> " << (aclPktCountAfter)
                 << "\n"
                 << "aclBytesCounter(" << getCounterName(aclType)
                 << "): " << aclBytesCountBefore << " -> "
                 << aclBytesCountAfter;

      if (bumpOnHit) {
        EXPECT_EVENTUALLY_GT(pktsAfter, pktsBefore);
        // On some ASICs looped back pkt hits the ACL before being
        // dropped in the ingress pipeline, hence GE
        EXPECT_EVENTUALLY_GE(aclPktCountAfter, aclPktCountBefore + 1);
        // At most we should get a pkt bump of 2
        EXPECT_EVENTUALLY_LE(aclPktCountAfter, aclPktCountBefore + 2);
        EXPECT_EVENTUALLY_GE(
            aclBytesCountAfter, aclBytesCountBefore + sizeOfPacketSent);
        // On native BCM we see 4 extra bytes in the acl counter. This is
        // likely due to ingress vlan getting imposed and getting counted
        // when packet hits acl in ingress pipeline
        if (!getAgentEnsemble()->isSai()) {
          EXPECT_EVENTUALLY_LE(
              aclBytesCountAfter,
              aclBytesCountBefore + (2 * sizeOfPacketSent) + 4);
        }
      } else {
        EXPECT_EVENTUALLY_EQ(aclPktCountBefore, aclPktCountAfter);
        EXPECT_EVENTUALLY_EQ(aclBytesCountBefore, aclBytesCountAfter);
      }
    });
  }

  // order of verification is sometimes important due to acl priority
  void verifyAcl(AclType aclType) {
    switch (aclType) {
      case AclType::UDF_ACK:
        verifyAclType(true, AclType::UDF_ACK);
        break;
      case AclType::UDF_NAK:
        verifyAclType(true, AclType::UDF_NAK);
        break;
      case AclType::UDF_ACK_WITH_NAK:
        verifyAclType(true, AclType::UDF_ACK);
        verifyAclType(true, AclType::UDF_NAK);
        break;
      case AclType::UDF_WR_IMM_ZERO:
        verifyAclType(true, AclType::UDF_WR_IMM_ZERO);
        break;
      case AclType::FLOWLET:
        verifyAclType(true, AclType::FLOWLET);
        break;
      case AclType::FLOWLET_WITH_UDF_ACK:
        verifyAclType(true, AclType::FLOWLET);
        verifyAclType(true, AclType::UDF_ACK);
        break;
      case AclType::FLOWLET_WITH_UDF_NAK:
        verifyAclType(true, AclType::FLOWLET);
        verifyAclType(true, AclType::UDF_NAK);
        break;
      case AclType::UDF_FLOWLET:
        verifyAclType(true, AclType::UDF_FLOWLET);
        break;
      case AclType::UDF_FLOWLET_WITH_UDF_ACK:
        verifyAclType(true, AclType::UDF_FLOWLET);
        verifyAclType(true, AclType::UDF_ACK);
        break;
      case AclType::UDF_FLOWLET_WITH_UDF_NAK:
        verifyAclType(true, AclType::UDF_FLOWLET);
        verifyAclType(true, AclType::UDF_NAK);
        break;
      default:
        break;
    }
  }

  std::vector<cfg::AclUdfEntry> addUdfTable(
      const std::vector<std::string>& udfGroups,
      const std::vector<std::vector<int8_t>>& roceBytes,
      const std::vector<std::vector<int8_t>>& roceMask) const {
    std::vector<cfg::AclUdfEntry> udfTable;
    for (int i = 0; i < udfGroups.size(); i++) {
      cfg::AclUdfEntry aclUdfEntry;
      aclUdfEntry.udfGroup() = udfGroups[i];
      aclUdfEntry.roceBytes() = roceBytes[i];
      aclUdfEntry.roceMask() = roceMask[i];
      udfTable.push_back(aclUdfEntry);
    }
    return udfTable;
  }

  void addRoceAcl(
      cfg::SwitchConfig* config,
      const std::string& aclName,
      const std::string& counterName,
      bool isSai,
      const std::optional<std::string>& udfGroups,
      const std::optional<int>& roceOpcode,
      const std::optional<int>& roceBytes,
      const std::optional<int>& roceMask,
      const std::optional<std::vector<cfg::AclUdfEntry>>& udfTable) const {
    cfg::AclEntry aclEntry;
    aclEntry.name() = aclName;
    aclEntry.actionType() = aclActionType_;
    auto acl = utility::addAcl(config, aclEntry, cfg::AclStage::INGRESS);
    std::vector<cfg::CounterType> setCounterTypes{
        cfg::CounterType::PACKETS, cfg::CounterType::BYTES};
    if (udfTable.has_value()) {
      acl->udfTable() = udfTable.value();
    }
    if (isSai) {
      std::vector<std::string> groups;
      std::vector<std::vector<int8_t>> data;
      std::vector<std::vector<int8_t>> mask;
      if (udfGroups.has_value()) {
        groups = {udfGroups.value()};
      }
      if (roceOpcode.has_value()) {
        data = {{static_cast<signed char>(utility::kUdfRoceOpcodeAck)}};
        mask = {{static_cast<signed char>(utility::kUdfRoceOpcodeMask)}};
      }
      if (roceBytes.has_value()) {
        data = {{static_cast<signed char>(roceBytes.value())}};
      }
      if (roceMask.has_value()) {
        mask = {{static_cast<signed char>(roceMask.value())}};
      }
      if (!udfTable.has_value()) {
        acl->udfTable() = addUdfTable(groups, data, mask);
      }
    } else {
      if (udfGroups.has_value()) {
        acl->udfGroups() = {udfGroups.value()};
      }
      if (roceOpcode.has_value()) {
        acl->roceOpcode() = roceOpcode.value();
      }
      if (roceBytes.has_value()) {
        acl->roceBytes() = {static_cast<signed char>(roceBytes.value())};
      }
      if (roceMask.has_value()) {
        acl->roceMask() = {static_cast<signed char>(roceMask.value())};
      }
    }
    if (aclName == getAclName(AclType::UDF_NAK)) {
      cfg::Ttl ttl;
      ttl.value() = 255;
      ttl.mask() = 0xFF;
      acl->ttl() = ttl;
    }
    if (aclName == getAclName(AclType::UDF_FLOWLET)) {
      acl->proto() = 17;
      acl->l4DstPort() = 4791;
      acl->dstIp() = "2001::/16";
      auto asic = checkSameAndGetAsic(this->getAgentEnsemble()->getL3Asics());
      utility::addEtherTypeToAcl(asic, acl, cfg::EtherType::IPv6);
    }
    if (aclName == getAclName(AclType::UDF_ACK)) {
      // set dscp value to 30 and send to queue 6
      utility::addAclDscpQueueAction(
          config, aclName, counterName, kDscp, kOutQueue);
    } else {
      utility::addAclStat(
          config, aclName, counterName, std::move(setCounterTypes));
    }
  }

  std::vector<std::string> getUdfGroupsForAcl(AclType aclType) const {
    std::vector<std::string> retUdfGroups = {};
    switch (aclType) {
      case AclType::UDF_ACK:
        retUdfGroups.push_back(utility::kUdfAclRoceOpcodeGroupName);
        break;
      case AclType::UDF_NAK: {
        retUdfGroups.push_back(utility::kUdfAclRoceOpcodeGroupName);
        retUdfGroups.push_back(utility::kUdfAclAethNakGroupName);
      } break;
      case AclType::UDF_WR_IMM_ZERO: {
        retUdfGroups.push_back(utility::kUdfAclRoceOpcodeGroupName);
        retUdfGroups.push_back(utility::kUdfAclRethWrImmZeroGroupName);
      } break;
      case AclType::UDF_ACK_WITH_NAK: {
        retUdfGroups.push_back(utility::kUdfAclRoceOpcodeGroupName);
        retUdfGroups.push_back(utility::kUdfAclAethNakGroupName);
      } break;
      case AclType::FLOWLET:
        break;
      case AclType::FLOWLET_WITH_UDF_ACK:
        retUdfGroups.push_back(utility::kUdfAclRoceOpcodeGroupName);
        break;
      case AclType::UDF_FLOWLET:
        retUdfGroups.push_back(utility::kRoceUdfFlowletGroupName);
        break;
      case AclType::UDF_FLOWLET_WITH_UDF_ACK:
        retUdfGroups.push_back(utility::kUdfAclRoceOpcodeGroupName);
        retUdfGroups.push_back(utility::kRoceUdfFlowletGroupName);
        break;
      case AclType::UDF_FLOWLET_WITH_UDF_NAK: {
        retUdfGroups.push_back(utility::kUdfAclRoceOpcodeGroupName);
        retUdfGroups.push_back(utility::kUdfAclAethNakGroupName);
        retUdfGroups.push_back(utility::kRoceUdfFlowletGroupName);
      } break;
      default:
        break;
    }
    return retUdfGroups;
  }

  void addAclAndStat(cfg::SwitchConfig* config, AclType aclType, bool isSai)
      const {
    auto aclName = getAclName(aclType);
    auto counterName = getCounterName(aclType);
    const signed char bm = 0xFF;
    std::vector<signed char> dmaLengthZeros = {0x0, 0x0};
    std::vector<signed char> dmaLengthMask = {bm, bm};
    switch (aclType) {
      case AclType::UDF_ACK:
        config->udfConfig() =
            utility::addUdfAclConfig(utility::kUdfOffsetBthOpcode);
        addRoceAcl(
            config,
            aclName,
            counterName,
            isSai,
            utility::kUdfAclRoceOpcodeGroupName,
            utility::kUdfRoceOpcodeAck,
            std::nullopt,
            std::nullopt,
            std::nullopt);
        break;
      case AclType::UDF_NAK: {
        config->udfConfig() = utility::addUdfAclConfig(
            utility::kUdfOffsetBthOpcode | utility::kUdfOffsetAethSyndrome);
        auto udfTable = addUdfTable(
            {utility::kUdfAclRoceOpcodeGroupName,
             utility::kUdfAclAethNakGroupName},
            {{utility::kUdfRoceOpcodeAck}, {utility::kAethSyndromeWithNak}},
            {{utility::kUdfRoceOpcodeMask}, {utility::kAethSyndromeWithNak}});
        addRoceAcl(
            config,
            aclName,
            counterName,
            isSai,
            std::nullopt,
            std::nullopt,
            std::nullopt,
            std::nullopt,
            std::move(udfTable));
      } break;
      case AclType::UDF_WR_IMM_ZERO: {
        config->udfConfig() = utility::addUdfAclConfig(
            utility::kUdfOffsetBthOpcode | utility::kUdfOffsetRethDmaLength);
        auto udfTable = addUdfTable(
            {utility::kUdfAclRoceOpcodeGroupName,
             utility::kUdfAclRethWrImmZeroGroupName},
            {{utility::kUdfRoceOpcodeWriteImmediate},
             std::move(dmaLengthZeros)},
            {{utility::kUdfRoceOpcodeMask}, std::move(dmaLengthMask)});
        addRoceAcl(
            config,
            aclName,
            counterName,
            isSai,
            std::nullopt,
            std::nullopt,
            std::nullopt,
            std::nullopt,
            std::move(udfTable));
      } break;
      case AclType::UDF_ACK_WITH_NAK: {
        config->udfConfig() = utility::addUdfAclConfig(
            utility::kUdfOffsetBthOpcode | utility::kUdfOffsetAethSyndrome);
        auto udfTable = addUdfTable(
            {utility::kUdfAclRoceOpcodeGroupName,
             utility::kUdfAclAethNakGroupName},
            {{utility::kUdfRoceOpcodeAck}, {utility::kAethSyndromeWithNak}},
            {{utility::kUdfRoceOpcodeMask}, {utility::kAethSyndromeWithNak}});
        addRoceAcl(
            config,
            aclName,
            counterName,
            isSai,
            std::nullopt,
            std::nullopt,
            std::nullopt,
            std::nullopt,
            std::move(udfTable));
        addRoceAcl(
            config,
            getAclName(AclType::UDF_ACK),
            getCounterName(AclType::UDF_ACK),
            isSai,
            utility::kUdfAclRoceOpcodeGroupName,
            utility::kUdfRoceOpcodeAck,
            std::nullopt,
            std::nullopt,
            std::nullopt);
      } break;
      case AclType::FLOWLET:
        utility::addFlowletAcl(*config, isSai, aclName, counterName, false);
        break;
      case AclType::FLOWLET_WITH_UDF_ACK:
        config->udfConfig() =
            utility::addUdfAclConfig(utility::kUdfOffsetBthOpcode);
        addRoceAcl(
            config,
            getAclName(AclType::UDF_ACK),
            getCounterName(AclType::UDF_ACK),
            isSai,
            utility::kUdfAclRoceOpcodeGroupName,
            utility::kUdfRoceOpcodeAck,
            std::nullopt,
            std::nullopt,
            std::nullopt);
        utility::addFlowletAcl(*config, isSai, aclName, counterName, false);
        break;
      case AclType::UDF_FLOWLET:
        config->udfConfig() =
            utility::addUdfAclConfig(utility::kUdfOffsetBthReserved);
        addRoceAcl(
            config,
            getAclName(AclType::UDF_FLOWLET),
            getCounterName(AclType::UDF_FLOWLET),
            isSai,
            utility::kRoceUdfFlowletGroupName,
            std::nullopt,
            utility::kRoceReserved,
            utility::kRoceReserved,
            std::nullopt);
        break;
      case AclType::UDF_FLOWLET_WITH_UDF_ACK:
        config->udfConfig() = utility::addUdfAclConfig(
            utility::kUdfOffsetBthOpcode | utility::kUdfOffsetBthReserved);
        addRoceAcl(
            config,
            getAclName(AclType::UDF_ACK),
            getCounterName(AclType::UDF_ACK),
            isSai,
            utility::kUdfAclRoceOpcodeGroupName,
            utility::kUdfRoceOpcodeAck,
            std::nullopt,
            std::nullopt,
            std::nullopt);
        addRoceAcl(
            config,
            getAclName(AclType::UDF_FLOWLET),
            getCounterName(AclType::UDF_FLOWLET),
            isSai,
            utility::kRoceUdfFlowletGroupName,
            std::nullopt,
            utility::kRoceReserved,
            utility::kRoceReserved,
            std::nullopt);
        break;
      case AclType::UDF_FLOWLET_WITH_UDF_NAK: {
        config->udfConfig() = utility::addUdfAclConfig(
            utility::kUdfOffsetBthOpcode | utility::kUdfOffsetBthReserved |
            utility::kUdfOffsetAethSyndrome);
        auto udfTable = addUdfTable(
            {utility::kUdfAclRoceOpcodeGroupName,
             utility::kUdfAclAethNakGroupName},
            {{utility::kUdfRoceOpcodeAck}, {utility::kAethSyndromeWithNak}},
            {{utility::kUdfRoceOpcodeMask}, {utility::kAethSyndromeWithNak}});
        addRoceAcl(
            config,
            getAclName(AclType::UDF_NAK),
            getCounterName(AclType::UDF_NAK),
            isSai,
            std::nullopt,
            std::nullopt,
            std::nullopt,
            std::nullopt,
            std::move(udfTable));
        addRoceAcl(
            config,
            getAclName(AclType::UDF_FLOWLET),
            getCounterName(AclType::UDF_FLOWLET),
            isSai,
            utility::kRoceUdfFlowletGroupName,
            std::nullopt,
            utility::kRoceReserved,
            utility::kRoceReserved,
            std::nullopt);
      } break;
      default:
        break;
    }
  }

  void generatePrefixes() {
    std::vector<PortID> portIds = masterLogicalInterfacePortIds();
    std::vector<PortDescriptor> portDescriptorIds;
    std::transform(
        portIds.begin(),
        portIds.end(),
        std::back_inserter(portDescriptorIds),
        [](const PortID& portId) { return PortDescriptor(portId); });

    std::vector<std::vector<PortDescriptor>> allCombinations =
        utility::generateEcmpGroupScale(
            portDescriptorIds, 512, portDescriptorIds.size());
    for (const auto& combination : allCombinations) {
      nhopSets.emplace_back(combination.begin(), combination.end());
    }
    applyNewState(
        [&portDescriptorIds, this](const std::shared_ptr<SwitchState>& in) {
          return helper_->resolveNextHops(
              in,
              flat_set<PortDescriptor>(
                  std::make_move_iterator(portDescriptorIds.begin()),
                  std::make_move_iterator(portDescriptorIds.end())));
        });

    std::generate_n(std::back_inserter(prefixes), 512, [i = 0]() mutable {
      return RoutePrefixV6{
          folly::IPAddressV6(folly::to<std::string>(2401, "::", i++)), 128};
    });
  }

  static inline constexpr auto kEcmpWidth = 4;
  static inline constexpr auto kOutQueue = 6;
  static inline constexpr auto kDscp = 30;
  std::unique_ptr<utility::EcmpSetupTargetedPorts6> helper_;
  std::vector<flat_set<PortDescriptor>> nhopSets;
  std::vector<RoutePrefixV6> prefixes;
};

class AgentFlowletSwitchingTest : public AgentAclCounterTestBase {
 public:
  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    return {
        production_features::ProductionFeature::DLB,
        production_features::ProductionFeature::SINGLE_ACL_TABLE};
  }

 protected:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto cfg = utility::onePortPerInterfaceConfig(
        ensemble.getSw(),
        ensemble.masterLogicalPortIds(),
        true /*interfaceHasSubnet*/);
    utility::addFlowletConfigs(
        cfg,
        ensemble.masterLogicalPortIds(),
        ensemble.isSai(),
        cfg::SwitchingMode::PER_PACKET_QUALITY);
    return cfg;
  }

  void setCmdLineFlagOverrides() const override {
    AgentHwTest::setCmdLineFlagOverrides();
    FLAGS_flowletSwitchingEnable = true;
    FLAGS_force_init_fp = false;
  }
};

class AgentFlowletAclPriorityTest : public AgentFlowletSwitchingTest {
 public:
  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    return {
        production_features::ProductionFeature::DLB,
        production_features::ProductionFeature::UDF_WR_IMMEDIATE_ACL,
        production_features::ProductionFeature::SINGLE_ACL_TABLE};
  }
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto cfg = AgentFlowletSwitchingTest::initialConfig(ensemble);
    std::vector<std::string> udfGroups = {
        utility::kUdfAclRoceOpcodeGroupName,
        utility::kRoceUdfFlowletGroupName,
        utility::kUdfAclAethNakGroupName,
        utility::kUdfAclRethWrImmZeroGroupName,
    };
    addAclTableConfig(cfg, udfGroups);
    cfg.udfConfig() = utility::addUdfAclConfig(
        utility::kUdfOffsetBthOpcode | utility::kUdfOffsetBthReserved |
        utility::kUdfOffsetAethSyndrome | utility::kUdfOffsetRethDmaLength);
    return cfg;
  }
};

class AgentFlowletMirrorTest : public AgentFlowletSwitchingTest {
 public:
  // TH* supports upto 4 different source types to mirror to same egress port.
  // Here IFP mirror action and ingress port sflow actions can generate 2 copies
  // going to same VIP or different VIP (different egress port in the test)
  enum MirrorScope {
    MIRROR_ONLY,
    MIRROR_SFLOW_SAME_VIP,
    MIRROR_SFLOW_DIFFERENT_VIP,
  };
  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    return {
        production_features::ProductionFeature::DLB,
        production_features::ProductionFeature::SFLOWv6_SAMPLING,
        production_features::ProductionFeature::INGRESS_MIRRORING,
        production_features::ProductionFeature::SINGLE_ACL_TABLE};
  }

 protected:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto cfg = AgentFlowletSwitchingTest::initialConfig(ensemble);
    std::vector<std::string> udfGroups = getUdfGroupsForAcl(AclType::UDF_NAK);
    addAclTableConfig(cfg, udfGroups);
    addAclAndStat(&cfg, AclType::UDF_NAK, ensemble.isSai());
    // overwrite existing traffic policy which only has a counter action
    // It is added in addAclAndStat above
    cfg.dataPlaneTrafficPolicy() = cfg::TrafficPolicyConfig();
    std::string counterName = getCounterName(AclType::UDF_NAK);
    utility::addAclMirrorAction(
        &cfg, getAclName(AclType::UDF_NAK), counterName, aclMirror);

    // mirror session for acl
    utility::configureSflowMirror(
        cfg, aclMirror, false /* truncate */, aclDestinationVIP, 6344);

    return cfg;
  }

  void verifyMirror(MirrorScope scope) {
    // In addition to counting ACL hit with verifyAcl, verify packet mirrored
    auto mirrorPort = helper_->ecmpPortDescriptorAt(1).phyPortID();
    auto sflowPort = helper_->ecmpPortDescriptorAt(2).phyPortID();
    auto pktsMirrorBefore =
        *getNextUpdatedPortStats(mirrorPort).outUnicastPkts__ref();
    auto pktsSflowBefore =
        *getNextUpdatedPortStats(sflowPort).outUnicastPkts__ref();

    verifyAcl(AclType::UDF_NAK);

    WITH_RETRIES({
      auto pktsMirrorAfter =
          *getNextUpdatedPortStats(mirrorPort).outUnicastPkts__ref();
      auto pktsSflowAfter =
          *getNextUpdatedPortStats(sflowPort).outUnicastPkts__ref();
      XLOG(DBG2) << "PacketMirrorCounter: " << pktsMirrorBefore << " -> "
                 << pktsMirrorAfter
                 << " PacketSflowCounter: " << pktsSflowBefore << " -> "
                 << pktsSflowAfter;
      if (scope == MirrorScope::MIRROR_ONLY) {
        EXPECT_EVENTUALLY_GT(pktsMirrorAfter, pktsMirrorBefore);
      } else if (scope == MirrorScope::MIRROR_SFLOW_SAME_VIP) {
        EXPECT_EVENTUALLY_GE(pktsMirrorAfter, pktsMirrorBefore + 2);
      } else if (scope == MirrorScope::MIRROR_SFLOW_DIFFERENT_VIP) {
        EXPECT_EVENTUALLY_GT(pktsMirrorAfter, pktsMirrorBefore);
        EXPECT_EVENTUALLY_GT(pktsSflowAfter, pktsSflowBefore);
      }
    });
  }
};

// empty to UDF A
TEST_F(AgentFlowletSwitchingTest, VerifyFlowletToUdfFlowlet) {
  flowletSwitchingAclHitHelper(AclType::FLOWLET, AclType::UDF_FLOWLET);
}

// empty to UDF A + B
TEST_F(AgentFlowletSwitchingTest, VerifyFlowletToUdfFlowletWithUdfAck) {
  flowletSwitchingAclHitHelper(
      AclType::FLOWLET, AclType::UDF_FLOWLET_WITH_UDF_ACK);
}

// UDF A to UDF B
TEST_F(AgentFlowletSwitchingTest, VerifyUdfAckToUdfFlowlet) {
  flowletSwitchingAclHitHelper(AclType::UDF_ACK, AclType::UDF_FLOWLET);
}

// UDF A to UDF A + B
TEST_F(AgentFlowletSwitchingTest, VerifyUdfFlowletToUdfFlowletWithUdfAck) {
  flowletSwitchingAclHitHelper(
      AclType::UDF_FLOWLET, AclType::UDF_FLOWLET_WITH_UDF_ACK);
}

// UDF A to UDF A + B + C
TEST_F(AgentFlowletSwitchingTest, VerifyUdfFlowletToUdfFlowletWithUdfNak) {
  flowletSwitchingAclHitHelper(
      AclType::UDF_FLOWLET, AclType::UDF_FLOWLET_WITH_UDF_NAK);
}

// UDF A + B + C to UDF B + C
TEST_F(AgentFlowletSwitchingTest, VerifyUdfFlowletWithUdfNakToUdfNak) {
  flowletSwitchingAclHitHelper(
      AclType::UDF_FLOWLET_WITH_UDF_NAK, AclType::UDF_NAK);
}

// UDF A + B to UDF B
TEST_F(AgentFlowletSwitchingTest, VerifyUdfFlowletWithUdfAckToUdfAck) {
  flowletSwitchingAclHitHelper(
      AclType::UDF_FLOWLET_WITH_UDF_ACK, AclType::UDF_ACK);
}

// UDF A to UDF B
TEST_F(AgentFlowletSwitchingTest, VerifyUdfFlowletToUdfAck) {
  flowletSwitchingAclHitHelper(AclType::UDF_FLOWLET, AclType::UDF_ACK);
}

// UDF A + B to empty
TEST_F(AgentFlowletSwitchingTest, VerifyUdfFlowletWithUdfAckToFlowlet) {
  flowletSwitchingAclHitHelper(
      AclType::UDF_FLOWLET_WITH_UDF_ACK, AclType::FLOWLET);
}

TEST_F(AgentFlowletSwitchingTest, VerifyEcmp) {
  auto setup = [this]() {
    this->setup(kEcmpWidth);
    generateApplyConfig(AclType::FLOWLET);
  };

  auto verify = [this]() {
    setEcmpMemberStatus(getAgentEnsemble());

    auto verifyCounts = [this](int destPort, bool bumpOnHit) {
      // gather stats for all ECMP members
      int pktsBefore[kEcmpWidth];
      int pktsBeforeTotal = 0;
      for (int i = 0; i < kEcmpWidth; i++) {
        auto ecmpEgressPort = helper_->ecmpPortDescriptorAt(i).phyPortID();
        pktsBefore[i] =
            *getNextUpdatedPortStats(ecmpEgressPort).outUnicastPkts__ref();
        pktsBeforeTotal += pktsBefore[i];
      }
      auto aclPktCountBefore = utility::getAclInOutPackets(
          getSw(), getCounterName(AclType::FLOWLET));
      int packetCount = 1000;

      std::vector<uint8_t> rethHdr(16);
      rethHdr[15] = 0xFF; // non-zero sized packet
      auto egressPort = helper_->ecmpPortDescriptorAt(4).phyPortID();
      sendRoceTraffic(
          egressPort,
          utility::kUdfRoceOpcodeWriteImmediate,
          rethHdr,
          packetCount,
          destPort);

      WITH_RETRIES({
        auto aclPktCountAfter = utility::getAclInOutPackets(
            getSw(), getCounterName(AclType::FLOWLET));

        int pktsAfter[kEcmpWidth];
        int pktsAfterTotal = 0;
        for (int i = 0; i < kEcmpWidth; i++) {
          auto ecmpEgressPort = helper_->ecmpPortDescriptorAt(i).phyPortID();
          pktsAfter[i] =
              *getNextUpdatedPortStats(ecmpEgressPort).outUnicastPkts__ref();
          pktsAfterTotal += pktsAfter[i];
          XLOG(DBG2) << "Ecmp egress Port: " << ecmpEgressPort
                     << ", Count: " << pktsBefore[i] << " -> " << pktsAfter[i];
        }

        XLOG(DBG2) << "\n"
                   << "aclPacketCounter(" << getCounterName(AclType::FLOWLET)
                   << "): " << aclPktCountBefore << " -> " << (aclPktCountAfter)
                   << "\n";

        // Irrespective of which LB mechanism is used, all packets should egress
        EXPECT_EVENTUALLY_GE(pktsAfterTotal, pktsBeforeTotal + packetCount);

        // Verify ACL count also for hit and miss
        if (bumpOnHit) {
          EXPECT_EVENTUALLY_GE(
              aclPktCountAfter, aclPktCountBefore + packetCount);
        } else {
          EXPECT_EVENTUALLY_EQ(aclPktCountAfter, aclPktCountBefore);
          // also verify traffic is not load-balanced, implying,
          // 3 out of the 4 egress ports should have 0 count
          int zeroCount = 0;
          for (int i = 0; i < kEcmpWidth; i++) {
            if (pktsAfter[i] - pktsBefore[i] == 0) {
              zeroCount++;
            }
          }
          EXPECT_EVENTUALLY_EQ(kEcmpWidth - 1, zeroCount);
        }
      });
    };

    // Verify DLB is hit with ACL matching packet
    verifyCounts(4791, true);
    // Verify packet is still ECMP'd without DLB using static hash
    verifyCounts(1024, false);
  };

  verifyAcrossWarmBoots(setup, verify);
}

// UDF A to empty
TEST_F(AgentFlowletSwitchingTest, VerifyUdfFlowletToFlowlet) {
  flowletSwitchingAclHitHelper(AclType::UDF_FLOWLET, AclType::FLOWLET);
}

TEST_F(AgentFlowletSwitchingTest, VerifyUdfFlowletToUdfWrImmZero) {
  flowletSwitchingAclHitHelper(AclType::UDF_FLOWLET, AclType::UDF_WR_IMM_ZERO);
}

TEST_F(AgentFlowletSwitchingTest, VerifyUdfWrImmZeroToUdfFlowlet) {
  flowletSwitchingAclHitHelper(AclType::UDF_WR_IMM_ZERO, AclType::UDF_FLOWLET);
}

TEST_F(AgentFlowletSwitchingTest, VerifyOneUdfGroupAddition) {
  verifyUdfAddDelete(AclType::UDF_FLOWLET, AclType::UDF_FLOWLET_WITH_UDF_ACK);
}

TEST_F(AgentFlowletSwitchingTest, VerifyOneUdfGroupDeletion) {
  verifyUdfAddDelete(AclType::UDF_FLOWLET_WITH_UDF_ACK, AclType::UDF_FLOWLET);
}

TEST_F(AgentFlowletSwitchingTest, VerifyUdfNakToUdfAckWithNak) {
  flowletSwitchingAclHitHelper(AclType::UDF_NAK, AclType::UDF_ACK_WITH_NAK);
}

TEST_F(AgentFlowletSwitchingTest, VerifyUdfAckWithNakToUdfNak) {
  flowletSwitchingAclHitHelper(AclType::UDF_ACK_WITH_NAK, AclType::UDF_NAK);
}

TEST_F(AgentFlowletSwitchingTest, VerifyUdfAndSendQueueAction) {
  auto setup = [this]() {
    this->setup();
    generateApplyConfig(AclType::UDF_ACK);
  };

  auto verify = [this]() {
    auto outPort = helper_->ecmpPortDescriptorAt(0).phyPortID();
    auto portStatsBefore = getNextUpdatedPortStats(outPort);
    auto pktsBefore = *portStatsBefore.outUnicastPkts__ref();
    auto pktsQueueBefore = portStatsBefore.queueOutPackets_()[kOutQueue];

    verifyAcl(AclType::UDF_ACK);

    WITH_RETRIES({
      auto portStatsAfter = getNextUpdatedPortStats(outPort);
      auto pktsAfter = *portStatsAfter.outUnicastPkts__ref();
      auto pktsQueueAfter = portStatsAfter.queueOutPackets_()[kOutQueue];
      XLOG(DBG2) << "Port Counter: " << pktsBefore << " -> " << pktsAfter
                 << "\nPort Queue " << kOutQueue
                 << " Counter: " << pktsQueueBefore << " -> " << pktsQueueAfter;
      EXPECT_EVENTUALLY_GT(pktsAfter, pktsBefore);
      EXPECT_EVENTUALLY_GT(pktsQueueAfter, pktsQueueBefore);
    });
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentFlowletMirrorTest, VerifyUdfNakMirrorAction) {
  auto setup = [this]() {
    this->setup();
    resolveMirror(aclMirror, utility::kMirrorToPortIndex);
  };

  auto verify = [this]() { verifyMirror(MirrorScope::MIRROR_ONLY); };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentFlowletMirrorTest, VerifyUdfNakMirrorSflowSameVip) {
  auto setup = [this]() {
    this->setup();
    auto newCfg{initialConfig(*getAgentEnsemble())};

    // mirror session for ingress port sflow
    // use same VIP as ACL mirror, only dst port varies
    utility::configureSflowMirror(
        newCfg, kSflowMirrorName, false /* truncate */, aclDestinationVIP);
    // configure sampling on traffic port
    addSamplingConfig(newCfg);

    applyNewConfig(newCfg);
    resolveMirror(aclMirror, utility::kMirrorToPortIndex);
  };

  auto verify = [this]() { verifyMirror(MirrorScope::MIRROR_SFLOW_SAME_VIP); };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentFlowletMirrorTest, VerifyUdfNakMirrorSflowDifferentVip) {
  auto setup = [this]() {
    this->setup();
    auto newCfg{initialConfig(*getAgentEnsemble())};

    // mirror session for ingress port sflow
    utility::configureSflowMirror(
        newCfg, kSflowMirrorName, false /* truncate */, sflowDestinationVIP);
    // configure sampling on traffic port
    addSamplingConfig(newCfg);

    applyNewConfig(newCfg);
    resolveMirror(aclMirror, utility::kMirrorToPortIndex);
    resolveMirror(kSflowMirrorName, utility::kSflowToPortIndex);
  };

  auto verify = [this]() {
    verifyMirror(MirrorScope::MIRROR_SFLOW_DIFFERENT_VIP);
  };

  verifyAcrossWarmBoots(setup, verify);
}

// Skip this and next test due to lack of TCAM in ACL table on TH3
TEST_F(AgentFlowletAclPriorityTest, VerifyUdfAclPriority) {
  auto setup = [this]() {
    this->setup();
    const auto& ensemble = *getAgentEnsemble();
    auto newCfg{initialConfig(ensemble)};
    // production priorities
    addAclAndStat(&newCfg, AclType::UDF_NAK, ensemble.isSai());
    addAclAndStat(&newCfg, AclType::UDF_ACK, ensemble.isSai());
    addAclAndStat(&newCfg, AclType::UDF_WR_IMM_ZERO, ensemble.isSai());
    addAclAndStat(&newCfg, AclType::UDF_FLOWLET, ensemble.isSai());
    // Keep this at the end since each of the above calls update udfConfig
    // differently
    newCfg.udfConfig() = utility::addUdfAclConfig(
        utility::kUdfOffsetBthOpcode | utility::kUdfOffsetBthReserved |
        utility::kUdfOffsetAethSyndrome | utility::kUdfOffsetRethDmaLength);
    applyNewConfig(newCfg);
  };

  auto verify = [this]() {
    verifyAcl(AclType::UDF_NAK);
    verifyAcl(AclType::UDF_ACK);
    verifyAcl(AclType::UDF_WR_IMM_ZERO);
    verifyAcl(AclType::UDF_FLOWLET);
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentFlowletAclPriorityTest, VerifyUdfAclPriorityWB) {
  auto setup = [this]() {
    this->setup();
    auto newCfg{initialConfig(*getAgentEnsemble())};
    applyNewConfig(newCfg);
  };

  auto setupPostWarmboot = [this]() {
    this->setup();
    const auto& ensemble = *getAgentEnsemble();
    auto newCfg{initialConfig(ensemble)};
    // production priorities
    addAclAndStat(&newCfg, AclType::UDF_NAK, ensemble.isSai());
    addAclAndStat(&newCfg, AclType::UDF_ACK, ensemble.isSai());
    addAclAndStat(&newCfg, AclType::UDF_WR_IMM_ZERO, ensemble.isSai());
    addAclAndStat(&newCfg, AclType::UDF_FLOWLET, ensemble.isSai());
    // Keep this at the end since each of the above calls update udfConfig
    // differently
    newCfg.udfConfig() = utility::addUdfAclConfig(
        utility::kUdfOffsetBthOpcode | utility::kUdfOffsetBthReserved |
        utility::kUdfOffsetAethSyndrome | utility::kUdfOffsetRethDmaLength);
    applyNewConfig(newCfg);
  };

  auto verifyPostWarmboot = [this]() {
    verifyAcl(AclType::UDF_NAK);
    verifyAcl(AclType::UDF_ACK);
    verifyAcl(AclType::UDF_WR_IMM_ZERO);
    verifyAcl(AclType::UDF_FLOWLET);
  };

  verifyAcrossWarmBoots(setup, []() {}, setupPostWarmboot, verifyPostWarmboot);
}

TEST_F(AgentFlowletSwitchingTest, CreateMaxDlbGroups) {
  auto verify = [this] {
    generatePrefixes();
    const auto kMaxDlbEcmpGroup =
        utility::getMaxDlbEcmpGroups(getAgentEnsemble()->getL3Asics());
    // install 60% of max DLB ecmp groups
    {
      int count = static_cast<int>(0.6 * kMaxDlbEcmpGroup);
      auto wrapper = getSw()->getRouteUpdater();
      std::vector<RoutePrefixV6> prefixes60 = {
          prefixes.begin(), prefixes.begin() + count};
      std::vector<flat_set<PortDescriptor>> nhopSets60 = {
          nhopSets.begin(), nhopSets.begin() + count};
      helper_->programRoutes(&wrapper, nhopSets60, prefixes60);
    }
    // install 128 groups, failed update
    {
      auto wrapper = getSw()->getRouteUpdater();
      std::vector<RoutePrefixV6> prefixes128 = {
          prefixes.begin(), prefixes.begin() + kMaxDlbEcmpGroup};
      std::vector<flat_set<PortDescriptor>> nhopSets128 = {
          nhopSets.begin(), nhopSets.begin() + kMaxDlbEcmpGroup};
      EXPECT_THROW(
          helper_->programRoutes(&wrapper, nhopSets128, prefixes128),
          FbossError);

      // overflow the dlb groups and ensure that the dlb resource stat
      // is updated. Also once routes are removed, the stat should reset.
      // TODO - Add support for SAI
      if (!getAgentEnsemble()->isSai()) {
        FLAGS_dlbResourceCheckEnable = false;
        std::vector<RoutePrefixV6> prefixes129 = {
            prefixes.begin(), prefixes.begin() + kMaxDlbEcmpGroup + 1};
        std::vector<flat_set<PortDescriptor>> nhopSets129 = {
            nhopSets.begin(), nhopSets.begin() + kMaxDlbEcmpGroup + 1};
        EXPECT_NO_THROW(
            helper_->programRoutes(&wrapper, nhopSets129, prefixes129));
        auto switchId = getSw()
                            ->getScopeResolver()
                            ->scope(masterLogicalPortIds()[0])
                            .switchId();
        WITH_RETRIES({
          auto stats = getHwSwitchStats(switchId);
          EXPECT_EVENTUALLY_TRUE(*stats.arsExhausted());
        });
        helper_->unprogramRoutes(&wrapper, prefixes129);
        WITH_RETRIES({
          auto stats =
              getAgentEnsemble()->getSw()->getHwSwitchStatsExpensive(switchId);
          EXPECT_EVENTUALLY_FALSE(*stats.arsExhausted());
        });
        FLAGS_dlbResourceCheckEnable = true;
      }
    }
    // install 10% of max DLB ecmp groups
    {
      int count = static_cast<int>(0.1 * kMaxDlbEcmpGroup);
      auto wrapper = getSw()->getRouteUpdater();
      std::vector<RoutePrefixV6> prefixes10 = {
          prefixes.begin() + kMaxDlbEcmpGroup,
          prefixes.begin() + kMaxDlbEcmpGroup + count};
      std::vector<flat_set<PortDescriptor>> nhopSets10 = {
          nhopSets.begin() + kMaxDlbEcmpGroup,
          nhopSets.begin() + kMaxDlbEcmpGroup + count};
      EXPECT_NO_THROW(helper_->programRoutes(&wrapper, nhopSets10, prefixes10));
      helper_->unprogramRoutes(&wrapper, prefixes10);
    }
  };
  verifyAcrossWarmBoots([]() {}, verify);
}

TEST_F(AgentFlowletSwitchingTest, ApplyDlbResourceCheck) {
  // Start with 60% ECMP groups
  auto setup = [this]() {
    generatePrefixes();
    const auto kMaxDlbEcmpGroup =
        utility::getMaxDlbEcmpGroups(getAgentEnsemble()->getL3Asics());
    int count = static_cast<int>(0.6 * kMaxDlbEcmpGroup);
    auto wrapper = getSw()->getRouteUpdater();
    std::vector<RoutePrefixV6> prefixes60 = {
        prefixes.begin(), prefixes.begin() + count};
    std::vector<flat_set<PortDescriptor>> nhopSets60 = {
        nhopSets.begin(), nhopSets.begin() + count};
    helper_->programRoutes(&wrapper, nhopSets60, prefixes60);
  };
  // Post warmboot, dlb resource check is enforced since >75%
  auto setupPostWarmboot = [this]() {
    generatePrefixes();
    const auto kMaxDlbEcmpGroup =
        utility::getMaxDlbEcmpGroups(getAgentEnsemble()->getL3Asics());
    {
      auto wrapper = getSw()->getRouteUpdater();
      std::vector<RoutePrefixV6> prefixes128 = {
          prefixes.begin(), prefixes.begin() + kMaxDlbEcmpGroup};
      std::vector<flat_set<PortDescriptor>> nhopSets128 = {
          nhopSets.begin(), nhopSets.begin() + kMaxDlbEcmpGroup};
      EXPECT_THROW(
          helper_->programRoutes(&wrapper, nhopSets128, prefixes128),
          FbossError);
    }
    // install 10% of max DLB ecmp groups
    {
      int count = static_cast<int>(0.1 * kMaxDlbEcmpGroup);
      auto wrapper = getSw()->getRouteUpdater();
      std::vector<RoutePrefixV6> prefixes10 = {
          prefixes.begin(), prefixes.begin() + count};
      std::vector<flat_set<PortDescriptor>> nhopSets10 = {
          nhopSets.begin(), nhopSets.begin() + count};
      helper_->programRoutes(&wrapper, nhopSets10, prefixes10);
    }
  };
  verifyAcrossWarmBoots(setup, [] {}, setupPostWarmboot, [] {});
}

TEST_F(AgentFlowletSwitchingTest, VerifyEcmpSwitchingMode) {
  auto setup = [this]() { this->setup(4); };

  auto verify = [this]() {
    auto switchId = getSw()
                        ->getScopeResolver()
                        ->scope(masterLogicalPortIds()[0])
                        .switchId();
    auto client = getAgentEnsemble()->getHwAgentTestClient(switchId);
    auto prefix = folly::CIDRNetwork{folly::IPAddress("::"), 0};
    auto resolvedRoute = findRoute<folly::IPAddressV6>(
        RouterID(0), prefix, getProgrammedState());
    state::RouteNextHopEntry entry{};
    entry.adminDistance() = AdminDistance::EBGP;
    entry.nexthops() = util::fromRouteNextHopSet(
        resolvedRoute->getForwardInfo().getNextHopSet());
    WITH_RETRIES({
      EXPECT_EVENTUALLY_EQ(
          client->sync_getFwdSwitchingMode(entry),
          cfg::SwitchingMode::PER_PACKET_QUALITY);
    });
  };

  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
