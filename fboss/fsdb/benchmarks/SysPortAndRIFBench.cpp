// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <folly/Benchmark.h>
#include <folly/logging/xlog.h>
#include <gflags/gflags.h>

#include <folly/synchronization/Latch.h>
#include "fboss/fsdb/benchmarks/FsdbBenchmarkTestHelper.h"
#include "fboss/fsdb/benchmarks/StateGenerator.h"

DEFINE_int32(n_switchIDs, 1, "number of NPUs in a device");
// 43 sys ports per device. So to achieve 30k scale, we need ~700 devices
// https://fburl.com/gdoc/o8oq59ab
DEFINE_int32(n_cluster_size, 700, "number of Devices in a cluster");
DEFINE_int32(n_system_ports, 43, "number of System Ports per device");

namespace facebook::fboss::fsdb::test {

BENCHMARK(FsdbPublishSysPortsAndRIFs) {
  folly::BenchmarkSuspender suspender;

  // setup FsdbTestServer
  FsdbBenchmarkTestHelper helper;
  helper.setup();

  // start publisher and publish initial empty stats
  helper.startPublisher(false /* stats*/);

  auto state = std::make_shared<state::SwitchState>();
  helper.publishStatePatch(*state, 1);

  StateGenerator::fillSwitchState(
      state.get(), FLAGS_n_switchIDs, FLAGS_n_system_ports);

  helper.waitForPublisherConnected();

  // benchmark time taken for publisher to be able to publish 1 state update
  suspender.dismiss();

  helper.publishStatePatch(*state, 2);

  // wait for server to process all the updates
  while (true) {
    auto md = helper.getPublisherRootMetadata(false);
    if (md->operMetadata.lastConfirmedAt().value() == 2) {
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  suspender.rehire();

  helper.TearDown();
}
} // namespace facebook::fboss::fsdb::test
