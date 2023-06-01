/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/hw_test/SaiRollbackTest.h"
#include "fboss/agent/hw/test/dataplane_tests/HwTestQueuePerHostUtils.h"

namespace facebook::fboss {

class SaiQPHRollbackTest : public SaiRollbackTest {
 public:
  cfg::SwitchConfig initialConfig() const override {
    auto cfg = utility::onePortPerInterfaceConfig(
        getHwSwitch(),
        masterLogicalPortIds(),
        getAsic()->desiredLoopbackModes());
    if (isSupported(HwAsic::Feature::L3_QOS)) {
      utility::addQueuePerHostQueueConfig(&cfg);
      utility::addQueuePerHostAcls(&cfg);
    }
    return cfg;
  }
};

TEST_F(SaiQPHRollbackTest, rollback) {
  if (!isSupported(HwAsic::Feature::L3_QOS)) {
#if defined(GTEST_SKIP)
    GTEST_SKIP();
#endif
    return;
  }
  auto verify = [this] {
    auto origState = getProgrammedState();
    rollback(origState);
    EXPECT_EQ(origState, getProgrammedState());
  };
  verifyAcrossWarmBoots([]() {}, verify);
}
} // namespace facebook::fboss
