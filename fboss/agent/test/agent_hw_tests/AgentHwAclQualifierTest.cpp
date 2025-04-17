// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/AgentHwTest.h"

#include "fboss/agent/test/utils/AclTestUtils.h"
#include "fboss/agent/test/utils/AsicUtils.h"

namespace {

using namespace facebook::fboss;

enum class QualifierType : uint8_t {
  LOOKUPCLASS_L2,
  LOOKUPCLASS_NEIGHBOR,
  LOOKUPCLASS_ROUTE,
  LOOKUPCLASS_IGNORE,
};

template <typename T, typename U>
void configureQualifier(
    apache::thrift::optional_field_ref<T&> ref,
    bool enable,
    const U& val) {
  if (enable) {
    ref = val;
  } else {
    ref.reset();
  }
}

void configureAllIpQualifiers(
    cfg::AclEntry* acl,
    bool enable,
    cfg::IpType ipType,
    cfg::AsicType asicType) {
  cfg::Ttl ttl;
  std::tie(*ttl.value(), *ttl.mask()) = std::make_tuple(0x80, 0x80);
  bool enableSrcIpQualifier =
      (asicType != cfg::AsicType::ASIC_TYPE_CHENAB) && enable;
  bool enableIpFragQualifier =
      (asicType != cfg::AsicType::ASIC_TYPE_CHENAB) && enable;
  bool enableLookupClass =
      (asicType != cfg::AsicType::ASIC_TYPE_CHENAB) && enable;
  bool enableEtherTpe = (asicType == cfg::AsicType::ASIC_TYPE_CHENAB) && enable;

  if (asicType != cfg::AsicType::ASIC_TYPE_JERICHO3) {
    // TODO(daiweix): remove after J3 ACL supports IP_TYPE
    configureQualifier(acl->ipType(), enable, ipType);
  }
  if (ipType == cfg::IpType::IP6) {
    configureQualifier(acl->srcIp(), enableSrcIpQualifier, "::ffff:c0a8:1");
    configureQualifier(
        acl->dstIp(), enable, "2401:db00:3020:70e2:face:0:63:0/64");

    configureQualifier(
        acl->lookupClassNeighbor(),
        enableLookupClass,
        cfg::AclLookupClass::DST_CLASS_L3_LOCAL_2);
    configureQualifier(
        acl->lookupClassRoute(),
        enableLookupClass,
        cfg::AclLookupClass::DST_CLASS_L3_LOCAL_2);

  } else {
    configureQualifier(acl->srcIp(), enableSrcIpQualifier, "192.168.0.1");
    configureQualifier(acl->dstIp(), enable, "192.168.0.0/24");

    configureQualifier(
        acl->lookupClassNeighbor(),
        enableLookupClass,
        cfg::AclLookupClass::DST_CLASS_L3_LOCAL_1);
    configureQualifier(
        acl->lookupClassRoute(),
        enableLookupClass,
        cfg::AclLookupClass::DST_CLASS_L3_LOCAL_1);
  }
  configureQualifier(
      acl->ipFrag(),
      enableIpFragQualifier,
      cfg::IpFragMatch::MATCH_FIRST_FRAGMENT);
  configureQualifier(acl->dscp(), enable, 0x24);
  configureQualifier(acl->ttl(), enable, ttl);

  if (ipType == cfg::IpType::IP6) {
    configureQualifier(acl->etherType(), enableEtherTpe, cfg::EtherType::IPv6);
  } else {
    configureQualifier(acl->etherType(), enableEtherTpe, cfg::EtherType::IPv4);
  }
}

void configureAllTcpQualifiers(
    cfg::AclEntry* acl,
    bool enable,
    cfg::AsicType asicType) {
  configureQualifier(acl->l4SrcPort(), enable, 10);
  configureQualifier(acl->l4DstPort(), enable, 20);
  configureQualifier(acl->proto(), enable, 6);
  bool enableTcpFlags =
      (enable && (asicType != cfg::AsicType::ASIC_TYPE_CHENAB));
  configureQualifier(acl->tcpFlagsBitMap(), enableTcpFlags, 16);
}

void configureAllIcmpQualifiers(
    cfg::AclEntry* acl,
    bool enable,
    cfg::IpType ipType,
    cfg::AsicType asicType) {
  bool enableEtherTypeQualifier = (asicType == cfg::AsicType::ASIC_TYPE_CHENAB);

  if (ipType == cfg::IpType::IP6) {
    configureQualifier(acl->proto(), enable, 58); // Icmp v6
    configureQualifier(acl->icmpType(), enable, 1); // Destination unreachable
    configureQualifier(acl->icmpCode(), enable, 4); // Port unreachable
    configureQualifier(
        acl->etherType(), enableEtherTypeQualifier, cfg::EtherType::IPv6);
  } else {
    configureQualifier(acl->proto(), enable, 1); // Icmp v4
    configureQualifier(acl->icmpType(), enable, 3); // Destination unreachable
    configureQualifier(acl->icmpCode(), enable, 3); // Port unreachable
    configureQualifier(
        acl->etherType(), enableEtherTypeQualifier, cfg::EtherType::IPv4);
  }
}

} // unnamed namespace

namespace facebook::fboss {

class AgentHwAclQualifierTest : public AgentHwTest {
 public:
  bool addQualifiers = false;

  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    if (!FLAGS_enable_acl_table_group) {
      return {
          production_features::ProductionFeature::SINGLE_ACL_TABLE,
          production_features::ProductionFeature::MODIFY_ACL_QUALIFIERS};
    }
    return {
        production_features::ProductionFeature::MULTI_ACL_TABLE,
        production_features::ProductionFeature::MODIFY_ACL_QUALIFIERS};
  }

  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto newCfg = AgentHwTest::initialConfig(ensemble);
    if (utility::checkSameAndGetAsicType(newCfg) ==
        cfg::AsicType::ASIC_TYPE_CHENAB) {
      utility::addAclTable(
          &newCfg,
          "icmp_acl_table",
          2 /* priority */,
          {},
          {
              cfg::AclTableQualifier::SRC_PORT,
              cfg::AclTableQualifier::ETHER_TYPE,
              cfg::AclTableQualifier::SRC_IPV4,
              cfg::AclTableQualifier::DST_IPV4,
              cfg::AclTableQualifier::IP_PROTOCOL_NUMBER,
              cfg::AclTableQualifier::ICMPV4_TYPE,
              cfg::AclTableQualifier::ICMPV4_CODE,
              cfg::AclTableQualifier::SRC_IPV6,
              cfg::AclTableQualifier::DST_IPV6,
              cfg::AclTableQualifier::IPV6_NEXT_HEADER,
              cfg::AclTableQualifier::ICMPV6_TYPE,
              cfg::AclTableQualifier::ICMPV6_CODE,
          });
    }
    return newCfg;
  }

  void configureAllHwQualifiers(
      cfg::AclEntry* acl,
      bool enable,
      SwitchID switchID = SwitchID(0)) {
    auto masterLogicalPorts =
        getAgentEnsemble()->masterLogicalInterfacePortIds({switchID});
    configureQualifier(acl->srcPort(), enable, masterLogicalPorts[0]);
    if ((hwAsicForSwitch(switchID)->getAsicType() !=
         cfg::AsicType::ASIC_TYPE_JERICHO2) &&
        (hwAsicForSwitch(switchID)->getAsicType() !=
         cfg::AsicType::ASIC_TYPE_CHENAB) &&
        (hwAsicForSwitch(switchID)->getAsicType() !=
         cfg::AsicType::ASIC_TYPE_JERICHO3)) {
      // No out port support on J2. Out port not used in prod
      // No out support on Chenab in ingress stage
      configureQualifier(acl->dstPort(), enable, masterLogicalPorts[1]);
    }
  }

  void configureAllL2QualifiersHelper(
      cfg::AclEntry* acl,
      SwitchID switchID = SwitchID(0)) {
    auto asic = hwAsicForSwitch(switchID);
    bool dstMacEnabled =
        (asic->getAsicType() != cfg::AsicType::ASIC_TYPE_CHENAB);
    configureQualifier(acl->dstMac(), dstMacEnabled, "00:11:22:33:44:55");
    /*
     * lookupClassL2 is not configured for Trident2 or else we run out of
     * resources.
     * Note: lookupclassL2 is needed for MH-NIC queue-per-host solution.
     * However, the solution is not applicable for Trident2 as we don't
     * implement queues on trident2.
     */
    auto switchType = getAgentEnsemble()
                          ->getSw()
                          ->getSwitchInfoTable()
                          .getSwitchIdToSwitchInfo()
                          .at(switchID)
                          .switchType()
                          .value();
    if (asic->getAsicType() != cfg::AsicType::ASIC_TYPE_TRIDENT2 &&
        asic->getAsicType() !=
            cfg::AsicType::ASIC_TYPE_CHENAB && // no l2 lookup class in chenab
                                               // in ingress stage L2 switching
                                               // only on NPU type switch
        switchType == cfg::SwitchType::NPU) {
      configureQualifier(
          acl->lookupClassL2(),
          true,
          cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_1);
    }
  }

  void configureIp4QualifiersHelper(
      cfg::AclEntry* acl,
      SwitchID switchID = SwitchID(0)) {
    auto asicType = getAsicType(switchID);
    bool enableSrcIpQualifier = (asicType != cfg::AsicType::ASIC_TYPE_CHENAB);
    bool enableEtherTypeQualifier =
        (asicType == cfg::AsicType::ASIC_TYPE_CHENAB);
    cfg::Ttl ttl;
    std::tie(*ttl.value(), *ttl.mask()) = std::make_tuple(0x80, 0x80);

    if (asicType != cfg::AsicType::ASIC_TYPE_JERICHO3) {
      // TODO(daiweix): remove after J3 ACL supports IP_TYPE
      configureQualifier(acl->ipType(), true, cfg::IpType::IP4);
    }
    configureQualifier(acl->srcIp(), enableSrcIpQualifier, "192.168.0.1");
    configureQualifier(acl->dstIp(), true, "192.168.0.0/24");
    configureQualifier(acl->dscp(), true, 0x24);
    configureQualifier(acl->ttl(), true, ttl);
    configureQualifier(acl->proto(), true, 6);
    configureQualifier(
        acl->etherType(), enableEtherTypeQualifier, cfg::EtherType::IPv4);
  }

  void configureIp6QualifiersHelper(
      cfg::AclEntry* acl,
      SwitchID switchID = SwitchID(0)) {
    auto asicType = getAsicType(switchID);
    auto enableSrcIpQualifier = (asicType != cfg::AsicType::ASIC_TYPE_CHENAB);
    bool enableEtherTypeQualifier =
        (asicType == cfg::AsicType::ASIC_TYPE_CHENAB);
    cfg::Ttl ttl;
    std::tie(*ttl.value(), *ttl.mask()) = std::make_tuple(0x80, 0x80);

    if (asicType != cfg::AsicType::ASIC_TYPE_JERICHO3) {
      // TODO(daiweix): remove after J3 ACL supports IP_TYPE
      configureQualifier(acl->ipType(), true, cfg::IpType::IP6);
    }

    configureQualifier(acl->srcIp(), enableSrcIpQualifier, "::ffff:c0a8:1");
    configureQualifier(
        acl->dstIp(), true, "2401:db00:3020:70e2:face:0:63:0/64");
    configureQualifier(acl->dscp(), true, 0x24);
    configureQualifier(acl->ttl(), true, ttl);
    configureQualifier(acl->proto(), true, 6);
    configureQualifier(
        acl->etherType(), enableEtherTypeQualifier, cfg::EtherType::IPv6);
  }

  std::string kAclName() const {
    return "acl0";
  }

  void aclSetupHelper(
      bool isIpV4,
      QualifierType lookupClassType,
      bool addQualifiers = false,
      SwitchID switchID = SwitchID(0)) {
    this->addQualifiers = addQualifiers;
    auto newCfg = initialConfig(*getAgentEnsemble());
    if (FLAGS_enable_acl_table_group) {
      utility::addAclTableGroup(
          &newCfg,
          cfg::AclStage::INGRESS,
          utility::kDefaultAclTableGroupName());

      std::vector<cfg::AclTableQualifier> defaultQualifiers = {
          cfg::AclTableQualifier::DST_IPV6,
          cfg::AclTableQualifier::DST_IPV4,
          cfg::AclTableQualifier::L4_SRC_PORT,
          cfg::AclTableQualifier::L4_DST_PORT,
          cfg::AclTableQualifier::IP_PROTOCOL_NUMBER,
          cfg::AclTableQualifier::IPV6_NEXT_HEADER,
          cfg::AclTableQualifier::SRC_PORT,
          cfg::AclTableQualifier::DSCP,
          cfg::AclTableQualifier::TTL,
          cfg::AclTableQualifier::IP_TYPE,
          cfg::AclTableQualifier::ETHER_TYPE,
          cfg::AclTableQualifier::OUTER_VLAN,
      };

      if (getAsicType() != cfg::AsicType::ASIC_TYPE_CHENAB) {
        defaultQualifiers.clear();
      }
      std::vector<cfg::AclTableActionType> actions = {};
      std::vector<cfg::AclTableQualifier> qualifiers = addQualifiers
          ? utility::genAclQualifiersConfig(this->getAsicType())
          : defaultQualifiers;
      utility::addAclTable(
          &newCfg,
          utility::kDefaultAclTable(),
          0 /* priority */,
          actions,
          qualifiers);
    }

    cfg::AclEntry acl{};
    acl.name() = kAclName();
    acl.actionType() = cfg::AclActionType::DENY;

    if (isIpV4) {
      configureIp4QualifiersHelper(&acl, switchID);
    } else {
      configureIp6QualifiersHelper(&acl, switchID);
    }

    switch (lookupClassType) {
      case QualifierType::LOOKUPCLASS_L2:
        configureQualifier(
            acl.lookupClassL2(),
            true,
            cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_1);
        break;
      case QualifierType::LOOKUPCLASS_NEIGHBOR:
        configureQualifier(
            acl.lookupClassNeighbor(),
            true,
            isIpV4 ? cfg::AclLookupClass::DST_CLASS_L3_LOCAL_1
                   : cfg::AclLookupClass::DST_CLASS_L3_LOCAL_2);
        break;
      case QualifierType::LOOKUPCLASS_ROUTE:
        configureQualifier(
            acl.lookupClassRoute(),
            true,
            isIpV4 ? cfg::AclLookupClass::DST_CLASS_L3_LOCAL_1
                   : cfg::AclLookupClass::DST_CLASS_L3_LOCAL_2);
        break;
      case QualifierType::LOOKUPCLASS_IGNORE:
        // This case is here exactly to not do anything
        break;
      default:
        CHECK(false);
    }
    utility::addAcl(&newCfg, acl, cfg::AclStage::INGRESS);

    applyNewConfig(newCfg);
  }

  void aclVerifyHelper() {
    auto client = getAgentEnsemble()->getHwAgentTestClient(SwitchID(0));
    auto state = getAgentEnsemble()->getProgrammedState();
    auto acl =
        utility::getAclEntry(state, kAclName(), FLAGS_enable_acl_table_group)
            ->toThrift();

    EXPECT_EQ(
        client->sync_getAclTableNumAclEntries(utility::kDefaultAclTable()), 1);
    EXPECT_TRUE(client->sync_isAclEntrySame(acl, utility::kDefaultAclTable()));
  }

  cfg::AsicType getAsicType(SwitchID switchID = SwitchID(0)) {
    return hwAsicForSwitch(switchID)->getAsicType();
  }
};

TEST_F(AgentHwAclQualifierTest, AclIp4TcpQualifiers) {
  auto setup = [=, this]() {
    auto& ensemble = *getAgentEnsemble();
    auto newCfg = initialConfig(ensemble);
    auto l3Asics = ensemble.getSw()->getHwAsicTable()->getL3Asics();

    cfg::AclEntry acl1{};
    acl1.name() = "ip4_tcp";
    acl1.actionType() = cfg::AclActionType::DENY;

    configureAllHwQualifiers(&acl1, true);
    configureAllL2QualifiersHelper(&acl1);
    configureAllIpQualifiers(&acl1, true, cfg::IpType::IP4, getAsicType());
    configureAllTcpQualifiers(&acl1, true, getAsicType());
    utility::addAcl(&newCfg, acl1, cfg::AclStage::INGRESS);
    applyNewConfig(newCfg);
  };

  auto verify = [=, this]() {
    auto client = getAgentEnsemble()->getHwAgentTestClient(SwitchID(0));
    auto state = getAgentEnsemble()->getProgrammedState();
    auto acl =
        utility::getAclEntry(state, "ip4_tcp", FLAGS_enable_acl_table_group)
            ->toThrift();
    EXPECT_EQ(
        client->sync_getAclTableNumAclEntries(utility::kDefaultAclTable()), 1);
    EXPECT_TRUE(client->sync_isAclEntrySame(acl, utility::kDefaultAclTable()));
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentHwAclQualifierTest, AclIp6TcpQualifiers) {
  auto setup = [=, this]() {
    auto& ensemble = *getAgentEnsemble();
    auto newCfg = initialConfig(ensemble);
    auto l3Asics = ensemble.getSw()->getHwAsicTable()->getL3Asics();

    cfg::AclEntry acl1{};
    acl1.name() = "ip6_tcp";
    acl1.actionType() = cfg::AclActionType::DENY;

    configureAllHwQualifiers(&acl1, true);
    configureAllL2QualifiersHelper(&acl1);
    configureAllIpQualifiers(
        &acl1, true, cfg::IpType::IP6, this->getAsicType());
    configureAllTcpQualifiers(&acl1, true, getAsicType());
    utility::addAcl(&newCfg, acl1, cfg::AclStage::INGRESS);

    applyNewConfig(newCfg);
  };

  auto verify = [=, this]() {
    auto client = getAgentEnsemble()->getHwAgentTestClient(SwitchID(0));
    auto state = getAgentEnsemble()->getProgrammedState();
    auto acl =
        utility::getAclEntry(state, "ip6_tcp", FLAGS_enable_acl_table_group)
            ->toThrift();

    EXPECT_EQ(
        client->sync_getAclTableNumAclEntries(utility::kDefaultAclTable()), 1);
    EXPECT_TRUE(client->sync_isAclEntrySame(acl, utility::kDefaultAclTable()));
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentHwAclQualifierTest, AclIcmp4Qualifiers) {
  auto setup = [=, this]() {
    auto& ensemble = *getAgentEnsemble();
    auto newCfg = initialConfig(ensemble);
    auto l3Asics = ensemble.getSw()->getHwAsicTable()->getL3Asics();

    cfg::AclEntry acl1{};
    acl1.name() = "icmp4";
    acl1.actionType() = cfg::AclActionType::DENY;

    configureAllHwQualifiers(&acl1, true);
    configureAllL2QualifiersHelper(&acl1);
    configureAllIcmpQualifiers(&acl1, true, cfg::IpType::IP4, getAsicType());

    utility::addAcl(&newCfg, acl1, cfg::AclStage::INGRESS);
    applyNewConfig(newCfg);
  };

  auto verify = [=, this]() {
    auto client = getAgentEnsemble()->getHwAgentTestClient(SwitchID(0));
    auto state = getAgentEnsemble()->getProgrammedState();
    auto acl =
        utility::getAclEntry(state, "icmp4", FLAGS_enable_acl_table_group)
            ->toThrift();

    if (getAsicType() == cfg::AsicType::ASIC_TYPE_CHENAB) {
      EXPECT_EQ(client->sync_getAclTableNumAclEntries("icmp_acl_table"), 1);
      EXPECT_TRUE(client->sync_isAclEntrySame(acl, "icmp_acl_table"));
    } else {
      EXPECT_EQ(
          client->sync_getAclTableNumAclEntries(utility::kDefaultAclTable()),
          1);
      EXPECT_TRUE(
          client->sync_isAclEntrySame(acl, utility::kDefaultAclTable()));
    }
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentHwAclQualifierTest, AclIcmp6Qualifiers) {
  auto setup = [=, this]() {
    auto& ensemble = *getAgentEnsemble();
    auto newCfg = initialConfig(ensemble);
    auto l3Asics = ensemble.getSw()->getHwAsicTable()->getL3Asics();

    cfg::AclEntry acl1{};
    acl1.name() = "icmp6";
    acl1.actionType() = cfg::AclActionType::DENY;

    configureAllHwQualifiers(&acl1, true);
    configureAllL2QualifiersHelper(&acl1);
    configureAllIcmpQualifiers(&acl1, true, cfg::IpType::IP6, getAsicType());
    utility::addAcl(&newCfg, acl1, cfg::AclStage::INGRESS);
    applyNewConfig(newCfg);
  };

  auto verify = [=, this]() {
    auto client = getAgentEnsemble()->getHwAgentTestClient(SwitchID(0));
    auto state = getAgentEnsemble()->getProgrammedState();
    auto acl =
        utility::getAclEntry(state, "icmp6", FLAGS_enable_acl_table_group)
            ->toThrift();
    if (getAsicType() == cfg::AsicType::ASIC_TYPE_CHENAB) {
      EXPECT_EQ(client->sync_getAclTableNumAclEntries("icmp_acl_table"), 1);
      EXPECT_TRUE(client->sync_isAclEntrySame(acl, "icmp_acl_table"));

    } else {
      EXPECT_EQ(
          client->sync_getAclTableNumAclEntries(utility::kDefaultAclTable()),
          1);
      EXPECT_TRUE(
          client->sync_isAclEntrySame(acl, utility::kDefaultAclTable()));
    }
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentHwAclQualifierTest, AclRemove) {
  auto setup = [=, this]() {
    auto& ensemble = *getAgentEnsemble();
    auto newCfg = initialConfig(ensemble);
    auto l3Asics = ensemble.getSw()->getHwAsicTable()->getL3Asics();
    auto asic = utility::checkSameAndGetAsic(l3Asics);

    cfg::AclEntry acl0{};
    acl0.name() = "acl0";
    acl0.proto() = 6;
    if (asic->isSupported(HwAsic::Feature::ACL_ENTRY_ETHER_TYPE)) {
      acl0.etherType() = cfg::EtherType::IPv6;
    }
    acl0.actionType() = cfg::AclActionType::DENY;
    utility::addAcl(&newCfg, acl0, cfg::AclStage::INGRESS);
    cfg::AclEntry acl1{};
    acl1.name() = "acl1";
    acl1.proto() = 58;
    acl1.actionType() = cfg::AclActionType::DENY;
    if (asic->isSupported(HwAsic::Feature::ACL_ENTRY_ETHER_TYPE)) {
      acl1.etherType() = cfg::EtherType::IPv6;
    }
    utility::addAcl(&newCfg, acl1, cfg::AclStage::INGRESS);
    applyNewConfig(newCfg);

    utility::delAcl(&newCfg, "acl0");
    applyNewConfig(newCfg);
  };

  auto verify = [=, this]() {
    auto client = getAgentEnsemble()->getHwAgentTestClient(SwitchID(0));
    auto state = getAgentEnsemble()->getProgrammedState();
    auto acl = utility::getAclEntry(state, "acl1", FLAGS_enable_acl_table_group)
                   ->toThrift();
    EXPECT_EQ(
        client->sync_getAclTableNumAclEntries(utility::kDefaultAclTable()), 1);

    EXPECT_TRUE(client->sync_isAclEntrySame(acl, utility::kDefaultAclTable()));
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentHwAclQualifierTest, AclModifyQualifier) {
  auto setup = [=, this]() {
    auto& ensemble = *getAgentEnsemble();
    auto newCfg = initialConfig(ensemble);
    cfg::AclEntry acl{};
    acl.name() = "acl0";
    acl.actionType() = cfg::AclActionType::DENY;
    // icmp6
    this->configureAllHwQualifiers(&acl, true);
    this->configureAllL2QualifiersHelper(&acl);
    configureAllIcmpQualifiers(
        &acl, true, cfg::IpType::IP6, this->getAsicType());
    utility::addAcl(&newCfg, acl, cfg::AclStage::INGRESS);
    applyNewConfig(newCfg);
    // ip6 tcp
    configureAllIcmpQualifiers(
        &acl, false, cfg::IpType::IP6, this->getAsicType());
    configureAllIpQualifiers(&acl, true, cfg::IpType::IP6, this->getAsicType());
    newCfg = initialConfig(ensemble);
    utility::addAcl(&newCfg, acl, cfg::AclStage::INGRESS);
    applyNewConfig(newCfg);
    // imcp6
    configureAllIpQualifiers(
        &acl, false, cfg::IpType::IP6, this->getAsicType());
    configureAllIcmpQualifiers(
        &acl, true, cfg::IpType::IP6, this->getAsicType());
    newCfg = initialConfig(ensemble);
    utility::addAcl(&newCfg, acl, cfg::AclStage::INGRESS);
    applyNewConfig(newCfg);
  };

  auto verify = [=, this]() {
    auto client = getAgentEnsemble()->getHwAgentTestClient(SwitchID(0));
    auto state = getAgentEnsemble()->getProgrammedState();
    auto acl = utility::getAclEntry(state, "acl0", FLAGS_enable_acl_table_group)
                   ->toThrift();
    if (getAsicType() == cfg::AsicType::ASIC_TYPE_CHENAB) {
      EXPECT_EQ(client->sync_getAclTableNumAclEntries("icmp_acl_table"), 1);
      EXPECT_TRUE(client->sync_isAclEntrySame(acl, "icmp_acl_table"));
    } else {
      EXPECT_EQ(
          client->sync_getAclTableNumAclEntries(utility::kDefaultAclTable()),
          1);
      EXPECT_TRUE(
          client->sync_isAclEntrySame(acl, utility::kDefaultAclTable()));
    }
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentHwAclQualifierTest, AclEmptyCodeIcmp) {
  auto setup = [=, this]() {
    auto& ensemble = *getAgentEnsemble();
    auto newCfg = initialConfig(ensemble);

    cfg::AclEntry acl{};
    // add a icmp rule w/ type and code value
    // Destination Unreachable(type=3):Source host isolated(code=8)
    acl.name() = "acl0";
    auto l3Asics = ensemble.getL3Asics();
    auto asic = utility::checkSameAndGetAsic(l3Asics);
    if (asic->isSupported(HwAsic::Feature::ACL_ENTRY_ETHER_TYPE)) {
      acl.etherType() = cfg::EtherType::IPv4;
    }
    acl.proto() = 58;
    acl.icmpType() = 3;
    acl.icmpCode() = 8;
    acl.actionType() = cfg::AclActionType::DENY;
    auto cfg1 = newCfg;
    utility::addAcl(&cfg1, acl, cfg::AclStage::INGRESS);
    this->applyNewConfig(cfg1);

    // change the rule to empty code icmp rule
    // Reserved for security(type=19 code is unset)
    acl.icmpType() = 19;
    acl.icmpCode().reset();
    utility::addAcl(&newCfg, acl, cfg::AclStage::INGRESS);
    applyNewConfig(newCfg);
  };

  auto verify = [=, this]() {
    auto client = getAgentEnsemble()->getHwAgentTestClient(SwitchID(0));
    auto state = getAgentEnsemble()->getProgrammedState();
    auto acl =
        utility::getAclEntry(state, "acl0", FLAGS_enable_acl_table_group);
    ASSERT_NE(acl, nullptr);
    auto aclThrift = acl->toThrift();
    if (this->getAsicType() == cfg::AsicType::ASIC_TYPE_CHENAB) {
      EXPECT_TRUE(client->sync_isAclEntrySame(aclThrift, "icmp_acl_table"));
    } else {
      EXPECT_TRUE(
          client->sync_isAclEntrySame(aclThrift, utility::kDefaultAclTable()));
    }
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentHwAclQualifierTest, AclVlanIDQualifier) {
  auto setup = [=, this]() {
    auto& ensemble = *getAgentEnsemble();
    auto newCfg = initialConfig(ensemble);
    cfg::AclEntry acl{};
    acl.name() = "acl0";
    acl.actionType() = cfg::AclActionType::DENY;
    acl.vlanID() = 2001;
    utility::addAcl(&newCfg, acl, cfg::AclStage::INGRESS);
    applyNewConfig(newCfg);
  };

  auto verify = [=, this]() {
    auto client = getAgentEnsemble()->getHwAgentTestClient(SwitchID(0));
    auto state = getAgentEnsemble()->getProgrammedState();
    EXPECT_EQ(
        client->sync_getAclTableNumAclEntries(utility::kDefaultAclTable()), 1);
    auto acl = utility::getAclEntry(state, "acl0", FLAGS_enable_acl_table_group)
                   ->toThrift();
    EXPECT_TRUE(client->sync_isAclEntrySame(acl, utility::kDefaultAclTable()));
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentHwAclQualifierTest, AclIp4Qualifiers) {
  auto setup = [=, this]() {
    auto& ensemble = *getAgentEnsemble();
    auto newCfg = initialConfig(ensemble);

    cfg::AclEntry acl{};
    acl.name() = "ip4";
    acl.actionType() = cfg::AclActionType::DENY;
    configureIp4QualifiersHelper(&acl);
    utility::addAcl(&newCfg, acl, cfg::AclStage::INGRESS);
    applyNewConfig(newCfg);
  };

  auto verify = [=, this]() {
    auto client = getAgentEnsemble()->getHwAgentTestClient(SwitchID(0));
    auto state = getAgentEnsemble()->getProgrammedState();

    EXPECT_EQ(
        client->sync_getAclTableNumAclEntries(utility::kDefaultAclTable()), 1);
    auto acl = utility::getAclEntry(state, "ip4", FLAGS_enable_acl_table_group)
                   ->toThrift();
    EXPECT_TRUE(client->sync_isAclEntrySame(acl, utility::kDefaultAclTable()));
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentHwAclQualifierTest, AclIp6Qualifiers) {
  auto setup = [=, this]() {
    auto& ensemble = *getAgentEnsemble();
    auto newCfg = initialConfig(ensemble);
    auto l3Asics = ensemble.getL3Asics();
    auto asic = utility::checkSameAndGetAsic(l3Asics);

    cfg::AclEntry acl{};
    acl.name() = "ip6";
    acl.actionType() = cfg::AclActionType::DENY;
    if (asic->isSupported(HwAsic::Feature::ACL_ENTRY_ETHER_TYPE)) {
      acl.etherType() = cfg::EtherType::IPv6;
    }
    configureIp6QualifiersHelper(&acl);
    utility::addAcl(&newCfg, acl, cfg::AclStage::INGRESS);

    applyNewConfig(newCfg);
  };

  auto verify = [=, this]() {
    auto client = getAgentEnsemble()->getHwAgentTestClient(SwitchID(0));
    auto state = getAgentEnsemble()->getProgrammedState();

    EXPECT_EQ(
        client->sync_getAclTableNumAclEntries(utility::kDefaultAclTable()), 1);
    auto acl = utility::getAclEntry(state, "ip6", FLAGS_enable_acl_table_group)
                   ->toThrift();
    EXPECT_TRUE(client->sync_isAclEntrySame(acl, utility::kDefaultAclTable()));
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

// canary on for qualifiers from default to coop
TEST_F(AgentHwAclQualifierTest, AclQualifiersCanaryOn) {
  auto setup = [=, this]() {
    this->aclSetupHelper(true, QualifierType::LOOKUPCLASS_IGNORE, false);
  };

  auto verify = [=, this]() { this->aclVerifyHelper(); };

  auto setupPostWarmboot = [=, this]() {
    this->aclSetupHelper(true, QualifierType::LOOKUPCLASS_IGNORE, true);
  };

  this->verifyAcrossWarmBoots(setup, verify, setupPostWarmboot, []() {});
}

// canary off for qualifiers from coop to default
TEST_F(AgentHwAclQualifierTest, AclQualifiersCanaryOff) {
  auto setup = [=, this]() {
    this->aclSetupHelper(true, QualifierType::LOOKUPCLASS_IGNORE, true);
  };

  auto verify = [=, this]() { this->aclVerifyHelper(); };

  auto setupPostWarmboot = [=, this]() {
    this->aclSetupHelper(true, QualifierType::LOOKUPCLASS_IGNORE, false);
  };

  this->verifyAcrossWarmBoots(setup, verify, setupPostWarmboot, []() {});
}

struct AgentHwAclLookupClassQualifierTest : public AgentHwAclQualifierTest {
  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    auto productionFeatures =
        AgentHwAclQualifierTest::getProductionFeaturesVerified();
    productionFeatures.push_back(
        production_features::ProductionFeature::QUEUE_PER_HOST);
    return productionFeatures;
  }
};

TEST_F(AgentHwAclLookupClassQualifierTest, AclIp4LookupClassL2) {
  auto setup = [=, this]() {
    this->aclSetupHelper(true /* isIpV4 */, QualifierType::LOOKUPCLASS_L2);
  };

  auto verify = [=, this]() { this->aclVerifyHelper(); };

  this->verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentHwAclLookupClassQualifierTest, AclIp4LookupClassNeighbor) {
  auto setup = [=, this]() {
    this->aclSetupHelper(
        true /* isIpV4 */, QualifierType::LOOKUPCLASS_NEIGHBOR);
  };

  auto verify = [=, this]() { this->aclVerifyHelper(); };

  this->verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentHwAclLookupClassQualifierTest, AclIp4LookupClassRoute) {
  auto setup = [=, this]() {
    this->aclSetupHelper(true /* isIpV4 */, QualifierType::LOOKUPCLASS_ROUTE);
  };

  auto verify = [=, this]() { this->aclVerifyHelper(); };

  this->verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentHwAclLookupClassQualifierTest, AclIp6LookupClassL2) {
  auto setup = [=, this]() {
    this->aclSetupHelper(false /* isIpV6 */, QualifierType::LOOKUPCLASS_L2);
  };

  auto verify = [=, this]() { this->aclVerifyHelper(); };

  this->verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentHwAclLookupClassQualifierTest, AclIp6LookupClassNeighbor) {
  auto setup = [=, this]() {
    this->aclSetupHelper(
        false /* isIpV6 */, QualifierType::LOOKUPCLASS_NEIGHBOR);
  };

  auto verify = [=, this]() { this->aclVerifyHelper(); };

  this->verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentHwAclLookupClassQualifierTest, AclIp6LookupClassRoute) {
  auto setup = [=, this]() {
    this->aclSetupHelper(false /* isIpV6 */, QualifierType::LOOKUPCLASS_ROUTE);
  };

  auto verify = [=, this]() { this->aclVerifyHelper(); };

  this->verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
