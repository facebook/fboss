// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/VoqUtils.h"

namespace facebook::fboss {

struct RemoteIntfRouteAudit {
  IntfRouteTable missing;
  RouterIDToPrefixes extra;
  // FIB entries we could not interpret (multi-nexthop, missing intfID). Bumped
  // for observability; not included in missing/extra.
  size_t malformedRemoteIntfRoute{0};
  // Prefixes that appeared on more than one remote RIF in state. Surfaces a
  // class of state corruption that `auditRemoteInterfaceRoutes` would
  // otherwise hide via the cancel-out semantics of
  // `processRemoteInterfaceRoutes`.
  RouterIDToPrefixes duplicateAcrossRifs;

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

} // namespace facebook::fboss
