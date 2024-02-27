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
#include "fboss/agent/hw/test/dataplane_tests/HwTestQueuePerHostUtils.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/utils/TrafficPolicyTestUtils.h"

#include <string>

using namespace facebook::fboss;

namespace {

void popOneMatchToAction(cfg::SwitchConfig* config) {
  config->dataPlaneTrafficPolicy()->matchToAction()->pop_back();
}
void checkSwActionDscpValue(
    std::shared_ptr<SwitchState> state,
    const std::string& aclName,
    int32_t dscpValue) {
  auto acl = utility::getAclEntryByName(state, aclName);
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

template <bool enableMultiAclTable>
struct EnableMultiAclTableT {
  static constexpr auto multiAclTableEnabled = enableMultiAclTable;
};

using TestTypes =
    ::testing::Types<EnableMultiAclTableT<false>, EnableMultiAclTableT<true>>;

template <typename EnableMultiAclTableT>
class HwAclMatchActionsTest : public HwTest {
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

TYPED_TEST_SUITE(HwAclMatchActionsTest, TestTypes);

TYPED_TEST(HwAclMatchActionsTest, AddTrafficPolicy) {
  constexpr uint32_t kDscp = 0x24;
  constexpr int kQueueId = 4;

  auto setup = [this]() {
    auto newCfg = this->initialConfig();
    utility::addDscpAclToCfg(&newCfg, "acl1", kDscp);
    utility::addQueueMatcher(
        &newCfg, "acl1", kQueueId, this->getHwSwitchEnsemble()->isSai());
    this->applyNewConfig(newCfg);
  };
  auto verify = [this]() {
    EXPECT_EQ(utility::getAclTableNumAclEntries(this->getHwSwitch()), 1);
    utility::checkSwHwAclMatch(
        this->getHwSwitch(), this->getProgrammedState(), "acl1");
    utility::checkSwAclSendToQueue(
        this->getProgrammedState(), "acl1", false, kQueueId);
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwAclMatchActionsTest, SetDscpMatchAction) {
  constexpr uint32_t kDscp = 0x24;
  constexpr uint32_t kDscp2 = 0x8;

  auto setup = [this]() {
    auto newCfg = this->initialConfig();
    utility::addDscpAclToCfg(&newCfg, "acl1", kDscp);
    addSetDscpAction(&newCfg, "acl1", kDscp2);
    this->applyNewConfig(newCfg);
  };
  auto verify = [this]() {
    EXPECT_EQ(utility::getAclTableNumAclEntries(this->getHwSwitch()), 1);
    utility::checkSwHwAclMatch(
        this->getHwSwitch(), this->getProgrammedState(), "acl1");
    checkSwActionDscpValue(this->getProgrammedState(), "acl1", kDscp2);
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwAclMatchActionsTest, AddSameMatcherTwice) {
  auto setup = [this]() {
    auto newCfg = this->initialConfig();
    utility::addDscpAclToCfg(&newCfg, "acl1", 0);
    utility::addQueueMatcher(
        &newCfg, "acl1", 0, this->getHwSwitchEnsemble()->isSai());
    utility::addQueueMatcher(
        &newCfg, "acl1", 0, this->getHwSwitchEnsemble()->isSai());
    utility::addDscpAclToCfg(&newCfg, "acl2", 0);
    addSetDscpAction(&newCfg, "acl2", 8);
    addSetDscpAction(&newCfg, "acl2", 8);
    this->applyNewConfig(newCfg);
  };
  auto verify = [this]() {
    EXPECT_EQ(utility::getAclTableNumAclEntries(this->getHwSwitch()), 2);
    utility::checkSwHwAclMatch(
        this->getHwSwitch(), this->getProgrammedState(), "acl1");
    utility::checkSwAclSendToQueue(
        this->getProgrammedState(), "acl1", false, 0);
    utility::checkSwHwAclMatch(
        this->getHwSwitch(), this->getProgrammedState(), "acl2");
    checkSwActionDscpValue(this->getProgrammedState(), "acl2", 8);
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwAclMatchActionsTest, AddMultipleActions) {
  auto setup = [this]() {
    auto newCfg = this->initialConfig();
    utility::addDscpAclToCfg(&newCfg, "acl1", 0);
    utility::addDscpAclToCfg(&newCfg, "acl2", 0);
    utility::addDscpAclToCfg(&newCfg, "acl3", 0);
    utility::addQueueMatcher(
        &newCfg, "acl1", 0, this->getHwSwitchEnsemble()->isSai());
    utility::addQueueMatcher(
        &newCfg, "acl2", 0, this->getHwSwitchEnsemble()->isSai());
    addSetDscpAction(&newCfg, "acl3", 8);
    this->applyNewConfig(newCfg);
  };
  auto verify = [this]() {
    EXPECT_EQ(utility::getAclTableNumAclEntries(this->getHwSwitch()), 3);
    for (const auto& matcher : {"acl1", "acl2"}) {
      utility::checkSwHwAclMatch(
          this->getHwSwitch(), this->getProgrammedState(), matcher);
      utility::checkSwAclSendToQueue(
          this->getProgrammedState(), matcher, false, 0);
    }
    utility::checkSwHwAclMatch(
        this->getHwSwitch(), this->getProgrammedState(), "acl3");
    checkSwActionDscpValue(this->getProgrammedState(), "acl3", 8);
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwAclMatchActionsTest, AddRemoveActions) {
  auto setup = [this]() {
    auto newCfg = this->initialConfig();
    utility::addDscpAclToCfg(&newCfg, "acl1", 0);
    utility::addQueueMatcher(
        &newCfg, "acl1", 0, this->getHwSwitchEnsemble()->isSai());
    utility::addDscpAclToCfg(&newCfg, "acl2", 0);
    addSetDscpAction(&newCfg, "acl2", 8);
    this->applyNewConfig(newCfg);

    popOneMatchToAction(&newCfg);
    popOneMatchToAction(&newCfg);
    this->applyNewConfig(newCfg);
  };

  auto verify = [this]() {
    EXPECT_EQ(utility::getAclTableNumAclEntries(this->getHwSwitch()), 0);
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

TYPED_TEST(HwAclMatchActionsTest, AddTrafficPolicyMultipleRemoveOne) {
  auto setup = [this]() {
    auto newCfg = this->initialConfig();
    utility::addDscpAclToCfg(&newCfg, "acl1", 0);
    utility::addQueueMatcher(
        &newCfg, "acl1", 0, this->getHwSwitchEnsemble()->isSai());
    utility::addDscpAclToCfg(&newCfg, "acl2", 0);
    utility::addQueueMatcher(
        &newCfg, "acl2", 0, this->getHwSwitchEnsemble()->isSai());
    this->applyNewConfig(newCfg);

    popOneMatchToAction(&newCfg);
    this->applyNewConfig(newCfg);
  };
  auto verify = [this]() {
    EXPECT_EQ(utility::getAclTableNumAclEntries(this->getHwSwitch()), 1);
    utility::checkSwHwAclMatch(
        this->getHwSwitch(), this->getProgrammedState(), "acl1");
    utility::checkSwAclSendToQueue(
        this->getProgrammedState(), "acl1", false, 0);
  };
  this->verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
