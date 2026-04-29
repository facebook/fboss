// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <memory>
#include <unordered_map>
#include <vector>
#include "fboss/agent/StateObserver.h"
#include "fboss/agent/rib/RoutingInformationBase.h"
#include "fboss/agent/state/RouteNextHopEntry.h"
#include "fboss/agent/state/StateDelta.h"
#include "fboss/agent/types.h"

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
      const std::vector<std::shared_ptr<MySid>>& mySidsOnIntf,
      const std::shared_ptr<AddedNeighborEntryT>& addedEntry);

  template <typename RemovedNeighborEntryT>
  void processRemoved(
      const std::vector<std::shared_ptr<MySid>>& mySidsOnIntf,
      const std::shared_ptr<RemovedNeighborEntryT>& removedEntry);

  template <typename ChangedNeighborEntryT>
  void processChanged(
      const std::vector<std::shared_ptr<MySid>>& mySidsOnIntf,
      const std::shared_ptr<ChangedNeighborEntryT>& oldEntry,
      const std::shared_ptr<ChangedNeighborEntryT>& newEntry);

  // True iff this MySid is a uA candidate that needs an initial
  // resolution: type is ADJACENCY_MICRO_SID and no unresolved next-hop
  // id has been allocated yet. Used by the MySid-added/changed handler
  // and by the neighbor-added walk.
  bool needsResolve(const std::shared_ptr<MySid>& mySid) const;

  // Bucket every uA MySid in `state` by its `adjacencyInterfaceId`.
  // Built once per stateUpdated to make per-neighbor lookups O(K)
  // (K = uA MySids on the intf) rather than re-scanning the full
  // MySidMap for each neighbor delta event.
  std::unordered_map<InterfaceID, std::vector<std::shared_ptr<MySid>>>
  buildIntfToUASidsMap(const std::shared_ptr<SwitchState>& state) const;

  // Check if a neighbor entry is relevant for MySid resolution:
  // must be reachable and non-link-local.
  template <typename NeighborEntryT>
  bool isReachableNonLinkLocalNeighbor(
      const std::shared_ptr<NeighborEntryT>& entry) const;

  // Bind the neighbor's IP to every uA MySid in `mySidsOnIntf` (already
  // filtered to the neighbor's intf) that matches the neighbor's IP
  // family and does not yet have an unresolved next hop.
  template <typename NeighborEntryT>
  void assignNeighborToMySids(
      const std::vector<std::shared_ptr<MySid>>& mySidsOnIntf,
      const std::shared_ptr<NeighborEntryT>& neighborEntry);

  // For every bound uA MySid in `mySidsOnIntf` matching the neighbor's
  // IP family, queue a conditional unresolve keyed by the removed
  // neighbor's IP. RIB does the precise match against the materialized
  // unresolved next-hop set (the observer can't, since it only sees a
  // NextHopSetID).
  template <typename NeighborEntryT>
  void clearNeighborFromMySids(
      const std::vector<std::shared_ptr<MySid>>& mySidsOnIntf,
      const std::shared_ptr<NeighborEntryT>& neighborEntry);

  // Scan neighbor table for the MySid's interface and queue resolution
  // if a reachable non-link-local neighbor is found.
  void handleMySidAddedOrChanged(
      const std::shared_ptr<SwitchState>& state,
      const std::shared_ptr<MySid>& newEntry);

  // Queue a positive resolution: assign neighborIP as the MySid's
  // unresolved next hop. RIB unconditionally overwrites.
  void queueResolve(
      const std::shared_ptr<MySid>& mySid,
      folly::IPAddress neighborIP);

  // Queue a conditional unresolve: clear the MySid's unresolved + resolved
  // ids only if the materialized unresolved next-hop set contains
  // removedIp. RIB-side check.
  void queueReresolveIfMatch(
      const std::shared_ptr<MySid>& mySid,
      folly::IPAddress removedIp);

  // Flush all queued resolves and conditional unresolves as a single
  // async rib->update() on the RIB event-base thread.
  void flushPendingResolutions();

  SwSwitch* sw_;

  // Accumulated during a single stateUpdated call, flushed at the end.
  std::vector<MySidWithNextHops> pendingResolves_;
  std::vector<MySidNeighborRemoved> pendingUnresolveIfMatch_;
};

} // namespace facebook::fboss
