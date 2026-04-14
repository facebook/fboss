// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/if/gen-cpp2/common_types.h"
#include "fboss/agent/rib/RoutingInformationBase.h"

#include <optional>

namespace facebook::fboss {

RibMySidToSwitchStateFunction createRibMySidToSwitchStateFunction(
    const std::optional<StateDeltaApplication>& deltaApplicationBehavior);

} // namespace facebook::fboss
