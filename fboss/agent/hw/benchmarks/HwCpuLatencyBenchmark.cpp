// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/benchmarks/HwCpuLatencyBenchmarkHelper.h"

namespace facebook::fboss {

// Single packet test - baseline latency measurement
CPU_LATENCY_BENCHMARK(cpuLatency1Batch1Packet, 1, 1)

// Multiple iterations of single packet - consistency measurement
CPU_LATENCY_BENCHMARK(cpuLatency10Batches1Packet, 10, 1)

} // namespace facebook::fboss
