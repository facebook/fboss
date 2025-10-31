// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/thrift_cow/storage/tests/CowStorageBenchHelper.h"

namespace facebook::fboss::thrift_cow::test {

int64_t timevalToUsec(const timeval& tv) {
  const int64_t kUsecPerSecond = 1000000;
  return (int64_t(tv.tv_sec) * kUsecPerSecond) + tv.tv_usec;
}

void printBenchmarkResults(const struct rusage& startUsage) {
  struct rusage endUsage{};

  getrusage(RUSAGE_SELF, &endUsage);

  auto cpuTime =
      (timevalToUsec(endUsage.ru_stime) - timevalToUsec(startUsage.ru_stime)) +
      (timevalToUsec(endUsage.ru_utime) - timevalToUsec(startUsage.ru_utime));

  folly::dynamic rusageJson = folly::dynamic::object;
  rusageJson["cpu_time_usec"] = cpuTime;
  std::cout << toPrettyJson(rusageJson) << std::endl;
}
} // namespace facebook::fboss::thrift_cow::test
