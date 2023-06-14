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
#include "fboss/agent/hw/test/dataplane_tests/HwTestDscpMarkingUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwTestQueuePerHostUtils.h"

#include "fboss/agent/state/SwitchState.h"

#include <string>

DECLARE_bool(enable_acl_table_group);

using namespace facebook::fboss;

namespace {

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
#if defined(TAJO_SDK_VERSION_1_42_1) || defined(TAJO_SDK_VERSION_1_42_8)
    multipleAclTableSupport = false;
#endif
    return multipleAclTableSupport;
  }

  void addDefaultAclTable(cfg::SwitchConfig& cfg) {
    /* Create default ACL table similar to whats being done in Agent today */
    std::vector<cfg::AclTableQualifier> qualifiers = {};
    std::vector<cfg::AclTableActionType> actions = {};
    utility::addAclTable(
        &cfg, kDefaultAclTable(), 0 /* priority */, actions, qualifiers);
  }

  void addAclTable1(cfg::SwitchConfig& cfg) {
    std::vector<cfg::AclTableQualifier> qualifiers = {
        cfg::AclTableQualifier::DSCP};
    std::vector<cfg::AclTableActionType> actions = {
        cfg::AclTableActionType::PACKET_ACTION};
#if defined(TAJO_SDK_VERSION_1_62_0)
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
        {cfg::AclTableQualifier::TTL});
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

  std::string kDefaultAclTable() const {
    return "AclTable1";
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

  void addQphDscpAclTable(cfg::SwitchConfig* newCfg) {
    // Table 1: For QPH and Dscp Acl.
    utility::addAclTable(
        newCfg,
        kQphDscpTable(),
        1 /* priority */,
        {cfg::AclTableActionType::PACKET_ACTION,
         cfg::AclTableActionType::COUNTER,
         cfg::AclTableActionType::SET_TC,
         cfg::AclTableActionType::SET_DSCP},
        {cfg::AclTableQualifier::L4_SRC_PORT,
         cfg::AclTableQualifier::L4_DST_PORT,
         cfg::AclTableQualifier::IP_PROTOCOL,
         cfg::AclTableQualifier::ICMPV4_TYPE,
         cfg::AclTableQualifier::ICMPV4_CODE,
         cfg::AclTableQualifier::ICMPV6_TYPE,
         cfg::AclTableQualifier::ICMPV6_CODE,
         cfg::AclTableQualifier::DSCP,
         cfg::AclTableQualifier::LOOKUP_CLASS_L2,
         cfg::AclTableQualifier::LOOKUP_CLASS_NEIGHBOR,
         cfg::AclTableQualifier::LOOKUP_CLASS_ROUTE});

    utility::addQueuePerHostAclEntry(newCfg, kQphDscpTable());
    utility::addDscpAclEntryWithCounter(newCfg, kQphDscpTable(), getAsic());
  }

  void addTwoAclTables(cfg::SwitchConfig* newCfg) {
    utility::addAclTableGroup(newCfg, kAclStage(), "Ingress Table Group");

    // Table 1: Create QPH and DSCP ACLs in the same table.
    addQphDscpAclTable(newCfg);

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
        utility::addAclTableGroup(&newCfg, kAclStage(), "Ingress Table Group");
        // Add Table 1: Create QPH and DSCP ACLs in the same table.
        addQphDscpAclTable(&newCfg);
        break;
      case tableAddType::table2:
        utility::addAclTableGroup(&newCfg, kAclStage(), "Ingress Table Group");
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
};

TEST_F(SaiAclTableGroupTest, SingleAclTableGroup) {
  ASSERT_TRUE(isSupported());

  auto setup = [this]() {
    auto newCfg = initialConfig();

    utility::addAclTableGroup(&newCfg, kAclStage(), "Ingress Table Group");

    applyNewConfig(newCfg);
  };

  auto verify = [=]() {
    ASSERT_TRUE(isAclTableGroupEnabled(getHwSwitch(), SAI_ACL_STAGE_INGRESS));
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(SaiAclTableGroupTest, MultipleTablesNoEntries) {
  ASSERT_TRUE(isSupported());

  auto setup = [this]() {
    auto newCfg = initialConfig();

    utility::addAclTableGroup(&newCfg, kAclStage(), "Ingress Table Group");
    addAclTable1(newCfg);
    addAclTable2(newCfg);

    applyNewConfig(newCfg);
  };

  auto verify = [=]() {
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

    utility::addAclTableGroup(&newCfg, kAclStage(), "Ingress Table Group");
    addAclTable1(newCfg);
    addAclTable1Entry1(newCfg, kAclTable1());
    addAclTable2(newCfg);
    addAclTable2Entry1(newCfg);

    applyNewConfig(newCfg);
  };

  auto verify = [=]() { verifyMultipleTableWithEntriesHelper(); };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(SaiAclTableGroupTest, AddTablesThenEntries) {
  ASSERT_TRUE(isSupported());

  auto setup = [this]() {
    auto newCfg = initialConfig();

    utility::addAclTableGroup(&newCfg, kAclStage(), "Ingress Table Group");
    addAclTable1(newCfg);
    addAclTable2(newCfg);
    applyNewConfig(newCfg);

    addAclTable1Entry1(newCfg, kAclTable1());
    addAclTable2Entry1(newCfg);
    applyNewConfig(newCfg);
  };

  auto verify = [=]() { verifyMultipleTableWithEntriesHelper(); };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(SaiAclTableGroupTest, RemoveAclTable) {
  ASSERT_TRUE(isSupported());

  auto setup = [this]() {
    auto newCfg = initialConfig();

    utility::addAclTableGroup(&newCfg, kAclStage(), "Ingress Table Group");
    addAclTable1(newCfg);
    addAclTable1Entry1(newCfg, kAclTable1());
    addAclTable2(newCfg);
    applyNewConfig(newCfg);

    // Remove one Table with entries, and another with no entries
    utility::delAclTable(&newCfg, kAclTable1());
    utility::delAclTable(&newCfg, kAclTable2());
    applyNewConfig(newCfg);
  };

  auto verify = [=]() {
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

  auto verify = [=]() {
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

  auto verify = [=]() {
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
    addQphDscpAclTable(&newCfg);
    applyNewConfig(newCfg);
  };

  auto verify = [=]() {
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

  auto verify = [=]() {
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

  auto setupPostWarmboot = [=]() {
    warmbootSetupHelper(tableAddType::tableBoth);
  };

  auto verifyPostWarmboot = [=]() {
    ASSERT_TRUE(isAclTableGroupEnabled(getHwSwitch(), SAI_ACL_STAGE_INGRESS));
    ASSERT_TRUE(utility::isAclTableEnabled(getHwSwitch(), kQphDscpTable()));
    ASSERT_TRUE(utility::isAclTableEnabled(
        getHwSwitch(), utility::getTtlAclTableName()));
  };

  verifyAcrossWarmBoots(
      setup, []() {}, setupPostWarmboot, verifyPostWarmboot);
}

TEST_F(SaiAclTableGroupTest, AddSecondTableAfterWarmboot) {
  ASSERT_TRUE(isSupported());

  auto setup = [this]() { warmbootSetupHelper(tableAddType::table1); };

  auto setupPostWarmboot = [=]() {
    warmbootSetupHelper(tableAddType::tableBoth);
  };

  auto verifyPostWarmboot = [=]() {
    ASSERT_TRUE(isAclTableGroupEnabled(getHwSwitch(), SAI_ACL_STAGE_INGRESS));
    ASSERT_TRUE(utility::isAclTableEnabled(getHwSwitch(), kQphDscpTable()));
    ASSERT_TRUE(utility::isAclTableEnabled(
        getHwSwitch(), utility::getTtlAclTableName()));
  };

  verifyAcrossWarmBoots(
      setup, []() {}, setupPostWarmboot, verifyPostWarmboot);
}

TEST_F(SaiAclTableGroupTest, DeleteFirstTableAfterWarmboot) {
  ASSERT_TRUE(isSupported());

  auto setup = [this]() { warmbootSetupHelper(tableAddType::tableBoth); };

  auto setupPostWarmboot = [=]() { warmbootSetupHelper(tableAddType::table2); };

  auto verifyPostWarmboot = [=]() {
    ASSERT_TRUE(isAclTableGroupEnabled(getHwSwitch(), SAI_ACL_STAGE_INGRESS));
    ASSERT_FALSE(utility::isAclTableEnabled(getHwSwitch(), kQphDscpTable()));
    ASSERT_TRUE(utility::isAclTableEnabled(
        getHwSwitch(), utility::getTtlAclTableName()));
  };

  verifyAcrossWarmBoots(
      setup, []() {}, setupPostWarmboot, verifyPostWarmboot);
}

TEST_F(SaiAclTableGroupTest, DeleteSecondTableAfterWarmboot) {
  ASSERT_TRUE(isSupported());

  auto setup = [this]() { warmbootSetupHelper(tableAddType::tableBoth); };

  auto setupPostWarmboot = [=]() { warmbootSetupHelper(tableAddType::table1); };

  auto verifyPostWarmboot = [=]() {
    ASSERT_TRUE(isAclTableGroupEnabled(getHwSwitch(), SAI_ACL_STAGE_INGRESS));
    ASSERT_TRUE(utility::isAclTableEnabled(getHwSwitch(), kQphDscpTable()));
    ASSERT_FALSE(utility::isAclTableEnabled(
        getHwSwitch(), utility::getTtlAclTableName()));
  };

  verifyAcrossWarmBoots(
      setup, []() {}, setupPostWarmboot, verifyPostWarmboot);
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

    utility::addAclTableGroup(&newCfg, kAclStage(), "ingress-ACL-Table-Group");
    addDefaultAclTable(newCfg);
    applyNewConfig(newCfg);

    addAclTable1Entry1(newCfg, kDefaultAclTable());
    applyNewConfig(newCfg);
  };

  auto verify = [=]() {
    verifyAclTableHelper(kDefaultAclTable(), kAclTable1Entry1(), 1);
  };

  verifyAcrossWarmBoots(setup, verify);
}
} // namespace facebook::fboss
