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
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"

#include "fboss/agent/hw/test/ConfigFactory.h"

#include <fb303/ServiceData.h>

namespace facebook::fboss {

class HwAclStatTest : public HwTest {
 protected:
  cfg::SwitchConfig initialConfig() const {
    return utility::onePortPerInterfaceConfig(
        getHwSwitch(),
        masterLogicalPortIds(),
        getAsic()->desiredLoopbackModes());
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

TEST_F(HwAclStatTest, AclStatCreateDeleteCreate) {
  auto setup = [=]() {
    auto newCfg = initialConfig();
    addDscpAcl(&newCfg, "acl0");
    utility::addAclStat(&newCfg, "acl0", "stat0");
    applyNewConfig(newCfg);
  };

  auto verify = [=]() {
    EXPECT_TRUE(facebook::fb303::fbData->getStatMap()->contains(
        utility::statNameFromCounterType("stat0", cfg::CounterType::PACKETS)));

    auto newCfg = initialConfig();
    applyNewConfig(newCfg);
    EXPECT_FALSE(facebook::fb303::fbData->getStatMap()->contains(
        utility::statNameFromCounterType("stat0", cfg::CounterType::PACKETS)));

    auto newCfg2 = initialConfig();
    addDscpAcl(&newCfg2, "acl0");
    utility::addAclStat(&newCfg2, "acl0", "stat0");
    applyNewConfig(newCfg2);
    EXPECT_TRUE(facebook::fb303::fbData->getStatMap()->contains(
        utility::statNameFromCounterType("stat0", cfg::CounterType::PACKETS)));
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
    *queueAction.queueId() = 0;
    cfg::MatchAction matchAction = cfg::MatchAction();
    matchAction.sendToQueue() = queueAction;
    cfg::MatchToAction action = cfg::MatchToAction();
    *action.matcher() = "acl0";
    *action.action() = matchAction;
    newCfg.dataPlaneTrafficPolicy()->matchToAction()->push_back(action);
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

TEST_F(HwAclStatTest, AclStatCreatePostWarmBoot) {
  auto setup = [=]() {
    auto newCfg = initialConfig();
    applyNewConfig(newCfg);
  };

  auto verify = [=]() {};

  auto setupPostWB = [&]() {
    auto newCfg = initialConfig();
    addDscpAcl(&newCfg, "acl0");
    utility::addAclStat(&newCfg, "acl0", "stat0");
    applyNewConfig(newCfg);
  };

  auto verifyPostWB = [=]() {
    utility::checkAclEntryAndStatCount(
        getHwSwitch(), /*ACLs*/ 1, /*stats*/ 1, /*counters*/ 1);
    utility::checkAclStat(
        getHwSwitch(), getProgrammedState(), {"acl0"}, "stat0");
  };

  verifyAcrossWarmBoots(setup, verify, setupPostWB, verifyPostWB);
}

TEST_F(HwAclStatTest, AclStatDeleteSharedPostWarmBoot) {
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

TEST_F(HwAclStatTest, AclStatCreateSharedPostWarmBoot) {
  auto setup = [=]() {
    auto newCfg = initialConfig();
    applyNewConfig(newCfg);
  };

  auto verify = [=]() {};

  auto setupPostWB = [=]() {
    auto newCfg = initialConfig();
    addDscpAcl(&newCfg, "acl0");
    addDscpAcl(&newCfg, "acl1");
    utility::addAclStat(&newCfg, "acl0", "stat");
    utility::addAclStat(&newCfg, "acl1", "stat");
    applyNewConfig(newCfg);
  };

  auto verifyPostWB = [=]() {
    utility::checkAclEntryAndStatCount(
        getHwSwitch(), /*ACLs*/ 2, /*stats*/ 1, /*counters*/ 1);
    utility::checkAclStat(
        getHwSwitch(), getProgrammedState(), {"acl0", "acl1"}, "stat");
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
  auto state = getProgrammedState();
  auto newCfg = initialConfig();
  addDscpAcl(&newCfg, "acl0");
  utility::addAclStat(&newCfg, "acl0", "stat0");
  applyNewConfig(newCfg);
  StateDelta delta(state, getProgrammedState());

  if (FLAGS_enable_state_oper_delta) {
    // adding same ACL twice with oper delta and state maintained in HW switch
    // leads to process change. not process added.
    EXPECT_NO_THROW(getHwSwitch()->stateChanged(delta));
  } else {
    EXPECT_THROW(getHwSwitch()->stateChanged(delta), FbossError);
  }
}

TEST_F(HwAclStatTest, AclStatDeleteNonExistent) {
  auto newCfg = initialConfig();
  addDscpAcl(&newCfg, "acl0");
  utility::addAclStat(&newCfg, "acl0", "stat0");
  applyNewConfig(newCfg);

  auto state = getProgrammedState();
  utility::delAcl(&newCfg, "acl0");
  utility::delAclStat(&newCfg, "acl0", "stat0");
  applyNewConfig(newCfg);
  StateDelta delta(state, getProgrammedState());

  // TODO(pshaikh): uncomment next release
  // EXPECT_THROW(getHwSwitch()->stateChanged(delta), FbossError);
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
    acl->proto() = 58;
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

TEST_F(HwAclStatTest, EmptyAclCheck) {
  // TODO(joseph5wu) Current this test failed on TH4 because of CS00011697882
  auto setup = [this]() {
    auto newCfg = initialConfig();
    applyNewConfig(newCfg);
  };
  auto verify = [this]() {
    EXPECT_EQ(utility::getAclTableNumAclEntries(getHwSwitch()), 0);
    utility::checkAclEntryAndStatCount(
        getHwSwitch(), /*ACLs*/ 0, /*stats*/ 0, /*counters*/ 0);
  };
  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
