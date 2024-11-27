/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/sai/switch/SaiAclTableGroupManager.h"
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwTest.h"
#include "fboss/agent/hw/test/HwTestAclUtils.h"
#include "fboss/agent/test/utils/DscpMarkingUtils.h"
#include "fboss/agent/test/utils/QueuePerHostTestUtils.h"

#include "fboss/agent/state/SwitchState.h"

#include <string>

DECLARE_bool(enable_acl_table_group);

using namespace facebook::fboss;

namespace {

const std::int16_t kMaxDefaultAcls = 5;
const std::int16_t kMaxAclTables = 2;

bool isAclTableGroupEnabled(
    const HwSwitch* hwSwitch,
    sai_acl_stage_t aclStage) {
  const auto& aclTableGroupManager = static_cast<const SaiSwitch*>(hwSwitch)
                                         ->managerTable()
                                         ->aclTableGroupManager();

  auto aclTableGroupHandle =
      aclTableGroupManager.getAclTableGroupHandle(aclStage);
  return aclTableGroupHandle != nullptr;
}

} // namespace

namespace facebook::fboss {

class SaiAclTableGroupTest : public HwTest {
 protected:
  void SetUp() override {
    FLAGS_enable_acl_table_group = true;
    HwTest::SetUp();
  }

  cfg::SwitchConfig initialConfig() const {
    return utility::onePortPerInterfaceConfig(
        getHwSwitch(),
        masterLogicalPortIds(),
        getAsic()->desiredLoopbackModes());
  }

  bool isSupported() const {
    bool multipleAclTableSupport =
        HwTest::isSupported(HwAsic::Feature::MULTIPLE_ACL_TABLES);
#if defined(TAJO_SDK_VERSION_1_42_8)
    multipleAclTableSupport = false;
#endif
    return multipleAclTableSupport;
  }

  void addAclTable1(cfg::SwitchConfig& cfg) {
    std::vector<cfg::AclTableQualifier> qualifiers = {
        cfg::AclTableQualifier::DSCP};
    std::vector<cfg::AclTableActionType> actions = {
        cfg::AclTableActionType::PACKET_ACTION,
        cfg::AclTableActionType::COUNTER};
#if defined(TAJO_SDK_GTE_24_4_90)
    qualifiers.push_back(cfg::AclTableQualifier::TTL);
    actions.push_back(cfg::AclTableActionType::COUNTER);
#endif
    // table1 with dscp matcher ACL entry
    utility::addAclTable(
        &cfg, kAclTable1(), 1 /* priority */, actions, qualifiers);
  }

  void addAclTable2(cfg::SwitchConfig& cfg) {
    // table2 with ttl matcher ACL entry
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
    auto* acl1 = utility::addAcl(
        &cfg, kAclTable1Entry1(), cfg::AclActionType::DENY, aclTableName);
    acl1->dscp() = 0x20;
  }

  void addAclTable2Entry1(cfg::SwitchConfig& cfg) {
    auto* acl2 = utility::addAcl(
        &cfg, kAclTable2Entry1(), cfg::AclActionType::DENY, kAclTable2());
    cfg::Ttl ttl;
    std::tie(*ttl.value(), *ttl.mask()) = std::make_tuple(0x80, 0x80);
    acl2->ttl() = ttl;
  }

  void verifyAclTableHelper(
      const std::string& aclTableName,
      const std::string& aclName,
      int numEntries) {
    ASSERT_TRUE(utility::isAclTableEnabled(getHwSwitch(), aclTableName));
    EXPECT_EQ(
        utility::getAclTableNumAclEntries(getHwSwitch(), aclTableName),
        numEntries);
    utility::checkSwHwAclMatch(
        getHwSwitch(),
        getProgrammedState(),
        aclName,
        kAclStage(),
        aclTableName);
  }

  void verifyMultipleTableWithEntriesHelper() {
    ASSERT_TRUE(isAclTableGroupEnabled(getHwSwitch(), SAI_ACL_STAGE_INGRESS));
    verifyAclTableHelper(kAclTable1(), kAclTable1Entry1(), 1);
    verifyAclTableHelper(kAclTable2(), kAclTable2Entry1(), 1);
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
    return "qph_dscp_table";
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
        cfg::AclTableQualifier::IP_PROTOCOL,
        cfg::AclTableQualifier::ICMPV4_TYPE,
        cfg::AclTableQualifier::ICMPV4_CODE,
        cfg::AclTableQualifier::ICMPV6_TYPE,
        cfg::AclTableQualifier::ICMPV6_CODE,
        cfg::AclTableQualifier::LOOKUP_CLASS_L2,
        cfg::AclTableQualifier::LOOKUP_CLASS_NEIGHBOR,
        cfg::AclTableQualifier::LOOKUP_CLASS_ROUTE,
        cfg::AclTableQualifier::DSCP};

    if (addExtraQualifier) {
      /*
       * This field is used to modify the properties of the ACL table.
       * This will force a recreate of the acl table during delta processing.
       */
      qualifiers.push_back(cfg::AclTableQualifier::OUTER_VLAN);
    }

    // Table 1: For QPH and Dscp Acl.
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

  void addQphDscpAclTableWithEntry(cfg::SwitchConfig* newCfg) {
    // Table 1: For QPH and Dscp Acl.
    addQphDscpAclTable(newCfg);

    utility::addQueuePerHostAclEntry(
        newCfg, kQphDscpTable(), getHwSwitchEnsemble()->isSai());
    utility::addDscpAclEntryWithCounter(
        newCfg, kQphDscpTable(), getHwSwitchEnsemble()->isSai());
  }

  void addTwoAclTables(cfg::SwitchConfig* newCfg) {
    utility::addAclTableGroup(newCfg, kAclStage(), kAclTableGroup());

    // Table 1: Create QPH and DSCP ACLs in the same table.
    addQphDscpAclTableWithEntry(newCfg);

    // Table 2: Create TTL acl Table follwed by entry.
    // This utlity call adds the TTL Acl entry as well.
    utility::addTtlAclTable(newCfg, 2 /* priority */);
    applyNewConfig(*newCfg);
  }

  // Delete the QphDscpAclTable
  void deleteQphDscpAclTable(cfg::SwitchConfig* newCfg) {
    utility::delAclTable(newCfg, kQphDscpTable());
    utility::deleteQueuePerHostMatchers(newCfg);
    utility::delDscpMatchers(newCfg);
    applyNewConfig(*newCfg);
  }

  // Delete the TtlAclTable
  void deleteTtlAclTable(cfg::SwitchConfig* newCfg) {
    utility::delAclTable(newCfg, utility::getTtlAclTableName());
    utility::deleteTtlCounters(newCfg);
    applyNewConfig(*newCfg);
  }

  enum tableAddType {
    table1,
    table2,
    tableBoth,
  };

  void warmbootSetupHelper(tableAddType tableAdd) {
    auto newCfg = initialConfig();

    switch (tableAdd) {
      case tableAddType::table1:
        utility::addAclTableGroup(&newCfg, kAclStage(), kAclTableGroup());
        // Add Table 1: Create QPH and DSCP ACLs in the same table.
        addQphDscpAclTableWithEntry(&newCfg);
        break;
      case tableAddType::table2:
        utility::addAclTableGroup(&newCfg, kAclStage(), kAclTableGroup());
        // Add Table 2: TtlTable
        utility::addTtlAclTable(&newCfg, 2 /* priority */);
        break;
      case tableAddType::tableBoth:
        addTwoAclTables(&newCfg);
        break;
      default:
        throw("Invalid Table Add Enum type");
        break;
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
    auto* counterAcl = utility::addAcl(
        cfg, aclEntryName, cfg::AclActionType::PERMIT, aclTableName);
    utility::addAclStat(cfg, aclEntryName, counterName, counterTypes);
    counterAcl->dscp() = dscp;
    if (addVlan) {
      counterAcl->vlanID() = 2000;
    }
  }

  void addCounterAclsToQphTable(cfg::SwitchConfig* cfg) {
    addCounterAclToAclTable(
        cfg, kQphDscpTable(), kTable1CounterAcl1(), kTable1Counter1Name(), 1);
    addCounterAclToAclTable(
        cfg, kQphDscpTable(), kTable1CounterAcl2(), kTable1Counter2Name(), 2);
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
          // Need to reverse the order of acls
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
  void verifyAclStatCount(
      const std::string& aclTableName,
      int aclCount,
      int statCount,
      int counterCount) {
    utility::checkAclEntryAndStatCount(
        getHwSwitch(), aclCount, statCount, counterCount, aclTableName);
  }

  /*
   * Helper API to configure 2 acl tables with the below entries
   * Table 1: QPH, DSCP table. Table 1 entries: QPH acls
   * Table 2: TtlTable. Table 2 entries: Ttl acls
   * If dscpqualifier flag is set, adds dscp qualifier to table 1.
   */
  cfg::SwitchConfig getMultiAclConfig(bool addExtraQualifier = false) {
    auto newCfg = initialConfig();

    utility::addAclTableGroup(&newCfg, kAclStage(), kAclTableGroup());
    addQphDscpAclTable(&newCfg, addExtraQualifier);
    utility::addQueuePerHostAclEntry(
        &newCfg, kQphDscpTable(), getHwSwitchEnsemble()->isSai());
    utility::addTtlAclTable(&newCfg, 2 /* priority */, addExtraQualifier);

    return newCfg;
  }

  void verifyAclEntryTestHelper(int table1EntryCount, int table2EntryCount) {
    ASSERT_TRUE(utility::isAclTableEnabled(getHwSwitch(), kQphDscpTable()));
    ASSERT_TRUE(utility::isAclTableEnabled(
        getHwSwitch(), utility::getTtlAclTableName()));
    EXPECT_EQ(
        utility::getAclTableNumAclEntries(getHwSwitch(), kQphDscpTable()),
        table1EntryCount);
    EXPECT_EQ(
        utility::getAclTableNumAclEntries(
            getHwSwitch(), utility::getTtlAclTableName()),
        table2EntryCount);
  }

  // This helper has dual functionality and performs canary on/off
  //
  // The first argument indicates canary on or off. canaryOn=True would add an
  // ACL entry (and qualifier depending on the 2nd argument) post warmboot
  // and vice-versa for false
  //
  // The 2nd argument adds an ACL qualifier when moving between canary phases
  //  - When canaryOn, qualifier=true would add a qualifier post warmboot
  //  - when canaryOff, qualifier=true would add a qualifier pre warmboot
  void verifyAclEntryModificationTestHelper(
      bool canaryOn,
      bool addRemoveAclQualifier) {
    ASSERT_TRUE(isSupported());

    auto setup = [this]() {
      /*
       * Retrieve the config for 2 acl tables with QPH acls added to table 1
       * and TTL acls added to table 2.
       */
      auto newCfg = getMultiAclConfig(false);
      // Add 2 counter acls to table 1 (QPH table) to test remove and changed
      // acl functionality
      addCounterAclsToQphTable(&newCfg);
      // Add 2 counter acls to table 2 (TTL table) to test remove and changed
      // acl functionality
      addCounterAclsToTtlTable(&newCfg);
      applyNewConfig(newCfg);
    };

    auto verify = [=, this]() {
      verifyAclEntryTestHelper(
          17 /* table1EntryCount*/, 3 /*table2EntryCount*/);
    };

    auto setupPostWarmboot = [=, this]() {
      auto newCfg = getMultiAclConfig(addRemoveAclQualifier);
      // Add Dscp acl to table 1 post warmboot
      utility::addDscpAclEntryWithCounter(
          &newCfg, kQphDscpTable(), this->getHwSwitchEnsemble()->isSai());
      // Add a new counter acl to table 2 post warmboot
      addCounterAclToAclTable(
          &newCfg,
          utility::getTtlAclTableName(),
          kTable2CounterAcl3(),
          kTable2Counter3Name(),
          5);

      /*
       * Add one counter to table 1 and 2 in a new position so the priority is
       * changed triggering changed acl entries code path
       */
      addCounterAclToAclTable(
          &newCfg,
          kQphDscpTable(),
          kTable1CounterAcl2(),
          kTable1Counter2Name(),
          2);
      addCounterAclToAclTable(
          &newCfg,
          utility::getTtlAclTableName(),
          kTable2CounterAcl1(),
          kTable2Counter1Name(),
          4);
      // add an ACL entry in table 1 using the new qualifier
      addCounterAclToAclTable(
          &newCfg,
          kQphDscpTable(),
          kTable1CounterAcl3(),
          kTable1Counter3Name(),
          3,
          addRemoveAclQualifier /* addVlan */);
      applyNewConfig(newCfg);
    };

    auto verifyPostWarmboot = [=, this]() {
      verifyAclEntryTestHelper(
          38 /* table1EntryCount*/, 3 /* table1EntryCount*/);
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

TEST_F(SaiAclTableGroupTest, SingleAclTableGroup) {
  ASSERT_TRUE(isSupported());

  auto setup = [this]() {
    auto newCfg = initialConfig();

    utility::addAclTableGroup(&newCfg, kAclStage(), kAclTableGroup());

    applyNewConfig(newCfg);
  };

  auto verify = [=, this]() {
    ASSERT_TRUE(isAclTableGroupEnabled(getHwSwitch(), SAI_ACL_STAGE_INGRESS));
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(SaiAclTableGroupTest, MultipleTablesNoEntries) {
  ASSERT_TRUE(isSupported());

  auto setup = [this]() {
    auto newCfg = initialConfig();

    utility::addAclTableGroup(&newCfg, kAclStage(), kAclTableGroup());
    addAclTable1(newCfg);
    addAclTable2(newCfg);

    applyNewConfig(newCfg);
  };

  auto verify = [=, this]() {
    ASSERT_TRUE(isAclTableGroupEnabled(getHwSwitch(), SAI_ACL_STAGE_INGRESS));
    ASSERT_TRUE(utility::isAclTableEnabled(getHwSwitch(), kAclTable1()));
    ASSERT_TRUE(utility::isAclTableEnabled(getHwSwitch(), kAclTable2()));
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(SaiAclTableGroupTest, MultipleTablesWithEntries) {
  ASSERT_TRUE(isSupported());

  auto setup = [this]() {
    auto newCfg = initialConfig();

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

TEST_F(SaiAclTableGroupTest, AddTablesThenEntries) {
  ASSERT_TRUE(isSupported());

  auto setup = [this]() {
    auto newCfg = initialConfig();

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

TEST_F(SaiAclTableGroupTest, RemoveAclTable) {
  ASSERT_TRUE(isSupported());

  auto setup = [this]() {
    auto newCfg = initialConfig();

    utility::addAclTableGroup(&newCfg, kAclStage(), kAclTableGroup());
    addAclTable1(newCfg);
    addAclTable1Entry1(newCfg, kAclTable1());
    addAclTable2(newCfg);
    applyNewConfig(newCfg);

    // Remove one Table with entries, and another with no entries
    utility::delAclTable(&newCfg, kAclTable1());
    utility::delAclTable(&newCfg, kAclTable2());
    applyNewConfig(newCfg);
  };

  auto verify = [=, this]() {
    ASSERT_TRUE(isAclTableGroupEnabled(getHwSwitch(), SAI_ACL_STAGE_INGRESS));
    ASSERT_FALSE(utility::isAclTableEnabled(getHwSwitch(), kAclTable1()));
    ASSERT_FALSE(utility::isAclTableEnabled(getHwSwitch(), kAclTable2()));
  };

  verifyAcrossWarmBoots(setup, verify);
}

/*
 * The below testcases vary from the above in the sense that they add the QPH
 * and DSCP ACLs in the Table 1 and TTL ACL in Table 2 (production use case) and
 * then tests deletion and addition of these tables in the config
 */
TEST_F(SaiAclTableGroupTest, AddTwoTablesDeleteFirst) {
  ASSERT_TRUE(isSupported());

  auto setup = [this]() {
    auto newCfg = initialConfig();
    addTwoAclTables(&newCfg);
    // Delete the First table
    deleteQphDscpAclTable(&newCfg);
  };

  auto verify = [=, this]() {
    ASSERT_TRUE(isAclTableGroupEnabled(getHwSwitch(), SAI_ACL_STAGE_INGRESS));
    ASSERT_FALSE(utility::isAclTableEnabled(getHwSwitch(), kQphDscpTable()));
    ASSERT_TRUE(utility::isAclTableEnabled(
        getHwSwitch(), utility::getTtlAclTableName()));
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(SaiAclTableGroupTest, AddTwoTablesDeleteSecond) {
  ASSERT_TRUE(isSupported());

  auto setup = [this]() {
    auto newCfg = initialConfig();
    addTwoAclTables(&newCfg);
    // Delete the Second Table
    deleteTtlAclTable(&newCfg);
  };

  auto verify = [=, this]() {
    ASSERT_TRUE(isAclTableGroupEnabled(getHwSwitch(), SAI_ACL_STAGE_INGRESS));
    ASSERT_TRUE(utility::isAclTableEnabled(getHwSwitch(), kQphDscpTable()));
    ASSERT_FALSE(utility::isAclTableEnabled(
        getHwSwitch(), utility::getTtlAclTableName()));
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(SaiAclTableGroupTest, AddTwoTablesDeleteAddFirst) {
  ASSERT_TRUE(isSupported());

  auto setup = [this]() {
    auto newCfg = initialConfig();
    addTwoAclTables(&newCfg);
    // Delete and Readd the first Acl Table
    deleteQphDscpAclTable(&newCfg);
    addQphDscpAclTableWithEntry(&newCfg);
    applyNewConfig(newCfg);
  };

  auto verify = [=, this]() {
    ASSERT_TRUE(isAclTableGroupEnabled(getHwSwitch(), SAI_ACL_STAGE_INGRESS));
    ASSERT_TRUE(utility::isAclTableEnabled(getHwSwitch(), kQphDscpTable()));
    ASSERT_TRUE(utility::isAclTableEnabled(
        getHwSwitch(), utility::getTtlAclTableName()));
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(SaiAclTableGroupTest, AddTwoTablesDeleteAddSecond) {
  ASSERT_TRUE(isSupported());

  auto setup = [this]() {
    auto newCfg = initialConfig();
    addTwoAclTables(&newCfg);
    // Delete and Readd the second Acl Table
    deleteTtlAclTable(&newCfg);
    utility::addTtlAclTable(&newCfg, 2 /* priority */);
    applyNewConfig(newCfg);
  };

  auto verify = [=, this]() {
    ASSERT_TRUE(isAclTableGroupEnabled(getHwSwitch(), SAI_ACL_STAGE_INGRESS));
    ASSERT_TRUE(utility::isAclTableEnabled(getHwSwitch(), kQphDscpTable()));
    ASSERT_TRUE(utility::isAclTableEnabled(
        getHwSwitch(), utility::getTtlAclTableName()));
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(SaiAclTableGroupTest, AddFirstTableAfterWarmboot) {
  ASSERT_TRUE(isSupported());

  auto setup = [this]() { warmbootSetupHelper(tableAddType::table2); };

  auto setupPostWarmboot = [=, this]() {
    warmbootSetupHelper(tableAddType::tableBoth);
  };

  auto verifyPostWarmboot = [=, this]() {
    ASSERT_TRUE(isAclTableGroupEnabled(getHwSwitch(), SAI_ACL_STAGE_INGRESS));
    ASSERT_TRUE(utility::isAclTableEnabled(getHwSwitch(), kQphDscpTable()));
    ASSERT_TRUE(utility::isAclTableEnabled(
        getHwSwitch(), utility::getTtlAclTableName()));
  };

  verifyAcrossWarmBoots(setup, []() {}, setupPostWarmboot, verifyPostWarmboot);
}

TEST_F(SaiAclTableGroupTest, AddSecondTableAfterWarmboot) {
  ASSERT_TRUE(isSupported());

  auto setup = [this]() { warmbootSetupHelper(tableAddType::table1); };

  auto setupPostWarmboot = [=, this]() {
    warmbootSetupHelper(tableAddType::tableBoth);
  };

  auto verifyPostWarmboot = [=, this]() {
    ASSERT_TRUE(isAclTableGroupEnabled(getHwSwitch(), SAI_ACL_STAGE_INGRESS));
    ASSERT_TRUE(utility::isAclTableEnabled(getHwSwitch(), kQphDscpTable()));
    ASSERT_TRUE(utility::isAclTableEnabled(
        getHwSwitch(), utility::getTtlAclTableName()));
  };

  verifyAcrossWarmBoots(setup, []() {}, setupPostWarmboot, verifyPostWarmboot);
}

TEST_F(SaiAclTableGroupTest, DeleteFirstTableAfterWarmboot) {
  ASSERT_TRUE(isSupported());

  auto setup = [this]() { warmbootSetupHelper(tableAddType::tableBoth); };

  auto setupPostWarmboot = [=, this]() {
    warmbootSetupHelper(tableAddType::table2);
  };

  auto verifyPostWarmboot = [=, this]() {
    ASSERT_TRUE(isAclTableGroupEnabled(getHwSwitch(), SAI_ACL_STAGE_INGRESS));
    ASSERT_FALSE(utility::isAclTableEnabled(getHwSwitch(), kQphDscpTable()));
    ASSERT_TRUE(utility::isAclTableEnabled(
        getHwSwitch(), utility::getTtlAclTableName()));
  };

  verifyAcrossWarmBoots(setup, []() {}, setupPostWarmboot, verifyPostWarmboot);
}

TEST_F(SaiAclTableGroupTest, DeleteSecondTableAfterWarmboot) {
  ASSERT_TRUE(isSupported());

  auto setup = [this]() { warmbootSetupHelper(tableAddType::tableBoth); };

  auto setupPostWarmboot = [=, this]() {
    warmbootSetupHelper(tableAddType::table1);
  };

  auto verifyPostWarmboot = [=, this]() {
    ASSERT_TRUE(isAclTableGroupEnabled(getHwSwitch(), SAI_ACL_STAGE_INGRESS));
    ASSERT_TRUE(utility::isAclTableEnabled(getHwSwitch(), kQphDscpTable()));
    ASSERT_FALSE(utility::isAclTableEnabled(
        getHwSwitch(), utility::getTtlAclTableName()));
  };

  verifyAcrossWarmBoots(setup, []() {}, setupPostWarmboot, verifyPostWarmboot);
}

TEST_F(SaiAclTableGroupTest, TestAclTableGroupRoundtrip) {
  ASSERT_TRUE(isSupported());
  /*
   * Create ACL table group in the same format as the current agent config.
   * This will allow us to test warmboot roundtrip tests from and to prod
   * while running HWemtpytest inbetween
   */

  auto setup = [this]() {
    auto newCfg = initialConfig();

    utility::addAclTableGroup(
        &newCfg,
        kAclStage(),
        cfg::switch_config_constants::DEFAULT_INGRESS_ACL_TABLE_GROUP());
    utility::addDefaultAclTable(newCfg);
    applyNewConfig(newCfg);

    addAclTable1Entry1(newCfg, utility::kDefaultAclTable());
    applyNewConfig(newCfg);
  };

  auto verify = [=, this]() {
    verifyAclTableHelper(utility::kDefaultAclTable(), kAclTable1Entry1(), 1);
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(SaiAclTableGroupTest, RepositionAclEntriesPostWarmboot) {
  ASSERT_TRUE(isSupported());

  auto setup = [this]() {
    auto newCfg = initialConfig();

    utility::addAclTableGroup(&newCfg, kAclStage(), kAclTableGroup());
    addAclTable1(newCfg);
    addAclTable2(newCfg);
    addDefaultCounterAclsToTable(newCfg, false);
  };

  auto verify = [=, this]() {
    verifyAclStatCount(
        kAclTable1(),
        /*ACLs*/ kMaxDefaultAcls,
        /*stats*/ kMaxDefaultAcls,
        /*counters*/ kMaxDefaultAcls);
    verifyAclStatCount(
        kAclTable2(), kMaxDefaultAcls, kMaxDefaultAcls, kMaxDefaultAcls);
  };

  auto setupPostWarmboot = [=, this]() {
    auto newCfg = initialConfig();

    utility::addAclTableGroup(&newCfg, kAclStage(), kAclTableGroup());
    addAclTable1(newCfg);
    addAclTable2(newCfg);
    addDefaultCounterAclsToTable(newCfg, true);
  };

  auto verifyPostWarmboot = [=, this]() {
    verifyAclStatCount(
        kAclTable1(),
        /*ACLs*/ kMaxDefaultAcls,
        /*stats*/ kMaxDefaultAcls,
        /*counters*/ kMaxDefaultAcls);
    verifyAclStatCount(
        kAclTable2(), kMaxDefaultAcls, kMaxDefaultAcls, kMaxDefaultAcls);
  };
  verifyAcrossWarmBoots(setup, verify, setupPostWarmboot, verifyPostWarmboot);
}

/*
 * This tests the ability to change acl table properties and adding acl entries
 * post warmboot. Add Acl table 1 with all the qualifiers, qph acl entries
 * and acl table 2 with ttl. On warmboot, add dscp acl entries to table 1.
 * Verify that all entries are present.
 */
TEST_F(SaiAclTableGroupTest, AddAclEntriesToAclTablesPostWarmboot) {
  verifyAclEntryModificationTestHelper(true, false);
}

/*
 * This tests the ability to change acl table properties and adding acl entries
 * post warmboot. Add Acl table 1 with qph qualifiers and acl entries and acl
 * table 2 with ttl on warmboot, add DSCP qualifier and dscp acl entries to
 * table 1. Verify that all entries are present.
 */
TEST_F(
    SaiAclTableGroupTest,
    AddAclEntriesAndQualifiersToAclTablesPostWarmboot) {
  verifyAclEntryModificationTestHelper(true, true);
}

// only ACL entry removed, no change to table
TEST_F(SaiAclTableGroupTest, RemoveAclEntriesFromAclTablesPostWarmboot) {
  verifyAclEntryModificationTestHelper(false, false);
}

// ACL entry is removed and table is modified
TEST_F(
    SaiAclTableGroupTest,
    RemoveAclEntriesAndQualifiersFromAclTablesPostWarmboot) {
  verifyAclEntryModificationTestHelper(false, true);
}
} // namespace facebook::fboss
