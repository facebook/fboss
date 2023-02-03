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

#include "fboss/agent/hw/sai/api/DebugCounterApi.h"
#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/platforms/sai/SaiPlatform.h"

extern "C" {
#include <sai.h>
}

namespace facebook::fboss {

void SaiDebugCounterManager::setupDebugCounters() {
  setupPortL3BlackHoleCounter();
  setupMPLSLookupFailedCounter();
}

void SaiDebugCounterManager::setupPortL3BlackHoleCounter() {
  SaiDebugCounterTraits::CreateAttributes attrs{
      SAI_DEBUG_COUNTER_TYPE_PORT_IN_DROP_REASONS,
      SAI_DEBUG_COUNTER_BIND_METHOD_AUTOMATIC,
      SaiDebugCounterTraits::Attributes::InDropReasons{
          {SAI_IN_DROP_REASON_FDB_AND_BLACKHOLE_DISCARDS}}};

  auto& debugCounterStore = saiStore_->get<SaiDebugCounterTraits>();
  portL3BlackHoleCounter_ = debugCounterStore.setObject(attrs, attrs);
  portL3BlackHoleCounterStatId_ = SAI_PORT_STAT_IN_DROP_REASON_RANGE_BASE +
      SaiApiTable::getInstance()->debugCounterApi().getAttribute(
          portL3BlackHoleCounter_->adapterKey(),
          SaiDebugCounterTraits::Attributes::Index{});
}

void SaiDebugCounterManager::setupMPLSLookupFailedCounter() {
  if (!platform_->getAsic()->isSupported(
          HwAsic::Feature::SAI_MPLS_LABEL_LOOKUP_FAIL_COUNTER)) {
    return;
  }
#if SAI_API_VERSION >= SAI_VERSION(1, 9, 0)
  SaiDebugCounterTraits::CreateAttributes attrs{
      SAI_DEBUG_COUNTER_TYPE_PORT_IN_DROP_REASONS,
      SAI_DEBUG_COUNTER_BIND_METHOD_AUTOMATIC,
      SaiDebugCounterTraits::Attributes::InDropReasons{
          {SAI_IN_DROP_REASON_MPLS_MISS}}};
  auto& debugCounterStore = saiStore_->get<SaiDebugCounterTraits>();
  mplsLookupFailCounter_ = debugCounterStore.setObject(attrs, attrs);
  mplsLookupFailCounterStatId_ = SAI_SWITCH_STAT_IN_DROP_REASON_RANGE_BASE +
      SaiApiTable::getInstance()->debugCounterApi().getAttribute(
          mplsLookupFailCounter_->adapterKey(),
          SaiDebugCounterTraits::Attributes::Index{});
#endif
}
} // namespace facebook::fboss
