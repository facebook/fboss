/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/dataplane_tests/HwTestAqmUtils.h"
#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/hw/sai/api/SwitchApi.h"
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"

namespace facebook::fboss::utility {
uint32_t getEgressSharedPoolLimitBytes(HwSwitch* hwSwitch) {
  SaiSwitchTraits::Attributes::EgressPoolAvaialableSize sz{};
  const auto saiSwitch = static_cast<const SaiSwitch*>(hwSwitch);
  SwitchSaiId switchId =
      saiSwitch->managerTable()->switchManager().getSwitchSaiId();
  return SaiApiTable::getInstance()->switchApi().getAttribute(switchId, sz);
}
} // namespace facebook::fboss::utility
