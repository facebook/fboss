// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/benchmarks/AgentBenchmarks.h"

#include <gtest/gtest.h>

namespace facebook::fboss {

int benchmarksMain(PlatformInitFn initPlatform) {
  initEnsemble(initPlatform);
  return RUN_ALL_TESTS();
}

} // namespace facebook::fboss
