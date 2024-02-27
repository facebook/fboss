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
#include "fboss/agent/hw/test/HwTest.h"

#include "fboss/agent/ApplyThriftConfig.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/test/HwTestAclUtils.h"
#include "fboss/agent/hw/test/HwTestCoppUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwTestQueuePerHostUtils.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"

#include "fboss/agent/hw/test/ConfigFactory.h"

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

void addDenyPortAcl(cfg::SwitchConfig& cfg, const std::string& aclName) {
  auto acl = cfg::AclEntry();
  *acl.name() = aclName;
  *acl.actionType() = cfg::AclActionType::DENY;
  acl.dscp() = 0x24;
  addAclEntry(cfg, &acl);
}

void addPermitIpAcl(
    cfg::SwitchConfig& cfg,
    const std::string& aclName,
    folly::IPAddress ip) {
  auto acl = cfg::AclEntry();
  acl.name() = aclName;
  acl.actionType() = cfg::AclActionType::PERMIT;
  acl.dstIp() = ip.str();
  acl.dscp() = 0x24;
  addAclEntry(cfg, &acl);
}

} // unnamed namespace

namespace facebook::fboss {

template <bool enableMultiAclTable>
struct EnableMultiAclTableT {
  static constexpr auto multiAclTableEnabled = enableMultiAclTable;
};

using TestTypes =
    ::testing::Types<EnableMultiAclTableT<false>, EnableMultiAclTableT<true>>;

template <typename EnableMultiAclTableT>
class HwAclPriorityTest : public HwTest {
  static auto constexpr isMultiAclEnabled =
      EnableMultiAclTableT::multiAclTableEnabled;

 protected:
  void SetUp() override {
    FLAGS_enable_acl_table_group = isMultiAclEnabled;
    HwTest::SetUp();
    /*
     * Native SDK does not support multi acl feature.
     * So skip multi acl tests for fake bcm sdk
     */
    if ((this->getPlatform()->getAsic()->getAsicType() ==
         cfg::AsicType::ASIC_TYPE_FAKE) &&
        (isMultiAclEnabled)) {
      GTEST_SKIP();
    }
  }

  cfg::SwitchConfig initialConfig() const {
    auto cfg = utility::onePortPerInterfaceConfig(
        getHwSwitch(),
        masterLogicalPortIds(),
        getAsic()->desiredLoopbackModes());
    if (isMultiAclEnabled) {
      utility::addAclTableGroup(
          &cfg, cfg::AclStage::INGRESS, utility::getAclTableGroupName());
      utility::addDefaultAclTable(cfg);
    }
    return cfg;
  }
};

TYPED_TEST_SUITE(HwAclPriorityTest, TestTypes);

TYPED_TEST(HwAclPriorityTest, CheckAclPriorityOrder) {
  auto setup = [this]() {
    auto newCfg = this->initialConfig();
    addDenyPortAcl(newCfg, "A");
    addDenyPortAcl(newCfg, "B");
    this->applyNewConfig(newCfg);
  };

  auto verify = [=, this]() {
    for (auto acl : {"A", "B"}) {
      checkSwHwAclMatch(this->getHwSwitch(), this->getProgrammedState(), acl);
    }
    auto aAcl = utility::getAclEntryByName(this->getProgrammedState(), "A");
    int aPrio = aAcl->getPriority();
    auto bAcl = utility::getAclEntryByName(this->getProgrammedState(), "B");
    int bPrio = bAcl->getPriority();
    EXPECT_EQ(aPrio + 1, bPrio);
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwAclPriorityTest, CheckAclPriortyOrderInsertMiddle) {
  auto setup = [this]() {
    auto newCfg = this->initialConfig();
    addDenyPortAcl(newCfg, "A");
    addDenyPortAcl(newCfg, "B");
    this->applyNewConfig(newCfg);
    utility::delLastAddedAcl(&newCfg);
    addDenyPortAcl(newCfg, "C");
    addDenyPortAcl(newCfg, "B");
    this->applyNewConfig(newCfg);
  };

  auto verify = [=, this]() {
    for (auto acl : {"A", "B", "C"}) {
      checkSwHwAclMatch(this->getHwSwitch(), this->getProgrammedState(), acl);
    }
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
TYPED_TEST(HwAclPriorityTest, AclNameChange) {
  auto setup = [this]() {
    auto newCfg = this->initialConfig();
    addDenyPortAcl(newCfg, "A");
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
    checkSwHwAclMatch(this->getHwSwitch(), this->getProgrammedState(), "AA");
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwAclPriorityTest, AclsChanged) {
  const folly::IPAddress kIp("2400::1");
  auto setup = [this, kIp]() {
    auto config = this->initialConfig();
    addDenyPortAcl(config, "acl0");
    // Get Acls from COPP policy
    setDefaultCpuTrafficPolicyConfig(
        config,
        this->getPlatform()->getAsic(),
        this->getHwSwitchEnsemble()->isSai());
    addPermitIpAcl(config, "acl1", kIp);
    this->applyNewConfig(config);
  };

  auto setupPostWb = [this, kIp]() {
    auto config = this->initialConfig();
    // Drop COPP acls
    addDenyPortAcl(config, "acl0");
    addPermitIpAcl(config, "acl1", kIp);
    this->applyNewConfig(config);
  };

  this->verifyAcrossWarmBoots(
      setup, []() {}, setupPostWb, []() {});
}

TYPED_TEST(HwAclPriorityTest, Reprioritize) {
  auto setup = [=, this]() {
    auto config = this->initialConfig();
    addPermitIpAcl(config, "B", folly::IPAddress("1::2"));
    addPermitIpAcl(config, "A", folly::IPAddress("1::3"));

    cfg::CPUTrafficPolicyConfig cpuConfig;
    cfg::TrafficPolicyConfig trafficConfig;
    trafficConfig.matchToAction()->resize(2);
    cfg::MatchAction matchAction = getToQueueAction(
        1, this->getHwSwitchEnsemble()->isSai(), cfg::ToCpuAction::TRAP);
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
    auto config = this->initialConfig();
    addPermitIpAcl(config, "A", folly::IPAddress("1::3"));
    addPermitIpAcl(config, "B", folly::IPAddress("1::2"));

    cfg::CPUTrafficPolicyConfig cpuConfig;
    cfg::TrafficPolicyConfig trafficConfig;
    trafficConfig.matchToAction()->resize(2);
    cfg::MatchAction matchAction = getToQueueAction(
        1, this->getHwSwitchEnsemble()->isSai(), cfg::ToCpuAction::TRAP);
    for (int i = 0; i < 2; i++) {
      auto& acls = utility::getAcls(&config, std::nullopt);
      trafficConfig.matchToAction()[i].matcher() = *acls[i].name();
      trafficConfig.matchToAction()[i].action() = matchAction;
    }
    cpuConfig.trafficPolicy() = trafficConfig;
    config.cpuTrafficPolicy() = cpuConfig;
    this->applyNewConfig(config);
  };

  this->verifyAcrossWarmBoots(
      setup, []() {}, setupPostWb, []() {});
}
} // namespace facebook::fboss
