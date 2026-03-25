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

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/test/HwTestAclUtils.h"
#include "fboss/agent/state/SwitchState.h"

#include "fboss/agent/hw/test/ConfigFactory.h"

namespace facebook::fboss {

class HwAclStatTest : public HwTest {
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
      utility::setupDefaultAclTableGroups(cfg);
    }
    return cfg;
  }

  cfg::AclEntry* addDscpAcl(
      cfg::SwitchConfig* cfg,
      const std::string& aclName) {
    cfg::AclEntry acl{};
    acl.name() = aclName;
    acl.actionType() = cfg::AclActionType::PERMIT;
    // ACL requires at least one qualifier
    acl.dscp() = 0x24;
    utility::addEtherTypeToAcl(getAsic(), &acl, cfg::EtherType::IPv6);
    return utility::addAcl(cfg, acl, cfg::AclStage::INGRESS);
  }
};

} // namespace facebook::fboss
