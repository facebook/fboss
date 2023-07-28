// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/state/SwitchState.h"
#include "fboss/fsdb/common/Utils.h"

namespace facebook::fboss {

// This template is expensive and compiled here to parallelize the build
template fsdb::OperDelta fsdb::computeOperDelta<SwitchState>(
    const std::shared_ptr<SwitchState>&,
    const std::shared_ptr<SwitchState>&,
    const std::vector<std::string>&,
    bool);

} // namespace facebook::fboss
