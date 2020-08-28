/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/switch/SaiDebugCounterManager.h"

#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"

namespace facebook::fboss {

void SaiDebugCounterManager::setupDebugCounters() {
  SaiDebugCounterTraits::CreateAttributes attrs{
      SAI_DEBUG_COUNTER_TYPE_PORT_IN_DROP_REASONS,
      SAI_DEBUG_COUNTER_BIND_METHOD_AUTOMATIC,
      SaiDebugCounterTraits::Attributes::InDropReasons{
          {SAI_IN_DROP_REASON_END + 1}},
      std::nullopt};

  portL3BlackHoleCounter_ = std::make_unique<SaiDebugCounter>(
      attrs, attrs, managerTable_->switchManager().getSwitchSaiId());
}
} // namespace facebook::fboss
