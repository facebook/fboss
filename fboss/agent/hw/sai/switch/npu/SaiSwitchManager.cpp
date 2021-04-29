/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"

#include "fboss/agent/hw/sai/api/AdapterKeySerializers.h"
#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/hw/sai/api/SwitchApi.h"

namespace facebook::fboss {

void SaiSwitchManager::initCpuPort() {
  cpuPort_ = SaiApiTable::getInstance()->switchApi().getAttribute(
      switch_->adapterKey(), SaiSwitchTraits::Attributes::CpuPort{});
}

} // namespace facebook::fboss
