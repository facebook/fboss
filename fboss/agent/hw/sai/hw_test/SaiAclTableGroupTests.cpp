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
#include "fboss/agent/hw/sai/switch/SaiAclTableGroupManager.h"
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwTest.h"
#include "fboss/agent/hw/test/HwTestAclUtils.h"
#include "fboss/agent/state/SwitchState.h"

#include <string>

DECLARE_bool(enable_acl_table_group);

using namespace facebook::fboss;

namespace {

bool isAclTableGroupEnabled(
    const HwSwitch* hwSwitch,
    sai_acl_stage_t aclStage) {
  const auto& aclTableGroupManager = static_cast<const SaiSwitch*>(hwSwitch)
                                         ->managerTable()
                                         ->aclTableGroupManager();

  auto aclTableGroupHandle =
      aclTableGroupManager.getAclTableGroupHandle(aclStage);
  return aclTableGroupHandle != nullptr;
}

} // namespace

namespace facebook::fboss {

class SaiAclTableGroupTest : public HwTest {
 protected:
  void SetUp() override {
    FLAGS_enable_acl_table_group = true;
    HwTest::SetUp();
  }

  cfg::SwitchConfig initialConfig() const {
    return utility::oneL3IntfConfig(getHwSwitch(), masterLogicalPortIds()[0]);
  }

  bool isSupported() const {
    return HwTest::isSupported(HwAsic::Feature::MULTIPLE_ACL_TABLES);
  }

  void addAclTable1(cfg::SwitchConfig& cfg) {
    // table1 with dscp matcher ACL entry
    utility::addAclTable(
        &cfg,
        kAclTable1(),
        1 /* priority */,
        {cfg::AclTableActionType::PACKET_ACTION},
        {cfg::AclTableQualifier::DSCP});
  }

  void addAclTable2(cfg::SwitchConfig& cfg) {
    // table2 with ttl matcher ACL entry
    utility::addAclTable(
        &cfg,
        kAclTable2(),
        2 /* priority */,
        {cfg::AclTableActionType::COUNTER},
        {cfg::AclTableQualifier::TTL});
  }

  std::string kAclTable1() const {
    return "table1";
  }

  std::string kAclTable2() const {
    return "table2";
  }
};

TEST_F(SaiAclTableGroupTest, SingleAclTableGroup) {
  ASSERT_TRUE(isSupported());

  auto setup = [this]() {
    auto newCfg = initialConfig();

    utility::addAclTableGroup(
        &newCfg, cfg::AclStage::INGRESS, "Ingress Table Group");

    applyNewConfig(newCfg);
  };

  auto verify = [=]() {
    ASSERT_TRUE(isAclTableGroupEnabled(getHwSwitch(), SAI_ACL_STAGE_INGRESS));
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(SaiAclTableGroupTest, MultipleTablesNoEntries) {
  ASSERT_TRUE(isSupported());

  auto setup = [this]() {
    auto newCfg = initialConfig();

    utility::addAclTableGroup(
        &newCfg, cfg::AclStage::INGRESS, "Ingress Table Group");
    addAclTable1(newCfg);
    addAclTable2(newCfg);

    applyNewConfig(newCfg);
  };

  auto verify = [=]() {
    ASSERT_TRUE(isAclTableGroupEnabled(getHwSwitch(), SAI_ACL_STAGE_INGRESS));
    ASSERT_TRUE(utility::isAclTableEnabled(getHwSwitch(), kAclTable1()));
    ASSERT_TRUE(utility::isAclTableEnabled(getHwSwitch(), kAclTable2()));
  };

  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
