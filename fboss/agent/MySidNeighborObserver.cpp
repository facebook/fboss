// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/MySidNeighborObserver.h"

#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/SwSwitchMySidUpdater.h"
#include "fboss/agent/state/ArpEntry.h"
#include "fboss/agent/state/MySid.h"
#include "fboss/agent/state/NdpEntry.h"
#include "fboss/agent/state/RouteNextHop.h"
#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss {

MySidNeighborObserver::MySidNeighborObserver(SwSwitch* sw) : sw_(sw) {
  sw_->registerStateObserver(this, "MySidNeighborObserver");
}

MySidNeighborObserver::~MySidNeighborObserver() {
  sw_->unregisterStateObserver(this);
}

void MySidNeighborObserver::stateUpdated(const StateDelta& /*delta*/) {}

void MySidNeighborObserver::queueResolution(
    const std::shared_ptr<MySid>& /*mySid*/,
    std::optional<folly::IPAddress> /*neighborIP*/) {}

void MySidNeighborObserver::flushPendingResolutions() {}

// Stub implementations for neighbor callbacks.
template <typename AddedNeighborEntryT>
void MySidNeighborObserver::processAdded(
    const std::shared_ptr<SwitchState>& /*switchState*/,
    const std::shared_ptr<AddedNeighborEntryT>& /*addedEntry*/) {}

template <typename RemovedNeighborEntryT>
void MySidNeighborObserver::processRemoved(
    const std::shared_ptr<SwitchState>& /*switchState*/,
    const std::shared_ptr<RemovedNeighborEntryT>& /*removedEntry*/) {}

template <typename ChangedNeighborEntryT>
void MySidNeighborObserver::processChanged(
    const StateDelta& /*stateDelta*/,
    const std::shared_ptr<ChangedNeighborEntryT>& /*oldEntry*/,
    const std::shared_ptr<ChangedNeighborEntryT>& /*newEntry*/) {}

// Explicit template instantiations
template void MySidNeighborObserver::processAdded<NdpEntry>(
    const std::shared_ptr<SwitchState>&,
    const std::shared_ptr<NdpEntry>&);
template void MySidNeighborObserver::processAdded<ArpEntry>(
    const std::shared_ptr<SwitchState>&,
    const std::shared_ptr<ArpEntry>&);
template void MySidNeighborObserver::processRemoved<NdpEntry>(
    const std::shared_ptr<SwitchState>&,
    const std::shared_ptr<NdpEntry>&);
template void MySidNeighborObserver::processRemoved<ArpEntry>(
    const std::shared_ptr<SwitchState>&,
    const std::shared_ptr<ArpEntry>&);
template void MySidNeighborObserver::processChanged<NdpEntry>(
    const StateDelta&,
    const std::shared_ptr<NdpEntry>&,
    const std::shared_ptr<NdpEntry>&);
template void MySidNeighborObserver::processChanged<ArpEntry>(
    const StateDelta&,
    const std::shared_ptr<ArpEntry>&,
    const std::shared_ptr<ArpEntry>&);

} // namespace facebook::fboss
