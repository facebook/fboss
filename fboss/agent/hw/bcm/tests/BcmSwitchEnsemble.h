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
  const BcmTestPlatform* getPlatform() const override {
    return static_cast<const BcmTestPlatform*>(HwSwitchEnsemble::getPlatform());
  }
  BcmSwitch* getHwSwitch() override {
    return static_cast<BcmSwitch*>(HwSwitchEnsemble::getHwSwitch());
  }
  const BcmSwitch* getHwSwitch() const override {
    return static_cast<const BcmSwitch*>(HwSwitchEnsemble::getHwSwitch());
  }
  const std::vector<PortID>& logicalPortIds() const override;
  const std::vector<PortID>& masterLogicalPortIds() const override;
  std::vector<PortID> getAllPortsinGroup(PortID portID) const override;
  std::vector<FlexPortMode> getSupportedFlexPortModes() const override;

  void dumpHwCounters() const override;
  bool warmBootSupported() const override {
    return true;
  }
  void recreateHwSwitchFromWBState() override;

  std::map<PortID, HwPortStats> getLatestPortStats(
      const std::vector<PortID>& ports) override;

  void stopHwCallLogging() const override;

 private:
  std::unique_ptr<HwLinkStateToggler> createLinkToggler(
      HwSwitch* hwSwitch,
      cfg::PortLoopbackMode desiredLoopbackMode);
  folly::dynamic createWarmBootSwitchState() const;
};

} // namespace fboss
} // namespace facebook
