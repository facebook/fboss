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

#include "fboss/agent/ApplyThriftConfig.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/bcm/BcmAclEntry.h"
#include "fboss/agent/hw/bcm/BcmAclTable.h"
#include "fboss/agent/hw/bcm/BcmFieldProcessorUtils.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/test/HwTestCoppUtils.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"

#include "fboss/agent/hw/test/ConfigFactory.h"

using namespace facebook::fboss;
using namespace facebook::fboss::utility;

DECLARE_int32(acl_gid);

namespace {

void checkCoppAclMatch(
    std::shared_ptr<SwitchState> state,
    BcmSwitch* hw,
    int unit) {
  auto& swAcls = state->getAcls();
  int coppAclsCount = fpGroupNumAclEntries(unit, FLAGS_acl_gid);
  ASSERT_EQ(swAcls->size(), coppAclsCount);
  // check all coop acls are sync between h/w and s/w
  for (auto& swAcl : *swAcls) {
    auto hwAcl = hw->getAclTable()->getAclIf(swAcl->getPriority());
    ASSERT_NE(nullptr, hwAcl);
    ASSERT_TRUE(
        BcmAclEntry::isStateSame(hw, FLAGS_acl_gid, hwAcl->getHandle(), swAcl));
  }
}

void addInterface(cfg::SwitchConfig* config) {
  // add one interface, ipAddress
  config->interfaces_ref()->resize(2);
  *config->interfaces_ref()[1].intfID_ref() = 2;
  config->vlans_ref()->resize(3);
  *config->vlans_ref()[2].id_ref() = 2;
  *config->interfaces_ref()[1].vlanID_ref() = 2;
  *config->interfaces_ref()[1].routerID_ref() = 0;
  config->interfaces_ref()[1].mac_ref() = "00:01:00:11:22:33";
  config->interfaces_ref()[1].ipAddresses_ref()->resize(1);
  config->interfaces_ref()[1].ipAddresses_ref()[0] = "::11:11:11/120";
}

void delInterface(cfg::SwitchConfig* config) {
  // remove the newly added interface and new ip of intf1
  config->interfaces_ref()->pop_back();
  config->vlans_ref()->pop_back();
}

void addIpAddress(cfg::SwitchConfig* config, int idx, const std::string& ip) {
  config->interfaces_ref()[idx].ipAddresses_ref()->push_back(ip);
}

} // namespace

namespace facebook::fboss {

class BcmAclCoppTest : public BcmTest {
 protected:
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::oneL3IntfNoIPAddrConfig(
        getHwSwitch(), masterLogicalPortIds()[0]);
    utility::setDefaultCpuTrafficPolicyConfig(
        cfg, getAsic(), getPlatform()->getLocalMac());
    return cfg;
  }
};

TEST_F(BcmAclCoppTest, CoppAcl) {
  auto setup = [this]() { applyNewConfig(initialConfig()); };
  auto verify = [this]() {
    checkCoppAclMatch(getProgrammedState(), getHwSwitch(), getUnit());
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(BcmAclCoppTest, CoppAclAddDelInterface) {
  auto setup = [this]() {
    auto newCfg = initialConfig();
    addInterface(&newCfg);
    applyNewConfig(newCfg);
    delInterface(&newCfg);
    applyNewConfig(newCfg);
  };
  auto verify = [this]() {
    // With DstClassL3 implementation, adding or deleting interface won't
    // affect copp acls amount
    checkCoppAclMatch(getProgrammedState(), getHwSwitch(), getUnit());
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(BcmAclCoppTest, CoppAclAddInterfaceAddr) {
  auto setup = [this]() {
    auto newCfg = initialConfig();
    addIpAddress(&newCfg, 0, "10.1.1.1/24");
    applyNewConfig(newCfg);
  };
  auto verify = [this]() {
    // With DstClassL3 implementation, adding or deleting local ip won't
    // affect copp acls amount
    checkCoppAclMatch(getProgrammedState(), getHwSwitch(), getUnit());
  };
  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(BcmAclCoppTest, CoppAclAddInterface) {
  auto setup = [this]() {
    auto newCfg = initialConfig();
    addInterface(&newCfg);
    applyNewConfig(newCfg);
  };
  auto verify = [this]() {
    // With DstClassL3 implementation, adding or deleting local ip won't
    // affect copp acls amount
    checkCoppAclMatch(getProgrammedState(), getHwSwitch(), getUnit());
  };
  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
