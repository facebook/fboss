#include "fboss/agent/hw/benchmarks/HwRouteScaleBenchmarkHelpers.h"

#include "fboss/agent/test/RouteScaleGenerators.h"

namespace facebook::fboss {

ROUTE_DEL_BENCHMARK(
    HwTurboFabricRouteDelBenchmark,
    utility::TurboFSWRouteScaleGenerator);
} // namespace facebook::fboss
