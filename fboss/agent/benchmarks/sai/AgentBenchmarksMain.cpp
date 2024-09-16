// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/benchmarks/AgentBenchmarksMain.h"

#include "fboss/agent/benchmarks/AgentBenchmarks.h"
#include "fboss/agent/platforms/sai/SaiPlatformInit.h"

namespace facebook::fboss {

void initBenchmarks() {
  std::optional<facebook::fboss::cfg::StreamType> streamType = std::nullopt;
#if defined(TAJO_SDK_GTE_24_4_90)
  streamType = facebook::fboss::cfg::StreamType::UNICAST;
#endif
  benchmarksMain(initSaiPlatform, streamType);
}

} // namespace facebook::fboss
