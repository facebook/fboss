// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/RemoteIntfRouteAuditor.h"

#include "fboss/agent/FibHelpers.h"
#include "fboss/agent/SwitchStats.h"
#include "fboss/agent/rib/FibUpdateHelpers.h"
#include "fboss/agent/rib/RoutingInformationBase.h"
#include "fboss/agent/state/RouteNextHopEntry.h"
#include "fboss/agent/state/SwitchState.h"

#include <folly/container/F14Map.h>
#include <folly/logging/xlog.h>

#include <chrono>
#include <utility>

namespace facebook::fboss {

namespace {

// Per-VRF accumulators: outer flat_map keyed by RouterID (few VRFs), inner
// F14FastMap keyed by prefix. F14 was chosen over flat_map / std::unordered_map
// / sorted-vector by benchmark — ~2.2x over flat_map at N>=10k prefixes, whose
// per-insert cost grows with N (~O(N^1.3)).
using FibRemoteIntfRoutes = boost::container::
    flat_map<RouterID, folly::F14FastMap<folly::CIDRNetwork, InterfaceID>>;
using ExpectedRemoteIntfRoutes = boost::container::
    flat_map<RouterID, folly::F14FastMap<folly::CIDRNetwork, IntfAddress>>;

std::pair<FibRemoteIntfRoutes, size_t> collectRemoteIntfRoutesFromFib(
    const std::shared_ptr<SwitchState>& state) {
  FibRemoteIntfRoutes result;
  size_t malformedCount = 0;
  forAllRoutes(
      state, [&result, &malformedCount](RouterID rid, const auto& route) {
        auto entry = route->getEntryForClient(ClientID::REMOTE_INTERFACE_ROUTE);
        if (!entry) {
          return;
        }
        const auto& nhSet = entry->getNextHopSet();
        if (nhSet.size() != 1) {
          ++malformedCount;
          XLOG(ERR) << "REMOTE_INTERFACE_ROUTE entry has " << nhSet.size()
                    << " nexthops; expected 1. Skipping vrf=" << rid
                    << " prefix=" << route->prefix().str();
          return;
        }
        auto intfId = nhSet.begin()->intfID();
        if (!intfId.has_value()) {
          ++malformedCount;
          XLOG(ERR) << "REMOTE_INTERFACE_ROUTE nexthop has no InterfaceID. "
                    << "Skipping vrf=" << rid
                    << " prefix=" << route->prefix().str();
          return;
        }
        result[rid].emplace(route->prefix().toCidrNetwork(), *intfId);
      });
  return {std::move(result), malformedCount};
}

std::pair<ExpectedRemoteIntfRoutes, DuplicatePrefixSet>
computeExpectedRemoteIntfRoutes(const std::shared_ptr<SwitchState>& state) {
  ExpectedRemoteIntfRoutes expected;
  DuplicatePrefixSet duplicateAcrossRifs;
  RouterIDToPrefixes discardedDeletions;
  for (const auto& [_, intfMap] :
       std::as_const(*state->getRemoteInterfaces())) {
    for (const auto& [_, rif] : std::as_const(*intfMap)) {
      IntfRouteTable perRif;
      processRemoteInterfaceRoutes(
          rif, state, /*add=*/true, perRif, discardedDeletions);
      for (auto& [vrf, routes] : perRif) {
        for (auto& [prefix, intfAddr] : routes) {
          bool inserted =
              expected[vrf].emplace(prefix, std::move(intfAddr)).second;
          if (!inserted) {
            duplicateAcrossRifs[vrf].insert(prefix);
          }
        }
      }
    }
  }
  return {std::move(expected), std::move(duplicateAcrossRifs)};
}

// Lookup in an F14FastMap-backed per-VRF table. Templated on the inner value
// type so it serves both the FIB table (value InterfaceID) and the expected
// table (value IntfAddress) via the `extract` projection.
template <typename Map, typename ExtractIntf>
bool containsRoute(
    const Map& m,
    RouterID vrf,
    const folly::CIDRNetwork& prefix,
    InterfaceID intf,
    ExtractIntf extract) {
  auto vrfIter = m.find(vrf);
  if (vrfIter == m.end()) {
    return false;
  }
  auto routeIter = vrfIter->second.find(prefix);
  return routeIter != vrfIter->second.end() &&
      extract(routeIter->second) == intf;
}

// True if `missing[vrf]` already has an entry at `prefix`. Used by the
// extra-loop IP-swap suppression to skip queuing a delete that would stomp
// the matching re-add.
bool routeInMissingSet(
    const IntfRouteTable& missing,
    RouterID vrf,
    const folly::CIDRNetwork& prefix) {
  auto vrfIter = missing.find(vrf);
  return vrfIter != missing.end() &&
      vrfIter->second.find(prefix) != vrfIter->second.end();
}

// Cap per-category ERR lines so a bulk drift after a bad warmboot cannot flood
// the log; the aggregate counts are still logged by the caller's summary line.
constexpr size_t kMaxLoggedPrefixesPerCategory = 50;

template <typename PerVrfMap, typename LogOne>
void logCappedMismatch(
    const PerVrfMap& perVrf,
    const char* category,
    const LogOne& logOne) {
  size_t logged = 0;
  size_t total = 0;
  for (const auto& [vrf, items] : perVrf) {
    for (const auto& item : items) {
      ++total;
      if (logged < kMaxLoggedPrefixesPerCategory) {
        logOne(vrf, item);
        ++logged;
      }
    }
  }
  if (total > logged) {
    XLOG(ERR) << "... and " << (total - logged) << " more " << category
              << " remote interface route(s) not logged (capped at "
              << kMaxLoggedPrefixesPerCategory << ")";
  }
}

void logMismatchRemoteIntfRoutes(const RemoteIntfRouteAudit& audit) {
  logCappedMismatch(
      audit.missing, "missing", [](RouterID vrf, const auto& route) {
        const auto& [prefix, intfAddr] = route;
        XLOG(ERR) << "Remote interface route missing: vrf=" << vrf
                  << " prefix=" << prefix.first.str() << "/"
                  << static_cast<int>(prefix.second)
                  << " intf=" << intfAddr.first;
      });
  logCappedMismatch(audit.extra, "extra", [](RouterID vrf, const auto& route) {
    const auto& [prefix, intf] = route;
    XLOG(ERR) << "Remote interface route extra: vrf=" << vrf
              << " prefix=" << prefix.first.str() << "/"
              << static_cast<int>(prefix.second) << " intf=" << intf;
  });
  logCappedMismatch(
      audit.duplicateAcrossRifs,
      "duplicate",
      [](RouterID vrf, const folly::CIDRNetwork& prefix) {
        XLOG(ERR) << "Remote interface route prefix appears on multiple RIFs:"
                  << " vrf=" << vrf << " prefix=" << prefix.first.str() << "/"
                  << static_cast<int>(prefix.second);
      });
}

} // namespace

size_t RemoteIntfRouteAudit::totalMismatchCount() const {
  size_t mismatch = 0;
  for (const auto& [_, routes] : missing) {
    mismatch += routes.size();
  }
  for (const auto& [_, prefixes] : extra) {
    mismatch += prefixes.size();
  }
  return mismatch;
}

size_t RemoteIntfRouteAudit::duplicateAcrossRifsCount() const {
  size_t count = 0;
  for (const auto& [_, prefixes] : duplicateAcrossRifs) {
    count += prefixes.size();
  }
  return count;
}

RemoteIntfRouteAudit auditRemoteInterfaceRoutes(
    const std::shared_ptr<SwitchState>& state) {
  auto [expectedFromRIF, duplicateAcrossRifs] =
      computeExpectedRemoteIntfRoutes(state);
  RemoteIntfRouteAudit audit;
  audit.duplicateAcrossRifs = std::move(duplicateAcrossRifs);
  auto [currentFromFIB, malformedCount] = collectRemoteIntfRoutesFromFib(state);
  audit.malformedRemoteIntfRoute = malformedCount;

  for (const auto& [vrf, vrfRoutes] : expectedFromRIF) {
    for (const auto& [prefix, intfAndAddr] : vrfRoutes) {
      if (!containsRoute(
              currentFromFIB,
              vrf,
              prefix,
              intfAndAddr.first,
              [](const auto& v) { return v; })) {
        audit.missing[vrf].emplace(prefix, intfAndAddr);
      }
    }
  }

  for (const auto& [vrf, vrfRoutes] : currentFromFIB) {
    for (const auto& [prefix, intf] : vrfRoutes) {
      if (containsRoute(expectedFromRIF, vrf, prefix, intf, [](const auto& v) {
            return v.first;
          })) {
        continue;
      }
      // IP-swap case: same prefix appears in both `missing` (correct intf)
      // and `extra` (stale intf). RIB processes adds before deletes, so the
      // `missing` add overwrites the stale entry in place; queuing a delete
      // here would then stomp that re-add. Relies on the missing-loop above
      // having already populated audit.missing.
      //
      // Example — two RIFs swap addresses on the same device:
      //   state: intf1 -> prefix2, intf2 -> prefix1   (post-swap)
      //   FIB:   prefix1 -> intf1, prefix2 -> intf2   (pre-swap, stale)
      // Missing loop emits audit.missing = {prefix1 -> intf2, prefix2 ->
      // intf1}; the extra loop suppresses both via routeInMissingSet, so RIB's
      // add-before-delete rewrites the stale entries in place via
      // addOrReplaceRouteImpl. Final FIB matches state; no delete fires.
      if (routeInMissingSet(audit.missing, vrf, prefix)) {
        continue;
      }
      audit.extra[vrf].emplace_back(prefix, intf);
    }
  }
  return audit;
}

std::shared_ptr<SwitchState> reconcileRemoteInterfaceRoutes(
    const std::shared_ptr<SwitchState>& state,
    RoutingInformationBase& rib,
    const SwitchIdScopeResolver& resolver,
    SwitchStats& stats) {
  // Best-effort drift recovery: a failure here must degrade gracefully rather
  // than abort agent warmboot. Report via SwitchStats and skip reconciliation.
  try {
    auto auditStart = std::chrono::steady_clock::now();
    auto audit = auditRemoteInterfaceRoutes(state);
    auto auditMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::steady_clock::now() - auditStart)
                       .count();
    XLOG(INFO) << "RemoteIntfRouteAudit completed in " << auditMs << " ms; "
               << "mismatch=" << audit.totalMismatchCount()
               << " duplicate=" << audit.duplicateAcrossRifsCount()
               << " malformed=" << audit.malformedRemoteIntfRoute;
    if (!audit.noMismatches() || !audit.duplicateAcrossRifs.empty()) {
      logMismatchRemoteIntfRoutes(audit);
    }
    if (audit.noMismatches()) {
      return state;
    }
    stats.warmbootRemoteIntfRoutesInconsistency(
        static_cast<int64_t>(audit.totalMismatchCount()));
    auto reconciled = state;
    auto reconcileStart = std::chrono::steady_clock::now();
    rib.updateRemoteInterfaceRoutes(
        &resolver,
        audit.missing,
        audit.extra,
        &ribToSwitchStateUpdate,
        static_cast<void*>(&reconciled));
    auto reconcileMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                           std::chrono::steady_clock::now() - reconcileStart)
                           .count();
    XLOG(INFO) << "Remote interface route RIB update completed in "
               << reconcileMs << " ms";
    return reconciled;
  } catch (const std::exception& ex) {
    stats.warmbootRemoteIntfRoutesReconcileError(1);
    XLOG(ERR) << "Remote interface route reconcile failed; skipping "
              << "reconciliation and continuing warmboot: " << ex.what();
    return state;
  }
}

} // namespace facebook::fboss
