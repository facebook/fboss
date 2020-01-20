/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/hw/bcm/tests/dataplane_tests/BcmConfigSetupTests.h"
#include "fboss/agent/hw/test/ConfigFactory.h"

namespace facebook::fboss {

class BcmConfigVerifyQosTest : public BcmConfigSetupTest {
 private:
  cfg::SwitchConfig getFallbackConfig() const override {
    return utility::oneL3IntfConfig(
        getHwSwitch(), masterLogicalPortIds()[0], cfg::PortLoopbackMode::MAC);
  }
};

TEST_F(BcmConfigVerifyQosTest, verifyDscpQueueMapping) {
  auto verifyPostWb = [&]() {
    // TODO: verify that DSCP Queue mapping should honor either 2Q config or
    // Olympic config.
  };

  verifyAcrossWarmBoots(
      testSetup(), testVerify(), testSetupPostWb(), verifyPostWb);
}

} // namespace facebook::fboss
