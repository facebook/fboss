// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/if/gen-cpp2/agent_hw_test_ctrl_types.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/utils/AclTestUtils.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/DscpMarkingUtils.h"
#include "fboss/agent/test/utils/QueuePerHostTestUtils.h"

#include <string>

DECLARE_bool(enable_acl_table_group);

namespace facebook::fboss {

namespace {
const std::int16_t kMaxDefaultAcls = 5;
const std::int16_t kMaxAclTables = 2;
} // namespace

class AgentAclTableGroupTest : public AgentHwTest {
 public:
  std::vector<ProductionFeature> getProductionFeaturesVerified()
      const override {
    return {
        ProductionFeature::MULTI_ACL_TABLE, ProductionFeature::L3_FORWARDING};
  }

  void setCmdLineFlagOverrides() const override {
    AgentHwTest::setCmdLineFlagOverrides();
    FLAGS_enable_acl_table_group = true;
  }

 protected:
  void addAclTable1(cfg::SwitchConfig& cfg) {
    std::vector<cfg::AclTableQualifier> qualifiers = {
        cfg::AclTableQualifier::DSCP};
    std::vector<cfg::AclTableActionType> actions = {
        cfg::AclTableActionType::PACKET_ACTION,
        cfg::AclTableActionType::COUNTER};
#if defined(TAJO_SDK_GTE_24_8_3001)
    qualifiers.push_back(cfg::AclTableQualifier::TTL);
    actions.push_back(cfg::AclTableActionType::COUNTER);
#endif
    utility::addAclTable(
        &cfg, kAclTable1(), 1 /* priority */, actions, qualifiers);
  }

  void addAclTable2(cfg::SwitchConfig& cfg) {
    utility::addAclTable(
        &cfg,
        kAclTable2(),
        2 /* priority */,
        {cfg::AclTableActionType::PACKET_ACTION,
         cfg::AclTableActionType::COUNTER},
        {cfg::AclTableQualifier::TTL, cfg::AclTableQualifier::DSCP});
  }

  void addAclTable1Entry1(
      cfg::SwitchConfig& cfg,
      const std::string& aclTableName) {
    cfg::AclEntry acl1{};
    acl1.name() = kAclTable1Entry1();
    acl1.actionType() = cfg::AclActionType::DENY;
    acl1.dscp() = 0x20;
    utility::addAclEntry(&cfg, acl1, aclTableName);
  }

  void addAclTable2Entry1(cfg::SwitchConfig& cfg) {
    cfg::AclEntry acl2{};
    acl2.name() = kAclTable2Entry1();
    acl2.actionType() = cfg::AclActionType::DENY;
    cfg::Ttl ttl;
    std::tie(*ttl.value(), *ttl.mask()) = std::make_tuple(0x80, 0x80);
    acl2.ttl() = ttl;
    utility::addAclEntry(&cfg, acl2, kAclTable2());
  }

  void verifyAclTableHelper(const std::string& aclTableName, int numEntries) {
    auto& ensemble = *getAgentEnsemble();
    auto switchId = scopeResolver().scope(masterLogicalPortIds()[0]).switchId();
    auto client = ensemble.getHwAgentTestClient(switchId);

    ASSERT_TRUE(client->sync_isAclTableEnabled(aclTableName));
    EXPECT_EQ(client->sync_getAclTableNumAclEntries(aclTableName), numEntries);
  }

  void verifyMultipleTableWithEntriesHelper() {
    auto& ensemble = *getAgentEnsemble();
    auto switchId = scopeResolver().scope(masterLogicalPortIds()[0]).switchId();
    auto client = ensemble.getHwAgentTestClient(switchId);

    // SAI_ACL_STAGE_INGRESS = 0
    ASSERT_TRUE(client->sync_isAclTableGroupEnabled(0));
    verifyAclTableHelper(kAclTable1(), 1);
    verifyAclTableHelper(kAclTable2(), 1);
  }

  std::string kAclTable1() const {
    return "table1";
  }
  std::string kAclTable2() const {
    return "table2";
  }
  std::string kAclTable1Entry1() const {
    return "table1_entry1";
  }
  std::string kAclTable2Entry1() const {
    return "table2_entry1";
  }
  cfg::AclStage kAclStage() const {
    return cfg::AclStage::INGRESS;
  }

  std::string kQphDscpTable() const {
    return "acl-table-qph-dscp";
  }
  std::string kDscpTable() const {
    return "acl-table-dscp";
  }

  std::string kAclTable3() const {
    if (isSupportedOnAllAsics(HwAsic::Feature::ACL_METADATA_QUALIFER)) {
      return kQphDscpTable();
    }
    return kDscpTable();
  }

  std::string kTable1CounterAcl1() const {
    return "table1_counter_acl1";
  }
  std::string kTable1CounterAcl2() const {
    return "table1_counter_acl2";
  }
  std::string kTable1CounterAcl3() const {
    return "table1_counter_acl3";
  }
  std::string kTable1Counter1Name() const {
    return "table1_counter1";
  }
  std::string kTable1Counter2Name() const {
    return "table1_counter2";
  }
  std::string kTable1Counter3Name() const {
    return "table1_counter3";
  }
  std::string kTable2CounterAcl1() const {
    return "table2_counter_acl1";
  }
  std::string kTable2CounterAcl2() const {
    return "table2_counter_acl2";
  }
  std::string kTable2CounterAcl3() const {
    return "table2_counter_acl3";
  }
  std::string kTable2Counter1Name() const {
    return "table2_counter1";
  }
  std::string kTable2Counter2Name() const {
    return "table2_counter2";
  }
  std::string kTable2Counter3Name() const {
    return "table2_counter3";
  }
  std::string kAclTableGroup() const {
    return "Ingress Table Group";
  }

  void addQphDscpAclTable(
      cfg::SwitchConfig* newCfg,
      bool addExtraQualifier = false) {
    std::vector<cfg::AclTableQualifier> qualifiers = {
        cfg::AclTableQualifier::L4_SRC_PORT,
        cfg::AclTableQualifier::L4_DST_PORT,
        cfg::AclTableQualifier::IP_PROTOCOL_NUMBER,
        cfg::AclTableQualifier::ICMPV4_TYPE,
        cfg::AclTableQualifier::ICMPV4_CODE,
        cfg::AclTableQualifier::ICMPV6_TYPE,
        cfg::AclTableQualifier::ICMPV6_CODE,
        cfg::AclTableQualifier::LOOKUP_CLASS_L2,
        cfg::AclTableQualifier::LOOKUP_CLASS_NEIGHBOR,
        cfg::AclTableQualifier::LOOKUP_CLASS_ROUTE,
        cfg::AclTableQualifier::DSCP};

    if (addExtraQualifier) {
      qualifiers.push_back(cfg::AclTableQualifier::OUTER_VLAN);
    }

#if defined(TAJO_SDK_GTE_24_8_3001)
    qualifiers.push_back(cfg::AclTableQualifier::TTL);
#endif

    utility::addAclTable(
        newCfg,
        kQphDscpTable(),
        1 /* priority */,
        {cfg::AclTableActionType::PACKET_ACTION,
         cfg::AclTableActionType::COUNTER,
         cfg::AclTableActionType::SET_TC,
         cfg::AclTableActionType::SET_DSCP},
        qualifiers);
  }

  void addQphDscpAclTableWithEntry(
      cfg::SwitchConfig* newCfg,
      bool addExtraQualifier = false,
      bool withStats = true) {
    addQphDscpAclTable(newCfg, addExtraQualifier);
    utility::addQueuePerHostAclEntry(
        newCfg, kQphDscpTable(), getAgentEnsemble()->isSai());
    if (withStats) {
      utility::addDscpAclEntryWithCounter(
          newCfg, kQphDscpTable(), getAgentEnsemble()->isSai());
    }
  }

  void addDscpAclTable(
      cfg::SwitchConfig* newCfg,
      bool addExtraQualifier = false) {
    std::vector<cfg::AclTableQualifier> qualifiers = {
        cfg::AclTableQualifier::L4_SRC_PORT,
        cfg::AclTableQualifier::L4_DST_PORT,
        cfg::AclTableQualifier::IP_PROTOCOL_NUMBER,
        cfg::AclTableQualifier::IPV6_NEXT_HEADER,
        cfg::AclTableQualifier::ICMPV4_TYPE,
        cfg::AclTableQualifier::ICMPV4_CODE,
        cfg::AclTableQualifier::ICMPV6_TYPE,
        cfg::AclTableQualifier::ICMPV6_CODE,
        cfg::AclTableQualifier::ETHER_TYPE,
        cfg::AclTableQualifier::DSCP};

    if (addExtraQualifier) {
      qualifiers.push_back(cfg::AclTableQualifier::OUTER_VLAN);
    }

#if defined(TAJO_SDK_GTE_24_8_3001)
    qualifiers.push_back(cfg::AclTableQualifier::TTL);
#endif

    utility::addAclTable(
        newCfg,
        kDscpTable(),
        1 /* priority */,
        {cfg::AclTableActionType::PACKET_ACTION,
         cfg::AclTableActionType::COUNTER,
         cfg::AclTableActionType::SET_TC,
         cfg::AclTableActionType::SET_DSCP},
        qualifiers);
  }

  void addDscpAclTableWithEntry(
      cfg::SwitchConfig* newCfg,
      bool addExtraQualifier,
      bool withStats) {
    addDscpAclTable(newCfg, addExtraQualifier);
    cfg::AclEntry dscpAcl;
    dscpAcl.name() = utility::kDscpCounterAclName();
    dscpAcl.actionType() = cfg::AclActionType::PERMIT;
    dscpAcl.dscp() = utility::kIcpDscp();
    utility::addAclEntry(newCfg, dscpAcl, kDscpTable());

    if (withStats) {
      utility::addDscpAclEntryWithCounter(
          newCfg, kDscpTable(), getAgentEnsemble()->isSai());
    }
  }

  void addAclTable3WithEntry(
      cfg::SwitchConfig* newCfg,
      bool addExtraQualifier = false,
      bool withStats = true) {
    if (isSupportedOnAllAsics(HwAsic::Feature::ACL_METADATA_QUALIFER)) {
      addQphDscpAclTableWithEntry(newCfg, addExtraQualifier, withStats);
    } else {
      addDscpAclTableWithEntry(newCfg, addExtraQualifier, withStats);
    }
  }

  void addTwoAclTables(cfg::SwitchConfig* newCfg) {
    utility::addAclTableGroup(newCfg, kAclStage(), kAclTableGroup());
    addAclTable3WithEntry(newCfg);
    utility::addTtlAclTable(newCfg, 2 /* priority */);
    applyNewConfig(*newCfg);
  }

  void deleteAclTable3(cfg::SwitchConfig* newCfg) {
    if (isSupportedOnAllAsics(HwAsic::Feature::ACL_METADATA_QUALIFER)) {
      utility::delAclTable(newCfg, kQphDscpTable());
      utility::deleteQueuePerHostMatchers(newCfg);
      utility::delDscpMatchers(newCfg);
    } else {
      utility::delAclTable(newCfg, kDscpTable());
      utility::delDscpMatchers(newCfg);
    }
    applyNewConfig(*newCfg);
  }

  void deleteTtlAclTable(cfg::SwitchConfig* newCfg) {
    utility::delAclTable(newCfg, utility::getTtlAclTableName());
    utility::deleteTtlCounters(newCfg);
    applyNewConfig(*newCfg);
  }

  enum tableAddType { table1, table2, tableBoth };

  void warmbootSetupHelper(tableAddType tableAdd) {
    auto& ensemble = *getAgentEnsemble();
    auto newCfg = initialConfig(ensemble);

    switch (tableAdd) {
      case tableAddType::table1:
        utility::addAclTableGroup(&newCfg, kAclStage(), kAclTableGroup());
        addAclTable3WithEntry(&newCfg);
        break;
      case tableAddType::table2:
        utility::addAclTableGroup(&newCfg, kAclStage(), kAclTableGroup());
        utility::addTtlAclTable(&newCfg, 2 /* priority */);
        break;
      case tableAddType::tableBoth:
        addTwoAclTables(&newCfg);
        break;
      default:
        throw std::invalid_argument("Invalid Table Add Enum type");
    }
    applyNewConfig(newCfg);
  }

  void addCounterAclToAclTable(
      cfg::SwitchConfig* cfg,
      const std::string& aclTableName,
      const std::string& aclEntryName,
      const std::string& counterName,
      uint8_t dscp,
      bool addVlan = false) {
    std::vector<cfg::CounterType> counterTypes{cfg::CounterType::PACKETS};
    cfg::AclEntry counterAcl;
    counterAcl.name() = aclEntryName;
    counterAcl.actionType() = cfg::AclActionType::PERMIT;
    counterAcl.dscp() = dscp;
    if (addVlan) {
      counterAcl.vlanID() = 2000;
    }
    utility::addAclEntry(cfg, counterAcl, aclTableName);
    utility::addAclStat(cfg, aclEntryName, counterName, counterTypes);
  }

  void addCounterAclsToAclable3(cfg::SwitchConfig* cfg) {
    addCounterAclToAclTable(
        cfg, kAclTable3(), kTable1CounterAcl1(), kTable1Counter1Name(), 1);
    addCounterAclToAclTable(
        cfg, kAclTable3(), kTable1CounterAcl2(), kTable1Counter2Name(), 2);
  }

  void addCounterAclsToTtlTable(cfg::SwitchConfig* cfg) {
    addCounterAclToAclTable(
        cfg,
        utility::getTtlAclTableName(),
        kTable2CounterAcl1(),
        kTable2Counter1Name(),
        3);
    addCounterAclToAclTable(
        cfg,
        utility::getTtlAclTableName(),
        kTable2CounterAcl2(),
        kTable2Counter2Name(),
        4);
  }

  void addDefaultCounterAclsToTable(cfg::SwitchConfig& cfg, bool reverse) {
    for (int table = 1; table <= kMaxAclTables; table++) {
      for (int num = 1; num <= kMaxDefaultAcls; num++) {
        auto aclNum = num;
        if (reverse) {
          aclNum = (kMaxDefaultAcls + 1) - num;
        }
        auto tableName = folly::to<std::string>("table", table);
        auto aclEntryName =
            folly::to<std::string>(tableName, "_counter_acl", aclNum);
        auto counterName =
            folly::to<std::string>(tableName, "_counter", aclNum);
        addCounterAclToAclTable(
            &cfg, tableName, aclEntryName, counterName, aclNum);
      }
    }
    applyNewConfig(cfg);
  }

  void verifyAclEntryTestHelper(int table1EntryCount, int table2EntryCount) {
    auto& ensemble = *getAgentEnsemble();
    auto switchId = scopeResolver().scope(masterLogicalPortIds()[0]).switchId();
    auto client = ensemble.getHwAgentTestClient(switchId);

    ASSERT_TRUE(client->sync_isAclTableEnabled(kAclTable3()));
    ASSERT_TRUE(client->sync_isAclTableEnabled(utility::getTtlAclTableName()));
    EXPECT_EQ(
        client->sync_getAclTableNumAclEntries(kAclTable3()), table1EntryCount);
    EXPECT_EQ(
        client->sync_getAclTableNumAclEntries(utility::getTtlAclTableName()),
        table2EntryCount);
  }

  cfg::SwitchConfig getMultiAclConfig(bool addExtraQualifier = false) {
    auto& ensemble = *getAgentEnsemble();
    auto newCfg = initialConfig(ensemble);
    utility::addAclTableGroup(&newCfg, kAclStage(), kAclTableGroup());
    addAclTable3WithEntry(&newCfg, addExtraQualifier, false);
    utility::addTtlAclTable(&newCfg, 2 /* priority */, addExtraQualifier);
    return newCfg;
  }

  void verifyAclEntryModificationTestHelper(
      bool canaryOn,
      bool addRemoveAclQualifier) {
    ASSERT_TRUE(isSupportedOnAllAsics(HwAsic::Feature::MULTIPLE_ACL_TABLES));

    auto setup = [this]() {
      auto newCfg = getMultiAclConfig(false);
      addCounterAclsToAclable3(&newCfg);
      addCounterAclsToTtlTable(&newCfg);
      applyNewConfig(newCfg);
    };

    auto verify = [=, this]() {
      if (isSupportedOnAllAsics(HwAsic::Feature::ACL_METADATA_QUALIFER)) {
        verifyAclEntryTestHelper(17, 3);
      } else {
        verifyAclEntryTestHelper(3, 3);
      }
    };

    auto setupPostWarmboot = [=, this]() {
      auto newCfg = getMultiAclConfig(addRemoveAclQualifier);
      utility::addDscpAclEntryWithCounter(
          &newCfg, kAclTable3(), getAgentEnsemble()->isSai());
      addCounterAclToAclTable(
          &newCfg,
          utility::getTtlAclTableName(),
          kTable2CounterAcl3(),
          kTable2Counter3Name(),
          5);
      addCounterAclToAclTable(
          &newCfg,
          kAclTable3(),
          kTable1CounterAcl2(),
          kTable1Counter2Name(),
          2);
      addCounterAclToAclTable(
          &newCfg,
          utility::getTtlAclTableName(),
          kTable2CounterAcl1(),
          kTable2Counter1Name(),
          4);
      addCounterAclToAclTable(
          &newCfg,
          kAclTable3(),
          kTable1CounterAcl3(),
          kTable1Counter3Name(),
          3,
          addRemoveAclQualifier);
      applyNewConfig(newCfg);
    };

    auto verifyPostWarmboot = [=, this]() {
      if (isSupportedOnAllAsics(HwAsic::Feature::ACL_METADATA_QUALIFER)) {
        verifyAclEntryTestHelper(38, 3);
      } else {
        verifyAclEntryTestHelper(23, 3);
      }
    };

    if (canaryOn) {
      verifyAcrossWarmBoots(
          setup, verify, setupPostWarmboot, verifyPostWarmboot);
    } else {
      verifyAcrossWarmBoots(
          setupPostWarmboot, verifyPostWarmboot, setup, verify);
    }
  }
};

TEST_F(AgentAclTableGroupTest, SingleAclTableGroup) {
  ASSERT_TRUE(isSupportedOnAllAsics(HwAsic::Feature::MULTIPLE_ACL_TABLES));

  auto setup = [this]() {
    auto& ensemble = *getAgentEnsemble();
    auto newCfg = initialConfig(ensemble);
    utility::addAclTableGroup(&newCfg, kAclStage(), kAclTableGroup());
    applyNewConfig(newCfg);
  };

  auto verify = [=, this]() {
    auto& ensemble = *getAgentEnsemble();
    auto switchId = scopeResolver().scope(masterLogicalPortIds()[0]).switchId();
    auto client = ensemble.getHwAgentTestClient(switchId);
    // SAI_ACL_STAGE_INGRESS = 0
    ASSERT_TRUE(client->sync_isAclTableGroupEnabled(0));
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentAclTableGroupTest, MultipleTablesNoEntries) {
  ASSERT_TRUE(isSupportedOnAllAsics(HwAsic::Feature::MULTIPLE_ACL_TABLES));

  auto setup = [this]() {
    auto& ensemble = *getAgentEnsemble();
    auto newCfg = initialConfig(ensemble);
    utility::addAclTableGroup(&newCfg, kAclStage(), kAclTableGroup());
    addAclTable1(newCfg);
    addAclTable2(newCfg);
    applyNewConfig(newCfg);
  };

  auto verify = [=, this]() {
    auto& ensemble = *getAgentEnsemble();
    auto switchId = scopeResolver().scope(masterLogicalPortIds()[0]).switchId();
    auto client = ensemble.getHwAgentTestClient(switchId);
    ASSERT_TRUE(client->sync_isAclTableGroupEnabled(0));
    ASSERT_TRUE(client->sync_isAclTableEnabled(kAclTable1()));
    ASSERT_TRUE(client->sync_isAclTableEnabled(kAclTable2()));
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentAclTableGroupTest, MultipleTablesWithEntries) {
  ASSERT_TRUE(isSupportedOnAllAsics(HwAsic::Feature::MULTIPLE_ACL_TABLES));

  auto setup = [this]() {
    auto& ensemble = *getAgentEnsemble();
    auto newCfg = initialConfig(ensemble);
    utility::addAclTableGroup(&newCfg, kAclStage(), kAclTableGroup());
    addAclTable1(newCfg);
    addAclTable1Entry1(newCfg, kAclTable1());
    addAclTable2(newCfg);
    addAclTable2Entry1(newCfg);
    applyNewConfig(newCfg);
  };

  auto verify = [=, this]() { verifyMultipleTableWithEntriesHelper(); };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentAclTableGroupTest, AddTablesThenEntries) {
  ASSERT_TRUE(isSupportedOnAllAsics(HwAsic::Feature::MULTIPLE_ACL_TABLES));

  auto setup = [this]() {
    auto& ensemble = *getAgentEnsemble();
    auto newCfg = initialConfig(ensemble);
    utility::addAclTableGroup(&newCfg, kAclStage(), kAclTableGroup());
    addAclTable1(newCfg);
    addAclTable2(newCfg);
    applyNewConfig(newCfg);
    addAclTable1Entry1(newCfg, kAclTable1());
    addAclTable2Entry1(newCfg);
    applyNewConfig(newCfg);
  };

  auto verify = [=, this]() { verifyMultipleTableWithEntriesHelper(); };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentAclTableGroupTest, RemoveAclTable) {
  ASSERT_TRUE(isSupportedOnAllAsics(HwAsic::Feature::MULTIPLE_ACL_TABLES));

  auto setup = [this]() {
    auto& ensemble = *getAgentEnsemble();
    auto newCfg = initialConfig(ensemble);
    utility::addAclTableGroup(&newCfg, kAclStage(), kAclTableGroup());
    addAclTable1(newCfg);
    addAclTable1Entry1(newCfg, kAclTable1());
    addAclTable2(newCfg);
    applyNewConfig(newCfg);
    utility::delAclTable(&newCfg, kAclTable1());
    utility::delAclTable(&newCfg, kAclTable2());
    applyNewConfig(newCfg);
  };

  auto verify = [=, this]() {
    auto& ensemble = *getAgentEnsemble();
    auto switchId = scopeResolver().scope(masterLogicalPortIds()[0]).switchId();
    auto client = ensemble.getHwAgentTestClient(switchId);
    ASSERT_TRUE(client->sync_isAclTableGroupEnabled(
        static_cast<int32_t>(cfg::AclStage::INGRESS)));
    ASSERT_FALSE(client->sync_isAclTableEnabled(kAclTable1()));
    ASSERT_FALSE(client->sync_isAclTableEnabled(kAclTable2()));
  };

  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
