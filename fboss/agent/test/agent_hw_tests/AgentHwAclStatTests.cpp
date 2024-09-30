// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/AgentHwTest.h"

#include "fboss/agent/test/utils/AclTestUtils.h"

namespace facebook::fboss {

class AgentHwAclStatTest : public AgentHwTest {
 public:
  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    if (!FLAGS_enable_acl_table_group) {
      return {
          production_features::ProductionFeature::SINGLE_ACL_TABLE,
          production_features::ProductionFeature::L3_FORWARDING};
    }
    return {
        production_features::ProductionFeature::MULTI_ACL_TABLE,
        production_features::ProductionFeature::L3_FORWARDING};
  }

  cfg::AclEntry* addDscpAcl(
      cfg::SwitchConfig* cfg,
      const std::string& aclName,
      SwitchID switchID = SwitchID(0)) {
    auto* acl = utility::addAcl(cfg, aclName);
    // ACL requires at least one qualifier
    acl->dscp() = 0x24;
    utility::addEtherTypeToAcl(
        hwAsicForSwitch(switchID), acl, cfg::EtherType::IPv6);
    return acl;
  }
};

TEST_F(AgentHwAclStatTest, AclStatCreate) {
  auto setup = [=, this]() {
    const auto& ensemble = *getAgentEnsemble();
    auto newCfg = initialConfig(ensemble);
    addDscpAcl(&newCfg, "acl0");
    utility::addAclStat(
        &newCfg,
        "acl0",
        "stat0",
        utility::getAclCounterTypes(ensemble.getHwAsicTable()->getL3Asics()));
    applyNewConfig(newCfg);
  };

  auto verify = [=, this]() {
    auto& ensemble = *getAgentEnsemble();
    auto client = ensemble.getHwAgentTestClient(SwitchID(0));

    EXPECT_TRUE(client->sync_isStatProgrammedInDefaultAclTable(
        {"acl0"},
        {"stat0"},
        utility::getAclCounterTypes(ensemble.getHwAsicTable()->getL3Asics())));
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentHwAclStatTest, AclStatCreateDeleteCreate) {
  auto setup = [=, this]() {
    auto& ensemble = *getAgentEnsemble();
    auto newCfg = initialConfig(ensemble);
    addDscpAcl(&newCfg, "acl0");
    utility::addAclStat(
        &newCfg,
        "acl0",
        "stat0",
        utility::getAclCounterTypes(ensemble.getHwAsicTable()->getL3Asics()));
    this->applyNewConfig(newCfg);
  };

  auto verify = [=, this]() {
    auto& ensemble = *getAgentEnsemble();
    auto client = ensemble.getHwAgentTestClient(SwitchID(0));
    auto newCfg = initialConfig(ensemble);
    applyNewConfig(newCfg);

    EXPECT_FALSE(client->sync_isStatProgrammedInDefaultAclTable(
        {"acl0"},
        {"stat0"},
        utility::getAclCounterTypes(ensemble.getHwAsicTable()->getL3Asics())));

    auto newCfg2 = initialConfig(ensemble);
    addDscpAcl(&newCfg2, "acl0");
    utility::addAclStat(
        &newCfg2,
        "acl0",
        "stat0",
        utility::getAclCounterTypes(ensemble.getHwAsicTable()->getL3Asics()));
    applyNewConfig(newCfg2);
    EXPECT_TRUE(client->sync_isStatProgrammedInDefaultAclTable(
        {"acl0"},
        {"stat0"},
        utility::getAclCounterTypes(ensemble.getHwAsicTable()->getL3Asics())));
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentHwAclStatTest, AclStatChangeCounterType) {
  auto setup = [=, this]() {
    auto& ensemble = *getAgentEnsemble();
    auto newCfg = initialConfig(ensemble);
    addDscpAcl(&newCfg, "acl0");
    utility::addAclStat(&newCfg, "acl0", "stat0", {cfg::CounterType::PACKETS});
    this->applyNewConfig(newCfg);
  };

  auto verify = [=, this]() {
    auto& ensemble = *getAgentEnsemble();
    auto client = ensemble.getHwAgentTestClient(SwitchID(0));

    EXPECT_TRUE(client->sync_isStatProgrammedInDefaultAclTable(
        {"acl0"}, {"stat0"}, {cfg::CounterType::PACKETS}));
  };

  auto setupPostWB = [=, this]() {
    auto& ensemble = *getAgentEnsemble();
    auto newCfg = initialConfig(ensemble);
    addDscpAcl(&newCfg, "acl0");
    utility::addAclStat(&newCfg, "acl0", "stat0", {cfg::CounterType::BYTES});
    this->applyNewConfig(newCfg);
  };

  auto verifyPostWB = [=, this]() {
    auto& ensemble = *getAgentEnsemble();
    auto client = ensemble.getHwAgentTestClient(SwitchID(0));

    EXPECT_TRUE(client->sync_isStatProgrammedInDefaultAclTable(
        {"acl0"}, {"stat0"}, {cfg::CounterType::BYTES}));
  };
  verifyAcrossWarmBoots(setup, verify, setupPostWB, verifyPostWB);
}

TEST_F(AgentHwAclStatTest, AclStatCreateShared) {
  auto setup = [=, this]() {
    auto& ensemble = *getAgentEnsemble();
    auto newCfg = initialConfig(ensemble);
    addDscpAcl(&newCfg, "acl0");
    addDscpAcl(&newCfg, "acl1");
    utility::addAclStat(
        &newCfg,
        "acl0",
        "stat",
        utility::getAclCounterTypes(ensemble.getHwAsicTable()->getL3Asics()));
    utility::addAclStat(
        &newCfg,
        "acl1",
        "stat",
        utility::getAclCounterTypes(ensemble.getHwAsicTable()->getL3Asics()));
    applyNewConfig(newCfg);
  };

  auto verify = [=, this]() {
    auto& ensemble = *getAgentEnsemble();
    auto client = ensemble.getHwAgentTestClient(SwitchID(0));
    EXPECT_TRUE(client->sync_isStatProgrammedInDefaultAclTable(
        {"acl0", "acl1"},
        {"stat"},
        utility::getAclCounterTypes(ensemble.getHwAsicTable()->getL3Asics())));
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentHwAclStatTest, AclStatCreateMultiple) {
  auto setup = [=, this]() {
    auto& ensemble = *getAgentEnsemble();
    auto newCfg = initialConfig(ensemble);
    addDscpAcl(&newCfg, "acl0");
    addDscpAcl(&newCfg, "acl1");
    utility::addAclStat(
        &newCfg,
        "acl0",
        "stat0",
        utility::getAclCounterTypes(ensemble.getHwAsicTable()->getL3Asics()));
    utility::addAclStat(
        &newCfg,
        "acl1",
        "stat1",
        utility::getAclCounterTypes(ensemble.getHwAsicTable()->getL3Asics()));
    applyNewConfig(newCfg);
  };

  auto verify = [=, this]() {
    auto& ensemble = *getAgentEnsemble();
    auto client = ensemble.getHwAgentTestClient(SwitchID(0));
    EXPECT_TRUE(client->sync_isStatProgrammedInDefaultAclTable(
        {"acl0"},
        "stat0",
        utility::getAclCounterTypes(ensemble.getHwAsicTable()->getL3Asics())));
    EXPECT_TRUE(client->sync_isStatProgrammedInDefaultAclTable(
        {"acl1"},
        "stat1",
        utility::getAclCounterTypes(ensemble.getHwAsicTable()->getL3Asics())));
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
