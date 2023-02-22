// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/benchmarks/AgentBenchmarks.h"

#include "fboss/agent/test/AgentEnsemble.h"

#include <folly/Benchmark.h>

namespace facebook::fboss {

void benchmarksMain(int argc, char* argv[], PlatformInitFn initPlatform) {
  ensembleMain(argc, argv, initPlatform);
}

} // namespace facebook::fboss
