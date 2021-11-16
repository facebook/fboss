// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/agent/hw/sai/switch/SaiDebugCounterManager.h"

#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/platforms/sai/SaiPlatform.h"

extern "C" {
#include <experimental/sai_attr_ext.h>
}

namespace facebook::fboss {
void SaiDebugCounterManager::setupMPLSLookupFailedCounter() {
  if (!platform_->getAsic()->isSupported(
          HwAsic::Feature::SAI_MPLS_LABEL_LOOKUP_FAIL_COUNTER)) {
    return;
  }
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
}

} // namespace facebook::fboss
