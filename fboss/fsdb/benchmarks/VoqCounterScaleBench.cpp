// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <folly/Benchmark.h>
#include <folly/logging/xlog.h>
#include <gflags/gflags.h>

#include "fboss/fsdb/benchmarks/FsdbBenchmarkTestHelper.h"
#include "fboss/fsdb/benchmarks/StateGenerator.h"

DEFINE_int32(n_sysports, 16000, "number of SysPorts");
DEFINE_int32(n_voqs_per_sysport, 4, "number of VOQs per SysPort");
DEFINE_int32(n_publish_iters, 1000, "number of iterations for stats publish");

namespace facebook::fboss::fsdb::test {

BENCHMARK(FsdbPublishVoqStats) {
  auto n_iterations = FLAGS_n_publish_iters;

  folly::BenchmarkSuspender suspender;

  // setup FsdbTestServer
  FsdbBenchmarkTestHelper helper;
  helper.setup();

  // start publisher and publish initial empty stats
  helper.startPublisher();

  auto stats = std::make_shared<AgentStats>();
  helper.publishPath(*stats, 0);

  StateGenerator::fillVoqStats(
      stats.get(), FLAGS_n_sysports, FLAGS_n_voqs_per_sysport);

  helper.waitForPublisherConnected();

  // benchmark test: publish N iterations of stats
  suspender.dismiss();

  for (int round = 0; round < n_iterations; round++) {
    StateGenerator::updateVoqStats(stats.get());
    helper.publishPath(*stats, round + 1);
  }

  // wait for server to process all the updates
  while (true) {
    auto md = helper.getPublisherRootMetadata(true);
    if (*md->operMetadata.get_lastConfirmedAt() == n_iterations) {
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  suspender.rehire();

  helper.TearDown();
}

} // namespace facebook::fboss::fsdb::test
