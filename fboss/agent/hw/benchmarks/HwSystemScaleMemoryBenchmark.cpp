#include "fboss/agent/test/AgentEnsemble.h"
#include "fboss/agent/test/RouteScaleGenerators.h"
#include "fboss/agent/test/utils/AgentFsdbIntegrationBenchmarkHelper.h"
#include "fboss/agent/test/utils/SystemScaleTestUtils.h"

#include <folly/Benchmark.h>

namespace facebook::fboss {

BENCHMARK(SystemScaleMemoryBenchmark) {
  auto helper = AgentFsdbIntegrationBenchmarkHelper();

  auto ensemble = createAgentEnsemble(
      utility::getSystemScaleTestSwitchConfiguration,
      false /*disableLinkStateToggler*/);

  auto generator = utility::ScaleTestRouteScaleGenerator(
      ensemble->getSw()->getState(),
      ensemble->getSw()->needL2EntryForNeighbor());

  // benchmark test: max scale agent state
  utility::initSystemScaleTest(ensemble.get(), generator);

  helper.awaitCompletion(ensemble.get());
}

} // namespace facebook::fboss
