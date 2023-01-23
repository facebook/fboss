// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/benchmarks/AgentBenchmarks.h"

#include "fboss/agent/test/AgentEnsemble.h"

namespace {
int kArgc;
char** kArgv;
facebook::fboss::PlatformInitFn kPlatformInitFn;
} // namespace

namespace facebook::fboss {

void benchmarksMain(int argc, char* argv[], PlatformInitFn initPlatform) {
  kArgc = argc;
  kArgv = argv;
  kPlatformInitFn = std::move(initPlatform);
}

std::unique_ptr<facebook::fboss::AgentEnsemble> createAgentEnsemble(
    AgentEnsembleConfigFn initialConfigFn,
    uint32_t featuresDesired) {
  auto ensemble = std::make_unique<AgentEnsemble>();
  ensemble->setupEnsemble(
      kArgc, kArgv, featuresDesired, kPlatformInitFn, initialConfigFn);
  return ensemble;
}

} // namespace facebook::fboss
