// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <folly/Benchmark.h>
#include <folly/dynamic.h>
#include <folly/init/Init.h>
#include <folly/json.h>
#include <folly/logging/Init.h>

#include <sys/resource.h>
#include <sys/time.h>

#include <iostream>

#include "fboss/agent/benchmarks/AgentBenchmarksMain.h"

#include "fboss/agent/benchmarks/AgentBenchmarks.h"

const int64_t kUsecPerSecond = 1000000;

DECLARE_int64(bm_max_iters);

inline int64_t timevalToUsec(const timeval& tv) {
  return (int64_t(tv.tv_sec) * kUsecPerSecond) + tv.tv_usec;
}

int main(int argc, char* argv[]) {
  FLAGS_bm_max_iters = 2;
  struct rusage startUsage, endUsage;
  getrusage(RUSAGE_SELF, &startUsage);

  facebook::fboss::initBenchmarks(argc, argv);
  folly::runBenchmarks();

  getrusage(RUSAGE_SELF, &endUsage);
  auto cpuTime =
      (timevalToUsec(endUsage.ru_stime) - timevalToUsec(startUsage.ru_stime)) +
      (timevalToUsec(endUsage.ru_utime) - timevalToUsec(endUsage.ru_utime));

  folly::dynamic rusageJson = folly::dynamic::object;
  rusageJson["cpu_time_usec"] = cpuTime;
  rusageJson["max_rss"] = endUsage.ru_maxrss;
  std::cout << toPrettyJson(rusageJson) << std::endl;

  return 0;
}
