// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/test/MonoAgentEnsemble.h"

DECLARE_bool(json);

namespace facebook::fboss {

void benchmarksMain(int argc, char* args[], PlatformInitFn initPlatform);

} // namespace facebook::fboss
