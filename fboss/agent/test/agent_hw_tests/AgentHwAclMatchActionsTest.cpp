// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/test/AgentHwTest.h"

#include "fboss/agent/test/utils/AclTestUtils.h"
#include "fboss/agent/test/utils/AsicUtils.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/TrafficPolicyTestUtils.h"

namespace facebook::fboss {

class AgentHwAclMatchActionsTest : public AgentHwTest {
 public:
  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    if (!FLAGS_enable_acl_table_group) {
      return {production_features::ProductionFeature::SINGLE_ACL_TABLE};
    }
    return {production_features::ProductionFeature::MULTI_ACL_TABLE};
  }

  void addSetDscpAction(
      cfg::SwitchConfig* config,
      const std::string& matcherName,
      int32_t dscpValue) {
    cfg::SetDscpMatchAction setDscpMatchAction;
    *setDscpMatchAction.dscpValue() = dscpValue;
    cfg::MatchAction matchAction = cfg::MatchAction();
    matchAction.setDscp() = setDscpMatchAction;
    utility::addMatcher(config, matcherName, matchAction);
  }

  std::shared_ptr<AclEntry> getAclEntry(const std::string& name) {
    return utility::getAclEntry(
        getSw()->getState(), name, FLAGS_enable_acl_table_group);
  }
};

TEST_F(AgentHwAclMatchActionsTest, AddTrafficPolicy) {
  constexpr uint32_t kDscp = 0x24;
  constexpr int kQueueId = 4;

  auto setup = [=, this]() {
    const auto& ensemble = *getAgentEnsemble();
    auto newCfg = initialConfig(ensemble);
    auto l3Asics = ensemble.getSw()->getHwAsicTable()->getL3Asics();
    auto asic = utility::checkSameAndGetAsic(l3Asics);

    utility::addDscpAclToCfg(asic, &newCfg, "acl1", kDscp);
    utility::addQueueMatcher(&newCfg, "acl1", kQueueId, ensemble.isSai());
    applyNewConfig(newCfg);
  };
  auto verify = [=, this]() {
    auto client = getAgentEnsemble()->getHwAgentTestClient(SwitchID(0));
    auto acl1 = getAclEntry("acl1")->toThrift();
    EXPECT_EQ(
        client->sync_getAclTableNumAclEntries(utility::kDefaultAclTable()), 1);
    EXPECT_TRUE(client->sync_isAclEntrySame(acl1, utility::kDefaultAclTable()));
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentHwAclMatchActionsTest, SetDscpMatchAction) {
  constexpr uint32_t kDscp = 0x24;
  constexpr uint32_t kDscp2 = 0x8;

  auto setup = [=, this]() {
    const auto& ensemble = *getAgentEnsemble();
    auto newCfg = initialConfig(ensemble);
    auto l3Asics = ensemble.getSw()->getHwAsicTable()->getL3Asics();
    auto asic = utility::checkSameAndGetAsic(l3Asics);

    utility::addDscpAclToCfg(asic, &newCfg, "acl1", kDscp);
    addSetDscpAction(&newCfg, "acl1", kDscp2);
    applyNewConfig(newCfg);
  };
  auto verify = [=, this]() {
    auto client = getAgentEnsemble()->getHwAgentTestClient(SwitchID(0));
    auto acl1 = getAclEntry("acl1")->toThrift();

    EXPECT_EQ(
        client->sync_getAclTableNumAclEntries(utility::kDefaultAclTable()), 1);
    EXPECT_TRUE(client->sync_isAclEntrySame(acl1, utility::kDefaultAclTable()));
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentHwAclMatchActionsTest, AddSameMatcherTwice) {
  constexpr uint32_t kDscp = 0x24;
  constexpr uint32_t kDscp2 = 0x8;

  auto setup = [=, this]() {
    const auto& ensemble = *getAgentEnsemble();
    auto newCfg = initialConfig(ensemble);
    auto l3Asics = ensemble.getSw()->getHwAsicTable()->getL3Asics();
    auto asic = utility::checkSameAndGetAsic(l3Asics);

    utility::addDscpAclToCfg(asic, &newCfg, "acl1", kDscp);

    utility::addQueueMatcher(&newCfg, "acl1", 0, ensemble.isSai());
    utility::addQueueMatcher(&newCfg, "acl1", 0, ensemble.isSai());

    utility::addDscpAclToCfg(asic, &newCfg, "acl2", 0);
    addSetDscpAction(&newCfg, "acl2", kDscp2);
    addSetDscpAction(&newCfg, "acl2", kDscp2);

    applyNewConfig(newCfg);
  };
  auto verify = [=, this]() {
    auto client = getAgentEnsemble()->getHwAgentTestClient(SwitchID(0));
    auto acl1 = getAclEntry("acl1")->toThrift();
    auto acl2 = getAclEntry("acl2")->toThrift();

    EXPECT_EQ(
        client->sync_getAclTableNumAclEntries(utility::kDefaultAclTable()), 2);
    EXPECT_TRUE(client->sync_isAclEntrySame(acl1, utility::kDefaultAclTable()));
    EXPECT_TRUE(client->sync_isAclEntrySame(acl2, utility::kDefaultAclTable()));
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentHwAclMatchActionsTest, AddMultipleActions) {
  auto setup = [this]() {
    const auto& ensemble = *getAgentEnsemble();
    auto newCfg = initialConfig(ensemble);
    auto l3Asics = ensemble.getSw()->getHwAsicTable()->getL3Asics();
    auto asic = utility::checkSameAndGetAsic(l3Asics);

    utility::addDscpAclToCfg(asic, &newCfg, "acl1", 0);
    utility::addDscpAclToCfg(asic, &newCfg, "acl2", 0);
    utility::addDscpAclToCfg(asic, &newCfg, "acl3", 0);
    utility::addQueueMatcher(&newCfg, "acl1", 0, ensemble.isSai());
    utility::addQueueMatcher(&newCfg, "acl2", 0, ensemble.isSai());

    addSetDscpAction(&newCfg, "acl3", 8);
    this->applyNewConfig(newCfg);
  };
  auto verify = [this]() {
    auto client = getAgentEnsemble()->getHwAgentTestClient(SwitchID(0));

    EXPECT_EQ(
        client->sync_getAclTableNumAclEntries(utility::kDefaultAclTable()), 3);
    for (const auto& matcher : {"acl1", "acl2", "acl3"}) {
      auto acl = getAclEntry(matcher)->toThrift();
      EXPECT_TRUE(
          client->sync_isAclEntrySame(acl, utility::kDefaultAclTable()));
    }
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentHwAclMatchActionsTest, AddRemoveActions) {
  auto setup = [this]() {
    const auto& ensemble = *getAgentEnsemble();
    auto newCfg = initialConfig(ensemble);
    auto l3Asics = ensemble.getSw()->getHwAsicTable()->getL3Asics();
    auto asic = utility::checkSameAndGetAsic(l3Asics);

    utility::addDscpAclToCfg(asic, &newCfg, "acl1", 0);
    utility::addQueueMatcher(&newCfg, "acl1", 0, ensemble.isSai());

    utility::addDscpAclToCfg(asic, &newCfg, "acl2", 0);
    addSetDscpAction(&newCfg, "acl2", 8);

    newCfg.dataPlaneTrafficPolicy()->matchToAction()->pop_back();
    newCfg.dataPlaneTrafficPolicy()->matchToAction()->pop_back();

    this->applyNewConfig(newCfg);
  };
  auto verify = [this]() {
    auto client = getAgentEnsemble()->getHwAgentTestClient(SwitchID(0));

    EXPECT_EQ(
        client->sync_getAclTableNumAclEntries(utility::kDefaultAclTable()), 2);
    for (const auto& matcher : {"acl1", "acl2"}) {
      auto acl = getAclEntry(matcher)->toThrift();
      EXPECT_TRUE(
          client->sync_isAclEntrySame(acl, utility::kDefaultAclTable()));
    }
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentHwAclMatchActionsTest, AddTrafficPolicyMultipleRemoveOne) {
  auto setup = [this]() {
    const auto& ensemble = *getAgentEnsemble();
    auto newCfg = initialConfig(ensemble);
    auto l3Asics = ensemble.getSw()->getHwAsicTable()->getL3Asics();
    auto asic = utility::checkSameAndGetAsic(l3Asics);

    utility::addDscpAclToCfg(asic, &newCfg, "acl1", 0);
    utility::addQueueMatcher(&newCfg, "acl1", 0, ensemble.isSai());
    utility::addDscpAclToCfg(asic, &newCfg, "acl2", 0);
    utility::addQueueMatcher(&newCfg, "acl2", 0, ensemble.isSai());
    this->applyNewConfig(newCfg);

    newCfg.dataPlaneTrafficPolicy()->matchToAction()->pop_back();

    this->applyNewConfig(newCfg);
  };
  auto verify = [this]() {
    auto client = getAgentEnsemble()->getHwAgentTestClient(SwitchID(0));

    EXPECT_EQ(
        client->sync_getAclTableNumAclEntries(utility::kDefaultAclTable()), 2);

    for (const auto& matcher : {"acl1", "acl2"}) {
      auto acl = getAclEntry(matcher)->toThrift();
      EXPECT_TRUE(
          client->sync_isAclEntrySame(acl, utility::kDefaultAclTable()));
    }
  };
  this->verifyAcrossWarmBoots(setup, verify);
}
} // namespace facebook::fboss
