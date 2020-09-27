/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/platforms/sai/SaiBcmPlatformPort.h"
#include "fboss/agent/platforms/sai/SaiPlatform.h"

#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/hw/sai/api/SwitchApi.h"

#include "fboss/agent/hw/sai/switch/SaiSwitch.h"

namespace facebook::fboss {

uint32_t SaiBcmPlatformPort::getPhysicalLaneId(
    uint32_t chipId,
    uint32_t logicalLane) const {
  auto platform = static_cast<SaiPlatform*>(getPlatform());
  return chipId * platform->numLanesPerCore() + logicalLane + 1;
}

bool SaiBcmPlatformPort::supportsTransceiver() const {
  return true;
}

uint32_t SaiBcmPlatformPort::getPhysicalPortId() const {
  return getHwPortLanes(getCurrentProfile()).front();
}

void SaiBcmPlatformPort::setLEDState(
    uint32_t led,
    uint32_t index,
    uint32_t status) {
  std::array<sai_uint32_t, 4> data{};
  data[0] = led;
  data[1] = 1; // data bank
  data[2] = index;
  data[3] = status;

  SaiSwitch* saiSwitch = static_cast<SaiSwitch*>(getPlatform()->getHwSwitch());
  auto switchID = saiSwitch->getSwitchId();
  XLOG(DBG5) << "setting led state : (" << led << "," << index << ","
             << std::hex << status << ")";
  SaiApiTable::getInstance()->switchApi().setAttribute(
      switchID,
      SaiSwitchTraits::Attributes::Led{
          std::vector<sai_uint32_t>(data.begin(), data.end())});
  currentLedState_ = status;
}

uint32_t SaiBcmPlatformPort::getLEDState(uint32_t led, uint32_t index) {
  std::array<sai_uint32_t, 4> data{};
  data[0] = led;
  data[1] = 1; // data bank
  data[2] = index;
  data[3] = 0;

  SaiSwitch* saiSwitch = static_cast<SaiSwitch*>(getPlatform()->getHwSwitch());
  auto switchID = saiSwitch->getSwitchId();

  auto switchLedAttribute =
      SaiApiTable::getInstance()->switchApi().getAttribute(
          switchID,
          SaiSwitchTraits::Attributes::Led{
              std::vector<sai_uint32_t>(data.begin(), data.end())});
  if (switchLedAttribute.size() != 4) {
    throw FbossError("error retreiving LED state");
  }
  return switchLedAttribute[3];
}

uint32_t SaiBcmPlatformPort::getCurrentLedState() const {
  return currentLedState_;
}

} // namespace facebook::fboss
