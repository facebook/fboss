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
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/utils/QueuePerHostTestUtils.h"

#include "fboss/agent/hw/test/ConfigFactory.h"

#include <fb303/ServiceData.h>

namespace facebook::fboss {

class HwAclStatTest : public HwTest {
 protected:
  void SetUp() override {
    HwTest::SetUp();
    /*
     * Native SDK does not support multi acl feature.
     * So skip multi acl tests for fake bcm sdk
     */
    if ((this->getPlatform()->getAsic()->getAsicType() ==
         cfg::AsicType::ASIC_TYPE_FAKE) &&
        (FLAGS_enable_acl_table_group)) {
      GTEST_SKIP();
    }
  }

  cfg::SwitchConfig initialConfig() const {
    auto cfg = utility::onePortPerInterfaceConfig(
        getHwSwitch(),
        masterLogicalPortIds(),
        getAsic()->desiredLoopbackModes());
    if (FLAGS_enable_acl_table_group) {
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
    utility::addEtherTypeToAcl(getAsic(), acl, cfg::EtherType::IPv6);

    return acl;
  }
};

TEST_F(HwAclStatTest, AclStatMultipleActions) {
  auto setup = [=, this]() {
    auto newCfg = this->initialConfig();
    this->addDscpAcl(&newCfg, "acl0");
    /* The ACL will have 2 actions: a counter and a queue */
    utility::addAclStat(
        &newCfg,
        "acl0",
        "stat0",
        utility::getAclCounterTypes(this->getHwSwitchEnsemble()->getL3Asics()));
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
        /*counters*/
        utility::getAclCounterTypes(this->getHwSwitchEnsemble()->getL3Asics())
            .size());
    utility::checkAclStat(
        this->getHwSwitch(),
        this->getProgrammedState(),
        {"acl0"},
        "stat0",
        utility::getAclCounterTypes(this->getHwSwitchEnsemble()->getL3Asics()));
  };

  this->verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwAclStatTest, AclStatDelete) {
  auto setup = [=, this]() {
    auto newCfg = this->initialConfig();
    this->addDscpAcl(&newCfg, "acl0");
    utility::addAclStat(
        &newCfg,
        "acl0",
        "stat0",
        utility::getAclCounterTypes(this->getHwSwitchEnsemble()->getL3Asics()));
    this->applyNewConfig(newCfg);
  };

  auto verify = [=, this]() {
    utility::checkAclEntryAndStatCount(
        this->getHwSwitch(),
        /* ACLs */ 1,
        /* Stats */ 1,
        /*counters*/
        utility::getAclCounterTypes(this->getHwSwitchEnsemble()->getL3Asics())
            .size());
    utility::checkAclStat(
        this->getHwSwitch(),
        this->getProgrammedState(),
        {"acl0"},
        "stat0",
        utility::getAclCounterTypes(this->getHwSwitchEnsemble()->getL3Asics()));
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

TEST_F(HwAclStatTest, AclStatCreatePostWarmBoot) {
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
        utility::getAclCounterTypes(this->getHwSwitchEnsemble()->getL3Asics()));
    this->applyNewConfig(newCfg);
  };

  auto verifyPostWB = [=, this]() {
    const auto& aclCounter =
        utility::getAclCounterTypes(this->getHwSwitchEnsemble()->getL3Asics());
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

TEST_F(HwAclStatTest, AclStatDeleteSharedPostWarmBoot) {
  auto setup = [=, this]() {
    auto newCfg = this->initialConfig();
    this->addDscpAcl(&newCfg, "acl0");
    this->addDscpAcl(&newCfg, "acl1");
    utility::addAclStat(
        &newCfg,
        "acl0",
        "stat",
        utility::getAclCounterTypes(this->getHwSwitchEnsemble()->getL3Asics()));
    utility::addAclStat(
        &newCfg,
        "acl1",
        "stat",
        utility::getAclCounterTypes(this->getHwSwitchEnsemble()->getL3Asics()));
    this->applyNewConfig(newCfg);
  };

  auto verify = [=, this]() {
    utility::checkAclEntryAndStatCount(
        this->getHwSwitch(),
        /* ACLs */ 2,
        /* Stats */ 1,
        /*counters*/
        utility::getAclCounterTypes(this->getHwSwitchEnsemble()->getL3Asics())
            .size());
    utility::checkAclStat(
        this->getHwSwitch(),
        this->getProgrammedState(),
        {"acl0", "acl1"},
        "stat",
        utility::getAclCounterTypes(this->getHwSwitchEnsemble()->getL3Asics()));
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

TEST_F(HwAclStatTest, AclStatCreateSharedPostWarmBoot) {
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
        utility::getAclCounterTypes(this->getHwSwitchEnsemble()->getL3Asics()));
    utility::addAclStat(
        &newCfg,
        "acl1",
        "stat",
        utility::getAclCounterTypes(this->getHwSwitchEnsemble()->getL3Asics()));
    this->applyNewConfig(newCfg);
  };

  auto verifyPostWB = [=, this]() {
    utility::checkAclEntryAndStatCount(
        this->getHwSwitch(),
        /*ACLs*/ 2,
        /*stats*/ 1,
        /*counters*/
        utility::getAclCounterTypes(this->getHwSwitchEnsemble()->getL3Asics())
            .size());
    utility::checkAclStat(
        this->getHwSwitch(),
        this->getProgrammedState(),
        {"acl0", "acl1"},
        "stat",
        utility::getAclCounterTypes(this->getHwSwitchEnsemble()->getL3Asics()));
  };

  this->verifyAcrossWarmBoots(setup, verify, setupPostWB, verifyPostWB);
}

TEST_F(HwAclStatTest, AclStatDeleteShared) {
  auto setup = [=, this]() {
    auto newCfg = this->initialConfig();
    this->addDscpAcl(&newCfg, "acl0");
    this->addDscpAcl(&newCfg, "acl1");
    utility::addAclStat(
        &newCfg,
        "acl0",
        "stat",
        utility::getAclCounterTypes(this->getHwSwitchEnsemble()->getL3Asics()));
    utility::addAclStat(
        &newCfg,
        "acl1",
        "stat",
        utility::getAclCounterTypes(this->getHwSwitchEnsemble()->getL3Asics()));
    this->applyNewConfig(newCfg);
  };

  auto verify = [=, this]() {
    utility::checkAclEntryAndStatCount(
        this->getHwSwitch(),
        /* ACLs */ 2,
        /* Stats */ 1,
        /*counters*/
        utility::getAclCounterTypes(this->getHwSwitchEnsemble()->getL3Asics())
            .size());
    utility::checkAclStat(
        this->getHwSwitch(),
        this->getProgrammedState(),
        {"acl0", "acl1"},
        "stat",
        utility::getAclCounterTypes(this->getHwSwitchEnsemble()->getL3Asics()));
  };

  auto setupPostWB = [=, this]() {
    auto newCfg = this->initialConfig();
    this->addDscpAcl(&newCfg, "acl1");
    utility::addAclStat(
        &newCfg,
        "acl1",
        "stat",
        utility::getAclCounterTypes(this->getHwSwitchEnsemble()->getL3Asics()));
    this->applyNewConfig(newCfg);
  };

  auto verifyPostWB = [=, this]() {
    utility::checkAclEntryAndStatCount(
        this->getHwSwitch(),
        /*ACLs*/ 1,
        /*stats*/ 1,
        /*counters*/
        utility::getAclCounterTypes(this->getHwSwitchEnsemble()->getL3Asics())
            .size());
    utility::checkAclStat(
        this->getHwSwitch(),
        this->getProgrammedState(),
        {"acl1"},
        "stat",
        utility::getAclCounterTypes(this->getHwSwitchEnsemble()->getL3Asics()));
  };

  this->verifyAcrossWarmBoots(setup, verify, setupPostWB, verifyPostWB);
}

TEST_F(HwAclStatTest, AclStatRename) {
  auto setup = [=, this]() {
    auto newCfg = this->initialConfig();
    this->addDscpAcl(&newCfg, "acl0");
    utility::addAclStat(
        &newCfg,
        "acl0",
        "stat0",
        utility::getAclCounterTypes(this->getHwSwitchEnsemble()->getL3Asics()));
    this->applyNewConfig(newCfg);
  };

  auto verify = [=, this]() {
    utility::checkAclEntryAndStatCount(
        this->getHwSwitch(),
        /* ACLs */ 1,
        /* Stats */ 1,
        /*counters*/
        utility::getAclCounterTypes(this->getHwSwitchEnsemble()->getL3Asics())
            .size());
    utility::checkAclStat(
        this->getHwSwitch(),
        this->getProgrammedState(),
        {"acl0"},
        "stat0",
        utility::getAclCounterTypes(this->getHwSwitchEnsemble()->getL3Asics()));
  };

  auto setupPostWB = [=, this]() {
    auto newCfg = this->initialConfig();
    this->addDscpAcl(&newCfg, "acl0");
    utility::addAclStat(
        &newCfg,
        "acl0",
        "stat1",
        utility::getAclCounterTypes(this->getHwSwitchEnsemble()->getL3Asics()));
    this->applyNewConfig(newCfg);
  };

  auto verifyPostWB = [=, this]() {
    utility::checkAclEntryAndStatCount(
        this->getHwSwitch(),
        /*ACLs*/ 1,
        /*stats*/ 1,
        /*counters*/
        utility::getAclCounterTypes(this->getHwSwitchEnsemble()->getL3Asics())
            .size());
    utility::checkAclStat(
        this->getHwSwitch(),
        this->getProgrammedState(),
        {"acl0"},
        "stat1",
        utility::getAclCounterTypes(this->getHwSwitchEnsemble()->getL3Asics()));
    utility::checkAclStatDeleted(this->getHwSwitch(), "stat0");
  };

  this->verifyAcrossWarmBoots(setup, verify, setupPostWB, verifyPostWB);
}

TEST_F(HwAclStatTest, AclStatRenameShared) {
  auto setup = [=, this]() {
    auto newCfg = this->initialConfig();
    this->addDscpAcl(&newCfg, "acl0");
    this->addDscpAcl(&newCfg, "acl1");
    utility::addAclStat(
        &newCfg,
        "acl0",
        "stat0",
        utility::getAclCounterTypes(this->getHwSwitchEnsemble()->getL3Asics()));
    utility::addAclStat(
        &newCfg,
        "acl1",
        "stat0",
        utility::getAclCounterTypes(this->getHwSwitchEnsemble()->getL3Asics()));
    this->applyNewConfig(newCfg);
  };

  auto verify = [=, this]() {
    utility::checkAclEntryAndStatCount(
        this->getHwSwitch(),
        /* ACLs */ 2,
        /* Stats */ 1,
        /*counters*/
        utility::getAclCounterTypes(this->getHwSwitchEnsemble()->getL3Asics())
            .size());
    utility::checkAclStat(
        this->getHwSwitch(),
        this->getProgrammedState(),
        {"acl0", "acl1"},
        "stat0",
        utility::getAclCounterTypes(this->getHwSwitchEnsemble()->getL3Asics()));
  };

  auto setupPostWB = [=, this]() {
    auto newCfg = this->initialConfig();
    this->addDscpAcl(&newCfg, "acl0");
    this->addDscpAcl(&newCfg, "acl1");
    utility::addAclStat(
        &newCfg,
        "acl0",
        "stat0",
        utility::getAclCounterTypes(this->getHwSwitchEnsemble()->getL3Asics()));
    utility::addAclStat(
        &newCfg,
        "acl1",
        "stat1",
        utility::getAclCounterTypes(this->getHwSwitchEnsemble()->getL3Asics()));
    this->applyNewConfig(newCfg);
  };

  auto verifyPostWB = [=, this]() {
    utility::checkAclEntryAndStatCount(
        this->getHwSwitch(),
        /*ACLs*/ 2,
        /*stats*/ 2,
        /*counters*/ 2 *
            utility::getAclCounterTypes(
                this->getHwSwitchEnsemble()->getL3Asics())
                .size());
    utility::checkAclStat(
        this->getHwSwitch(),
        this->getProgrammedState(),
        {"acl0"},
        "stat0",
        utility::getAclCounterTypes(this->getHwSwitchEnsemble()->getL3Asics()));
    utility::checkAclStat(
        this->getHwSwitch(),
        this->getProgrammedState(),
        {"acl1"},
        "stat1",
        utility::getAclCounterTypes(this->getHwSwitchEnsemble()->getL3Asics()));
  };

  this->verifyAcrossWarmBoots(setup, verify, setupPostWB, verifyPostWB);
}

TEST_F(HwAclStatTest, AclStatShuffle) {
  auto setup = [=, this]() {
    auto newCfg = this->initialConfig();
    this->addDscpAcl(&newCfg, "acl0");
    this->addDscpAcl(&newCfg, "acl1");
    utility::addAclStat(
        &newCfg,
        "acl0",
        "stat0",
        utility::getAclCounterTypes(this->getHwSwitchEnsemble()->getL3Asics()));
    utility::addAclStat(
        &newCfg,
        "acl1",
        "stat1",
        utility::getAclCounterTypes(this->getHwSwitchEnsemble()->getL3Asics()));
    this->applyNewConfig(newCfg);
  };

  auto verify = [=, this]() {
    utility::checkAclEntryAndStatCount(
        this->getHwSwitch(),
        /* ACLs */ 2,
        /* Stats */ 2,
        /*counters*/ 2 *
            utility::getAclCounterTypes(
                this->getHwSwitchEnsemble()->getL3Asics())
                .size());
    utility::checkAclStat(
        this->getHwSwitch(),
        this->getProgrammedState(),
        {"acl0"},
        "stat0",
        utility::getAclCounterTypes(this->getHwSwitchEnsemble()->getL3Asics()));
    utility::checkAclStat(
        this->getHwSwitch(),
        this->getProgrammedState(),
        {"acl1"},
        "stat1",
        utility::getAclCounterTypes(this->getHwSwitchEnsemble()->getL3Asics()));
  };

  auto setupPostWB = [=, this]() {
    auto newCfg = this->initialConfig();
    this->addDscpAcl(&newCfg, "acl1");
    this->addDscpAcl(&newCfg, "acl0");
    utility::addAclStat(
        &newCfg,
        "acl1",
        "stat1",
        utility::getAclCounterTypes(this->getHwSwitchEnsemble()->getL3Asics()));
    utility::addAclStat(
        &newCfg,
        "acl0",
        "stat0",
        utility::getAclCounterTypes(this->getHwSwitchEnsemble()->getL3Asics()));
    this->applyNewConfig(newCfg);
  };

  this->verifyAcrossWarmBoots(setup, verify, setupPostWB, verify);
}

TEST_F(HwAclStatTest, StatNumberOfCounters) {
  auto setup = [=, this]() {
    auto newCfg = this->initialConfig();
    this->addDscpAcl(&newCfg, "acl0");
    utility::addAclStat(
        &newCfg,
        "acl0",
        "stat0",
        utility::getAclCounterTypes(this->getHwSwitchEnsemble()->getL3Asics()));
    this->applyNewConfig(newCfg);
  };

  auto verify = [=, this]() {
    utility::checkAclEntryAndStatCount(
        this->getHwSwitch(),
        /* ACLs */ 1,
        /* Stats */ 1,
        /*counters*/
        utility::getAclCounterTypes(this->getHwSwitchEnsemble()->getL3Asics())
            .size());
    utility::checkAclStat(
        this->getHwSwitch(),
        this->getProgrammedState(),
        {"acl0"},
        "stat0",
        utility::getAclCounterTypes(this->getHwSwitchEnsemble()->getL3Asics()));
  };

  auto setupPostWB = [=]() {};

  auto verifyPostWB = [=, this]() {
    verify();
    utility::checkAclStatSize(this->getHwSwitch(), "stat0");
  };
  this->verifyAcrossWarmBoots(setup, verify, setupPostWB, verifyPostWB);
}

} // namespace facebook::fboss
