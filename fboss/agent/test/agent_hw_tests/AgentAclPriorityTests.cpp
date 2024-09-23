/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/ApplyThriftConfig.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/AgentHwTest.h"
#include "fboss/agent/test/utils/AclTestUtils.h"
#include "fboss/agent/test/utils/CoppTestUtils.h"
#include "fboss/agent/test/utils/QueuePerHostTestUtils.h"

#include <string>

namespace {

using namespace facebook::fboss;
using namespace facebook::fboss::utility;

void addAclEntry(cfg::SwitchConfig& cfg, cfg::AclEntry* acl) {
  if (FLAGS_enable_acl_table_group) {
    int tableNumber = getAclTableIndex(&cfg, utility::kDefaultAclTable());
    if (cfg.aclTableGroup()) {
      cfg.aclTableGroup()->aclTables()[tableNumber].aclEntries()->push_back(
          *acl);
    }
  } else {
    cfg.acls()->push_back(*acl);
  }
}

} // unnamed namespace

namespace facebook::fboss {

class AgentAclPriorityTest : public AgentHwTest {
  void setCmdLineFlagOverrides() const override {
    AgentHwTest::setCmdLineFlagOverrides();
  }

 public:
  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    if (FLAGS_enable_acl_table_group) {
      return {production_features::ProductionFeature::SINGLE_ACL_TABLE};
    } else {
      return {production_features::ProductionFeature::MULTI_ACL_TABLE};
    }
  }

 protected:
  cfg::SwitchConfig initialConfig(
      const AgentEnsemble& ensemble) const override {
    auto cfg = utility::onePortPerInterfaceConfig(
        ensemble.getSw(),
        ensemble.masterLogicalPortIds(),
        true /*interfaceHasSubnet*/);
    if (FLAGS_enable_acl_table_group) {
      utility::addAclTableGroup(
          &cfg, cfg::AclStage::INGRESS, utility::getAclTableGroupName());
      utility::addDefaultAclTable(cfg);
    }
    return cfg;
  }

  void addDenyPortAcl(cfg::SwitchConfig& cfg, const std::string& aclName) {
    auto acl = cfg::AclEntry();
    auto asic =
        utility::checkSameAndGetAsic(this->getAgentEnsemble()->getL3Asics());
    *acl.name() = aclName;
    *acl.actionType() = cfg::AclActionType::DENY;
    acl.dscp() = 0x24;
    utility::addEtherTypeToAcl(asic, &acl, cfg::EtherType::IPv6);
    addAclEntry(cfg, &acl);
  }

  void addPermitIpAcl(
      cfg::SwitchConfig& cfg,
      const std::string& aclName,
      folly::IPAddress ip) {
    auto acl = cfg::AclEntry();
    auto asic =
        utility::checkSameAndGetAsic(this->getAgentEnsemble()->getL3Asics());
    acl.name() = aclName;
    acl.actionType() = cfg::AclActionType::PERMIT;
    acl.dstIp() = ip.str();
    acl.dscp() = 0x24;
    utility::addEtherTypeToAcl(asic, &acl, cfg::EtherType::IPv6);
    addAclEntry(cfg, &acl);
  }
};

// This test verifies that trafficPolicy configuration have no influence on
// ACL entry priority
TEST_F(AgentAclPriorityTest, CheckAclPriorityOrder) {
  const folly::IPAddress kIp("2400::1");
  auto setup = [this, kIp]() {
    auto newCfg = this->getSw()->getConfig();
    this->addDenyPortAcl(newCfg, "A");
    this->addPermitIpAcl(newCfg, "B", kIp);
    this->addDenyPortAcl(newCfg, "C");
    this->addPermitIpAcl(newCfg, "D", kIp);

    auto l3Asics = this->getAgentEnsemble()->getL3Asics();
    cfg::TrafficPolicyConfig trafficConfig;
    trafficConfig.matchToAction()->resize(4);
    newCfg.trafficCounters()->resize(4);
    // create traffic policy in reverse order
    for (int i = 3; i >= 0; i--) {
      auto& acls = utility::getAcls(&newCfg, std::nullopt);
      trafficConfig.matchToAction()[i].matcher() = *acls[i].name();
      trafficConfig.matchToAction()[i].action()->counter() = *acls[i].name();
      *newCfg.trafficCounters()[i].name() = *acls[i].name();
      *newCfg.trafficCounters()[i].types() =
          utility::getAclCounterTypes(l3Asics);
    }
    newCfg.dataPlaneTrafficPolicy() = trafficConfig;
    this->applyNewConfig(newCfg);
  };

  auto verify = [=, this]() {
    auto getPrio = [this](std::string aclName) {
      auto acl =
          utility::getAclEntryByName(this->getProgrammedState(), aclName);
      return acl->getPriority();
    };
    EXPECT_EQ(getPrio("A") + 1, getPrio("B"));
    EXPECT_EQ(getPrio("B") + 1, getPrio("C"));
    EXPECT_EQ(getPrio("C") + 1, getPrio("D"));
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentAclPriorityTest, CheckAclPriortyOrderInsertMiddle) {
  auto setup = [this]() {
    auto newCfg = this->getSw()->getConfig();
    this->addDenyPortAcl(newCfg, "A");
    this->addDenyPortAcl(newCfg, "B");
    this->applyNewConfig(newCfg);
    utility::delLastAddedAcl(&newCfg);
    this->addDenyPortAcl(newCfg, "C");
    this->addDenyPortAcl(newCfg, "B");
    this->applyNewConfig(newCfg);
  };

  auto verify = [=, this]() {
    auto aAcl = utility::getAclEntryByName(this->getProgrammedState(), "A");
    auto bAcl = utility::getAclEntryByName(this->getProgrammedState(), "B");
    auto cAcl = utility::getAclEntryByName(this->getProgrammedState(), "C");
    int aPrio = aAcl->getPriority();
    int bPrio = bAcl->getPriority();
    int cPrio = cAcl->getPriority();
    // Order should be A, C, B now
    EXPECT_EQ(aPrio + 1, cPrio);
    EXPECT_EQ(aPrio + 2, bPrio);
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

/**
 * This unit test case is to test we won't crash cause we're using aclName as
 * key of the aclMap in S/W while using priority as key of aclTable in H/W
 */
TEST_F(AgentAclPriorityTest, AclNameChange) {
  auto setup = [this]() {
    auto newCfg = this->getSw()->getConfig();
    this->addDenyPortAcl(newCfg, "A");
    this->applyNewConfig(newCfg);
    if (FLAGS_enable_acl_table_group) {
      *newCfg.aclTableGroup()
           ->aclTables()[utility::getAclTableIndex(
               &newCfg, utility::kDefaultAclTable())]
           .aclEntries()
           ->back()
           .name() = "AA";
    } else {
      *newCfg.acls()->back().name() = "AA";
    }
    this->applyNewConfig(newCfg);
  };

  auto verify = [&]() {
    // check s/w acl matches h/w
    ASSERT_EQ(
        nullptr, utility::getAclEntryByName(this->getProgrammedState(), "A"));
    ASSERT_NE(
        nullptr, utility::getAclEntryByName(this->getProgrammedState(), "AA"));
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TEST_F(AgentAclPriorityTest, AclsChanged) {
  const folly::IPAddress kIp("2400::1");
  auto setup = [this, kIp]() {
    auto config = this->getSw()->getConfig();
    this->addDenyPortAcl(config, "acl0");
    auto l3Asics = this->getAgentEnsemble()->getL3Asics();
    // Get Acls from COPP policy
    utility::setDefaultCpuTrafficPolicyConfig(
        config, l3Asics, this->getAgentEnsemble()->isSai());
    this->addPermitIpAcl(config, "acl1", kIp);
    this->applyNewConfig(config);
  };

  auto setupPostWb = [this, kIp]() {
    auto config = this->getSw()->getConfig();
    // Drop COPP acls
    this->addDenyPortAcl(config, "acl0");
    this->addPermitIpAcl(config, "acl1", kIp);
    this->applyNewConfig(config);
  };

  this->verifyAcrossWarmBoots(setup, []() {}, setupPostWb, []() {});
}

TEST_F(AgentAclPriorityTest, Reprioritize) {
  auto setup = [=, this]() {
    auto config = this->getSw()->getConfig();
    this->addPermitIpAcl(config, "B", folly::IPAddress("1::2"));
    this->addPermitIpAcl(config, "A", folly::IPAddress("1::3"));

    cfg::CPUTrafficPolicyConfig cpuConfig;
    cfg::TrafficPolicyConfig trafficConfig;
    trafficConfig.matchToAction()->resize(2);
    cfg::MatchAction matchAction = utility::getToQueueAction(
        1, this->getAgentEnsemble()->isSai(), cfg::ToCpuAction::TRAP);
    for (int i = 0; i < 2; i++) {
      auto& acls = utility::getAcls(&config, std::nullopt);
      trafficConfig.matchToAction()[i].matcher() = *acls[i].name();
      trafficConfig.matchToAction()[i].action() = matchAction;
    }
    cpuConfig.trafficPolicy() = trafficConfig;
    config.cpuTrafficPolicy() = cpuConfig;
    this->applyNewConfig(config);
  };

  auto setupPostWb = [=, this]() {
    auto config = this->getSw()->getConfig();
    this->addPermitIpAcl(config, "A", folly::IPAddress("1::3"));
    this->addPermitIpAcl(config, "B", folly::IPAddress("1::2"));

    cfg::CPUTrafficPolicyConfig cpuConfig;
    cfg::TrafficPolicyConfig trafficConfig;
    trafficConfig.matchToAction()->resize(2);
    cfg::MatchAction matchAction = utility::getToQueueAction(
        1, this->getAgentEnsemble()->isSai(), cfg::ToCpuAction::TRAP);
    for (int i = 0; i < 2; i++) {
      auto& acls = utility::getAcls(&config, std::nullopt);
      trafficConfig.matchToAction()[i].matcher() = *acls[i].name();
      trafficConfig.matchToAction()[i].action() = matchAction;
    }
    cpuConfig.trafficPolicy() = trafficConfig;
    config.cpuTrafficPolicy() = cpuConfig;
    this->applyNewConfig(config);
  };

  this->verifyAcrossWarmBoots(setup, []() {}, setupPostWb, []() {});
}
} // namespace facebook::fboss
