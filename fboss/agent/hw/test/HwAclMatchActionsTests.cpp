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
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwTest.h"
#include "fboss/agent/hw/test/HwTestAclUtils.h"
#include "fboss/agent/hw/test/TrafficPolicyUtils.h"
#include "fboss/agent/state/SwitchState.h"

#include <string>

using namespace facebook::fboss;

namespace {

void checkSwAclSendToQueue(
    std::shared_ptr<SwitchState> state,
    const std::string& aclName,
    bool sendToCPU,
    int queueId) {
  auto acl = state->getMultiSwitchAcls()->getNodeIf(aclName);
  ASSERT_TRUE(acl->getAclAction());
  ASSERT_TRUE(acl->getAclAction()->cref<switch_state_tags::sendToQueue>());
  ASSERT_EQ(
      acl->getAclAction()
          ->cref<switch_state_tags::sendToQueue>()
          ->cref<switch_state_tags::sendToCPU>()
          ->cref(),
      sendToCPU);
  ASSERT_EQ(
      acl->getAclAction()
          ->cref<switch_state_tags::sendToQueue>()
          ->cref<switch_state_tags::action>()
          ->cref<switch_config_tags::queueId>()
          ->cref(),
      queueId);
}
void popOneMatchToAction(cfg::SwitchConfig* config) {
  config->dataPlaneTrafficPolicy()->matchToAction()->pop_back();
}
void checkSwActionDscpValue(
    std::shared_ptr<SwitchState> state,
    const std::string& aclName,
    int32_t dscpValue) {
  auto acl = state->getMultiSwitchAcls()->getNodeIf(aclName);
  ASSERT_TRUE(acl->getAclAction());
  ASSERT_TRUE(acl->getAclAction()->cref<switch_state_tags::setDscp>());
  ASSERT_EQ(
      acl->getAclAction()
          ->cref<switch_state_tags::setDscp>()
          ->cref<switch_config_tags::dscpValue>()
          ->cref(),
      dscpValue);
}
void addSetDscpAction(
    cfg::SwitchConfig* config,
    const std::string& matcherName,
    int32_t dscpValue) {
  cfg::SetDscpMatchAction setDscpMatchAction;
  *setDscpMatchAction.dscpValue() = dscpValue;
  cfg::MatchAction matchAction = cfg::MatchAction();
  matchAction.setDscp() = setDscpMatchAction;
  utility::addMatcher(config, matcherName, matchAction);
}
} // namespace

namespace facebook::fboss {

class HwAclMatchActionsTest : public HwTest {
 protected:
  cfg::SwitchConfig initialConfig() const {
    return utility::onePortPerInterfaceConfig(
        getHwSwitch(), masterLogicalPortIds());
  }
};

TEST_F(HwAclMatchActionsTest, AddTrafficPolicy) {
  constexpr uint32_t kDscp = 0x24;
  constexpr int kQueueId = 4;

  auto setup = [this]() {
    auto newCfg = initialConfig();
    utility::addDscpAclToCfg(&newCfg, "acl1", kDscp);
    utility::addQueueMatcher(&newCfg, "acl1", kQueueId);
    applyNewConfig(newCfg);
  };
  auto verify = [this]() {
    EXPECT_EQ(utility::getAclTableNumAclEntries(getHwSwitch()), 1);
    utility::checkSwHwAclMatch(getHwSwitch(), getProgrammedState(), "acl1");
    checkSwAclSendToQueue(getProgrammedState(), "acl1", false, kQueueId);
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwAclMatchActionsTest, SetDscpMatchAction) {
  constexpr uint32_t kDscp = 0x24;
  constexpr uint32_t kDscp2 = 0x8;

  auto setup = [this]() {
    auto newCfg = initialConfig();
    utility::addDscpAclToCfg(&newCfg, "acl1", kDscp);
    addSetDscpAction(&newCfg, "acl1", kDscp2);
    applyNewConfig(newCfg);
  };
  auto verify = [this]() {
    EXPECT_EQ(utility::getAclTableNumAclEntries(getHwSwitch()), 1);
    utility::checkSwHwAclMatch(getHwSwitch(), getProgrammedState(), "acl1");
    checkSwActionDscpValue(getProgrammedState(), "acl1", kDscp2);
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwAclMatchActionsTest, AddSameMatcherTwice) {
  auto setup = [this]() {
    auto newCfg = initialConfig();
    utility::addDscpAclToCfg(&newCfg, "acl1", 0);
    utility::addQueueMatcher(&newCfg, "acl1", 0);
    utility::addQueueMatcher(&newCfg, "acl1", 0);
    utility::addDscpAclToCfg(&newCfg, "acl2", 0);
    addSetDscpAction(&newCfg, "acl2", 8);
    addSetDscpAction(&newCfg, "acl2", 8);
    applyNewConfig(newCfg);
  };
  auto verify = [this]() {
    EXPECT_EQ(utility::getAclTableNumAclEntries(getHwSwitch()), 2);
    utility::checkSwHwAclMatch(getHwSwitch(), getProgrammedState(), "acl1");
    checkSwAclSendToQueue(getProgrammedState(), "acl1", false, 0);
    utility::checkSwHwAclMatch(getHwSwitch(), getProgrammedState(), "acl2");
    checkSwActionDscpValue(getProgrammedState(), "acl2", 8);
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwAclMatchActionsTest, AddMultipleActions) {
  auto setup = [this]() {
    auto newCfg = initialConfig();
    utility::addDscpAclToCfg(&newCfg, "acl1", 0);
    utility::addDscpAclToCfg(&newCfg, "acl2", 0);
    utility::addDscpAclToCfg(&newCfg, "acl3", 0);
    utility::addQueueMatcher(&newCfg, "acl1", 0);
    utility::addQueueMatcher(&newCfg, "acl2", 0);
    addSetDscpAction(&newCfg, "acl3", 8);
    applyNewConfig(newCfg);
  };
  auto verify = [this]() {
    EXPECT_EQ(utility::getAclTableNumAclEntries(getHwSwitch()), 3);
    for (const auto& matcher : {"acl1", "acl2"}) {
      utility::checkSwHwAclMatch(getHwSwitch(), getProgrammedState(), matcher);
      checkSwAclSendToQueue(getProgrammedState(), matcher, false, 0);
    }
    utility::checkSwHwAclMatch(getHwSwitch(), getProgrammedState(), "acl3");
    checkSwActionDscpValue(getProgrammedState(), "acl3", 8);
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwAclMatchActionsTest, AddRemoveActions) {
  auto setup = [this]() {
    auto newCfg = initialConfig();
    utility::addDscpAclToCfg(&newCfg, "acl1", 0);
    utility::addQueueMatcher(&newCfg, "acl1", 0);
    utility::addDscpAclToCfg(&newCfg, "acl2", 0);
    addSetDscpAction(&newCfg, "acl2", 8);
    applyNewConfig(newCfg);

    popOneMatchToAction(&newCfg);
    popOneMatchToAction(&newCfg);
    applyNewConfig(newCfg);
  };

  auto verify = [this]() {
    EXPECT_EQ(utility::getAclTableNumAclEntries(getHwSwitch()), 0);
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwAclMatchActionsTest, AddTrafficPolicyMultipleRemoveOne) {
  auto setup = [this]() {
    auto newCfg = initialConfig();
    utility::addDscpAclToCfg(&newCfg, "acl1", 0);
    utility::addQueueMatcher(&newCfg, "acl1", 0);
    utility::addDscpAclToCfg(&newCfg, "acl2", 0);
    utility::addQueueMatcher(&newCfg, "acl2", 0);
    applyNewConfig(newCfg);

    popOneMatchToAction(&newCfg);
    applyNewConfig(newCfg);
  };
  auto verify = [this]() {
    EXPECT_EQ(utility::getAclTableNumAclEntries(getHwSwitch()), 1);
    utility::checkSwHwAclMatch(getHwSwitch(), getProgrammedState(), "acl1");
    checkSwAclSendToQueue(getProgrammedState(), "acl1", false, 0);
  };
  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
