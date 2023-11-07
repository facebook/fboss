// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/test/MonoAgentEnsemble.h"

DECLARE_bool(json);

namespace facebook::fboss {

int benchmarksMain(
    PlatformInitFn initPlatform,
    std::optional<facebook::fboss::cfg::StreamType> streamType = std::nullopt);

} // namespace facebook::fboss
