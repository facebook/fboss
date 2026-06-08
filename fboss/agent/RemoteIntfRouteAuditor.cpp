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
  RemoteIntfRouteAudit audit;
  // Walk the FIB to collect REMOTE_INTERFACE_ROUTE entries and the count of
  // malformed entries; expected-route computation and mismatch detection
  // follow in stack.
  auto [current, malformedCount] = collectRemoteIntfRoutesFromFib(state);
  audit.malformedRemoteIntfRoute = malformedCount;
  (void)current;
  return audit;
}

} // namespace facebook::fboss
