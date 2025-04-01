#include "fboss/agent/test/AgentEnsemble.h"
#include "fboss/agent/test/utils/AgentFsdbIntegrationBenchmarkHelper.h"
#include "fboss/agent/test/utils/SystemScaleTestUtils.h"

#include <folly/Benchmark.h>

namespace facebook::fboss {

BENCHMARK(SystemScaleMemoryBenchmark) {
  auto helper = AgentFsdbIntegrationBenchmarkHelper();

  auto ensemble = createAgentEnsemble(
      utility::getSystemScaleTestSwitchConfiguration,
      false /*disableLinkStateToggler*/);

  // benchmark test: max scale agent state
  utility::initSystemScaleTest(ensemble.get());

  helper.awaitCompletion(ensemble.get());
}

} // namespace facebook::fboss
