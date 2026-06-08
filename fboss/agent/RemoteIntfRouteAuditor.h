// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/VoqUtils.h"

#include <boost/container/flat_set.hpp>
#include <memory>

namespace facebook::fboss {

class SwitchState;

using DuplicatePrefixSet = boost::container::flat_map<
    facebook::fboss::RouterID,
    boost::container::flat_set<folly::CIDRNetwork>>;

struct RemoteIntfRouteAudit {
  IntfRouteTable missing;
  RouterIDToPrefixes extra;
  // FIB entries we could not interpret (multi-nexthop, missing intfID). Bumped
  // for observability; not included in missing/extra.
  size_t malformedRemoteIntfRoute{0};
  // Prefixes that appeared on more than one remote RIF in state. Surfaces a
  // class of state corruption that `auditRemoteInterfaceRoutes` would
  // otherwise hide via the cancel-out semantics of
  // `processRemoteInterfaceRoutes`. Deduped per prefix — a prefix on N RIFs
  // appears once, not N-1 times.
  DuplicatePrefixSet duplicateAcrossRifs;

  // True when the audit found nothing to reconcile via RIB (no missing/extra
  // mismatches). `malformedRemoteIntfRoute` and `duplicateAcrossRifs` are
  // intentionally not considered here — those are observability signals that
  // surface state corruption, not drift the reconciler should act on.
  bool noMismatches() const {
    return missing.empty() && extra.empty();
  }

  size_t totalMismatchCount() const;
  size_t duplicateAcrossRifsCount() const;
};

RemoteIntfRouteAudit auditRemoteInterfaceRoutes(
    const std::shared_ptr<SwitchState>& state);

} // namespace facebook::fboss
