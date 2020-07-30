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
#include "fboss/agent/hw/test/HwTestAclUtils.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"

#include "fboss/agent/hw/test/ConfigFactory.h"

namespace facebook::fboss {

class HwAclStatTest : public HwTest {
 protected:
  cfg::SwitchConfig initialConfig() const {
    return utility::oneL3IntfConfig(getHwSwitch(), masterLogicalPortIds()[0]);
  }

  cfg::AclEntry* addDscpAcl(
      cfg::SwitchConfig* cfg,
      const std::string& aclName) {
    auto* acl = utility::addAcl(cfg, aclName);
    // ACL requires at least one qualifier
    acl->dscp_ref() = 0x24;

    return acl;
  }
};

TEST_F(HwAclStatTest, AclStatCreate) {
  auto setup = [=]() {
    auto newCfg = initialConfig();
    addDscpAcl(&newCfg, "acl0");
    utility::addAclStat(&newCfg, "acl0", "stat0");
    applyNewConfig(newCfg);
  };

  auto verify = [=]() {
    utility::checkAclEntryAndStatCount(
        getHwSwitch(), /*ACLs*/ 1, /*stats*/ 1, /*counters*/ 1);
    utility::checkAclStat(
        getHwSwitch(), getProgrammedState(), {"acl0"}, "stat0");
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwAclStatTest, AclStatMultipleCounters) {
  auto setup = [=]() {
    auto newCfg = initialConfig();
    addDscpAcl(&newCfg, "acl0");
    utility::addAclStat(
        &newCfg,
        "acl0",
        "stat0",
        {cfg::CounterType::BYTES, cfg::CounterType::PACKETS});
    applyNewConfig(newCfg);
  };

  auto verify = [=]() {
    utility::checkAclEntryAndStatCount(
        getHwSwitch(), /*ACLs*/ 1, /*stats*/ 1, /*counters*/ 2);
    utility::checkAclStat(
        getHwSwitch(),
        getProgrammedState(),
        {"acl0"},
        "stat0",
        {cfg::CounterType::BYTES, cfg::CounterType::PACKETS});
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwAclStatTest, AclStatChangeCounterType) {
  auto setup = [=]() {
    auto newCfg = initialConfig();
    addDscpAcl(&newCfg, "acl0");
    utility::addAclStat(&newCfg, "acl0", "stat0", {cfg::CounterType::PACKETS});
    applyNewConfig(newCfg);
  };

  auto verify = [=]() {
    utility::checkAclEntryAndStatCount(
        getHwSwitch(), /*ACLs*/ 1, /*stats*/ 1, /*counters*/ 1);
    utility::checkAclStat(
        getHwSwitch(),
        getProgrammedState(),
        {"acl0"},
        "stat0",
        {cfg::CounterType::PACKETS});
  };

  auto setupPostWB = [&]() {
    auto newCfg = initialConfig();
    addDscpAcl(&newCfg, "acl0");
    utility::addAclStat(&newCfg, "acl0", "stat0", {cfg::CounterType::BYTES});
    applyNewConfig(newCfg);
  };

  auto verifyPostWB = [=]() {
    utility::checkAclEntryAndStatCount(
        getHwSwitch(), /*ACLs*/ 1, /*stats*/ 1, /*counters*/ 1);
    utility::checkAclStat(
        getHwSwitch(),
        getProgrammedState(),
        {"acl0"},
        "stat0",
        {cfg::CounterType::BYTES});
  };

  verifyAcrossWarmBoots(setup, verify, setupPostWB, verifyPostWB);
}

TEST_F(HwAclStatTest, AclStatCreateShared) {
  auto setup = [=]() {
    auto newCfg = initialConfig();
    addDscpAcl(&newCfg, "acl0");
    addDscpAcl(&newCfg, "acl1");
    utility::addAclStat(&newCfg, "acl0", "stat");
    utility::addAclStat(&newCfg, "acl1", "stat");
    applyNewConfig(newCfg);
  };

  auto verify = [=]() {
    utility::checkAclEntryAndStatCount(
        getHwSwitch(), /*ACLs*/ 2, /*stats*/ 1, /*counters*/ 1);
    utility::checkAclStat(
        getHwSwitch(), getProgrammedState(), {"acl0", "acl1"}, "stat");
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwAclStatTest, AclStatCreateMultiple) {
  auto setup = [=]() {
    auto newCfg = initialConfig();
    addDscpAcl(&newCfg, "acl0");
    addDscpAcl(&newCfg, "acl1");
    utility::addAclStat(&newCfg, "acl0", "stat0");
    utility::addAclStat(&newCfg, "acl1", "stat1");
    applyNewConfig(newCfg);
  };

  auto verify = [=]() {
    utility::checkAclEntryAndStatCount(
        getHwSwitch(), /*ACLs*/ 2, /*stats*/ 2, /*counters*/ 2);
    utility::checkAclStat(
        getHwSwitch(), getProgrammedState(), {"acl0"}, "stat0");
    utility::checkAclStat(
        getHwSwitch(), getProgrammedState(), {"acl1"}, "stat1");
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwAclStatTest, AclStatMultipleActions) {
  auto setup = [=]() {
    auto newCfg = initialConfig();
    addDscpAcl(&newCfg, "acl0");
    /* The ACL will have 2 actions: a counter and a queue */
    utility::addAclStat(&newCfg, "acl0", "stat0");
    cfg::QueueMatchAction queueAction;
    *queueAction.queueId_ref() = 0;
    cfg::MatchAction matchAction = cfg::MatchAction();
    matchAction.sendToQueue_ref() = queueAction;
    cfg::MatchToAction action = cfg::MatchToAction();
    *action.matcher_ref() = "acl0";
    *action.action_ref() = matchAction;
    newCfg.dataPlaneTrafficPolicy_ref()->matchToAction_ref()->push_back(action);
    applyNewConfig(newCfg);
  };

  auto verify = [=]() {
    utility::checkAclEntryAndStatCount(
        getHwSwitch(), /*ACLs*/ 1, /*stats*/ 1, /*counters*/ 1);
    utility::checkAclStat(
        getHwSwitch(), getProgrammedState(), {"acl0"}, "stat0");
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwAclStatTest, AclStatDelete) {
  auto setup = [=]() {
    auto newCfg = initialConfig();
    addDscpAcl(&newCfg, "acl0");
    utility::addAclStat(&newCfg, "acl0", "stat0");
    applyNewConfig(newCfg);
  };

  auto verify = [=]() {
    utility::checkAclEntryAndStatCount(
        getHwSwitch(), /* ACLs */ 1, /* Stats */ 1, /*counters*/ 1);
    utility::checkAclStat(
        getHwSwitch(), getProgrammedState(), {"acl0"}, "stat0");
  };

  auto setupPostWB = [&]() {
    auto newCfg = initialConfig();
    applyNewConfig(newCfg);
  };

  auto verifyPostWB = [=]() {
    utility::checkAclEntryAndStatCount(
        getHwSwitch(), /*ACLs*/ 0, /*stats*/ 0, /*counters*/ 0);
    utility::checkAclStatDeleted(getHwSwitch(), "stat0");
  };

  verifyAcrossWarmBoots(setup, verify, setupPostWB, verifyPostWB);
}

TEST_F(HwAclStatTest, AclStatDeleteShared) {
  auto setup = [=]() {
    auto newCfg = initialConfig();
    addDscpAcl(&newCfg, "acl0");
    addDscpAcl(&newCfg, "acl1");
    utility::addAclStat(&newCfg, "acl0", "stat");
    utility::addAclStat(&newCfg, "acl1", "stat");
    applyNewConfig(newCfg);
  };

  auto verify = [=]() {
    utility::checkAclEntryAndStatCount(
        getHwSwitch(), /* ACLs */ 2, /* Stats */ 1, /*counters*/ 1);
    utility::checkAclStat(
        getHwSwitch(), getProgrammedState(), {"acl0", "acl1"}, "stat");
  };

  auto setupPostWB = [=]() {
    auto newCfg = initialConfig();
    addDscpAcl(&newCfg, "acl1");
    utility::addAclStat(&newCfg, "acl1", "stat");
    applyNewConfig(newCfg);
  };

  auto verifyPostWB = [=]() {
    utility::checkAclEntryAndStatCount(
        getHwSwitch(), /*ACLs*/ 1, /*stats*/ 1, /*counters*/ 1);
    utility::checkAclStat(
        getHwSwitch(), getProgrammedState(), {"acl1"}, "stat");
  };

  verifyAcrossWarmBoots(setup, verify, setupPostWB, verifyPostWB);
}

TEST_F(HwAclStatTest, AclStatRename) {
  auto setup = [=]() {
    auto newCfg = initialConfig();
    addDscpAcl(&newCfg, "acl0");
    utility::addAclStat(&newCfg, "acl0", "stat0");
    applyNewConfig(newCfg);
  };

  auto verify = [=]() {
    utility::checkAclEntryAndStatCount(
        getHwSwitch(), /* ACLs */ 1, /* Stats */ 1, /*counters*/ 1);
    utility::checkAclStat(
        getHwSwitch(), getProgrammedState(), {"acl0"}, "stat0");
  };

  auto setupPostWB = [=]() {
    auto newCfg = initialConfig();
    addDscpAcl(&newCfg, "acl0");
    utility::addAclStat(&newCfg, "acl0", "stat1");
    applyNewConfig(newCfg);
  };

  auto verifyPostWB = [=]() {
    utility::checkAclEntryAndStatCount(
        getHwSwitch(), /*ACLs*/ 1, /*stats*/ 1, /*counters*/ 1);
    utility::checkAclStat(
        getHwSwitch(), getProgrammedState(), {"acl0"}, "stat1");
    utility::checkAclStatDeleted(getHwSwitch(), "stat0");
  };

  verifyAcrossWarmBoots(setup, verify, setupPostWB, verifyPostWB);
}

TEST_F(HwAclStatTest, AclStatRenameShared) {
  auto setup = [=]() {
    auto newCfg = initialConfig();
    addDscpAcl(&newCfg, "acl0");
    addDscpAcl(&newCfg, "acl1");
    utility::addAclStat(&newCfg, "acl0", "stat0");
    utility::addAclStat(&newCfg, "acl1", "stat0");
    applyNewConfig(newCfg);
  };

  auto verify = [=]() {
    utility::checkAclEntryAndStatCount(
        getHwSwitch(), /* ACLs */ 2, /* Stats */ 1, /*counters*/ 1);
    utility::checkAclStat(
        getHwSwitch(), getProgrammedState(), {"acl0", "acl1"}, "stat0");
  };

  auto setupPostWB = [=]() {
    auto newCfg = initialConfig();
    addDscpAcl(&newCfg, "acl0");
    addDscpAcl(&newCfg, "acl1");
    utility::addAclStat(&newCfg, "acl0", "stat0");
    utility::addAclStat(&newCfg, "acl1", "stat1");
    applyNewConfig(newCfg);
  };

  auto verifyPostWB = [=]() {
    utility::checkAclEntryAndStatCount(
        getHwSwitch(), /*ACLs*/ 2, /*stats*/ 2, /*counters*/ 2);
    utility::checkAclStat(
        getHwSwitch(), getProgrammedState(), {"acl0"}, "stat0");
    utility::checkAclStat(
        getHwSwitch(), getProgrammedState(), {"acl1"}, "stat1");
  };

  verifyAcrossWarmBoots(setup, verify, setupPostWB, verifyPostWB);
}

TEST_F(HwAclStatTest, AclStatCreateSameTwice) {
  auto newCfg = initialConfig();
  addDscpAcl(&newCfg, "acl0");
  utility::addAclStat(&newCfg, "acl0", "stat0");

  auto newState =
      applyThriftConfig(getProgrammedState(), &newCfg, getPlatform());
  newState->publish();
  StateDelta delta(getProgrammedState(), newState);
  getHwSwitch()->stateChanged(delta);
  EXPECT_THROW(getHwSwitch()->stateChanged(delta), FbossError);
}

TEST_F(HwAclStatTest, AclStatDeleteNonExistent) {
  auto newCfg = initialConfig();
  addDscpAcl(&newCfg, "acl0");
  utility::addAclStat(&newCfg, "acl0", "stat0");
  applyNewConfig(newCfg);

  utility::delAcl(&newCfg, "acl0");
  utility::delAclStat(&newCfg, "acl0", "stat0");
  auto newState =
      applyThriftConfig(getProgrammedState(), &newCfg, getPlatform());
  newState->publish();
  StateDelta delta(getProgrammedState(), newState);
  getHwSwitch()->stateChanged(delta);
  EXPECT_THROW(getHwSwitch()->stateChanged(delta), FbossError);
}

TEST_F(HwAclStatTest, AclStatModify) {
  auto setup = [=]() {
    auto newCfg = initialConfig();
    addDscpAcl(&newCfg, "acl0");
    utility::addAclStat(&newCfg, "acl0", "stat0");
    applyNewConfig(newCfg);
  };

  auto verify = [=]() {
    utility::checkAclEntryAndStatCount(
        getHwSwitch(), /* ACLs */ 1, /* Stats */ 1, /*counters*/ 1);
    utility::checkAclStat(
        getHwSwitch(), getProgrammedState(), {"acl0"}, "stat0");
  };

  auto setupPostWB = [=]() {
    auto newCfg = initialConfig();
    auto acl = addDscpAcl(&newCfg, "acl0");
    acl->proto_ref() = 58;
    utility::addAclStat(&newCfg, "acl0", "stat0");
    applyNewConfig(newCfg);
  };

  verifyAcrossWarmBoots(setup, verify, setupPostWB, verify);
}

TEST_F(HwAclStatTest, AclStatShuffle) {
  auto setup = [=]() {
    auto newCfg = initialConfig();
    addDscpAcl(&newCfg, "acl0");
    addDscpAcl(&newCfg, "acl1");
    utility::addAclStat(&newCfg, "acl0", "stat0");
    utility::addAclStat(&newCfg, "acl1", "stat1");
    applyNewConfig(newCfg);
  };

  auto verify = [=]() {
    utility::checkAclEntryAndStatCount(
        getHwSwitch(), /* ACLs */ 2, /* Stats */ 2, /*counters*/ 2);
    utility::checkAclStat(
        getHwSwitch(), getProgrammedState(), {"acl0"}, "stat0");
    utility::checkAclStat(
        getHwSwitch(), getProgrammedState(), {"acl1"}, "stat1");
  };

  auto setupPostWB = [=]() {
    auto newCfg = initialConfig();
    addDscpAcl(&newCfg, "acl1");
    addDscpAcl(&newCfg, "acl0");
    utility::addAclStat(&newCfg, "acl1", "stat1");
    utility::addAclStat(&newCfg, "acl0", "stat0");
    applyNewConfig(newCfg);
  };

  verifyAcrossWarmBoots(setup, verify, setupPostWB, verify);
}

TEST_F(HwAclStatTest, StatNumberOfCounters) {
  auto setup = [=]() {
    auto newCfg = initialConfig();
    addDscpAcl(&newCfg, "acl0");
    utility::addAclStat(&newCfg, "acl0", "stat0", {cfg::CounterType::PACKETS});
    applyNewConfig(newCfg);
  };

  auto verify = [=]() {
    utility::checkAclEntryAndStatCount(
        getHwSwitch(), /* ACLs */ 1, /* Stats */ 1, /*counters*/ 1);
    utility::checkAclStat(
        getHwSwitch(), getProgrammedState(), {"acl0"}, "stat0");
  };

  auto setupPostWB = [=]() {};

  auto verifyPostWB = [=]() {
    verify();
    utility::checkAclStatSize(getHwSwitch(), "stat0");
  };
  verifyAcrossWarmBoots(setup, verify, setupPostWB, verifyPostWB);
}

} // namespace facebook::fboss
