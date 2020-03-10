/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include <memory>
#include <thread>

#include <folly/dynamic.h>

#include "fboss/agent/Constants.h"
#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/hw/bcm/BcmConfig.h"
#include "fboss/agent/hw/bcm/BcmPort.h"
#include "fboss/agent/hw/bcm/BcmPortTable.h"
#include "fboss/agent/hw/bcm/BcmStatUpdater.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/BcmWarmBootCache.h"
#include "fboss/agent/hw/gen-cpp2/hardware_stats_types.h"
#include "fboss/agent/hw/test/HwTest.h"
#include "fboss/agent/platforms/common/PlatformProductInfo.h"
#include "fboss/agent/platforms/tests/utils/BcmTestPlatform.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss {

class BcmSwitch;
class BcmTestPlatform;
class SwitchState;
class HwSwitchEnsemble;

class BcmTest : public HwTest {
 public:
  BcmTest();
  ~BcmTest() override;

  BcmSwitch* getHwSwitch() override {
    return static_cast<BcmSwitch*>(HwTest::getHwSwitch());
  }
  BcmTestPlatform* getPlatform() override {
    return static_cast<BcmTestPlatform*>(HwTest::getPlatform());
  }
  const BcmSwitch* getHwSwitch() const override {
    return static_cast<const BcmSwitch*>(HwTest::getHwSwitch());
  }
  const BcmTestPlatform* getPlatform() const override {
    return static_cast<const BcmTestPlatform*>(HwTest::getPlatform());
  }

  int getUnit() const;

 protected:
  virtual cfg::SwitchConfig initialConfig() const {
    return cfg::SwitchConfig();
  }

 private:
  // Forbidden copy constructor and assignment operator
  BcmTest(BcmTest const&) = delete;
  BcmTest& operator=(BcmTest const&) = delete;
};

} // namespace facebook::fboss
