// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/MySidNeighborObserver.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/SwSwitchMySidUpdater.h"
#include "fboss/agent/state/ArpEntry.h"
#include "fboss/agent/state/Interface.h"
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

void MySidNeighborObserver::stateUpdated(const StateDelta& delta) {
  // Process MySid deltas — added or changed entries that need observer-
  // driven resolution. Removed MySids do not need an explicit unresolve
  // here: the config path (RibRouteTables::reconfigure) issues a matching
  // toDelete in the same delta, which clears the MySidTable entry and
  // releases its allocated next-hop ID.
  DeltaFunctions::forEachChanged(
      delta.getMySidsDelta(),
      [this, &delta](const auto& /*oldEntry*/, const auto& newEntry) {
        handleMySidAddedOrChanged(delta.newState(), newEntry);
      },
      [this, &delta](const auto& addedEntry) {
        handleMySidAddedOrChanged(delta.newState(), addedEntry);
      },
      [](const auto& /*removedEntry*/) {});

  // 2. Process neighbor deltas. Iterate the interface delta directly
  //    and dispatch ARP/NDP entry changes to the per-entry handlers.
  //
  //    Early-out when no MySids are configured: every ARP/NDP delta on
  //    the box would otherwise walk all interfaces — wasted work on the
  //    vast majority of switches that don't use MySid.
  //
  //    We do not skip removed interfaces: when an intf goes away its
  //    neighbor entries surface here as removals, and any MySid still
  //    bound to one must be cleared via the RIB match path.
  if (delta.newState()->getMySids()->numNodes() > 0) {
    // Bucket all uA MySids by their adjacencyInterfaceId once, so each
    // neighbor delta lookup is O(K) (K = uA MySids on that intf,
    // typically small) instead of O(M) over the full MySidMap. Total
    // cost across N neighbor events drops from O(N*M) to O(M + N*K),
    // which matters during warmboot replay where N is large.
    const auto intfToUASids = buildIntfToUASidsMap(delta.newState());

    auto processNeighborDelta = [this, &intfToUASids](const auto& nbrDelta) {
      const auto& oldEntry = nbrDelta.getOld();
      const auto& newEntry = nbrDelta.getNew();
      // Pick whichever side exists; for "changed" both sides have the
      // same intf (intf moves surface as remove+add).
      const auto& probe = newEntry ? newEntry : oldEntry;
      auto it = intfToUASids.find(probe->getIntfID());
      if (it == intfToUASids.end()) {
        // No uA MySids on this intf — nothing this observer can do.
        return;
      }
      const auto& mySidsOnIntf = it->second;
      if (!oldEntry) {
        processAdded(mySidsOnIntf, newEntry);
      } else if (!newEntry) {
        processRemoved(mySidsOnIntf, oldEntry);
      } else {
        processChanged(mySidsOnIntf, oldEntry, newEntry);
      }
    };
    for (const auto& intfDelta : delta.getIntfsDelta()) {
      for (const auto& arpDelta : intfDelta.getArpDelta()) {
        processNeighborDelta(arpDelta);
      }
      for (const auto& ndpDelta : intfDelta.getNdpDelta()) {
        processNeighborDelta(ndpDelta);
      }
    }
  }

  // 3. Flush all batched resolutions as a single rib->update()
  flushPendingResolutions();
}

void MySidNeighborObserver::handleMySidAddedOrChanged(
    const std::shared_ptr<SwitchState>& state,
    const std::shared_ptr<MySid>& newEntry) {
  if (!needsResolve(newEntry)) {
    return;
  }
  // needsResolve guarantees this is a uA entry. Every uA MySid that
  // reaches the observer must carry adjacencyInterfaceId and isV6 —
  // both are set by MySidConfigUtils::convertMySidConfig (config path)
  // and by tests that exercise the RIB API directly.
  CHECK(newEntry->getAdjacencyInterfaceId().has_value());
  CHECK(newEntry->getIsV6().has_value());

  auto intfId = InterfaceID(*newEntry->getAdjacencyInterfaceId());
  bool isV6 = *newEntry->getIsV6();

  auto intf = state->getInterfaces()->getNodeIf(intfId);
  if (!intf) {
    // No matching interface — leave unresolved; will be picked up when
    // a neighbor comes up on the right intf.
    return;
  }

  // Scan the neighbor table for the first reachable entry.
  auto findReachableNeighbor =
      [](const auto& table) -> std::optional<folly::IPAddress> {
    if (!table) {
      return std::nullopt;
    }
    for (const auto& [__, entry] : std::as_const(*table)) {
      if (entry->isReachable()) {
        return folly::IPAddress(entry->getIP());
      }
    }
    return std::nullopt;
  };
  auto neighborIP = isV6 ? findReachableNeighbor(intf->getNdpTable())
                         : findReachableNeighbor(intf->getArpTable());
  if (neighborIP.has_value()) {
    queueResolve(newEntry, *neighborIP);
  }
}

void MySidNeighborObserver::queueResolve(
    const std::shared_ptr<MySid>& mySid,
    folly::IPAddress neighborIP) {
  CHECK(mySid->getAdjacencyInterfaceId().has_value());
  const auto intfId = InterfaceID(*mySid->getAdjacencyInterfaceId());
  RouteNextHopSet nhops{ResolvedNextHop(neighborIP, intfId, ECMP_WEIGHT)};
  auto cloned = mySid->clone();
  cloned->setUnresolveNextHopsId(std::nullopt);
  cloned->setResolvedNextHopsId(std::nullopt);
  pendingResolves_.emplace_back(std::move(cloned), std::move(nhops));
}

void MySidNeighborObserver::queueReresolveIfMatch(
    const std::shared_ptr<MySid>& mySid,
    folly::IPAddress removedIp) {
  const auto cidr = mySid->getMySid();
  pendingUnresolveIfMatch_.emplace_back(
      folly::CIDRNetworkV6(cidr.first.asV6(), cidr.second), removedIp);
}

void MySidNeighborObserver::flushPendingResolutions() {
  if (pendingResolves_.empty() && pendingUnresolveIfMatch_.empty()) {
    return;
  }

  auto resolves = std::move(pendingResolves_);
  auto unresolves = std::move(pendingUnresolveIfMatch_);
  CHECK(pendingResolves_.empty());
  CHECK(pendingUnresolveIfMatch_.empty());

  auto* rib = sw_->getRib();
  if (!rib) {
    throw FbossError(
        "MySidNeighborObserver: RIB not available for ",
        resolves.size(),
        " resolve(s) and ",
        unresolves.size(),
        " unresolve-if-match(es)");
  }
  // Use updateAsync (not update) — stateUpdated runs on the SwSwitch update
  // thread, and the rib's sync update would block on the RIB event-base
  // thread, which itself schedules a state update back on the update thread.
  // That would deadlock.
  rib->updateAsync(
      sw_->getScopeResolver(),
      std::move(resolves),
      std::move(unresolves),
      std::vector<IpPrefix>{} /* toDelete */,
      "mySidNeighborResolve",
      createRibMySidToSwitchStateFunction(std::nullopt),
      sw_);
}

bool MySidNeighborObserver::needsResolve(
    const std::shared_ptr<MySid>& mySid) const {
  if (mySid->getType() != MySidType::ADJACENCY_MICRO_SID) {
    return false;
  }
  // For a uA MySid bound to a neighbor, both unresolveNextHopsId and
  // resolvedNextHopsId are set to the same NextHopSetID. Construction
  // sites are required to encode the next hop with intf already
  // populated (`{ResolvedNextHop(neighborIP, intf)}`) so
  // RibMySidUpdater::resolveNhop short-circuits and the content-keyed
  // NextHopIDManager returns the existing id for the resolved set. The
  // two ids' lifetimes are coupled via the RIB's releaseEntryNextHopIds
  // and toUnresolveIfMatch match-and-clear paths.
  if (mySid->getUnresolveNextHopsId().has_value()) {
    CHECK(mySid->getResolvedNextHopsId().has_value());
    CHECK_EQ(*mySid->getUnresolveNextHopsId(), *mySid->getResolvedNextHopsId());
    return false;
  }
  // Symmetric invariant: a uA MySid that has no unresolveNextHopsId
  // cannot have a resolvedNextHopsId either. A live resolved id without
  // an unresolved id would mean the RIB lost track of the original
  // binding.
  CHECK(!mySid->getResolvedNextHopsId().has_value());
  return true;
}

std::unordered_map<InterfaceID, std::vector<std::shared_ptr<MySid>>>
MySidNeighborObserver::buildIntfToUASidsMap(
    const std::shared_ptr<SwitchState>& state) const {
  std::unordered_map<InterfaceID, std::vector<std::shared_ptr<MySid>>>
      intfToUASids;
  for (const auto& [_, mySidMap] : std::as_const(*state->getMySids())) {
    for (const auto& [_key, mySid] : std::as_const(*mySidMap)) {
      if (mySid->getType() != MySidType::ADJACENCY_MICRO_SID) {
        continue;
      }
      // Every uA MySid in state must carry adjacencyInterfaceId — both
      // the config path (MySidConfigUtils::convertMySidConfig) and any
      // direct-RIB caller (tests, future RPCs) are required to set it.
      CHECK(mySid->getAdjacencyInterfaceId().has_value());
      intfToUASids[InterfaceID(*mySid->getAdjacencyInterfaceId())].push_back(
          mySid);
    }
  }
  return intfToUASids;
}

template <typename NeighborEntryT>
bool MySidNeighborObserver::isReachableNeighbor(
    const std::shared_ptr<NeighborEntryT>& entry) const {
  return entry->isReachable();
}

template <typename NeighborEntryT>
void MySidNeighborObserver::assignNeighborToMySids(
    const std::vector<std::shared_ptr<MySid>>& mySidsOnIntf,
    const std::shared_ptr<NeighborEntryT>& neighborEntry) {
  const bool neighborIsV6 = std::is_same_v<NeighborEntryT, NdpEntry>;
  for (const auto& mySid : mySidsOnIntf) {
    // Bucket already filtered to uA on the right intf. Skip MySids that
    // are already bound, then enforce the family invariant.
    if (!needsResolve(mySid)) {
      continue;
    }
    CHECK(mySid->getIsV6().has_value());
    if (*mySid->getIsV6() != neighborIsV6) {
      continue;
    }
    queueResolve(mySid, folly::IPAddress(neighborEntry->getIP()));
  }
}

template <typename NeighborEntryT>
void MySidNeighborObserver::clearNeighborFromMySids(
    const std::vector<std::shared_ptr<MySid>>& mySidsOnIntf,
    const std::shared_ptr<NeighborEntryT>& neighborEntry) {
  const bool neighborIsV6 = std::is_same_v<NeighborEntryT, NdpEntry>;
  const auto removedIp = folly::IPAddress(neighborEntry->getIP());
  for (const auto& mySid : mySidsOnIntf) {
    // Bucket already filtered to uA on the right intf. Skip unbound
    // MySids (nothing to clear), then enforce the family invariant.
    if (!mySid->getUnresolveNextHopsId().has_value()) {
      continue;
    }
    CHECK(mySid->getIsV6().has_value());
    if (*mySid->getIsV6() != neighborIsV6) {
      continue;
    }
    // Don't filter by IP on the observer side — only the rib's
    // NextHopIDManager can materialize unresolveNextHopsId. RIB does
    // the precise match-and-clear.
    queueReresolveIfMatch(mySid, removedIp);
  }
}

template <typename AddedNeighborEntryT>
void MySidNeighborObserver::processAdded(
    const std::vector<std::shared_ptr<MySid>>& mySidsOnIntf,
    const std::shared_ptr<AddedNeighborEntryT>& addedEntry) {
  if (!isReachableNeighbor(addedEntry)) {
    return;
  }
  assignNeighborToMySids(mySidsOnIntf, addedEntry);
}

template <typename RemovedNeighborEntryT>
void MySidNeighborObserver::processRemoved(
    const std::vector<std::shared_ptr<MySid>>& mySidsOnIntf,
    const std::shared_ptr<RemovedNeighborEntryT>& removedEntry) {
  if (!isReachableNeighbor(removedEntry)) {
    return;
  }
  clearNeighborFromMySids(mySidsOnIntf, removedEntry);
}

template <typename ChangedNeighborEntryT>
void MySidNeighborObserver::processChanged(
    const std::vector<std::shared_ptr<MySid>>& mySidsOnIntf,
    const std::shared_ptr<ChangedNeighborEntryT>& oldEntry,
    const std::shared_ptr<ChangedNeighborEntryT>& newEntry) {
  // IP is the key for NeighborEntries (`map<string, NeighborEntryFields>`
  // in switch_state.thrift). A "changed" delta therefore preserves the
  // IP — a different IP would surface as add+remove.
  CHECK_EQ(oldEntry->getIP(), newEntry->getIP());

  const bool isOldReachable = isReachableNeighbor(oldEntry);
  const bool isNewReachable = isReachableNeighbor(newEntry);

  if (!isOldReachable && isNewReachable) {
    // Became reachable — assign to MySids on the intf that have no
    // unresolved nhop yet.
    assignNeighborToMySids(mySidsOnIntf, newEntry);
  } else if (isOldReachable && !isNewReachable) {
    // No longer reachable — clear matching MySids (RIB checks by IP).
    clearNeighborFromMySids(mySidsOnIntf, oldEntry);
  }
  // Otherwise: reachability unchanged → metadata-only change (mac,
  // lastUpdated, etc). MySid bindings are by IP only, so no work needed.
}

// Explicit template instantiations
template void MySidNeighborObserver::processAdded<NdpEntry>(
    const std::vector<std::shared_ptr<MySid>>&,
    const std::shared_ptr<NdpEntry>&);
template void MySidNeighborObserver::processAdded<ArpEntry>(
    const std::vector<std::shared_ptr<MySid>>&,
    const std::shared_ptr<ArpEntry>&);
template void MySidNeighborObserver::processRemoved<NdpEntry>(
    const std::vector<std::shared_ptr<MySid>>&,
    const std::shared_ptr<NdpEntry>&);
template void MySidNeighborObserver::processRemoved<ArpEntry>(
    const std::vector<std::shared_ptr<MySid>>&,
    const std::shared_ptr<ArpEntry>&);
template void MySidNeighborObserver::processChanged<NdpEntry>(
    const std::vector<std::shared_ptr<MySid>>&,
    const std::shared_ptr<NdpEntry>&,
    const std::shared_ptr<NdpEntry>&);
template void MySidNeighborObserver::processChanged<ArpEntry>(
    const std::vector<std::shared_ptr<MySid>>&,
    const std::shared_ptr<ArpEntry>&,
    const std::shared_ptr<ArpEntry>&);

} // namespace facebook::fboss
