// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <folly/Benchmark.h>
#include <folly/logging/xlog.h>
#include <gflags/gflags.h>

#include <folly/synchronization/Baton.h>
#include "fboss/fsdb/benchmarks/FsdbBenchmarkTestHelper.h"
#include "fboss/fsdb/benchmarks/StateGenerator.h"

DEFINE_int32(n_sysports, 16000, "number of SysPorts");
DEFINE_int32(n_voqs_per_sysport, 4, "number of VOQs per SysPort");
DEFINE_int32(n_publish_iters, 1000, "number of iterations for stats publish");
DEFINE_int32(
    n_pubsub_iters,
    100,
    "number of iterations for stats publish and serve");

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

BENCHMARK(FsdbSubscribeVoqStats) {
  auto n_iterations = FLAGS_n_pubsub_iters;

  folly::BenchmarkSuspender suspender;

  // setup FsdbTestServer
  FsdbBenchmarkTestHelper helper;
  helper.setup();

  // start publisher and publish initial empty stats
  helper.startPublisher();

  auto stats = std::make_shared<AgentStats>();
  helper.publishPath(*stats, 0);
  helper.waitForPublisherConnected();

  // setup subscription and signal when updates are received
  folly::Baton<> updateReceived;

  fsdb::FsdbStateSubscriber::FsdbOperStateUpdateCb subscriptionCb =
      [&](fsdb::OperState&& /*operState*/) { updateReceived.post(); };
  helper.addSubscription(subscriptionCb);

  // complete initial sync of full stats before starting benchmark loop
  StateGenerator::fillVoqStats(
      stats.get(), FLAGS_n_sysports, FLAGS_n_voqs_per_sysport);
  uint64_t lastPublishedStamp = 0;
  helper.publishPath(*stats, ++lastPublishedStamp);

  updateReceived.wait();
  updateReceived.reset();

  // start benchmark for incremental publish and subscription serve
  suspender.dismiss();

  for (int round = 0; round < n_iterations; round++) {
    StateGenerator::updateVoqStats(stats.get());
    helper.publishPath(*stats, ++lastPublishedStamp);
    updateReceived.wait();
    updateReceived.reset();
  }

  suspender.rehire();

  helper.removeSubscription();
  helper.TearDown();
}

} // namespace facebook::fboss::fsdb::test
