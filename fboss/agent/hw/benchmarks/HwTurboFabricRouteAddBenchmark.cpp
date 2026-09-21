#include "fboss/agent/hw/benchmarks/HwRouteScaleBenchmarkHelpers.h"

#include "fboss/agent/test/RouteScaleGenerators.h"

namespace facebook::fboss {

ROUTE_ADD_BENCHMARK(
    HwTurboFabricRouteAddBenchmark,
    utility::TurboFSWRouteScaleGenerator);
} // namespace facebook::fboss
