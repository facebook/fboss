#include "fboss/agent/test/AgentEnsemble.h"
#include "fboss/agent/test/utils/AgentFsdbIntegrationBenchmarkHelper.h"
#include "fboss/agent/test/utils/SystemScaleTestUtils.h"

#include <folly/Benchmark.h>

namespace facebook::fboss {

BENCHMARK(SystemScaleChurnMemoryBenchmark) {
  auto helper = AgentFsdbIntegrationBenchmarkHelper();

  auto ensemble = createAgentEnsemble(
      utility::getSystemScaleTestSwitchConfiguration,
      false /*disableLinkStateToggler*/);

  // benchmark test
  utility::initSystemScaleChurnTest(ensemble.get());

  helper.awaitCompletion(ensemble.get());
}
} // namespace facebook::fboss
