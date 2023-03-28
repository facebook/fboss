// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/switch/SaiSwitch.h"

#include "fboss/agent/LockPolicy.h"
#include "fboss/agent/state/StateDelta.h"

namespace facebook::fboss {
void SaiSwitch::tamEventCallback(
    sai_object_id_t /*tam_event_id*/,
    sai_size_t /*buffer_size*/,
    const void* /*buffer*/,
    uint32_t /*attr_count*/,
    const sai_attribute_t* /*attr_list*/) {
  // noop;
}

void SaiSwitch::parityErrorSwitchEventCallback(
    sai_size_t /*buffer_size*/,
    const void* /*buffer*/,
    uint32_t /*event_type*/) {
  // noop;
}

template <typename LockPolicyT>
std::shared_ptr<SwitchState> SaiSwitch::stateChangedWithOperDeltaLocked(
    const StateDelta& legacyDelta,
    const LockPolicyT& lockPolicy) {
  return stateChanged(legacyDelta, lockPolicy);
}

template std::shared_ptr<SwitchState>
SaiSwitch::stateChangedWithOperDeltaLocked<FineGrainedLockPolicy>(
    const StateDelta& legacyDelta,
    const FineGrainedLockPolicy& lockPolicy);
} // namespace facebook::fboss
