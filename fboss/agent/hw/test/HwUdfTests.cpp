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
#include "fboss/agent/hw/test/HwTestUdfUtils.h"
#include "fboss/agent/hw/test/LoadBalancerUtils.h"

namespace facebook::fboss {

class HwUdfTest : public HwTest {
 protected:
  std::shared_ptr<SwitchState> setupUdfConfiguration(
      bool addConfig,
      bool udfHash = true) {
    auto udfConfigState = std::make_shared<UdfConfig>();
    cfg::UdfConfig udfConfig;
    if (addConfig) {
      if (udfHash) {
        udfConfig = utility::addUdfHashConfig();
      } else {
        udfConfig = utility::addUdfAclConfig();
      }
    }
    udfConfigState->fromThrift(udfConfig);

    auto state = getProgrammedState();
    state->modify(&state);
    auto switchSettings = util::getFirstNodeIf(state->getSwitchSettings());
    auto newSwitchSettings = switchSettings->modify(&state);
    newSwitchSettings->setUdfConfig(udfConfigState);
    return state;
  }
};

TEST_F(HwUdfTest, checkUdfHashConfiguration) {
  auto setup = [=]() { applyNewState(setupUdfConfiguration(true, true)); };
  auto verify = [=]() {
    utility::validateUdfConfig(
        getHwSwitch(), utility::kUdfHashGroupName, utility::kUdfPktMatcherName);
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwUdfTest, checkUdfAclConfiguration) {
  auto setup = [=]() { applyNewState(setupUdfConfiguration(true, false)); };
  auto verify = [=]() {
    utility::validateUdfConfig(
        getHwSwitch(),
        utility::kUdfRoceOpcodeAclGroupName,
        utility::kUdfPktMatcherName);
  };

  verifyAcrossWarmBoots(setup, verify);
}

TEST_F(HwUdfTest, deleteUdfConfig) {
  int udfGroupId;
  int udfPacketMatcherId;
  auto setup = [&]() {
    applyNewState(setupUdfConfiguration(true));
    udfGroupId =
        utility::getHwUdfGroupId(getHwSwitch(), utility::kUdfHashGroupName);
    udfPacketMatcherId = utility::getHwUdfPacketMatcherId(
        getHwSwitch(), utility::kUdfPktMatcherName);
    applyNewState(setupUdfConfiguration(false));
  };
  auto verify = [=]() {
    utility::validateRemoveUdfGroup(
        getHwSwitch(), utility::kUdfHashGroupName, udfGroupId);
    utility::validateRemoveUdfPacketMatcher(
        getHwSwitch(), utility::kUdfPktMatcherName, udfPacketMatcherId);
  };
  verifyAcrossWarmBoots(setup, verify);
}

} // namespace facebook::fboss
