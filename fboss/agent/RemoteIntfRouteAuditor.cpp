// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/RemoteIntfRouteAuditor.h"

#include "fboss/agent/FibHelpers.h"
#include "fboss/agent/state/RouteNextHopEntry.h"
#include "fboss/agent/state/SwitchState.h"

#include <folly/logging/xlog.h>

namespace facebook::fboss {

namespace {

using FibRemoteIntfRoutes = boost::container::flat_map<
    RouterID,
    boost::container::flat_map<folly::CIDRNetwork, InterfaceID>>;

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

std::pair<IntfRouteTable, DuplicatePrefixSet> computeExpectedRemoteIntfRoutes(
    const std::shared_ptr<SwitchState>& state) {
  IntfRouteTable expected;
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

  return audit;
}

} // namespace facebook::fboss
