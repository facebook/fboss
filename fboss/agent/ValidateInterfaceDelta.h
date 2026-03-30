// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <folly/MacAddress.h>

#include "fboss/agent/state/InterfaceMapDelta.h"
#include "fboss/agent/types.h"
#include "fboss/lib/UniqueRefMap.h"

namespace facebook::fboss {
class Interface;
class StateDelta;

// Validates interface deltas to ensure the single-MAC-per-interface
// invariant is maintained.
class IntfDeltaValidator {
 public:
  IntfDeltaValidator() = default;

  bool isValidDelta(const StateDelta& delta);
  void updateRejected(const StateDelta& delta);

 private:
  bool isValidIntfDelta(const InterfaceDelta& intfDelta);
  bool processNeighborDelete(const InterfaceDelta& intfDelta);
  bool processNeighborChange(const InterfaceDelta& intfDelta);
  bool processNeighborAdd(const InterfaceDelta& intfDelta);

  UniqueRefMap<InterfaceID, folly::MacAddress> intfToSingleNbrMac_;
};

} // namespace facebook::fboss
