/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/HwTestSwitchingModeUtils.h"

#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/hw/sai/api/SwitchApi.h"
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"

namespace facebook::fboss::utility {

int32_t getSwitchingModeFromHw(const facebook::fboss::HwSwitch* hw) {
  const auto saiSwitch = static_cast<const SaiSwitch*>(hw);
  auto switchId = saiSwitch->getSaiSwitchId();
  return SaiApiTable::getInstance()->switchApi().getAttribute(
      switchId, SaiSwitchTraits::Attributes::SwitchingMode{});
}

} // namespace facebook::fboss::utility
