// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/benchmarks/AgentBenchmarks.h"

#include <gtest/gtest.h>

namespace facebook::fboss {

int benchmarksMain(
    PlatformInitFn initPlatform,
    std::optional<facebook::fboss::cfg::StreamType> streamType) {
  initEnsemble(initPlatform, streamType);
  return RUN_ALL_TESTS();
}

} // namespace facebook::fboss
