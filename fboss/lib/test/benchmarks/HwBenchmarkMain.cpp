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
#include <folly/dynamic.h>
#include <folly/init/Init.h>
#include <folly/json.h>
#include <folly/logging/Init.h>

#include <sys/resource.h>
#include <sys/time.h>

#include <iostream>

const int64_t kUsecPerSecond = 1000000;

FOLLY_INIT_LOGGING_CONFIG("fboss=DBG2; default:async=true");
DECLARE_int64(bm_max_iters);

DEFINE_bool(
    setup_for_warmboot,
    false,
    "Set to true will prepare the device for warmboot");

inline int64_t timevalToUsec(const timeval& tv) {
  return (int64_t(tv.tv_sec) * kUsecPerSecond) + tv.tv_usec;
}

int main(int argc, char* argv[]) {
  folly::init(&argc, &argv, true);
  // For HW benchmarks its not always possible to teardown and re setup the HW
  // without exiting the process. So as benchmark framework to only a single
  // iteration (2^1, since folly benhmark counts in powers of 2)of benchmark
  // run.
  FLAGS_bm_max_iters = 2;
  struct rusage startUsage, endUsage;
  getrusage(RUSAGE_SELF, &startUsage);
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
