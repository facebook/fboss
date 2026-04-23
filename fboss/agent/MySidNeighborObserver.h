// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <memory>
#include <vector>
#include "fboss/agent/StateObserver.h"
#include "fboss/agent/rib/RoutingInformationBase.h"
#include "fboss/agent/state/RouteNextHopEntry.h"
#include "fboss/agent/state/StateDelta.h"

namespace facebook::fboss {

class SwSwitch;
class MySid;

/**
 * Observes MySID and neighbor state changes to resolve config-driven
 * ADJACENCY_MICRO_SID (uA) entries.
 *
 * All resolutions within a single stateUpdated call are batched and
 * enqueued as a single async rib->update() on the RIB event-base thread.
 * Asynchronous dispatch avoids deadlock: stateUpdated runs on the SwSwitch
 * update thread, and a synchronous rib->update would block waiting for the
 * RIB thread, which may itself need to schedule a state update back.
 */
class MySidNeighborObserver : public StateObserver {
 public:
  explicit MySidNeighborObserver(SwSwitch* sw);
  ~MySidNeighborObserver() override;

  // Observers register their address with SwSwitch; copying or moving would
  // invalidate that registration.
  MySidNeighborObserver(const MySidNeighborObserver&) = delete;
  MySidNeighborObserver& operator=(const MySidNeighborObserver&) = delete;
  MySidNeighborObserver(MySidNeighborObserver&&) = delete;
  MySidNeighborObserver& operator=(MySidNeighborObserver&&) = delete;

  void stateUpdated(const StateDelta& delta) override;

 private:
  template <typename AddedNeighborEntryT>
  void processAdded(
      const std::shared_ptr<SwitchState>& switchState,
      const std::shared_ptr<AddedNeighborEntryT>& addedEntry);

  template <typename RemovedNeighborEntryT>
  void processRemoved(
      const std::shared_ptr<SwitchState>& switchState,
      const std::shared_ptr<RemovedNeighborEntryT>& removedEntry);

  template <typename ChangedNeighborEntryT>
  void processChanged(
      const StateDelta& stateDelta,
      const std::shared_ptr<ChangedNeighborEntryT>& oldEntry,
      const std::shared_ptr<ChangedNeighborEntryT>& newEntry);

  // Queue a resolution (or unresolution) for batch dispatch.
  void queueResolution(
      const std::shared_ptr<MySid>& mySid,
      std::optional<folly::IPAddress> neighborIP);

  // Flush all queued resolutions as a single async rib->update() on the
  // RIB event-base thread.
  void flushPendingResolutions();

  SwSwitch* sw_;

  // Accumulated during a single stateUpdated call, flushed at the end.
  std::vector<MySidWithNextHops> pendingResolutions_;
};

} // namespace facebook::fboss
