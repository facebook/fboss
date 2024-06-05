/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/TxPacket.h"
#include "fboss/agent/packet/PktFactory.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/utils/AclTestUtils.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/LoadBalancerTestUtils.h"
#include "fboss/agent/test/utils/ScaleTestUtils.h"
#include "fboss/lib/CommonUtils.h"

DECLARE_bool(flowletSwitchingEnable);

namespace {
enum AclType {
  UDF_ACK,
  FLOWLET,
  FLOWLET_WITH_UDF_ACK,
  UDF_FLOWLET,
  UDF_FLOWLET_WITH_UDF_ACK,
};
}

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
    helper_ = std::make_unique<utility::EcmpSetupAnyNPorts6>(
        getProgrammedState(), RouterID(0));
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
      case AclType::FLOWLET:
      case AclType::FLOWLET_WITH_UDF_ACK:
        aclName = "test-flowlet-acl";
        break;
      case AclType::UDF_FLOWLET:
      case AclType::UDF_FLOWLET_WITH_UDF_ACK:
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

  void setup() {
    applyNewState([&](const std::shared_ptr<SwitchState>& in) {
      return helper_->resolveNextHops(in, 2);
    });
    auto wrapper = getSw()->getRouteUpdater();
    helper_->programRoutes(&wrapper, kEcmpWidth);

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

  void generateApplyConfig(AclType aclType) {
    auto newCfg{initialConfig(*getAgentEnsemble())};
    addAclAndStat(&newCfg, aclType);
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
      const PortID frontPanelEgrPort,
      int roceOpcode = utility::kUdfRoceOpcode) {
    auto vlanId = utility::firstVlanID(getProgrammedState());
    auto intfMac = utility::getFirstInterfaceMac(getProgrammedState());
    return utility::pumpRoCETraffic(
        true,
        utility::getAllocatePktFn(getAgentEnsemble()),
        utility::getSendPktFunc(getAgentEnsemble()),
        intfMac,
        vlanId,
        frontPanelEgrPort,
        utility::kUdfL4DstPort,
        255,
        std::nullopt,
        1 /* one packet */,
        roceOpcode,
        utility::kRoceReserved);
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

    switch (aclType) {
      case AclType::UDF_ACK:
        sizeOfPacketSent = sendRoceTraffic(egressPort);
        break;
      case AclType::FLOWLET:
      case AclType::UDF_FLOWLET:
        sizeOfPacketSent = sendRoceTraffic(egressPort, 11 /* not ack opcode */);
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
        EXPECT_EVENTUALLY_LE(
            aclBytesCountAfter,
            aclBytesCountBefore + (2 * sizeOfPacketSent) + 4);
      } else {
        EXPECT_EVENTUALLY_EQ(aclPktCountBefore, aclPktCountAfter);
        EXPECT_EVENTUALLY_EQ(aclBytesCountBefore, aclBytesCountAfter);
      }
    });
  }

  void verifyAcl(AclType aclType) {
    switch (aclType) {
      case AclType::UDF_ACK:
        verifyAclType(true, AclType::UDF_ACK);
        break;
      case AclType::FLOWLET:
        verifyAclType(true, AclType::FLOWLET);
        break;
      case AclType::FLOWLET_WITH_UDF_ACK:
        verifyAclType(true, AclType::FLOWLET);
        verifyAclType(true, AclType::UDF_ACK);
        break;
      case AclType::UDF_FLOWLET:
        verifyAclType(true, AclType::UDF_FLOWLET);
        break;
      case AclType::UDF_FLOWLET_WITH_UDF_ACK:
        verifyAclType(true, AclType::UDF_FLOWLET);
        verifyAclType(true, AclType::UDF_ACK);
        break;
      default:
        break;
    }
  }

  void addUdfAcl(
      cfg::SwitchConfig* config,
      const std::string& aclName,
      const std::string& counterName) const {
    auto acl = utility::addAcl(config, aclName, aclActionType_);
    std::vector<cfg::CounterType> setCounterTypes{
        cfg::CounterType::PACKETS, cfg::CounterType::BYTES};
    acl->udfGroups() = {utility::kUdfAclRoceOpcodeGroupName};
    acl->roceOpcode() = utility::kUdfRoceOpcode;
    utility::addAclStat(
        config, aclName, counterName, std::move(setCounterTypes));
  }

  void addAclAndStat(cfg::SwitchConfig* config, AclType aclType) const {
    auto aclName = getAclName(aclType);
    auto counterName = getCounterName(aclType);
    switch (aclType) {
      case AclType::UDF_ACK:
        config->udfConfig() = utility::addUdfAclConfig();
        addUdfAcl(config, aclName, counterName);
        break;
      case AclType::FLOWLET:
        utility::addFlowletAcl(*config, aclName, counterName, false);
        break;
      case AclType::FLOWLET_WITH_UDF_ACK:
        config->udfConfig() = utility::addUdfAclConfig();
        addUdfAcl(
            config,
            getAclName(AclType::UDF_ACK),
            getCounterName(AclType::UDF_ACK));
        utility::addFlowletAcl(*config, aclName, counterName, false);
        break;
      case AclType::UDF_FLOWLET:
        config->udfConfig() = utility::addUdfFlowletAclConfig();
        utility::addFlowletAcl(*config, aclName, counterName);
        break;
      case AclType::UDF_FLOWLET_WITH_UDF_ACK:
        config->udfConfig() = utility::addUdfAckAndFlowletAclConfig();
        addUdfAcl(
            config,
            getAclName(AclType::UDF_ACK),
            getCounterName(AclType::UDF_ACK));
        utility::addFlowletAcl(*config, aclName, counterName);
        break;
      default:
        break;
    }
  }

  static inline constexpr auto kEcmpWidth = 1;
  std::unique_ptr<utility::EcmpSetupAnyNPorts6> helper_;
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
    utility::addFlowletConfigs(cfg, ensemble.masterLogicalPortIds());
    return cfg;
  }

  void setCmdLineFlagOverrides() const override {
    AgentHwTest::setCmdLineFlagOverrides();
    FLAGS_flowletSwitchingEnable = true;
    FLAGS_force_init_fp = false;
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

// UDF A to empty
TEST_F(AgentFlowletSwitchingTest, VerifyUdfFlowletToFlowlet) {
  flowletSwitchingAclHitHelper(AclType::UDF_FLOWLET, AclType::FLOWLET);
}

TEST_F(AgentFlowletSwitchingTest, VerifyOneUdfGroupAddition) {
  verifyUdfAddDelete(AclType::UDF_FLOWLET, AclType::UDF_FLOWLET_WITH_UDF_ACK);
}

TEST_F(AgentFlowletSwitchingTest, VerifyOneUdfGroupDeletion) {
  verifyUdfAddDelete(AclType::UDF_FLOWLET_WITH_UDF_ACK, AclType::UDF_FLOWLET);
}

class AgentFlowletResourceTest : public AgentHwTest {
 public:
  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    return {production_features::ProductionFeature::DLB};
  }

 protected:
  void SetUp() override {
    AgentHwTest::SetUp();
    if (IsSkipped()) {
      return;
    }
    helper_ = std::make_unique<utility::EcmpSetupTargetedPorts6>(
        getProgrammedState());
  }
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto cfg = utility::onePortPerInterfaceConfig(
        ensemble.getSw(),
        ensemble.masterLogicalPortIds(),
        true /*interfaceHasSubnet*/);
    return cfg;
  }
  void setCmdLineFlagOverrides() const override {
    AgentHwTest::setCmdLineFlagOverrides();
    FLAGS_flowletSwitchingEnable = true;
  }
  void setup() {
    std::vector<PortID> portIds = masterLogicalInterfacePortIds();
    std::vector<PortDescriptor> portDescriptorIds;
    for (auto& portId : portIds) {
      portDescriptorIds.push_back(PortDescriptor(portId));
    }
    std::vector<std::vector<PortDescriptor>> allCombinations =
        utility::generateEcmpGroupScale(portDescriptorIds, 512);
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

  std::unique_ptr<utility::EcmpSetupTargetedPorts6> helper_;
  std::vector<flat_set<PortDescriptor>> nhopSets;
  std::vector<RoutePrefixV6> prefixes;
};

TEST_F(AgentFlowletResourceTest, CreateMaxDlbGroups) {
  auto setup = [&]() { this->setup(); };
  auto verify = [this] {
    const auto kMaxDlbEcmpGroup =
        utility::getMaxDlbEcmpGroups(getAgentEnsemble()->getL3Asics());
    this->setup();
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
      helper_->programRoutes(&wrapper, nhopSets10, prefixes10);
    }
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentFlowletResourceTest, IgnoreDlbResourceCheck) {
  // Start with 128 ECMP groups
  auto setup = [this]() {
    const auto kMaxDlbEcmpGroup =
        utility::getMaxDlbEcmpGroups(getAgentEnsemble()->getL3Asics());
    FLAGS_flowletSwitchingEnable = false;
    this->setup();
    auto wrapper = getSw()->getRouteUpdater();
    std::vector<RoutePrefixV6> prefixes128 = {
        prefixes.begin(), prefixes.begin() + kMaxDlbEcmpGroup};
    std::vector<flat_set<PortDescriptor>> nhopSets128 = {
        nhopSets.begin(), nhopSets.begin() + kMaxDlbEcmpGroup};
    helper_->programRoutes(&wrapper, nhopSets128, prefixes128);
  };
  // Post warmboot, since there are already 128, dlb resource check is disabled
  auto setupPostWarmboot = [this]() {
    const auto kMaxDlbEcmpGroup =
        utility::getMaxDlbEcmpGroups(getAgentEnsemble()->getL3Asics());
    FLAGS_flowletSwitchingEnable = true;
    this->setup();
    auto wrapper = getSw()->getRouteUpdater();
    std::vector<RoutePrefixV6> prefixes128 = {
        prefixes.begin() + kMaxDlbEcmpGroup,
        prefixes.begin() + 2 * kMaxDlbEcmpGroup};
    std::vector<flat_set<PortDescriptor>> nhopSets128 = {
        nhopSets.begin() + kMaxDlbEcmpGroup,
        nhopSets.begin() + 2 * kMaxDlbEcmpGroup};
    helper_->programRoutes(&wrapper, nhopSets128, prefixes128);
  };
  verifyAcrossWarmBoots(setup, [] {}, setupPostWarmboot, [] {});
}

TEST_F(AgentFlowletResourceTest, ApplyDlbResourceCheck) {
  // Start with 60% ECMP groups
  auto setup = [this]() {
    const auto kMaxDlbEcmpGroup =
        utility::getMaxDlbEcmpGroups(getAgentEnsemble()->getL3Asics());
    this->setup();
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
    const auto kMaxDlbEcmpGroup =
        utility::getMaxDlbEcmpGroups(getAgentEnsemble()->getL3Asics());
    this->setup();
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
} // namespace facebook::fboss
