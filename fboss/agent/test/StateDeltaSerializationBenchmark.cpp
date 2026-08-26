// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <folly/Benchmark.h>
#include <folly/init/Init.h>

#include "fboss/agent/FsdbHelper.h"
#include "fboss/agent/test/HwTestHandle.h"
#include "fboss/agent/test/RouteScaleGenerators.h"
#include "fboss/agent/test/TestUtils.h"
#include "fboss/fsdb/common/Utils.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"
#include "fboss/thrift_cow/nodes/Serializer.h"

// Raw thrift_cow::serialize cost per (route scale x protocol).

namespace facebook::fboss {

namespace {
template <typename RouteGeneratorT>
struct BenchmarkHelperForRouteScale {
  std::shared_ptr<SwitchState> getState() {
    auto cfg = testConfigA();
    auto handle = createTestHandle(&cfg);

    RouteGeneratorT generator(
        handle->getSw()->getState(), handle->getSw()->needL2EntryForNeighbor());
    handle->getSw()->updateStateBlocking(
        "update 1", [&](const std::shared_ptr<SwitchState>& state) {
          return generator.resolveNextHops(state);
        });
    auto rid = RouterID(0);
    auto client = ClientID::BGPD;
    auto routeChunks = generator.getThriftRoutes();
    auto updater = handle->getSw()->getRouteUpdater();
    for (const auto& routeChunk : routeChunks) {
      std::for_each(
          routeChunk.begin(),
          routeChunk.end(),
          [&updater, client, rid](const auto& route) {
            updater.addRoute(rid, client, route);
          });
      updater.program();
    }
    return handle->getSw()->getState();
  }
};

fsdb::OperDelta buildFullOperDelta(const std::shared_ptr<SwitchState>& state) {
  return fsdb::computeOperDelta(
      std::make_shared<SwitchState>(),
      state,
      fsdbAgentDataSwitchStateRootPath(),
      true);
}
} // namespace

#define DEFINE_SERIALIZE_BENCHMARK(SCALE, PROTO_NAME, PROTO_ENUM)         \
  BENCHMARK(Serialize_##SCALE##_##PROTO_NAME, numIters) {                 \
    BenchmarkHelperForRouteScale<utility::SCALE##Generator> helper;       \
    fsdb::OperDelta operDelta;                                            \
    BENCHMARK_SUSPEND {                                                   \
      operDelta = buildFullOperDelta(helper.getState());                  \
    }                                                                     \
    using TC = apache::thrift::type_class::structure;                     \
    for (size_t n = 0; n < numIters; ++n) {                               \
      auto serialized = thrift_cow::serialize<TC>(PROTO_ENUM, operDelta); \
      folly::doNotOptimizeAway(serialized);                               \
    }                                                                     \
  }

#define DEFINE_ALL_PROTOCOLS(SCALE)                                       \
  DEFINE_SERIALIZE_BENCHMARK(SCALE, BINARY, fsdb::OperProtocol::BINARY)   \
  DEFINE_SERIALIZE_BENCHMARK(SCALE, COMPACT, fsdb::OperProtocol::COMPACT) \
  DEFINE_SERIALIZE_BENCHMARK(                                             \
      SCALE, SIMPLE_JSON, fsdb::OperProtocol::SIMPLE_JSON)

DEFINE_ALL_PROTOCOLS(RSWRouteScale)
DEFINE_ALL_PROTOCOLS(FSWRouteScale)
DEFINE_ALL_PROTOCOLS(THAlpmRouteScale)
DEFINE_ALL_PROTOCOLS(HgridDuRouteScale)
DEFINE_ALL_PROTOCOLS(HgridUuRouteScale)
DEFINE_ALL_PROTOCOLS(AnticipatedRouteScale)

} // namespace facebook::fboss

int main(int argc, char** argv) {
  folly::Init init(&argc, &argv);
  folly::runBenchmarks();
  return 0;
}
