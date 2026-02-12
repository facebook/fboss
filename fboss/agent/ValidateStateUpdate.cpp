// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/ValidateStateUpdate.h"

#include "fboss/agent/state/StateDelta.h"

namespace facebook::fboss {
bool isStateUpdateValid(const StateDelta& delta) {
  return true;
}
} // namespace facebook::fboss
