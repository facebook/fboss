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

  // Flush all batched resolutions
  flushPendingResolutions();
}

void MySidNeighborObserver::handleMySidAddedOrChanged(
    const std::shared_ptr<SwitchState>& state,
    const std::shared_ptr<MySid>& newEntry) {
  // A MySid needs resolution iff it has an adjacency interface AND no
  // unresolved next-hop id yet. WB replay (preserved unresolveNextHopsId),
  // config-churn churn the rib path already cleared, and observer round-
  // trips that don't change the resolution state all fall out of this
  // single check. Neighbor-down clearing is handled separately in the
  // neighbor-removed path.
  if (!newEntry->getAdjacencyInterfaceId().has_value() ||
      newEntry->getUnresolveNextHopsId().has_value()) {
    return;
  }

  auto intfId = InterfaceID(newEntry->getAdjacencyInterfaceId().value());
  bool isV6 = newEntry->getIsV6().value_or(true);

  auto intf = state->getInterfaces()->getNodeIf(intfId);
  if (!intf) {
    // No matching interface — leave unresolved; will be picked up when
    // a neighbor comes up on the right intf.
    return;
  }

  // Scan the neighbor table for the first reachable non-link-local entry.
  auto findReachableNeighbor =
      [](const auto& table) -> std::optional<folly::IPAddress> {
    if (!table) {
      return std::nullopt;
    }
    for (const auto& [__, entry] : std::as_const(*table)) {
      if (entry->isReachable() && !entry->getIP().isLinkLocal()) {
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
  RouteNextHopSet nhops{UnresolvedNextHop(neighborIP, ECMP_WEIGHT)};
  auto cloned = mySid->clone();
  cloned->setUnresolveNextHopsId(std::nullopt);
  cloned->setResolvedNextHopsId(std::nullopt);
  pendingResolves_.emplace_back(std::move(cloned), std::move(nhops));
}

void MySidNeighborObserver::queueUnresolveIfMatch(
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

// Stub implementations for neighbor callbacks. The real impls and the
// neighbor-delta dispatch in stateUpdated() land in the next diff.
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
