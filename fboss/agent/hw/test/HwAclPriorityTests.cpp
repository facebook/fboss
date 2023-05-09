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
  *acl.name() = aclName;
  *acl.actionType() = cfg::AclActionType::DENY;
  acl.dscp() = 0x24;
  cfg.acls()->push_back(acl);
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
  cfg.acls()->push_back(acl);
}

} // unnamed namespace

namespace facebook::fboss {

class HwAclPriorityTest : public HwTest {
 protected:
  cfg::SwitchConfig initialConfig() const {
    return utility::onePortPerInterfaceConfig(
        getHwSwitch(), masterLogicalPortIds());
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
    int aPrio = getProgrammedState()
                    ->getMultiSwitchAcls()
                    ->getNodeIf("A")
                    ->getPriority();
    int bPrio = getProgrammedState()
                    ->getMultiSwitchAcls()
                    ->getNodeIf("B")
                    ->getPriority();
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
    newCfg.acls()->pop_back();
    addDenyPortAcl(newCfg, "C");
    addDenyPortAcl(newCfg, "B");
    applyNewConfig(newCfg);
  };

  auto verify = [=]() {
    for (auto acl : {"A", "B", "C"}) {
      checkSwHwAclMatch(getHwSwitch(), getProgrammedState(), acl);
    }
    int aPrio = getProgrammedState()
                    ->getMultiSwitchAcls()
                    ->getNodeIf("A")
                    ->getPriority();
    int bPrio = getProgrammedState()
                    ->getMultiSwitchAcls()
                    ->getNodeIf("B")
                    ->getPriority();
    int cPrio = getProgrammedState()
                    ->getMultiSwitchAcls()
                    ->getNodeIf("C")
                    ->getPriority();
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
    *newCfg.acls()->back().name() = "AA";
    applyNewConfig(newCfg);
  };

  auto verify = [&]() {
    // check s/w acl matches h/w
    ASSERT_EQ(
        nullptr, getProgrammedState()->getMultiSwitchAcls()->getNodeIf("A"));
    ASSERT_NE(
        nullptr, getProgrammedState()->getMultiSwitchAcls()->getNodeIf("AA"));
    checkSwHwAclMatch(getHwSwitch(), getProgrammedState(), "AA");
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwAclPriorityTest, AclsChanged) {
  const folly::IPAddress kIp("2400::1");
  auto setup = [this, kIp]() {
    auto config = initialConfig();
    addDenyPortAcl(config, "acl0");
    // Get Acls from COPP policy
    setDefaultCpuTrafficPolicyConfig(config, getPlatform()->getAsic());
    addPermitIpAcl(config, "acl1", kIp);
    applyNewConfig(config);
  };

  auto setupPostWb = [this, kIp]() {
    auto config = initialConfig();
    // Drop COPP acls
    addDenyPortAcl(config, "acl0");
    addPermitIpAcl(config, "acl1", kIp);
    applyNewConfig(config);
  };

  verifyAcrossWarmBoots(
      setup, []() {}, setupPostWb, []() {});
}

TEST_F(HwAclPriorityTest, Reprioritize) {
  auto setup = [=]() {
    auto config = initialConfig();
    addPermitIpAcl(config, "B", folly::IPAddress("1::2"));
    addPermitIpAcl(config, "A", folly::IPAddress("1::3"));

    cfg::CPUTrafficPolicyConfig cpuConfig;
    cfg::TrafficPolicyConfig trafficConfig;
    trafficConfig.matchToAction()->resize(2);
    cfg::MatchAction matchAction;
    cfg::QueueMatchAction queueAction;
    queueAction.queueId() = 1;
    matchAction.sendToQueue() = queueAction;
    matchAction.toCpuAction() = cfg::ToCpuAction::TRAP;
    for (int i = 0; i < 2; i++) {
      trafficConfig.matchToAction()[i].matcher() = *config.acls()[i].name();
      trafficConfig.matchToAction()[i].action() = matchAction;
    }
    cpuConfig.trafficPolicy() = trafficConfig;
    config.cpuTrafficPolicy() = cpuConfig;
    applyNewConfig(config);
  };

  auto setupPostWb = [=]() {
    auto config = initialConfig();
    addPermitIpAcl(config, "A", folly::IPAddress("1::3"));
    addPermitIpAcl(config, "B", folly::IPAddress("1::2"));

    cfg::CPUTrafficPolicyConfig cpuConfig;
    cfg::TrafficPolicyConfig trafficConfig;
    trafficConfig.matchToAction()->resize(2);
    cfg::MatchAction matchAction;
    cfg::QueueMatchAction queueAction;
    queueAction.queueId() = 1;
    matchAction.sendToQueue() = queueAction;
    matchAction.toCpuAction() = cfg::ToCpuAction::TRAP;
    for (int i = 0; i < 2; i++) {
      trafficConfig.matchToAction()[i].matcher() = *config.acls()[i].name();
      trafficConfig.matchToAction()[i].action() = matchAction;
    }
    cpuConfig.trafficPolicy() = trafficConfig;
    config.cpuTrafficPolicy() = cpuConfig;
    applyNewConfig(config);
  };

  verifyAcrossWarmBoots(
      setup, []() {}, setupPostWb, []() {});
}
} // namespace facebook::fboss
