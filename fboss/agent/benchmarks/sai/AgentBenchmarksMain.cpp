// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/benchmarks/AgentBenchmarksMain.h"

#include "fboss/agent/benchmarks/AgentBenchmarks.h"
#include "fboss/agent/platforms/sai/SaiPlatformInit.h"

namespace facebook::fboss {

void initBenchmarks() {
  benchmarksMain(initSaiPlatform);
}

} // namespace facebook::fboss
