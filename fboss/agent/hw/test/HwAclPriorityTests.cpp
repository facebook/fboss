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
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"

#include "fboss/agent/hw/test/ConfigFactory.h"

#include <string>

namespace {

using namespace facebook::fboss;
using namespace facebook::fboss::utility;

void addDenyPortAcl(cfg::SwitchConfig& cfg, const std::string& aclName) {
  auto acl = cfg::AclEntry();
  *acl.name_ref() = aclName;
  *acl.actionType_ref() = cfg::AclActionType::DENY;
  acl.dscp_ref() = 0x24;
  cfg.acls_ref()->push_back(acl);
}

void addCpuPolicingDstLocalAcl(
    bool isV6,
    cfg::SwitchConfig& cfg,
    const std::string& aclName) {
  auto acl = cfg::AclEntry();
  acl.name_ref() = aclName;
  acl.actionType_ref() = cfg::AclActionType::PERMIT;
  acl.lookupClassNeighbor_ref() = isV6
      ? cfg::AclLookupClass::DST_CLASS_L3_LOCAL_IP6
      : cfg::AclLookupClass::DST_CLASS_L3_LOCAL_IP4;
  acl.dscp_ref() = 48;
  cfg.acls_ref()->push_back(acl);
}

void addCpuPolicingDstLocalMatchAction(
    cfg::SwitchConfig& cfg,
    const std::string& aclName) {
  auto matchAction = cfg::MatchToAction();

  matchAction.matcher_ref() = aclName;
  cfg::QueueMatchAction queueMatchAction;
  queueMatchAction.queueId_ref() = 7;

  matchAction.action_ref()->sendToQueue_ref() = queueMatchAction;
  matchAction.action_ref()->toCpuAction_ref() = cfg::ToCpuAction::TRAP;
  if (!cfg.cpuTrafficPolicy_ref()) {
    cfg.cpuTrafficPolicy_ref() = cfg::CPUTrafficPolicyConfig();
  }
  if (!cfg.cpuTrafficPolicy_ref()->trafficPolicy_ref()) {
    cfg.cpuTrafficPolicy_ref()->trafficPolicy_ref() =
        cfg::TrafficPolicyConfig();
  }
  cfg.cpuTrafficPolicy_ref()
      ->trafficPolicy_ref()
      ->matchToAction_ref()
      ->push_back(matchAction);
}

} // unnamed namespace

namespace facebook::fboss {

class HwAclPriorityTest : public HwTest {
 protected:
  cfg::SwitchConfig initialConfig() const {
    return utility::oneL3IntfConfig(getHwSwitch(), masterLogicalPortIds()[0]);
  }
};

TEST_F(HwAclPriorityTest, CheckAclPriorityOrder) {
  auto setup = [this]() {
    auto newCfg = initialConfig();
    addDenyPortAcl(newCfg, "A");
    addDenyPortAcl(newCfg, "B");
    applyNewConfig(newCfg);
  };

  auto verify = [=]() {
    for (auto acl : {"A", "B"}) {
      checkSwHwAclMatch(getHwSwitch(), getProgrammedState(), acl);
    }
    int aPrio = getProgrammedState()->getAcl("A")->getPriority();
    int bPrio = getProgrammedState()->getAcl("B")->getPriority();
    EXPECT_EQ(aPrio + 1, bPrio);
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwAclPriorityTest, CheckAclPriortyOrderInsertMiddle) {
  auto setup = [this]() {
    auto newCfg = initialConfig();
    addDenyPortAcl(newCfg, "A");
    addDenyPortAcl(newCfg, "B");
    applyNewConfig(newCfg);
    newCfg.acls_ref()->pop_back();
    addDenyPortAcl(newCfg, "C");
    addDenyPortAcl(newCfg, "B");
    applyNewConfig(newCfg);
  };

  auto verify = [=]() {
    for (auto acl : {"A", "B", "C"}) {
      checkSwHwAclMatch(getHwSwitch(), getProgrammedState(), acl);
    }
    int aPrio = getProgrammedState()->getAcl("A")->getPriority();
    int bPrio = getProgrammedState()->getAcl("B")->getPriority();
    int cPrio = getProgrammedState()->getAcl("C")->getPriority();
    // Order should be A, C, B now
    EXPECT_EQ(aPrio + 1, cPrio);
    EXPECT_EQ(aPrio + 2, bPrio);
  };
  verifyAcrossWarmBoots(setup, verify);
}

/**
 * This unit test case is to test we won't crash cause we're using aclName as
 * key of the aclMap in S/W while using priority as key of aclTable in H/W
 */
TEST_F(HwAclPriorityTest, AclNameChange) {
  auto setup = [this]() {
    auto newCfg = initialConfig();
    addDenyPortAcl(newCfg, "A");
    applyNewConfig(newCfg);
    *newCfg.acls_ref()->back().name_ref() = "AA";
    applyNewConfig(newCfg);
  };

  auto verify = [&]() {
    // check s/w acl matches h/w
    ASSERT_EQ(nullptr, getProgrammedState()->getAcl("A"));
    ASSERT_NE(nullptr, getProgrammedState()->getAcl("AA"));
    checkSwHwAclMatch(getHwSwitch(), getProgrammedState(), "AA");
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwAclPriorityTest, AclsChanged) {
  auto setup = [this]() {
    auto config = initialConfig();
    addCpuPolicingDstLocalAcl(
        true, config, "cpuPolicing-high-NetworkControl-dstLocalIp6");
    addCpuPolicingDstLocalAcl(
        false, config, "cpuPolicing-high-NetworkControl-dstLocalIp4");
    setDefaultCpuTrafficPolicyConfig(config, getPlatform()->getAsic());
    addCpuPolicingDstLocalMatchAction(
        config, "cpuPolicing-high-NetworkControl-dstLocalIp6");
    addCpuPolicingDstLocalMatchAction(
        config, "cpuPolicing-high-NetworkControl-dstLocalIp4");
    applyNewConfig(config);
  };

  auto setupPostWb = [=]() {
    auto config = initialConfig();
    addCpuPolicingDstLocalAcl(
        true, config, "cpuPolicing-high-NetworkControl-dstLocalIp6");
    addCpuPolicingDstLocalAcl(
        false, config, "cpuPolicing-high-NetworkControl-dstLocalIp4");
    addCpuPolicingDstLocalMatchAction(
        config, "cpuPolicing-high-NetworkControl-dstLocalIp6");
    addCpuPolicingDstLocalMatchAction(
        config, "cpuPolicing-high-NetworkControl-dstLocalIp4");

    applyNewConfig(config);
  };

  verifyAcrossWarmBoots(
      setup, []() {}, setupPostWb, []() {});
}
} // namespace facebook::fboss
