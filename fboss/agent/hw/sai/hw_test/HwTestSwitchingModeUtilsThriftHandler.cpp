/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/hw/sai/api/SwitchApi.h"
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"
#include "fboss/agent/hw/test/HwTestThriftHandler.h"

namespace facebook::fboss::utility {

int32_t HwTestThriftHandler::getSwitchingModeFromHw() {
  auto saiSwitch = static_cast<const SaiSwitch*>(hwSwitch_);
  auto switchId = saiSwitch->getSaiSwitchId();
  auto mode = SaiApiTable::getInstance()->switchApi().getAttribute(
      switchId, SaiSwitchTraits::Attributes::SwitchingMode{});
  XLOG(DBG2) << "getSwitchingModeFromHw: " << mode;
  return mode;
}

} // namespace facebook::fboss::utility
