/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/tests/BcmTest.h"

#include <folly/Random.h>
#include "fboss/agent/ApplyThriftConfig.h"
#include "fboss/agent/FbossError.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/bcm/BcmAclEntry.h"
#include "fboss/agent/hw/bcm/BcmAclTable.h"
#include "fboss/agent/hw/bcm/BcmFieldProcessorUtils.h"
#include "fboss/agent/hw/bcm/BcmStatUpdater.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/SocUtils.h"
#include "fboss/agent/hw/bcm/tests/BcmAclUtils.h"
#include "fboss/agent/platforms/test_platforms/BcmTestPlatform.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"

#include "fboss/agent/hw/test/ConfigFactory.h"

extern "C" {
#include <bcm/field.h>
}

DECLARE_int32(acl_gid);

namespace {

using namespace facebook::fboss;
using namespace facebook::fboss::utility;

void checkFPEntryAndStatCount(
    BcmSwitch* hw,
    int aclCount,
    int aclStatCount,
    int counterCount) {
  const auto stats = hw->getStatUpdater()->getHwTableStats();
  ASSERT_EQ(aclCount, stats.acl_entries_used);
  ASSERT_EQ(aclCount, fpGroupNumAclEntries(hw->getUnit(), FLAGS_acl_gid));

  ASSERT_EQ(
      aclStatCount, fpGroupNumAclStatEntries(hw->getUnit(), FLAGS_acl_gid));
  ASSERT_EQ(counterCount, hw->getStatUpdater()->getCounterCount());
}

std::optional<cfg::TrafficCounter> getAclTrafficCounter(
    const std::shared_ptr<SwitchState> state,
    const std::string& aclName) {
  auto swAcl = state->getAcl(aclName);
  if (swAcl && swAcl->getAclAction()) {
    return swAcl->getAclAction()->getTrafficCounter();
  }
  return std::nullopt;
}

void checkAclStatDeleted(BcmSwitch* hw, const std::string& statName) {
  ASSERT_EQ(nullptr, hw->getAclTable()->getAclStatIf(statName));
}

void checkAclStat(
    BcmSwitch* hw,
    std::shared_ptr<SwitchState> state,
    std::vector<std::string> acls,
    const std::string& statName,
    std::vector<cfg::CounterType> counterTypes = {cfg::CounterType::PACKETS}) {
  auto aclTable = hw->getAclTable();

  // Check if the stat has been programmed
  auto hwStat = aclTable->getAclStat(statName);
  ASSERT_NE(nullptr, hwStat);

  // Check the ACL table refcount
  ASSERT_EQ(acls.size(), aclTable->getAclStatRefCount(statName));

  // Check that the SW and HW configs are the same
  for (const auto& aclName : acls) {
    auto swTrafficCounter = getAclTrafficCounter(state, aclName);
    ASSERT_TRUE(swTrafficCounter);
    ASSERT_EQ(statName, swTrafficCounter->name);
    ASSERT_EQ(counterTypes, swTrafficCounter->types);
    BcmAclStat::isStateSame(hw, hwStat->getHandle(), swTrafficCounter.value());
  }

  // Check the Stat Updater
  for (auto type : counterTypes) {
    ASSERT_NE(
        nullptr, hw->getStatUpdater()->getCounterIf(hwStat->getHandle(), type));
  }
}
} // unnamed namespace

namespace facebook::fboss {

class BcmAclStatTest : public BcmTest {
 protected:
  cfg::SwitchConfig initialConfig() const override {
    return utility::oneL3IntfConfig(getHwSwitch(), masterLogicalPortIds()[0]);
  }
};

TEST_F(BcmAclStatTest, AclStatCreate) {
  auto setup = [=]() {
    auto newCfg = initialConfig();
    addAcl(&newCfg, "acl0");
    addAclStat(&newCfg, "acl0", "stat0");
    applyNewConfig(newCfg);
  };

  auto verify = [=]() {
    checkFPEntryAndStatCount(
        getHwSwitch(), /*ACLs*/ 1, /*stats*/ 1, /*counters*/ 1);
    checkAclStat(getHwSwitch(), getProgrammedState(), {"acl0"}, "stat0");
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(BcmAclStatTest, AclStatMultipleCounters) {
  auto setup = [=]() {
    auto newCfg = initialConfig();
    addAcl(&newCfg, "acl0");
    addAclStat(
        &newCfg,
        "acl0",
        "stat0",
        {cfg::CounterType::BYTES, cfg::CounterType::PACKETS});
    applyNewConfig(newCfg);
  };

  auto verify = [=]() {
    checkFPEntryAndStatCount(
        getHwSwitch(), /*ACLs*/ 1, /*stats*/ 1, /*counters*/ 2);
    checkAclStat(
        getHwSwitch(),
        getProgrammedState(),
        {"acl0"},
        "stat0",
        {cfg::CounterType::BYTES, cfg::CounterType::PACKETS});
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(BcmAclStatTest, AclStatChangeCounterType) {
  auto setup = [=]() {
    auto newCfg = initialConfig();
    addAcl(&newCfg, "acl0");
    addAclStat(&newCfg, "acl0", "stat0", {cfg::CounterType::PACKETS});
    applyNewConfig(newCfg);
  };

  auto verify = [=]() {
    checkFPEntryAndStatCount(
        getHwSwitch(), /*ACLs*/ 1, /*stats*/ 1, /*counters*/ 1);
    checkAclStat(
        getHwSwitch(),
        getProgrammedState(),
        {"acl0"},
        "stat0",
        {cfg::CounterType::PACKETS});
  };

  auto setupPostWB = [&]() {
    auto newCfg = initialConfig();
    addAcl(&newCfg, "acl0");
    addAclStat(&newCfg, "acl0", "stat0", {cfg::CounterType::BYTES});
    applyNewConfig(newCfg);
  };

  auto verifyPostWB = [=]() {
    checkFPEntryAndStatCount(
        getHwSwitch(), /*ACLs*/ 1, /*stats*/ 1, /*counters*/ 1);
    checkAclStat(
        getHwSwitch(),
        getProgrammedState(),
        {"acl0"},
        "stat0",
        {cfg::CounterType::BYTES});
  };

  verifyAcrossWarmBoots(setup, verify, setupPostWB, verifyPostWB);
}

TEST_F(BcmAclStatTest, AclStatCreateShared) {
  auto setup = [=]() {
    auto newCfg = initialConfig();
    addAcl(&newCfg, "acl0");
    addAcl(&newCfg, "acl1");
    addAclStat(&newCfg, "acl0", "stat");
    addAclStat(&newCfg, "acl1", "stat");
    applyNewConfig(newCfg);
  };

  auto verify = [=]() {
    checkFPEntryAndStatCount(
        getHwSwitch(), /*ACLs*/ 2, /*stats*/ 1, /*counters*/ 1);
    checkAclStat(getHwSwitch(), getProgrammedState(), {"acl0", "acl1"}, "stat");
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(BcmAclStatTest, AclStatCreateMultiple) {
  auto setup = [=]() {
    auto newCfg = initialConfig();
    addAcl(&newCfg, "acl0");
    addAcl(&newCfg, "acl1");
    addAclStat(&newCfg, "acl0", "stat0");
    addAclStat(&newCfg, "acl1", "stat1");
    applyNewConfig(newCfg);
  };

  auto verify = [=]() {
    checkFPEntryAndStatCount(
        getHwSwitch(), /*ACLs*/ 2, /*stats*/ 2, /*counters*/ 2);
    checkAclStat(getHwSwitch(), getProgrammedState(), {"acl0"}, "stat0");
    checkAclStat(getHwSwitch(), getProgrammedState(), {"acl1"}, "stat1");
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(BcmAclStatTest, AclStatMultipleActions) {
  auto setup = [=]() {
    auto newCfg = initialConfig();
    addAcl(&newCfg, "acl0");
    /* The ACL will have 2 actions: a counter and a queue */
    addAclStat(&newCfg, "acl0", "stat0");
    cfg::QueueMatchAction queueAction;
    queueAction.queueId = 0;
    cfg::MatchAction matchAction = cfg::MatchAction();
    matchAction.sendToQueue_ref() = queueAction;
    cfg::MatchToAction action = cfg::MatchToAction();
    action.matcher = "acl0";
    action.action = matchAction;
    newCfg.dataPlaneTrafficPolicy_ref()->matchToAction.push_back(action);
    applyNewConfig(newCfg);
  };

  auto verify = [=]() {
    checkFPEntryAndStatCount(
        getHwSwitch(), /*ACLs*/ 1, /*stats*/ 1, /*counters*/ 1);
    checkAclStat(getHwSwitch(), getProgrammedState(), {"acl0"}, "stat0");
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(BcmAclStatTest, AclStatDelete) {
  auto setup = [=]() {
    auto newCfg = initialConfig();
    addAcl(&newCfg, "acl0");
    addAclStat(&newCfg, "acl0", "stat0");
    applyNewConfig(newCfg);
  };

  auto verify = [=]() {
    checkFPEntryAndStatCount(
        getHwSwitch(), /* ACLs */ 1, /* Stats */ 1, /*counters*/ 1);
    checkAclStat(getHwSwitch(), getProgrammedState(), {"acl0"}, "stat0");
  };

  auto setupPostWB = [&]() {
    auto newCfg = initialConfig();
    applyNewConfig(newCfg);
  };

  auto verifyPostWB = [=]() {
    checkFPEntryAndStatCount(
        getHwSwitch(), /*ACLs*/ 0, /*stats*/ 0, /*counters*/ 0);
    checkAclStatDeleted(getHwSwitch(), "stat0");
  };

  verifyAcrossWarmBoots(setup, verify, setupPostWB, verifyPostWB);
}

TEST_F(BcmAclStatTest, AclStatDeleteShared) {
  auto setup = [=]() {
    auto newCfg = initialConfig();
    addAcl(&newCfg, "acl0");
    addAcl(&newCfg, "acl1");
    addAclStat(&newCfg, "acl0", "stat");
    addAclStat(&newCfg, "acl1", "stat");
    applyNewConfig(newCfg);
  };

  auto verify = [=]() {
    checkFPEntryAndStatCount(
        getHwSwitch(), /* ACLs */ 2, /* Stats */ 1, /*counters*/ 1);
    checkAclStat(getHwSwitch(), getProgrammedState(), {"acl0", "acl1"}, "stat");
  };

  auto setupPostWB = [=]() {
    auto newCfg = initialConfig();
    addAcl(&newCfg, "acl1");
    addAclStat(&newCfg, "acl1", "stat");
    applyNewConfig(newCfg);
  };

  auto verifyPostWB = [=]() {
    checkFPEntryAndStatCount(
        getHwSwitch(), /*ACLs*/ 1, /*stats*/ 1, /*counters*/ 1);
    checkAclStat(getHwSwitch(), getProgrammedState(), {"acl1"}, "stat");
  };

  verifyAcrossWarmBoots(setup, verify, setupPostWB, verifyPostWB);
}

TEST_F(BcmAclStatTest, AclStatRename) {
  auto setup = [=]() {
    auto newCfg = initialConfig();
    addAcl(&newCfg, "acl0");
    addAclStat(&newCfg, "acl0", "stat0");
    applyNewConfig(newCfg);
  };

  auto verify = [=]() {
    checkFPEntryAndStatCount(
        getHwSwitch(), /* ACLs */ 1, /* Stats */ 1, /*counters*/ 1);
    checkAclStat(getHwSwitch(), getProgrammedState(), {"acl0"}, "stat0");
  };

  auto setupPostWB = [=]() {
    auto newCfg = initialConfig();
    addAcl(&newCfg, "acl0");
    addAclStat(&newCfg, "acl0", "stat1");
    applyNewConfig(newCfg);
  };

  auto verifyPostWB = [=]() {
    checkFPEntryAndStatCount(
        getHwSwitch(), /*ACLs*/ 1, /*stats*/ 1, /*counters*/ 1);
    checkAclStat(getHwSwitch(), getProgrammedState(), {"acl0"}, "stat1");
    checkAclStatDeleted(getHwSwitch(), "stat0");
  };

  verifyAcrossWarmBoots(setup, verify, setupPostWB, verifyPostWB);
}

TEST_F(BcmAclStatTest, AclStatRenameShared) {
  auto setup = [=]() {
    auto newCfg = initialConfig();
    addAcl(&newCfg, "acl0");
    addAcl(&newCfg, "acl1");
    addAclStat(&newCfg, "acl0", "stat0");
    addAclStat(&newCfg, "acl1", "stat0");
    applyNewConfig(newCfg);
  };

  auto verify = [=]() {
    checkFPEntryAndStatCount(
        getHwSwitch(), /* ACLs */ 2, /* Stats */ 1, /*counters*/ 1);
    checkAclStat(
        getHwSwitch(), getProgrammedState(), {"acl0", "acl1"}, "stat0");
  };

  auto setupPostWB = [=]() {
    auto newCfg = initialConfig();
    addAcl(&newCfg, "acl0");
    addAcl(&newCfg, "acl1");
    addAclStat(&newCfg, "acl0", "stat0");
    addAclStat(&newCfg, "acl1", "stat1");
    applyNewConfig(newCfg);
  };

  auto verifyPostWB = [=]() {
    checkFPEntryAndStatCount(
        getHwSwitch(), /*ACLs*/ 2, /*stats*/ 2, /*counters*/ 2);
    checkAclStat(getHwSwitch(), getProgrammedState(), {"acl0"}, "stat0");
    checkAclStat(getHwSwitch(), getProgrammedState(), {"acl1"}, "stat1");
  };

  verifyAcrossWarmBoots(setup, verify, setupPostWB, verifyPostWB);
}

TEST_F(BcmAclStatTest, AclStatCreateSameTwice) {
  auto newCfg = initialConfig();
  addAcl(&newCfg, "acl0");
  addAclStat(&newCfg, "acl0", "stat0");

  auto newState =
      applyThriftConfig(getProgrammedState(), &newCfg, getPlatform());
  newState->publish();
  StateDelta delta(getProgrammedState(), newState);
  getHwSwitch()->stateChanged(delta);
  EXPECT_THROW(getHwSwitch()->stateChanged(delta), FbossError);
}

TEST_F(BcmAclStatTest, AclStatDeleteNonExistent) {
  auto newCfg = initialConfig();
  addAcl(&newCfg, "acl0");
  addAclStat(&newCfg, "acl0", "stat0");
  applyNewConfig(newCfg);

  delAcl(&newCfg, "acl0");
  delAclStat(&newCfg, "acl0", "stat0");
  auto newState =
      applyThriftConfig(getProgrammedState(), &newCfg, getPlatform());
  newState->publish();
  StateDelta delta(getProgrammedState(), newState);
  getHwSwitch()->stateChanged(delta);
  EXPECT_THROW(getHwSwitch()->stateChanged(delta), FbossError);
}

TEST_F(BcmAclStatTest, AclStatModify) {
  auto setup = [=]() {
    auto newCfg = initialConfig();
    addAcl(&newCfg, "acl0");
    addAclStat(&newCfg, "acl0", "stat0");
    applyNewConfig(newCfg);
  };

  auto verify = [=]() {
    checkFPEntryAndStatCount(
        getHwSwitch(), /* ACLs */ 1, /* Stats */ 1, /*counters*/ 1);
    checkAclStat(getHwSwitch(), getProgrammedState(), {"acl0"}, "stat0");
  };

  auto setupPostWB = [=]() {
    auto newCfg = initialConfig();
    auto acl = addAcl(&newCfg, "acl0");
    acl->proto_ref() = 58;
    addAclStat(&newCfg, "acl0", "stat0");
    applyNewConfig(newCfg);
  };

  verifyAcrossWarmBoots(setup, verify, setupPostWB, verify);
}

TEST_F(BcmAclStatTest, AclStatShuffle) {
  auto setup = [=]() {
    auto newCfg = initialConfig();
    addAcl(&newCfg, "acl0");
    addAcl(&newCfg, "acl1");
    addAclStat(&newCfg, "acl0", "stat0");
    addAclStat(&newCfg, "acl1", "stat1");
    applyNewConfig(newCfg);
  };

  auto verify = [=]() {
    checkFPEntryAndStatCount(
        getHwSwitch(), /* ACLs */ 2, /* Stats */ 2, /*counters*/ 2);
    checkAclStat(getHwSwitch(), getProgrammedState(), {"acl0"}, "stat0");
    checkAclStat(getHwSwitch(), getProgrammedState(), {"acl1"}, "stat1");
  };

  auto setupPostWB = [=]() {
    auto newCfg = initialConfig();
    addAcl(&newCfg, "acl1");
    addAcl(&newCfg, "acl0");
    addAclStat(&newCfg, "acl1", "stat1");
    addAclStat(&newCfg, "acl0", "stat0");
    applyNewConfig(newCfg);
  };

  verifyAcrossWarmBoots(setup, verify, setupPostWB, verify);
}

TEST_F(BcmAclStatTest, BcmStatNumberOfCounters) {
  /*
   * Check the assumptions made in BcmAclStat::isStateSame()
   * See BcmAclStat.cpp for more info.
   *
   * Check that wedge40's asic is indeed programming *all* the types of
   * counters when asked to program only a subset.
   */

  auto setup = [=]() {
    auto newCfg = initialConfig();
    addAcl(&newCfg, "acl0");
    addAclStat(&newCfg, "acl0", "stat0", {cfg::CounterType::PACKETS});
    applyNewConfig(newCfg);
  };

  auto verify = [=]() {
    checkFPEntryAndStatCount(
        getHwSwitch(), /* ACLs */ 1, /* Stats */ 1, /*counters*/ 1);
    checkAclStat(getHwSwitch(), getProgrammedState(), {"acl0"}, "stat0");
  };

  auto setupPostWB = [=]() {};

  auto verifyPostWB = [=]() {
    verify();
    auto hwStat = getHwSwitch()->getAclTable()->getAclStat("stat0");
    int numCounters;
    auto rv = bcm_field_stat_size(getUnit(), hwStat->getHandle(), &numCounters);
    bcmCheckError(
        rv, "Unable to get stat size for acl stat=", hwStat->getHandle());

    // We only programmed a packet counter, but TD2 programmed both bytes and
    // packets counters.
    int expectedNumCounters = 1;
    if (SocUtils::isTrident2(getUnit()) &&
        getHwSwitch()->getBootType() == BootType::WARM_BOOT) {
      expectedNumCounters = 2;
    }
    ASSERT_EQ(expectedNumCounters, numCounters);
  };
  verifyAcrossWarmBoots(setup, verify, setupPostWB, verifyPostWB);
}

} // namespace facebook::fboss
