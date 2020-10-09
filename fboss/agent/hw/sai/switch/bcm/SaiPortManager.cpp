// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/switch/SaiPortManager.h"

namespace facebook::fboss {

// Hack as this SAI implementation does support port removal. Port can only be
// removed iff they're followed by add operation. So put removed ports in
// removedHandles_ so port does not get destroyed by invoking SAI's remove port
// API. However their dependent objects such as bridge ports, fdb entries  etc.
// must act as if the port has been removed. So notify those objects to mimic
// port removal.
void SaiPortManager::addRemovedHandle(PortID portID) {
  auto itr = handles_.find(portID);
  CHECK(itr != handles_.end());
  itr->second->queues.clear();
  itr->second->configuredQueues.clear();
  itr->second->bridgePort.reset();
  // instead of removing port, retain it
  itr->second->port->notifyBeforeDestroy();
  removedHandles_.emplace(itr->first, std::move(itr->second));
}

// Before adding port, remove the port from removedHandles_. This also deletes
// the port and invokes SAI's remove port API. This is acceptable, because port
// will be added again with new lanes later.
void SaiPortManager::removeRemovedHandleIf(PortID portID) {
  removedHandles_.erase(portID);
}

} // namespace facebook::fboss
