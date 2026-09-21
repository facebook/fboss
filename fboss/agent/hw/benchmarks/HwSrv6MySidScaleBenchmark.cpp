// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <folly/Benchmark.h>
#include <folly/logging/xlog.h>
#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/test/AgentEnsemble.h"
#include "fboss/agent/test/EcmpSetupHelper.h"
#include "fboss/agent/test/utils/ConfigUtils.h"
#include "fboss/agent/test/utils/Srv6TestUtils.h"

namespace facebook::fboss {

namespace {

AgentEnsembleSwitchConfigFn makeMySidConfigFn() {
  return [](const AgentEnsemble& ensemble) {
    auto cfg = utility::onePortPerInterfaceConfig(
        ensemble.getSw(),
        ensemble.masterLogicalPortIds(),
        true /*interfaceHasSubnet*/);
    return cfg;
  };
}

// Benchmark: programs numEntries ADJACENCY_MICRO_SID MySID entries in a single
// batched rib->update and measures the total time to program them to the ASIC.
void srv6MidpointMySidScaleBenchmark(int numEntries) {
  folly::BenchmarkSuspender suspender;
  // For scale benchmarking, do not cap MySID usage at the default 75%.
  // This aligns with other scale benchmarks that set resource percentages to
  // 100.
  FLAGS_enable_mysid_resource_protection = true;
  FLAGS_mysid_resource_percentage = 100;

  auto ensemble = createAgentEnsemble(
      makeMySidConfigFn(), false /*disableLinkStateToggler*/);

  // Resolve next hops for ADJACENCY_MICRO_SID entries
  utility::EcmpSetupAnyNPorts6 ecmpHelper(
      ensemble->getSw()->getState(),
      ensemble->getSw()->needL2EntryForNeighbor());
  auto numNhops =
      std::min(static_cast<int>(ecmpHelper.getNextHops().size()), 64);
  CHECK_GT(numNhops, 0);

  ensemble->applyNewState([&](const std::shared_ptr<SwitchState>& in) {
    return ecmpHelper.resolveNextHops(in, numNhops);
  });

  auto sw = ensemble->getSw();
  XLOG(INFO) << "HwSrv6MidpointMySidScaleBenchmark targetEntries="
             << numEntries;

  // Build all MySID entries up front (untimed). sidOffset 1 ->
  // 3001:db8:<i+1>::.
  auto toAdd = utility::makeAdjacencyMySidEntries(
      ecmpHelper, numNhops, numEntries, 1 /*sidOffset*/);
  auto programmedEntries = static_cast<int>(toAdd.size());

  // Timed: program all MySID entries in a single batched rib->update.
  suspender.dismiss();
  utility::programMySidEntries(sw, std::move(toAdd));
  suspender.rehire();
  XLOG(INFO) << "HwSrv6MidpointMySidScaleBenchmark programmedEntries="
             << programmedEntries;

  // Cleanup: delete all MySID entries (untimed).
  utility::deleteScaleMySidEntries(sw, programmedEntries, 1 /*sidOffset*/);
  XLOG(INFO) << "HwSrv6MidpointMySidScaleBenchmark cleanupEntries="
             << programmedEntries;
}

} // namespace

// Current validated capacity is 2042 MySID entries in SAI tests.
// Midpoint (ADJACENCY_MICRO_SID): each entry has a next hop for uSID shift
// and forwarding to the adjacency.
BENCHMARK(HwSrv6MidpointMySidScaleBenchmark) {
  srv6MidpointMySidScaleBenchmark(2042);
}

} // namespace facebook::fboss
