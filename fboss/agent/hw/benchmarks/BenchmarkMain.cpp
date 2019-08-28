/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "common/time/TimeUtil.h"

#include <folly/Benchmark.h>
#include <folly/dynamic.h>
#include <folly/init/Init.h>
#include <folly/json.h>

#include <sys/resource.h>
#include <sys/time.h>

#include <iostream>

using facebook::timevalToUsec;

int main(int argc, char* argv[]) {
  folly::init(&argc, &argv, true);
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
