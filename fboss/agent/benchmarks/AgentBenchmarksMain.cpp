// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <folly/Benchmark.h>
#include <folly/init/Init.h>
#include <folly/json/dynamic.h>
#include <folly/json/json.h>
#include <folly/logging/Init.h>
#include <gtest/gtest.h>

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

inline bool listBenchmarks(int* argc, char** argv) {
  for (int i = 0; i < *argc; i++) {
    if (strcmp(argv[i], "--bm_list") == 0) {
      return true;
    }
  }
  return false;
}

void setCmdLineFlagOverrides() {
  FLAGS_tun_intf = false;
  FLAGS_dsf_subscribe = false;
  FLAGS_hide_fabric_ports = false;
  FLAGS_detect_wrong_fabric_connections = false;
}

int main(int argc, char* argv[]) {
  FLAGS_bm_max_iters = 2;
  struct rusage startUsage, endUsage;
  getrusage(RUSAGE_SELF, &startUsage);
  if (!listBenchmarks(&argc, argv)) {
    testing::InitGoogleTest(&argc, argv);
    facebook::fboss::fbossCommonInit(argc, argv);
    facebook::fboss::initBenchmarks();
    setCmdLineFlagOverrides();
  } else {
    folly::Init init(&argc, &argv, false);
  }
  folly::runBenchmarks();

  getrusage(RUSAGE_SELF, &endUsage);
  auto cpuTime =
      (timevalToUsec(endUsage.ru_stime) - timevalToUsec(startUsage.ru_stime)) +
      (timevalToUsec(endUsage.ru_utime) - timevalToUsec(startUsage.ru_utime));

  folly::dynamic rusageJson = folly::dynamic::object;
  rusageJson["cpu_time_usec"] = cpuTime;
  rusageJson["max_rss"] = endUsage.ru_maxrss;
  std::cout << toPrettyJson(rusageJson) << std::endl;

  return 0;
}
