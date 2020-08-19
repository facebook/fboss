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

#include "fboss/agent/ApplyThriftConfig.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/test/HwTestAclUtils.h"
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

} // namespace facebook::fboss
