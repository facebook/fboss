// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/benchmarks/AgentBenchmarksMain.h"

#include "fboss/agent/benchmarks/AgentBenchmarks.h"
#include "fboss/agent/platforms/sai/SaiPlatformInit.h"

namespace facebook::fboss {

void initBenchmarks() {
  std::optional<facebook::fboss::cfg::StreamType> streamType = std::nullopt;
#if defined(TAJO_SDK_VERSION_1_65_0)
  streamType = facebook::fboss::cfg::StreamType::UNICAST;
#endif
  benchmarksMain(initSaiPlatform, streamType);
}

} // namespace facebook::fboss
