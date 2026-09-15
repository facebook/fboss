// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <folly/Benchmark.h>
#include <folly/Conv.h>
#include <folly/ScopeGuard.h>
#include <folly/Synchronized.h>
#include <folly/synchronization/Baton.h>

#include <glog/logging.h>
#include <thrift/lib/cpp2/op/Get.h>

#include <atomic>
#include <chrono>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/fsdb/benchmarks/FsdbBenchmarkTestHelper.h"
#include "fboss/fsdb/client/instantiations/FsdbCowStateSubManager.h"
#include "fboss/fsdb/oper/ExtendedPathBuilder.h"
#include "fboss/lib/thrift_service_client/ConnectionOptions.h"

namespace facebook::fboss::fsdb::test {

namespace {
// 1 prefix subscribed before the measured section + 100 added during it.
constexpr int kNumPrefixes = 101;
constexpr auto kSwitchIdList = "0";
constexpr int16_t kVrf = 0;
// ECMP nexthops per route, so each fibV6 entry has a realistic serialized size
// rather than an empty RouteFields.
constexpr int kNumEcmpNextHops = 4;
// Low state serve interval so the measured time reflects the add-paths +
// initial-sync cost rather than the server's serve-cycle latency floor (the
// helper default is 50ms, which otherwise dominates this measurement).
constexpr uint32_t kStateServeIntervalMs = 5;
// Generous upper bounds so a stalled subscription fails fast with a diagnostic
// rather than hanging the benchmark indefinitely.
constexpr auto kFirstSyncTimeout = std::chrono::seconds(30);
constexpr auto kAllReceivedTimeout = std::chrono::seconds(60);

// Distinct, deterministic fibV6 map keys: unique per index and identical
// between the published data and the subscribed path.
std::string makeV6Prefix(int idx) {
  return folly::to<std::string>("2401:db00:", idx, "::/64");
}

// Build the extended (id-token) path to a single fibV6 prefix entry:
//   agent/switchState/fibsInfoMap/<switchIdList>/fibsMap/<vrf>/fibV6/<prefix>
ExtendedOperPath makeFibV6PrefixPath(const std::string& prefix) {
  return ext_path_builder::raw(
             apache::thrift::op::get_field_id_v<
                 FsdbOperStateRoot,
                 apache::thrift::ident::agent>)
      .raw(
          apache::thrift::op::
              get_field_id_v<AgentData, apache::thrift::ident::switchState>)
      .raw(
          apache::thrift::op::get_field_id_v<
              state::SwitchState,
              apache::thrift::ident::fibsInfoMap>)
      .raw(kSwitchIdList)
      .raw(
          apache::thrift::op::get_field_id_v<
              state::FibInfoFields,
              apache::thrift::ident::fibsMap>)
      .raw(folly::to<std::string>(kVrf))
      .raw(
          apache::thrift::op::get_field_id_v<
              state::FibContainerFields,
              apache::thrift::ident::fibV6>)
      .raw(prefix)
      .get();
}

// A representative IPv6 route: a /64 prefix with kNumEcmpNextHops ECMP nexthops
// for realistic per-route serialization cost. Address bytes vary by index so
// routes are not byte-identical.
state::RouteFields makeRouteFields(int idx) {
  state::RouteFields route;
  route.prefix()->v6() = true;
  route.prefix()->mask() = 64;
  std::string prefixAddr(16, '\0');
  prefixAddr[0] = 0x24; // 2401:db00::/64-ish, last two bytes encode the index
  prefixAddr[1] = 0x01;
  prefixAddr[14] = static_cast<char>((idx >> 8) & 0xff);
  prefixAddr[15] = static_cast<char>(idx & 0xff);
  route.prefix()->prefix()->addr() = std::move(prefixAddr);

  auto& fwd = *route.fwd();
  fwd.action() = RouteForwardAction::NEXTHOPS;
  for (int nh = 0; nh < kNumEcmpNextHops; ++nh) {
    NextHopThrift nexthop;
    std::string nhAddr(16, '\0');
    nhAddr[0] = static_cast<char>(0xfe); // fe80::/64 link-local-ish nexthops
    nhAddr[1] = static_cast<char>(0x80);
    nhAddr[15] = static_cast<char>(nh + 1);
    nexthop.address()->addr() = std::move(nhAddr);
    nexthop.weight() = 1;
    fwd.nexthops()->push_back(std::move(nexthop));
  }
  return route;
}

state::SwitchState makeSwitchStateWithV6Prefixes(
    const std::vector<std::string>& prefixes) {
  state::FibContainerFields fibContainer;
  fibContainer.vrf() = kVrf;
  for (int i = 0; i < static_cast<int>(prefixes.size()); ++i) {
    fibContainer.fibV6()[prefixes[i]] = makeRouteFields(i);
  }
  state::FibInfoFields fibInfo;
  fibInfo.fibsMap()[kVrf] = std::move(fibContainer);
  state::SwitchState switchState;
  switchState.fibsInfoMap()[kSwitchIdList] = std::move(fibInfo);
  return switchState;
}
} // namespace

// Measures the FsdbSubManager-layer cost to extend an established extended
// patch subscription with 100 additional fibV6 prefix paths and receive a
// full-state initial sync for all 101 prefixes via its data callback. Setup
// (not measured): publish fibsInfoMap with 101 prefixes, subscribe to the
// first. Measured: add the remaining 100 paths and wait until the callback has
// received all 101.
BENCHMARK(FsdbSubManagerAddExtendedPathsToLiveSubscription) {
  folly::BenchmarkSuspender suspender;

  FsdbBenchmarkTestHelper helper;
  // Guarantee TearDown() runs on every exit path (including the timeout
  // exceptions thrown below), not just the normal-completion path.
  SCOPE_EXIT {
    helper.TearDown();
  };
  helper.setup(
      /*numSubscriptions=*/1,
      /*startFsdbTestServer=*/true,
      /*serviceFileName=*/std::nullopt,
      kStateServeIntervalMs);
  helper.startPublisher(false /* state */);
  helper.waitForPublisherConnected();

  std::vector<std::string> prefixes;
  prefixes.reserve(kNumPrefixes);
  for (int i = 0; i < kNumPrefixes; ++i) {
    prefixes.push_back(makeV6Prefix(i));
  }
  // Pre-build the extended paths so the measured section only exercises the
  // add-to-live-subscription cost, not client-side path construction.
  std::vector<ExtendedOperPath> paths;
  paths.reserve(kNumPrefixes);
  for (const auto& prefix : prefixes) {
    paths.push_back(makeFibV6PrefixPath(prefix));
  }
  // Publish the initial agent/switchState carrying all 101 prefixes so they are
  // available for initial sync as each path is subscribed.
  helper.publishStatePatch(
      makeSwitchStateWithV6Prefixes(prefixes), 1 /* stamp */);

  // Each subscribed prefix path carries a distinct SubscriptionKey; count the
  // distinct keys served to the data callback. 101 distinct keys => all
  // prefixes have been delivered.
  //
  // Declared before subMgr so they outlive it: the data callback captures them
  // by reference and subMgr's dtor joins its callback thread(s). Reverse
  // destruction order tears down subMgr (and its callback) first, avoiding UAF.
  folly::Synchronized<std::set<SubscriptionKey>> seenKeys;
  folly::Baton<> firstSynced;
  folly::Baton<> allReceived;
  std::atomic<bool> firstPosted{false};
  std::atomic<bool> allPosted{false};

  // FsdbSubManager-layer subscriber, subscribing initially to just the first
  // prefix's path.
  auto subMgr = std::make_unique<FsdbCowStateSubManager>(
      SubscriptionOptions("fsdb_add_paths_bench", false /* subscribeStats */),
      utils::ConnectionOptions("::1", helper.testServer().getFsdbPort()));

  CHECK(!paths.empty());
  subMgr->addExtendedPath(paths[0]);
  subMgr->subscribe([&](const auto& update) {
    size_t numSeen = 0;
    {
      auto keys = seenKeys.wlock();
      for (auto key : update.updatedKeys) {
        keys->insert(key);
      }
      numSeen = keys->size();
    }
    if (numSeen >= 1 && !firstPosted.exchange(true)) {
      firstSynced.post();
    }
    if (numSeen >= kNumPrefixes && !allPosted.exchange(true)) {
      allReceived.post();
    }
  });

  // Wait for the first prefix's initial sync: this confirms the subscription is
  // connected and the server uid has been captured, so the live-extend RPCs
  // below take effect in place rather than deferring to a reconnect.
  if (!firstSynced.try_wait_for(kFirstSyncTimeout)) {
    throw std::runtime_error("Timed out waiting for first prefix initial sync");
  }
  // Only the first prefix should have been delivered so far; otherwise the
  // measured section below would observe fewer than the intended 100 adds.
  // Only one path is subscribed at this point, so the set can only ever
  // contain that single key; GE guards against a spurious extra callback
  // invocation racing with this check.
  CHECK_GE(seenKeys.rlock()->size(), 1u);

  // ---- Measured section ----
  suspender.dismiss();
  for (int i = 1; i < kNumPrefixes; ++i) {
    subMgr->addExtendedPathToLiveSubscription(paths[i]);
  }
  // Wait until the data callback has received all 101 prefix entries.
  if (!allReceived.try_wait_for(kAllReceivedTimeout)) {
    suspender.rehire();
    throw std::runtime_error(
        folly::to<std::string>(
            "Timed out waiting for all prefixes; received ",
            seenKeys.rlock()->size(),
            "/",
            kNumPrefixes,
            " keys"));
  }
  suspender.rehire();
}

} // namespace facebook::fboss::fsdb::test
