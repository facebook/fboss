// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/benchmarks/AgentBenchmarks.h"

#include <vector>

namespace facebook::fboss {

struct CpuLatencyBenchmarkSetup {
  std::unique_ptr<AgentEnsemble> ensemble;
  std::vector<PortID> ports;
};

// Configure all interface ports with neighbor reachability and NDP entries
// so CpuLatencyManager considers them eligible for probing.
CpuLatencyBenchmarkSetup createCpuLatencyEnsemble();

// Runs the full CPU latency benchmark: sets up ensemble, waits for
// CpuLatencyManager to produce measurements on all ports, then reports
// results.
void runCpuLatencyBenchmark();

} // namespace facebook::fboss
