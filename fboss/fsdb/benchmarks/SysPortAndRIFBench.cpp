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
DEFINE_int32(n_sysports_to_add, 10, "number of System Ports to add per update");
DEFINE_int32(
    n_neighbor_tables,
    10,
    "number of Neighbor Tables to update per device");

namespace facebook::fboss::fsdb::test {

using switchStateUpdateFn = std::function<void(state::SwitchState*, int32_t)>;

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

BENCHMARK(FsdbSubscribeSysPortsAndRIFs) {
  const int numPeerDevices =
      FLAGS_n_cluster_size - 1; // publish to cluster size - 1 devices
  folly::Latch updateReceived(numPeerDevices);

  folly::BenchmarkSuspender suspender;

  // setup FsdbTestServer
  FsdbBenchmarkTestHelper helper;
  helper.setup(numPeerDevices);

  // start publisher and publish initial empty stats
  helper.startPublisher(false /* stats*/);

  auto state = std::make_shared<state::SwitchState>();
  helper.publishStatePatch(*state, 1);
  helper.waitForPublisherConnected();

  StateGenerator::fillSwitchState(
      state.get(), FLAGS_n_switchIDs, FLAGS_n_system_ports);

  std::vector<std::thread> threads;
  // Spawn numPeerDevices subscribers
  for (int i = 0; i < numPeerDevices; i++) {
    threads.emplace_back([&, i] {
      fsdb::FsdbPatchSubscriber::FsdbOperPatchUpdateCb subscriptionCb =
          [&]([[maybe_unused]] SubscriberChunk&& patch) {
            updateReceived.count_down(); // Decrement latch counter};
          };
      helper.addStatePatchSubscription(subscriptionCb, i);
    });
  }

  // benchmark test: publish state and wait for N subscribers to receive it
  suspender.dismiss();

  helper.publishStatePatch(*state, 2);

  updateReceived.wait(); // Wait until latch counter is zero
  suspender.rehire();

  for (auto& thread : threads) {
    thread.join();
  }
  for (int i = 0; i < numPeerDevices; i++) {
    helper.removeSubscription(false, i);
  }
  helper.TearDown();
}

void subscriberUpdateHelper(switchStateUpdateFn updateFn, int32_t numUpdates) {
  const int numPeerDevices =
      FLAGS_n_cluster_size - 1; // publish to cluster size - 1 devices
  folly::Latch subscriptionComplete(numPeerDevices);
  folly::Latch updateReceived(numPeerDevices);

  folly::BenchmarkSuspender suspender;

  // setup FsdbTestServer
  FsdbBenchmarkTestHelper helper;
  helper.setup(numPeerDevices);

  // start publisher and publish initial empty stats
  helper.startPublisher(false /* stats*/);

  auto state = std::make_shared<state::SwitchState>();
  helper.publishStatePatch(*state, 1);
  helper.waitForPublisherConnected();

  std::vector<std::thread> threads;
  // Spawn numPeerDevices subscribers
  for (int i = 0; i < numPeerDevices; i++) {
    threads.emplace_back([&, i] {
      fsdb::FsdbPatchSubscriber::FsdbOperPatchUpdateCb subscriptionCb =
          [&](SubscriberChunk&& patch) {
            for (const auto& [key, patches] : *patch.patchGroups()) {
              auto timestampVal =
                  patches[0].metadata()->lastConfirmedAt().value();
              if (timestampVal == 3) {
                updateReceived.count_down(); // Decrement update latch counter
              } else if (timestampVal == 2) {
                subscriptionComplete.count_down(); // Decrement subscription
                                                   // latch counter
              }
            }
          };
      helper.addStatePatchSubscription(subscriptionCb, i);
    });
  }

  StateGenerator::fillSwitchState(
      state.get(), FLAGS_n_switchIDs, FLAGS_n_system_ports);
  helper.publishStatePatch(*state, 2);
  // wait for all subscribers to be connected and received initial sync
  subscriptionComplete.wait(); // Wait until subscription latch counter is zero

  //  Add new sys ports to the switch state
  updateFn(state.get(), numUpdates);

  // benchmark test: update state and wait for N subscribers to receive it
  suspender.dismiss();
  helper.publishStatePatch(*state, 3);

  updateReceived.wait(); // Wait until latch counter is zero
  suspender.rehire();

  for (auto& thread : threads) {
    thread.join();
  }
  for (int i = 0; i < numPeerDevices; i++) {
    helper.removeSubscription(false, i);
  }
  helper.TearDown();
}

BENCHMARK(FsdbUpdateSysPortsAndRIFs) {
  auto sysPortUpdateFn = [](state::SwitchState* state, int32_t numUpdates) {
    StateGenerator::updateSysPorts(state, numUpdates);
  };
  subscriberUpdateHelper(sysPortUpdateFn, FLAGS_n_sysports_to_add);
}

BENCHMARK(FsdbUpdateNeighborTable) {
  auto neighborTableUpdateFn = [](state::SwitchState* state,
                                  int32_t numUpdates) {
    StateGenerator::updateNeighborTables(state, numUpdates);
  };
  subscriberUpdateHelper(neighborTableUpdateFn, FLAGS_n_neighbor_tables);
}
} // namespace facebook::fboss::fsdb::test
