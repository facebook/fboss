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
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/utils/QueuePerHostTestUtils.h"
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

class HwAclMatchActionsTest : public HwTest {
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
};

} // namespace facebook::fboss
