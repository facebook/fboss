/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/test/HwTest.h"

#include <folly/Random.h>
#include "fboss/agent/ApplyThriftConfig.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/CounterUtils.h"
#include "fboss/agent/hw/test/HwTestAclUtils.h"
#include "fboss/agent/hw/test/HwTestCoppUtils.h"
#include "fboss/agent/hw/test/dataplane_tests/HwTestQueuePerHostUtils.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"

#include "fboss/agent/hw/test/ConfigFactory.h"

#include <fb303/ServiceData.h>

namespace facebook::fboss {

template <bool enableMultiAclTable>
struct EnableMultiAclTableT {
  static constexpr auto multiAclTableEnabled = enableMultiAclTable;
};

using TestTypes =
    ::testing::Types<EnableMultiAclTableT<false>, EnableMultiAclTableT<true>>;

template <typename EnableMultiAclTableT>
class HwAclStatTest : public HwTest {
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

  cfg::AclEntry* addDscpAcl(
      cfg::SwitchConfig* cfg,
      const std::string& aclName) {
    auto* acl = utility::addAcl(cfg, aclName);
    // ACL requires at least one qualifier
    acl->dscp() = 0x24;

    return acl;
  }
};

TYPED_TEST_SUITE(HwAclStatTest, TestTypes);

TYPED_TEST(HwAclStatTest, AclStatCreate) {
  auto setup = [=, this]() {
    auto newCfg = this->initialConfig();
    this->addDscpAcl(&newCfg, "acl0");
    utility::addAclStat(
        &newCfg,
        "acl0",
        "stat0",
        utility::getAclCounterTypes(this->getHwSwitch()));
    this->applyNewConfig(newCfg);
  };

  auto verify = [=, this]() {
    utility::checkAclEntryAndStatCount(
        this->getHwSwitch(),
        /*ACLs*/ 1,
        /*stats*/ 1,
        /*counters*/ utility::getAclCounterTypes(this->getHwSwitch()).size());
    utility::checkAclStat(
        this->getHwSwitch(),
        this->getProgrammedState(),
        {"acl0"},
        "stat0",
        utility::getAclCounterTypes(this->getHwSwitch()));
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwAclStatTest, AclStatCreateDeleteCreate) {
  auto setup = [=, this]() {
    auto newCfg = this->initialConfig();
    this->addDscpAcl(&newCfg, "acl0");
    utility::addAclStat(
        &newCfg,
        "acl0",
        "stat0",
        utility::getAclCounterTypes(this->getHwSwitch()));
    this->applyNewConfig(newCfg);
  };

  auto verify = [=, this]() {
    EXPECT_TRUE(facebook::fb303::fbData->getStatMap()->contains(
        utility::statNameFromCounterType("stat0", cfg::CounterType::PACKETS)));

    auto newCfg = this->initialConfig();
    this->applyNewConfig(newCfg);
    EXPECT_FALSE(facebook::fb303::fbData->getStatMap()->contains(
        utility::statNameFromCounterType("stat0", cfg::CounterType::PACKETS)));

    auto newCfg2 = this->initialConfig();
    this->addDscpAcl(&newCfg2, "acl0");
    utility::addAclStat(
        &newCfg2,
        "acl0",
        "stat0",
        utility::getAclCounterTypes(this->getHwSwitch()));
    this->applyNewConfig(newCfg2);
    EXPECT_TRUE(facebook::fb303::fbData->getStatMap()->contains(
        utility::statNameFromCounterType("stat0", cfg::CounterType::PACKETS)));
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwAclStatTest, AclStatMultipleCounters) {
  auto setup = [=, this]() {
    auto newCfg = this->initialConfig();
    this->addDscpAcl(&newCfg, "acl0");
    utility::addAclStat(
        &newCfg,
        "acl0",
        "stat0",
        {cfg::CounterType::BYTES, cfg::CounterType::PACKETS});
    this->applyNewConfig(newCfg);
  };

  auto verify = [=, this]() {
    utility::checkAclEntryAndStatCount(
        this->getHwSwitch(), /*ACLs*/ 1, /*stats*/ 1, /*counters*/ 2);
    utility::checkAclStat(
        this->getHwSwitch(),
        this->getProgrammedState(),
        {"acl0"},
        "stat0",
        {cfg::CounterType::BYTES, cfg::CounterType::PACKETS});
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwAclStatTest, AclStatChangeCounterType) {
  auto setup = [=, this]() {
    auto newCfg = this->initialConfig();
    this->addDscpAcl(&newCfg, "acl0");
    utility::addAclStat(&newCfg, "acl0", "stat0", {cfg::CounterType::PACKETS});
    this->applyNewConfig(newCfg);
  };

  auto verify = [=, this]() {
    utility::checkAclEntryAndStatCount(
        this->getHwSwitch(), /*ACLs*/ 1, /*stats*/ 1, /*counters*/ 1);
    utility::checkAclStat(
        this->getHwSwitch(),
        this->getProgrammedState(),
        {"acl0"},
        "stat0",
        {cfg::CounterType::PACKETS});
  };

  auto setupPostWB = [&]() {
    auto newCfg = this->initialConfig();
    this->addDscpAcl(&newCfg, "acl0");
    utility::addAclStat(&newCfg, "acl0", "stat0", {cfg::CounterType::BYTES});
    this->applyNewConfig(newCfg);
  };

  auto verifyPostWB = [=, this]() {
    utility::checkAclEntryAndStatCount(
        this->getHwSwitch(), /*ACLs*/ 1, /*stats*/ 1, /*counters*/ 1);
    utility::checkAclStat(
        this->getHwSwitch(),
        this->getProgrammedState(),
        {"acl0"},
        "stat0",
        {cfg::CounterType::BYTES});
  };

  this->verifyAcrossWarmBoots(setup, verify, setupPostWB, verifyPostWB);
}

TYPED_TEST(HwAclStatTest, AclStatCreateShared) {
  auto setup = [=, this]() {
    auto newCfg = this->initialConfig();
    this->addDscpAcl(&newCfg, "acl0");
    this->addDscpAcl(&newCfg, "acl1");
    utility::addAclStat(
        &newCfg,
        "acl0",
        "stat",
        utility::getAclCounterTypes(this->getHwSwitch()));
    utility::addAclStat(
        &newCfg,
        "acl1",
        "stat",
        utility::getAclCounterTypes(this->getHwSwitch()));
    this->applyNewConfig(newCfg);
  };

  auto verify = [=, this]() {
    utility::checkAclEntryAndStatCount(
        this->getHwSwitch(),
        /*ACLs*/ 2,
        /*stats*/ 1,
        /*counters*/ utility::getAclCounterTypes(this->getHwSwitch()).size());
    utility::checkAclStat(
        this->getHwSwitch(),
        this->getProgrammedState(),
        {"acl0", "acl1"},
        "stat",
        utility::getAclCounterTypes(this->getHwSwitch()));
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwAclStatTest, AclStatCreateMultiple) {
  auto setup = [=, this]() {
    auto newCfg = this->initialConfig();
    this->addDscpAcl(&newCfg, "acl0");
    this->addDscpAcl(&newCfg, "acl1");
    utility::addAclStat(
        &newCfg,
        "acl0",
        "stat0",
        utility::getAclCounterTypes(this->getHwSwitch()));
    utility::addAclStat(
        &newCfg,
        "acl1",
        "stat1",
        utility::getAclCounterTypes(this->getHwSwitch()));
    this->applyNewConfig(newCfg);
  };

  auto verify = [=, this]() {
    utility::checkAclEntryAndStatCount(
        this->getHwSwitch(),
        /*ACLs*/ 2,
        /*stats*/ 2,
        /*counters*/ 2 *
            utility::getAclCounterTypes(this->getHwSwitch()).size());
    utility::checkAclStat(
        this->getHwSwitch(),
        this->getProgrammedState(),
        {"acl0"},
        "stat0",
        utility::getAclCounterTypes(this->getHwSwitch()));
    utility::checkAclStat(
        this->getHwSwitch(),
        this->getProgrammedState(),
        {"acl1"},
        "stat1",
        utility::getAclCounterTypes(this->getHwSwitch()));
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwAclStatTest, AclStatMultipleActions) {
  auto setup = [=, this]() {
    auto newCfg = this->initialConfig();
    this->addDscpAcl(&newCfg, "acl0");
    /* The ACL will have 2 actions: a counter and a queue */
    utility::addAclStat(
        &newCfg,
        "acl0",
        "stat0",
        utility::getAclCounterTypes(this->getHwSwitch()));
    cfg::MatchAction matchAction =
        utility::getToQueueAction(0, this->getHwSwitchEnsemble()->isSai());
    cfg::MatchToAction action = cfg::MatchToAction();
    *action.matcher() = "acl0";
    *action.action() = matchAction;
    newCfg.dataPlaneTrafficPolicy()->matchToAction()->push_back(action);
    this->applyNewConfig(newCfg);
  };

  auto verify = [=, this]() {
    utility::checkAclEntryAndStatCount(
        this->getHwSwitch(),
        /*ACLs*/ 1,
        /*stats*/ 1,
        /*counters*/ utility::getAclCounterTypes(this->getHwSwitch()).size());
    utility::checkAclStat(
        this->getHwSwitch(),
        this->getProgrammedState(),
        {"acl0"},
        "stat0",
        utility::getAclCounterTypes(this->getHwSwitch()));
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwAclStatTest, AclStatDelete) {
  auto setup = [=, this]() {
    auto newCfg = this->initialConfig();
    this->addDscpAcl(&newCfg, "acl0");
    utility::addAclStat(
        &newCfg,
        "acl0",
        "stat0",
        utility::getAclCounterTypes(this->getHwSwitch()));
    this->applyNewConfig(newCfg);
  };

  auto verify = [=, this]() {
    utility::checkAclEntryAndStatCount(
        this->getHwSwitch(),
        /* ACLs */ 1,
        /* Stats */ 1,
        /*counters*/ utility::getAclCounterTypes(this->getHwSwitch()).size());
    utility::checkAclStat(
        this->getHwSwitch(),
        this->getProgrammedState(),
        {"acl0"},
        "stat0",
        utility::getAclCounterTypes(this->getHwSwitch()));
  };

  auto setupPostWB = [&]() {
    auto newCfg = this->initialConfig();
    this->applyNewConfig(newCfg);
  };

  auto verifyPostWB = [=, this]() {
    utility::checkAclEntryAndStatCount(
        this->getHwSwitch(), /*ACLs*/ 0, /*stats*/ 0, /*counters*/ 0);
    utility::checkAclStatDeleted(this->getHwSwitch(), "stat0");
  };

  this->verifyAcrossWarmBoots(setup, verify, setupPostWB, verifyPostWB);
}

TYPED_TEST(HwAclStatTest, AclStatCreatePostWarmBoot) {
  auto setup = [=, this]() {
    auto newCfg = this->initialConfig();
    this->applyNewConfig(newCfg);
  };

  auto verify = [=]() {};

  auto setupPostWB = [&]() {
    auto newCfg = this->initialConfig();
    this->addDscpAcl(&newCfg, "acl0");
    utility::addAclStat(
        &newCfg,
        "acl0",
        "stat0",
        utility::getAclCounterTypes(this->getHwSwitch()));
    this->applyNewConfig(newCfg);
  };

  auto verifyPostWB = [=, this]() {
    const auto& aclCounter = utility::getAclCounterTypes(this->getHwSwitch());
    utility::checkAclEntryAndStatCount(
        this->getHwSwitch(),
        /*ACLs*/ 1,
        /*stats*/ 1,
        /*counters*/ 1 * aclCounter.size());
    utility::checkAclStat(
        this->getHwSwitch(),
        this->getProgrammedState(),
        {"acl0"},
        "stat0",
        aclCounter);
  };

  this->verifyAcrossWarmBoots(setup, verify, setupPostWB, verifyPostWB);
}

TYPED_TEST(HwAclStatTest, AclStatDeleteSharedPostWarmBoot) {
  auto setup = [=, this]() {
    auto newCfg = this->initialConfig();
    this->addDscpAcl(&newCfg, "acl0");
    this->addDscpAcl(&newCfg, "acl1");
    utility::addAclStat(
        &newCfg,
        "acl0",
        "stat",
        utility::getAclCounterTypes(this->getHwSwitch()));
    utility::addAclStat(
        &newCfg,
        "acl1",
        "stat",
        utility::getAclCounterTypes(this->getHwSwitch()));
    this->applyNewConfig(newCfg);
  };

  auto verify = [=, this]() {
    utility::checkAclEntryAndStatCount(
        this->getHwSwitch(),
        /* ACLs */ 2,
        /* Stats */ 1,
        /*counters*/ utility::getAclCounterTypes(this->getHwSwitch()).size());
    utility::checkAclStat(
        this->getHwSwitch(),
        this->getProgrammedState(),
        {"acl0", "acl1"},
        "stat",
        utility::getAclCounterTypes(this->getHwSwitch()));
  };

  auto setupPostWB = [&]() {
    auto newCfg = this->initialConfig();
    this->applyNewConfig(newCfg);
  };

  auto verifyPostWB = [=, this]() {
    utility::checkAclEntryAndStatCount(
        this->getHwSwitch(), /*ACLs*/ 0, /*stats*/ 0, /*counters*/ 0);
    utility::checkAclStatDeleted(this->getHwSwitch(), "stat0");
  };

  this->verifyAcrossWarmBoots(setup, verify, setupPostWB, verifyPostWB);
}

TYPED_TEST(HwAclStatTest, AclStatCreateSharedPostWarmBoot) {
  auto setup = [=, this]() {
    auto newCfg = this->initialConfig();
    this->applyNewConfig(newCfg);
  };

  auto verify = [=]() {};

  auto setupPostWB = [=, this]() {
    auto newCfg = this->initialConfig();
    this->addDscpAcl(&newCfg, "acl0");
    this->addDscpAcl(&newCfg, "acl1");
    utility::addAclStat(
        &newCfg,
        "acl0",
        "stat",
        utility::getAclCounterTypes(this->getHwSwitch()));
    utility::addAclStat(
        &newCfg,
        "acl1",
        "stat",
        utility::getAclCounterTypes(this->getHwSwitch()));
    this->applyNewConfig(newCfg);
  };

  auto verifyPostWB = [=, this]() {
    utility::checkAclEntryAndStatCount(
        this->getHwSwitch(),
        /*ACLs*/ 2,
        /*stats*/ 1,
        /*counters*/ utility::getAclCounterTypes(this->getHwSwitch()).size());
    utility::checkAclStat(
        this->getHwSwitch(),
        this->getProgrammedState(),
        {"acl0", "acl1"},
        "stat",
        utility::getAclCounterTypes(this->getHwSwitch()));
  };

  this->verifyAcrossWarmBoots(setup, verify, setupPostWB, verifyPostWB);
}

TYPED_TEST(HwAclStatTest, AclStatDeleteShared) {
  auto setup = [=, this]() {
    auto newCfg = this->initialConfig();
    this->addDscpAcl(&newCfg, "acl0");
    this->addDscpAcl(&newCfg, "acl1");
    utility::addAclStat(
        &newCfg,
        "acl0",
        "stat",
        utility::getAclCounterTypes(this->getHwSwitch()));
    utility::addAclStat(
        &newCfg,
        "acl1",
        "stat",
        utility::getAclCounterTypes(this->getHwSwitch()));
    this->applyNewConfig(newCfg);
  };

  auto verify = [=, this]() {
    utility::checkAclEntryAndStatCount(
        this->getHwSwitch(),
        /* ACLs */ 2,
        /* Stats */ 1,
        /*counters*/ utility::getAclCounterTypes(this->getHwSwitch()).size());
    utility::checkAclStat(
        this->getHwSwitch(),
        this->getProgrammedState(),
        {"acl0", "acl1"},
        "stat",
        utility::getAclCounterTypes(this->getHwSwitch()));
  };

  auto setupPostWB = [=, this]() {
    auto newCfg = this->initialConfig();
    this->addDscpAcl(&newCfg, "acl1");
    utility::addAclStat(
        &newCfg,
        "acl1",
        "stat",
        utility::getAclCounterTypes(this->getHwSwitch()));
    this->applyNewConfig(newCfg);
  };

  auto verifyPostWB = [=, this]() {
    utility::checkAclEntryAndStatCount(
        this->getHwSwitch(),
        /*ACLs*/ 1,
        /*stats*/ 1,
        /*counters*/ utility::getAclCounterTypes(this->getHwSwitch()).size());
    utility::checkAclStat(
        this->getHwSwitch(),
        this->getProgrammedState(),
        {"acl1"},
        "stat",
        utility::getAclCounterTypes(this->getHwSwitch()));
  };

  this->verifyAcrossWarmBoots(setup, verify, setupPostWB, verifyPostWB);
}

TYPED_TEST(HwAclStatTest, AclStatRename) {
  auto setup = [=, this]() {
    auto newCfg = this->initialConfig();
    this->addDscpAcl(&newCfg, "acl0");
    utility::addAclStat(
        &newCfg,
        "acl0",
        "stat0",
        utility::getAclCounterTypes(this->getHwSwitch()));
    this->applyNewConfig(newCfg);
  };

  auto verify = [=, this]() {
    utility::checkAclEntryAndStatCount(
        this->getHwSwitch(),
        /* ACLs */ 1,
        /* Stats */ 1,
        /*counters*/ utility::getAclCounterTypes(this->getHwSwitch()).size());
    utility::checkAclStat(
        this->getHwSwitch(),
        this->getProgrammedState(),
        {"acl0"},
        "stat0",
        utility::getAclCounterTypes(this->getHwSwitch()));
  };

  auto setupPostWB = [=, this]() {
    auto newCfg = this->initialConfig();
    this->addDscpAcl(&newCfg, "acl0");
    utility::addAclStat(
        &newCfg,
        "acl0",
        "stat1",
        utility::getAclCounterTypes(this->getHwSwitch()));
    this->applyNewConfig(newCfg);
  };

  auto verifyPostWB = [=, this]() {
    utility::checkAclEntryAndStatCount(
        this->getHwSwitch(),
        /*ACLs*/ 1,
        /*stats*/ 1,
        /*counters*/ utility::getAclCounterTypes(this->getHwSwitch()).size());
    utility::checkAclStat(
        this->getHwSwitch(),
        this->getProgrammedState(),
        {"acl0"},
        "stat1",
        utility::getAclCounterTypes(this->getHwSwitch()));
    utility::checkAclStatDeleted(this->getHwSwitch(), "stat0");
  };

  this->verifyAcrossWarmBoots(setup, verify, setupPostWB, verifyPostWB);
}

TYPED_TEST(HwAclStatTest, AclStatRenameShared) {
  auto setup = [=, this]() {
    auto newCfg = this->initialConfig();
    this->addDscpAcl(&newCfg, "acl0");
    this->addDscpAcl(&newCfg, "acl1");
    utility::addAclStat(
        &newCfg,
        "acl0",
        "stat0",
        utility::getAclCounterTypes(this->getHwSwitch()));
    utility::addAclStat(
        &newCfg,
        "acl1",
        "stat0",
        utility::getAclCounterTypes(this->getHwSwitch()));
    this->applyNewConfig(newCfg);
  };

  auto verify = [=, this]() {
    utility::checkAclEntryAndStatCount(
        this->getHwSwitch(),
        /* ACLs */ 2,
        /* Stats */ 1,
        /*counters*/ utility::getAclCounterTypes(this->getHwSwitch()).size());
    utility::checkAclStat(
        this->getHwSwitch(),
        this->getProgrammedState(),
        {"acl0", "acl1"},
        "stat0",
        utility::getAclCounterTypes(this->getHwSwitch()));
  };

  auto setupPostWB = [=, this]() {
    auto newCfg = this->initialConfig();
    this->addDscpAcl(&newCfg, "acl0");
    this->addDscpAcl(&newCfg, "acl1");
    utility::addAclStat(
        &newCfg,
        "acl0",
        "stat0",
        utility::getAclCounterTypes(this->getHwSwitch()));
    utility::addAclStat(
        &newCfg,
        "acl1",
        "stat1",
        utility::getAclCounterTypes(this->getHwSwitch()));
    this->applyNewConfig(newCfg);
  };

  auto verifyPostWB = [=, this]() {
    utility::checkAclEntryAndStatCount(
        this->getHwSwitch(),
        /*ACLs*/ 2,
        /*stats*/ 2,
        /*counters*/ 2 *
            utility::getAclCounterTypes(this->getHwSwitch()).size());
    utility::checkAclStat(
        this->getHwSwitch(),
        this->getProgrammedState(),
        {"acl0"},
        "stat0",
        utility::getAclCounterTypes(this->getHwSwitch()));
    utility::checkAclStat(
        this->getHwSwitch(),
        this->getProgrammedState(),
        {"acl1"},
        "stat1",
        utility::getAclCounterTypes(this->getHwSwitch()));
  };

  this->verifyAcrossWarmBoots(setup, verify, setupPostWB, verifyPostWB);
}

TYPED_TEST(HwAclStatTest, AclStatCreateSameTwice) {
  auto state = this->getProgrammedState();
  auto newCfg = this->initialConfig();
  this->addDscpAcl(&newCfg, "acl0");
  utility::addAclStat(
      &newCfg,
      "acl0",
      "stat0",
      utility::getAclCounterTypes(this->getHwSwitch()));
  this->applyNewConfig(newCfg);
  StateDelta delta(state, this->getProgrammedState());
  // adding same ACL twice with oper delta and state maintained in HW switch
  // leads to process change, not process added.
  EXPECT_NO_THROW(this->getHwSwitch()->stateChanged(delta));
}

TYPED_TEST(HwAclStatTest, AclStatDeleteNonExistent) {
  auto newCfg = this->initialConfig();
  this->addDscpAcl(&newCfg, "acl0");
  utility::addAclStat(
      &newCfg,
      "acl0",
      "stat0",
      utility::getAclCounterTypes(this->getHwSwitch()));
  this->applyNewConfig(newCfg);

  auto state = this->getProgrammedState();
  utility::delAcl(&newCfg, "acl0");
  utility::delAclStat(&newCfg, "acl0", "stat0");
  this->applyNewConfig(newCfg);
  StateDelta delta(state, this->getProgrammedState());

  // TODO(pshaikh): uncomment next release
  // EXPECT_THROW(getHwSwitch()->stateChanged(delta), FbossError);
}

TYPED_TEST(HwAclStatTest, AclStatModify) {
  auto setup = [=, this]() {
    auto newCfg = this->initialConfig();
    this->addDscpAcl(&newCfg, "acl0");
    utility::addAclStat(
        &newCfg,
        "acl0",
        "stat0",
        utility::getAclCounterTypes(this->getHwSwitch()));
    this->applyNewConfig(newCfg);
  };

  auto verify = [=, this]() {
    utility::checkAclEntryAndStatCount(
        this->getHwSwitch(),
        /* ACLs */ 1,
        /* Stats */ 1,
        /*counters*/ utility::getAclCounterTypes(this->getHwSwitch()).size());
    utility::checkAclStat(
        this->getHwSwitch(),
        this->getProgrammedState(),
        {"acl0"},
        "stat0",
        utility::getAclCounterTypes(this->getHwSwitch()));
  };

  auto setupPostWB = [=, this]() {
    auto newCfg = this->initialConfig();
    auto acl = this->addDscpAcl(&newCfg, "acl0");
    acl->proto() = 58;
    utility::addAclStat(
        &newCfg,
        "acl0",
        "stat0",
        utility::getAclCounterTypes(this->getHwSwitch()));
    this->applyNewConfig(newCfg);
  };

  this->verifyAcrossWarmBoots(setup, verify, setupPostWB, verify);
}

TYPED_TEST(HwAclStatTest, AclStatShuffle) {
  auto setup = [=, this]() {
    auto newCfg = this->initialConfig();
    this->addDscpAcl(&newCfg, "acl0");
    this->addDscpAcl(&newCfg, "acl1");
    utility::addAclStat(
        &newCfg,
        "acl0",
        "stat0",
        utility::getAclCounterTypes(this->getHwSwitch()));
    utility::addAclStat(
        &newCfg,
        "acl1",
        "stat1",
        utility::getAclCounterTypes(this->getHwSwitch()));
    this->applyNewConfig(newCfg);
  };

  auto verify = [=, this]() {
    utility::checkAclEntryAndStatCount(
        this->getHwSwitch(),
        /* ACLs */ 2,
        /* Stats */ 2,
        /*counters*/ 2 *
            utility::getAclCounterTypes(this->getHwSwitch()).size());
    utility::checkAclStat(
        this->getHwSwitch(),
        this->getProgrammedState(),
        {"acl0"},
        "stat0",
        utility::getAclCounterTypes(this->getHwSwitch()));
    utility::checkAclStat(
        this->getHwSwitch(),
        this->getProgrammedState(),
        {"acl1"},
        "stat1",
        utility::getAclCounterTypes(this->getHwSwitch()));
  };

  auto setupPostWB = [=, this]() {
    auto newCfg = this->initialConfig();
    this->addDscpAcl(&newCfg, "acl1");
    this->addDscpAcl(&newCfg, "acl0");
    utility::addAclStat(
        &newCfg,
        "acl1",
        "stat1",
        utility::getAclCounterTypes(this->getHwSwitch()));
    utility::addAclStat(
        &newCfg,
        "acl0",
        "stat0",
        utility::getAclCounterTypes(this->getHwSwitch()));
    this->applyNewConfig(newCfg);
  };

  this->verifyAcrossWarmBoots(setup, verify, setupPostWB, verify);
}

TYPED_TEST(HwAclStatTest, StatNumberOfCounters) {
  auto setup = [=, this]() {
    auto newCfg = this->initialConfig();
    this->addDscpAcl(&newCfg, "acl0");
    utility::addAclStat(
        &newCfg,
        "acl0",
        "stat0",
        utility::getAclCounterTypes(this->getHwSwitch()));
    this->applyNewConfig(newCfg);
  };

  auto verify = [=, this]() {
    utility::checkAclEntryAndStatCount(
        this->getHwSwitch(),
        /* ACLs */ 1,
        /* Stats */ 1,
        /*counters*/ utility::getAclCounterTypes(this->getHwSwitch()).size());
    utility::checkAclStat(
        this->getHwSwitch(),
        this->getProgrammedState(),
        {"acl0"},
        "stat0",
        utility::getAclCounterTypes(this->getHwSwitch()));
  };

  auto setupPostWB = [=]() {};

  auto verifyPostWB = [=, this]() {
    verify();
    utility::checkAclStatSize(this->getHwSwitch(), "stat0");
  };
  this->verifyAcrossWarmBoots(setup, verify, setupPostWB, verifyPostWB);
}

TYPED_TEST(HwAclStatTest, EmptyAclCheck) {
  // TODO(joseph5wu) Current this test failed on TH4 because of CS00011697882
  auto setup = [this]() {
    auto newCfg = this->initialConfig();
    this->applyNewConfig(newCfg);
  };
  auto verify = [this]() {
    EXPECT_EQ(utility::getAclTableNumAclEntries(this->getHwSwitch()), 0);
    utility::checkAclEntryAndStatCount(
        this->getHwSwitch(), /*ACLs*/ 0, /*stats*/ 0, /*counters*/ 0);
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
