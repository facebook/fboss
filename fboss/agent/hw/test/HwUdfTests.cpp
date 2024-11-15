/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/test/ConfigFactory.h"
#include "fboss/agent/hw/test/HwTest.h"
#include "fboss/agent/hw/test/HwTestAclUtils.h"
#include "fboss/agent/hw/test/HwTestCoppUtils.h"
#include "fboss/agent/hw/test/HwTestUdfUtils.h"

namespace facebook::fboss {

class HwUdfTest : public HwTest {
 protected:
  cfg::SwitchConfig initialConfig() const {
    auto cfg = utility::onePortPerInterfaceConfig(
        getHwSwitch(),
        masterLogicalPortIds(),
        getAsic()->desiredLoopbackModes());

    return cfg;
  }

  std::shared_ptr<SwitchState> setupUdfConfiguration(
      bool addConfig,
      bool udfHashEnabled = true,
      bool udfAclEnabled = false) {
    auto udfConfigState = std::make_shared<UdfConfig>();
    cfg::UdfConfig udfConfig;
    if (addConfig) {
      if (udfHashEnabled && udfAclEnabled) {
        udfConfig = utility::addUdfHashAclConfig();
      } else if (udfHashEnabled) {
        udfConfig = utility::addUdfHashConfig();
      } else {
        udfConfig = utility::addUdfAclConfig();
      }
    }
    udfConfigState->fromThrift(udfConfig);

    auto state = getProgrammedState();
    state->modify(&state);
    auto switchSettings = utility::getFirstNodeIf(state->getSwitchSettings());
    auto newSwitchSettings = switchSettings->modify(&state);
    newSwitchSettings->setUdfConfig(udfConfigState);
    return state;
  }
};

TEST_F(HwUdfTest, checkUdfHashConfiguration) {
  auto setup = [=, this]() {
    applyNewState(setupUdfConfiguration(true, true));
  };
  auto verify = [=, this]() {
    utility::validateUdfConfig(
        getHwSwitch(),
        utility::kUdfHashDstQueuePairGroupName,
        utility::kUdfL4UdpRocePktMatcherName);
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwUdfTest, checkUdfAclConfiguration) {
  auto setup = [=, this]() {
    applyNewState(setupUdfConfiguration(true, false));
  };
  auto verify = [=, this]() {
    utility::validateUdfConfig(
        getHwSwitch(),
        utility::kUdfAclRoceOpcodeGroupName,
        utility::kUdfL4UdpRocePktMatcherName);
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwUdfTest, deleteUdfHashConfig) {
  // Add UdfGroup and PacketMatcher configuration for UDF Hash
  applyNewState(setupUdfConfiguration(true));

  // Get UdfGroup and PacketMatcher Ids for verify
  int udfGroupId = utility::getHwUdfGroupId(
      getHwSwitch(), utility::kUdfHashDstQueuePairGroupName);
  int udfPacketMatcherId = utility::getHwUdfPacketMatcherId(
      getHwSwitch(), utility::kUdfL4UdpRocePktMatcherName);

  // Remove UdfGroup and PacketMatcher configuration for UDF Hash
  applyNewState(setupUdfConfiguration(false));

  auto verify = [=, this]() {
    // Verify that UdfGroup and PacketMatcher are deleted
    utility::validateRemoveUdfGroup(
        getHwSwitch(), utility::kUdfHashDstQueuePairGroupName, udfGroupId);
    utility::validateRemoveUdfPacketMatcher(
        getHwSwitch(),
        utility::kUdfL4UdpRocePktMatcherName,
        udfPacketMatcherId);
  };
  verifyAcrossWarmBoots([] {}, verify);
}

// This test is to verify that UdfGroup(roceOpcode) for UdfAcl and associated
// PacketMatcher can be successfully deleted.
TEST_F(HwUdfTest, deleteUdfAclConfig) {
  auto newCfg{initialConfig()};
  // Add UdfGroup and PacketMatcher configuration for UDF ACL
  newCfg.udfConfig() = utility::addUdfAclConfig();

  // Add ACL configuration
  auto acl = utility::addAcl(&newCfg, "test-udf-acl");
  acl->udfGroups() = {utility::kUdfAclRoceOpcodeGroupName};
  acl->roceOpcode() = utility::kUdfRoceOpcodeAck;
  applyNewConfig(newCfg);

  // Get UdfGroup and PacketMatcher Ids for verify
  int udfGroupId = utility::getHwUdfGroupId(
      getHwSwitch(), utility::kUdfAclRoceOpcodeGroupName);
  int udfPacketMatcherId = utility::getHwUdfPacketMatcherId(
      getHwSwitch(), utility::kUdfL4UdpRocePktMatcherName);

  // Delete UdfGroup and PacketMatcher configuration for UDF ACL
  auto resetCfg{initialConfig()};
  applyNewConfig(resetCfg);

  auto verify = [=, this]() {
    // Verify that UdfGroup and PacketMatcher are deleted
    utility::validateRemoveUdfGroup(
        getHwSwitch(), utility::kUdfAclRoceOpcodeGroupName, udfGroupId);
    utility::validateRemoveUdfPacketMatcher(
        getHwSwitch(),
        utility::kUdfL4UdpRocePktMatcherName,
        udfPacketMatcherId);
    // Verify that UdfGroupIds in Qset is deleted
    utility::validateUdfIdsInQset(
        getHwSwitch(),
        getHwSwitch()->getPlatform()->getAsic()->getDefaultACLGroupID(),
        false);
  };
  verifyAcrossWarmBoots([] {}, verify);
}

TEST_F(HwUdfTest, addAclConfig) {
  if (getPlatform()->getAsic()->getAsicType() ==
      cfg::AsicType::ASIC_TYPE_FAKE) {
    GTEST_SKIP();
  }
  auto setup = [&]() {
    auto cfg{initialConfig()};
    cfg = utility::addUdfAclRoceOpcodeConfig(cfg);

    applyNewConfig(cfg);
  };

  auto verify = [=, this]() {
    utility::validateUdfAclRoceOpcodeConfig(
        getHwSwitch(), getProgrammedState());
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwUdfTest, checkUdfHashAclConfiguration) {
  auto setup = [=, this]() {
    // Add Udf configuration for both hash and acl, first parameter is
    // addConfig and second udfHashEnabaled and third is udfAclEnabled
    applyNewState(setupUdfConfiguration(true, true, true));
  };
  auto verify = [=, this]() {
    // Verify that UdfHash configuration is added
    utility::validateUdfConfig(
        getHwSwitch(),
        utility::kUdfHashDstQueuePairGroupName,
        utility::kUdfL4UdpRocePktMatcherName);

    // Verify that UdfAcl configuration is added
    utility::validateUdfConfig(
        getHwSwitch(),
        utility::kUdfAclRoceOpcodeGroupName,
        utility::kUdfL4UdpRocePktMatcherName);
  };

  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
