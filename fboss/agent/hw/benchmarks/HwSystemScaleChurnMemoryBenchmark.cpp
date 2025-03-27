#include "fboss/agent/test/AgentEnsemble.h"
#include "fboss/agent/test/utils/SystemScaleTestUtils.h"

#include <folly/Benchmark.h>

namespace facebook::fboss {

BENCHMARK(SystemScaleChurnMemoryBenchmark) {
  auto ensemble = createAgentEnsemble(
      utility::getSystemScaleTestSwitchConfiguration,
      false /*disableLinkStateToggler*/);
  utility::initSystemScaleChurnTest(ensemble.get());
}
} // namespace facebook::fboss
