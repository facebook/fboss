// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/rib/ClassBasedPolicyUpdater.h"

#include "fboss/agent/state/SwitchState.h"

namespace facebook::fboss {

ClassBasedPolicyUpdater::ClassBasedPolicyUpdater(
    const NextHopIDManager* nextHopIDManager)
    : nextHopIDManager_(nextHopIDManager) {}

std::shared_ptr<SwitchState> ClassBasedPolicyUpdater::operator()(
    const std::shared_ptr<SwitchState>& state) {
  return state;
}

} // namespace facebook::fboss
