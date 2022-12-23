/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include <folly/Benchmark.h>
#include <folly/Random.h>
#include "fboss/agent/state/RouteNextHopEntry.h"

using namespace facebook::fboss;
using folly::IPAddress;

namespace {
const AdminDistance kDefaultAdminDistance = AdminDistance::EBGP;
static constexpr int kNumSSWs = 36;
static constexpr int kRSWEcmpWidth = 64;
static constexpr int kFSWEcmpWidth = 128;
static constexpr int kRSWNumPaths = 8;
static constexpr int kFSWNumPaths = 36;
static constexpr int kRSWNumRoutes = 10000;
static constexpr int kFSWNumRoutes = 30000;
} // namespace

void RouteNextHopEntryScaleOptimized(
    bool optimized,
    int ecmpWidth,
    int numPaths,
    int numRoutes) {
  std::vector<RouteNextHopEntry> rNhops;
  FLAGS_optimized_ucmp = optimized;
  FLAGS_ecmp_width = ecmpWidth;
  BENCHMARK_SUSPEND {
    // limiting for ease of generating nh address
    CHECK(10 + numPaths < 100);
    for (auto routeIndex = 0; routeIndex < numRoutes; ++routeIndex) {
      RouteNextHopEntry::NextHopSet nhops;
      for (auto pathIndex = 10; pathIndex < 10 + numPaths; ++pathIndex) {
        std::string nhAddrStr =
            fmt::format("2401:db{}:e112:9103:1028::01", pathIndex);
        nhops.emplace(ResolvedNextHop(
            folly::IPAddress(nhAddrStr),
            InterfaceID(pathIndex),
            folly::Random::rand32() % kNumSSWs));
      }
      auto nh = RouteNextHopEntry(nhops, kDefaultAdminDistance);
      rNhops.emplace_back(std::move(nh));
    }
  }

  for (auto& nh : rNhops) {
    auto normalizedNhops = nh.normalizedNextHops();
  }
}

void RouteNextHopEntryScaleOptimizedRSW(uint32_t /* iters */, bool optimized) {
  RouteNextHopEntryScaleOptimized(
      optimized, kRSWEcmpWidth, kRSWNumPaths, kRSWNumRoutes);
}

void RouteNextHopEntryScaleOptimizedFSW(uint32_t /* iters */, bool optimized) {
  RouteNextHopEntryScaleOptimized(
      optimized, kFSWEcmpWidth, kFSWNumPaths, kFSWNumRoutes);
}

BENCHMARK_PARAM(RouteNextHopEntryScaleOptimizedRSW, true);
BENCHMARK_PARAM(RouteNextHopEntryScaleOptimizedRSW, false);
BENCHMARK_PARAM(RouteNextHopEntryScaleOptimizedFSW, true);
BENCHMARK_PARAM(RouteNextHopEntryScaleOptimizedFSW, false);

int main(int argc, char** argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  folly::runBenchmarks();
  return 0;
}
