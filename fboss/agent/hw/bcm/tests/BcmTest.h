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
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/BcmWarmBootCache.h"
#include "fboss/agent/hw/bcm/BcmStatUpdater.h"
#include "fboss/agent/hw/bcm/BcmConfig.h"
#include "fboss/agent/hw/test/HwTest.h"
#include "fboss/agent/platforms/common/PlatformProductInfo.h"
#include "fboss/agent/hw/bcm/tests/platforms/BcmTestPlatform.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/StateDelta.h"

namespace facebook {
namespace fboss {

class BcmSwitch;
class BcmTestPlatform;
class SwitchState;

class BcmTest : public HwTest {
 public:
  BcmTest();
  ~BcmTest() override = default;

  cfg::PortSpeed getMaxPortSpeed();
  std::list<FlexPortMode> getSupportedFlexPortModes();


  BcmSwitch* getHwSwitch() const override {
    return static_cast<BcmSwitch*>(HwTest::getHwSwitch());
  }
  BcmTestPlatform* getPlatform() const override  {
    return static_cast<BcmTestPlatform*>(HwTest::getPlatform());
  }

  const std::vector<PortID>& logicalPortIds() const;
  const std::vector<PortID>& masterLogicalPortIds() const;
  std::vector<PortID> getAllPortsinGroup(PortID portID);
  int getUnit() const;

 protected:
  virtual cfg::SwitchConfig initialConfig() const {
    return cfg::SwitchConfig();
  }

 private:
  std::pair<std::unique_ptr<Platform>, std::unique_ptr<HwSwitch>> createHw()
      const override;
  virtual std::unique_ptr<std::thread> createThriftThread() const override;
  bool warmBootSupported() const override { return true; }
  void recreateHwSwitchFromWBState() override;

  /*
   * Most tests don't want packet RX or link scan enabled while running. Other
   * tests might (e.g. trunk tests watching for links going down). Provide
   * a default setting for features desired and let individual tests override
   * it.
   */
  virtual uint32_t featuresDesired() const {
    return (~BcmSwitch::LINKSCAN_DESIRED & ~BcmSwitch::PACKET_RX_DESIRED);
  }

  folly::dynamic createWarmBootSwitchState();

  // Forbidden copy constructor and assignment operator
  BcmTest(BcmTest const &) = delete;
  BcmTest& operator=(BcmTest const &) = delete;

};

} // namespace fboss
} // namespace facebook
