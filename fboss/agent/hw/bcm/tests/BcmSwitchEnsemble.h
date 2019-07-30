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

#include "fboss/agent/hw/test/HwSwitchEnsemble.h"

#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/platforms/test_platforms/BcmTestPlatform.h"

DECLARE_bool(flexports);
DECLARE_string(bcm_config);

namespace facebook {
namespace fboss {

class BcmSwitchEnsemble : public HwSwitchEnsemble {
 public:
  explicit BcmSwitchEnsemble(
      uint32_t featuresDesired =
          (HwSwitch::PACKET_RX_DESIRED | HwSwitch::LINKSCAN_DESIRED));
  BcmTestPlatform* getPlatform() override {
    return static_cast<BcmTestPlatform*>(HwSwitchEnsemble::getPlatform());
  }
  BcmSwitch* getHwSwitch() override {
    return static_cast<BcmSwitch*>(HwSwitchEnsemble::getHwSwitch());
  }

 private:
  std::unique_ptr<HwSwitch> createHwSwitch(
      Platform* platform,
      uint32_t featuresDesired) override;
  std::unique_ptr<HwLinkStateToggler> createLinkToggler(
      HwSwitch* hwSwitch) override;
};

std::unique_ptr<BcmSwitchEnsemble> setupEnsemble();

} // namespace fboss
} // namespace facebook
