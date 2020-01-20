/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmBstStatsMgr.h"
#include "fboss/agent/hw/bcm/BcmError.h"
#include "fboss/agent/hw/bcm/tests/BcmTest.h"
#include "fboss/agent/hw/bcm/tests/BcmTestUtils.h"

#include "fboss/agent/hw/test/ConfigFactory.h"

using std::shared_ptr;
using namespace facebook::fboss;

namespace facebook::fboss {

class BcmBstStatsMgrTest : public BcmTest {
 protected:
  cfg::SwitchConfig initialConfig() const override {
    return utility::twoL3IntfConfig(
        getHwSwitch(), masterLogicalPortIds()[0], masterLogicalPortIds()[1]);
  }
};

} // namespace facebook::fboss

TEST_F(BcmBstStatsMgrTest, StatEnableDisable) {
  auto setup = [this]() { applyNewConfig(initialConfig()); };

  auto verify = [this]() {
    // Enable BST stats ODS logging and verify
    EXPECT_EQ(
        getHwSwitch()->getBstStatsMgr()->startBufferStatCollection(), true);
    EXPECT_EQ(
        getHwSwitch()->getBstStatsMgr()->isBufferStatCollectionEnabled(), true);
    utility::assertSwitchControl(bcmSwitchBstEnable, 1);

    // Enable Scuba logging and verify if enabled
    EXPECT_EQ(
        getHwSwitch()->getBstStatsMgr()->startFineGrainedBufferStatLogging(),
        true);
    EXPECT_EQ(
        getHwSwitch()
            ->getBstStatsMgr()
            ->isFineGrainedBufferStatLoggingEnabled(),
        true);

    // Disable Scuba logging and verify if disabled
    EXPECT_EQ(
        getHwSwitch()->getBstStatsMgr()->stopFineGrainedBufferStatLogging(),
        true);
    EXPECT_EQ(
        getHwSwitch()
            ->getBstStatsMgr()
            ->isFineGrainedBufferStatLoggingEnabled(),
        false);

    // Disable BST stats ODS logging and verify if disabled
    EXPECT_EQ(
        getHwSwitch()->getBstStatsMgr()->stopBufferStatCollection(), true);
    EXPECT_EQ(
        getHwSwitch()->getBstStatsMgr()->isBufferStatCollectionEnabled(),
        false);
    utility::assertSwitchControl(bcmSwitchBstEnable, 0);
  };

  verifyAcrossWarmBoots(setup, verify);
}
