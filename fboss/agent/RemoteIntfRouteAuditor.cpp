// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/RemoteIntfRouteAuditor.h"

namespace facebook::fboss {

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

} // namespace facebook::fboss
