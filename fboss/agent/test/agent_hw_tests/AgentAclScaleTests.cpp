/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <folly/IPAddress.h>
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/utils/AclTestUtils.h"
#include "fboss/agent/test/utils/AsicUtils.h"

#include "fboss/agent/test/gen-cpp2/production_features_types.h"

namespace facebook::fboss {

enum class AclWidth : uint8_t {
  SINGLE_WIDE = 1,
  DOUBLE_WIDE,
  TRIPLE_WIDE,
};

class AgentAclScaleTest : public AgentHwTest {
 public:
  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    return {production_features::ProductionFeature::MULTI_ACL_TABLE};
  }

 protected:
  void SetUp() override {
    FLAGS_enable_acl_table_group = true;
    AgentHwTest::SetUp();
  }
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto cfg = utility::onePortPerInterfaceConfig(
        ensemble.getSw(),
        ensemble.masterLogicalPortIds(),
        true /*interfaceHasSubnet*/);
    utility::addAclTableGroup(
        &cfg, cfg::AclStage::INGRESS, utility::kDefaultAclTableGroupName());
    return cfg;
  }

  void setCmdLineFlagOverrides() const override {
    AgentHwTest::setCmdLineFlagOverrides();
    FLAGS_enable_acl_table_group = true;
  }
  std::vector<cfg::AclTableQualifier> setAclQualifiers(AclWidth width) const {
    std::vector<cfg::AclTableQualifier> qualifiers;

    if (width == AclWidth::SINGLE_WIDE) {
      qualifiers.push_back(cfg::AclTableQualifier::DSCP); // 8 bits
    } else if (width == AclWidth::DOUBLE_WIDE) {
      qualifiers.push_back(cfg::AclTableQualifier::SRC_IPV6); // 128 bits
      qualifiers.push_back(cfg::AclTableQualifier::DST_IPV6); // 128 bits
    } else if (width == AclWidth::TRIPLE_WIDE) {
      qualifiers.push_back(cfg::AclTableQualifier::SRC_IPV6); // 128 bits
      qualifiers.push_back(cfg::AclTableQualifier::DST_IPV6); // 128 bits
      qualifiers.push_back(cfg::AclTableQualifier::IP_TYPE); // 32 bits
      qualifiers.push_back(cfg::AclTableQualifier::L4_SRC_PORT); // 16 bits
      qualifiers.push_back(cfg::AclTableQualifier::L4_DST_PORT); // 16 bits
      qualifiers.push_back(cfg::AclTableQualifier::LOOKUP_CLASS_L2); // 16 bits
      qualifiers.push_back(cfg::AclTableQualifier::OUTER_VLAN); // 12 bits
      qualifiers.push_back(cfg::AclTableQualifier::DSCP); // 8 bits
    }
    return qualifiers;
  }

  void updateAclEntryFields(cfg::AclEntry* aclEntry, AclWidth width) {
    if (width == AclWidth::SINGLE_WIDE) {
      aclEntry->dscp() = 0x24;
    } else if (width == AclWidth::DOUBLE_WIDE) {
      aclEntry->srcIp() = "2401:db00::";
      aclEntry->dstIp() = "2401:db00::";
    } else if (width == AclWidth::TRIPLE_WIDE) {
      aclEntry->srcIp() = "2401:db00::";
      aclEntry->dstIp() = "2401:db00::";
      aclEntry->ipType() = cfg::IpType::IP6;
      aclEntry->l4SrcPort() = 8000;
      aclEntry->l4DstPort() = 8002;
      aclEntry->dscp() = 0x24;
    }
  }

  void addAclEntries(
      const int maxEntries,
      AclWidth width,
      cfg::SwitchConfig* cfg,
      const std::string& tableName) {
    for (auto i = 0; i < maxEntries; i++) {
      std::string aclEntryName = "Entry" + std::to_string(i);
      auto* aclEntry = utility::addAcl(
          cfg, aclEntryName, cfg::AclActionType::DENY, tableName);
      updateAclEntryFields(aclEntry, width);
    }
  }

  uint32_t getMaxSingleWideAclTables(const std::vector<const HwAsic*>& asics) {
    auto asic = utility::checkSameAndGetAsic(asics);
    auto maxAclTables = asic->getMaxAclTables();
    CHECK(maxAclTables.has_value());
    return maxAclTables.value();
  }

  uint32_t getMaxAclEntries(const std::vector<const HwAsic*>& asics) {
    auto asic = utility::checkSameAndGetAsic(asics);
    auto maxAclEntries = asic->getMaxAclEntries();
    CHECK(maxAclEntries.has_value());
    return maxAclEntries.value();
  }

  uint32_t getMaxDoubleWideAclTables() {
    // CS00012346871 - Failed to create 2 consecutive double wide tables
    // Hardcoding to create only 1 double wide table
    // ToDo @Stuti - Revisit this once CS00012346871 is fixed
    return 1;
  }

  // Create max number of single wide ACL tables
  void createSingleWideMaxAclTableHelper() {
    auto setup = [&]() {
      auto cfg{initialConfig(*getAgentEnsemble())};
      const int maxAclTables =
          getMaxSingleWideAclTables(getAgentEnsemble()->getL3Asics());
      std::vector<cfg::AclTableQualifier> qualifiers =
          setAclQualifiers(AclWidth::SINGLE_WIDE);

      for (auto i = 0; i < maxAclTables; i++) {
        std::string aclTableName = "aclTable" + std::to_string(i);
        utility::addAclTable(
            &cfg, aclTableName, i /* priority */, {}, qualifiers);

        auto* aclEntry = utility::addAcl(
            &cfg, "Entry0", cfg::AclActionType::DENY, aclTableName);
        updateAclEntryFields(aclEntry, AclWidth::SINGLE_WIDE);
      }
      applyNewConfig(cfg);
    };
    verifyAcrossWarmBoots(setup, [] {});
  }

  // Create max number of double wide ACL tables
  void createDoubleWideMaxAclTableHelper() {
    auto setup = [&]() {
      auto cfg{initialConfig(*getAgentEnsemble())};
      std::vector<cfg::AclTableQualifier> qualifiers =
          setAclQualifiers(AclWidth::DOUBLE_WIDE);

      for (auto i = 0; i < getMaxDoubleWideAclTables(); i++) {
        std::string aclTableName = "aclTable" + std::to_string(i);
        utility::addAclTable(
            &cfg, aclTableName, i /* priority */, {}, qualifiers);

        auto* aclEntry = utility::addAcl(
            &cfg, "Entry0", cfg::AclActionType::PERMIT, aclTableName);
        updateAclEntryFields(aclEntry, AclWidth::DOUBLE_WIDE);
      }

      applyNewConfig(cfg);
    };
    verifyAcrossWarmBoots(setup, [] {});
  }

  // Create max number of triple wide ACL tables
  void createTripleWideMaxAclTableHelper() {
    auto setup = [&]() {
      auto cfg{initialConfig(*getAgentEnsemble())};
      const int maxAclSingleWideTables =
          getMaxSingleWideAclTables(getAgentEnsemble()->getL3Asics());
      const int maxAclTables =
          maxAclSingleWideTables / (int)AclWidth::TRIPLE_WIDE;
      std::vector<cfg::AclTableQualifier> qualifiers =
          setAclQualifiers(AclWidth::TRIPLE_WIDE);

      for (auto i = 0; i < maxAclTables; i++) {
        std::string aclTableName = "aclTable" + std::to_string(i);
        utility::addAclTable(
            &cfg, aclTableName, i /* priority */, {}, qualifiers);

        auto* aclEntry = utility::addAcl(
            &cfg, "Entry0", cfg::AclActionType::PERMIT, aclTableName);
        updateAclEntryFields(aclEntry, AclWidth::TRIPLE_WIDE);
      }
      applyNewConfig(cfg);
    };
    verifyAcrossWarmBoots(setup, [] {});
  }

  // Create max number of ACL tables
  void createVariableWidthAclTableHelper() {
    auto setup = [&]() {
      auto cfg{initialConfig(*getAgentEnsemble())};
      const int maxAclSingleWideTables =
          getMaxSingleWideAclTables(getAgentEnsemble()->getL3Asics());
      std::vector<cfg::AclTableQualifier> qualifiers =
          setAclQualifiers(AclWidth::TRIPLE_WIDE);

      utility::addAclTable(&cfg, "aclTable0", 0 /* priority */, {}, qualifiers);
      auto* aclEntry0 = utility::addAcl(
          &cfg, "Entry0", cfg::AclActionType::PERMIT, "aclTable0");
      updateAclEntryFields(aclEntry0, AclWidth::TRIPLE_WIDE);

      qualifiers = setAclQualifiers(AclWidth::DOUBLE_WIDE);
      utility::addAclTable(&cfg, "aclTable1", 1 /* priority */, {}, qualifiers);
      auto* aclEntry1 = utility::addAcl(
          &cfg, "Entry0", cfg::AclActionType::PERMIT, "aclTable1");
      updateAclEntryFields(aclEntry1, AclWidth::DOUBLE_WIDE);

      int remainingAclTable = maxAclSingleWideTables -
          (int)AclWidth::TRIPLE_WIDE - (int)AclWidth::DOUBLE_WIDE;
      qualifiers = setAclQualifiers(AclWidth::SINGLE_WIDE);
      for (auto i = 2; i < remainingAclTable; i++) {
        std::string aclTableName = "aclTable" + std::to_string(i);
        utility::addAclTable(
            &cfg, aclTableName, i /* priority */, {}, qualifiers);

        auto* aclEntry = utility::addAcl(
            &cfg, "Entry0", cfg::AclActionType::PERMIT, aclTableName);
        updateAclEntryFields(aclEntry, AclWidth::SINGLE_WIDE);
      }
      applyNewConfig(cfg);
    };
    verifyAcrossWarmBoots(setup, [] {});
  }

  // Create max number of entires for single wide ACL table
  void createSingleWideTableMaxAclEntriesHelper() {
    auto setup = [&]() {
      auto cfg{initialConfig(*getAgentEnsemble())};
      const auto maxAclEntries =
          getMaxAclEntries(getAgentEnsemble()->getL3Asics());
      const int maxAclTables =
          getMaxSingleWideAclTables(getAgentEnsemble()->getL3Asics());
      std::vector<cfg::AclTableQualifier> qualifiers =
          setAclQualifiers(AclWidth::SINGLE_WIDE);

      utility::addAclTable(&cfg, "aclTable0", 0 /* priority */, {}, qualifiers);
      auto maxEntries = maxAclTables * maxAclEntries;
      addAclEntries(maxEntries, AclWidth::SINGLE_WIDE, &cfg, "aclTable0");
      applyNewConfig(cfg);
    };
    verifyAcrossWarmBoots(setup, [] {});
  }

  // Create max number of entires for tripe wide ACL table
  void createTripleWideTableMaxAclEntriesHelper() {
    auto setup = [&]() {
      auto cfg{initialConfig(*getAgentEnsemble())};
      const auto maxAclSingleWideEntries =
          getMaxAclEntries(getAgentEnsemble()->getL3Asics());
      const auto maxAclSingleWideTables =
          getMaxSingleWideAclTables(getAgentEnsemble()->getL3Asics());
      // Each triple wide will consume 3 single wide entries hence reducing
      // total numbers of entries by 3
      const int maxTripleWideAclTables =
          maxAclSingleWideTables / (int)AclWidth::TRIPLE_WIDE;
      const auto maxEntries = maxAclSingleWideEntries * maxTripleWideAclTables;

      std::vector<cfg::AclTableQualifier> qualifiers =
          setAclQualifiers(AclWidth::TRIPLE_WIDE);
      utility::addAclTable(&cfg, "aclTable1", 0 /* priority */, {}, qualifiers);
      addAclEntries(maxEntries, AclWidth::TRIPLE_WIDE, &cfg, "aclTable1");
      applyNewConfig(cfg);
    };
    verifyAcrossWarmBoots(setup, [] {});
  }

  // Create max number of ACL entires
  void createVariableWidthAclEntriesHelper() {
    auto setup = [&]() {
      auto cfg{initialConfig(*getAgentEnsemble())};
      const auto maxAclEntries =
          getMaxAclEntries(getAgentEnsemble()->getL3Asics());
      const int maxAclSingleWideTables =
          getMaxSingleWideAclTables(getAgentEnsemble()->getL3Asics());
      std::vector<cfg::AclTableQualifier> qualifiers =
          setAclQualifiers(AclWidth::TRIPLE_WIDE);

      utility::addAclTable(&cfg, "aclTable0", 0 /* priority */, {}, qualifiers);
      addAclEntries(maxAclEntries, AclWidth::TRIPLE_WIDE, &cfg, "aclTable0");

      qualifiers = setAclQualifiers(AclWidth::DOUBLE_WIDE);
      utility::addAclTable(&cfg, "aclTable1", 1 /* priority */, {}, qualifiers);
      addAclEntries(maxAclEntries, AclWidth::DOUBLE_WIDE, &cfg, "aclTable1");

      int remainingAclTable = maxAclSingleWideTables -
          (int)AclWidth::TRIPLE_WIDE - (int)AclWidth::DOUBLE_WIDE;
      qualifiers = setAclQualifiers(AclWidth::SINGLE_WIDE);
      utility::addAclTable(&cfg, "aclTable2", 2 /* priority */, {}, qualifiers);
      auto maxEntries = maxAclEntries * remainingAclTable;
      addAclEntries(maxEntries, AclWidth::SINGLE_WIDE, &cfg, "aclTable2");
      applyNewConfig(cfg);
    };
    verifyAcrossWarmBoots(setup, [] {});
  }
};

TEST_F(AgentAclScaleTest, CreateMaxAclSingleWideTables) {
  this->createSingleWideMaxAclTableHelper();
}

TEST_F(AgentAclScaleTest, CreateMaxAclDobleWideTables) {
  this->createDoubleWideMaxAclTableHelper();
}

TEST_F(AgentAclScaleTest, CreateMaxAclTripleWideTables) {
  this->createTripleWideMaxAclTableHelper();
}

TEST_F(AgentAclScaleTest, CreateVariableWidthAclTables) {
  this->createVariableWidthAclTableHelper();
}

TEST_F(AgentAclScaleTest, CreateSingleWideTableMaxAclEntries) {
  this->createSingleWideTableMaxAclEntriesHelper();
}

TEST_F(AgentAclScaleTest, CreateTripleWideTableMaxAclEntries) {
  this->createTripleWideTableMaxAclEntriesHelper();
}

TEST_F(AgentAclScaleTest, CreateVariableWidthAclEntries) {
  this->createVariableWidthAclEntriesHelper();
}

} // namespace facebook::fboss
