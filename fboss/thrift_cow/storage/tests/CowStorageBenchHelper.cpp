// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/thrift_cow/storage/tests/CowStorageBenchHelper.h"

#include <folly/String.h>

DEFINE_int32(
    bm_memory_iters,
    10,
    "Number of memory measurement iterations per benchmark");

DEFINE_string(
    bm_fsdb_path,
    "",
    "FSDB path to measure memory for a specific subtree (e.g. /agent/switchState/fibsMap)");
namespace facebook::fboss::thrift_cow::test {

std::vector<std::string> parseFsdbPath(const std::string& path) {
  if (path.empty()) {
    return {};
  }

  std::vector<std::string> tokens;
  // NOLINTNEXTLINE(facebook-folly-split-usage-check): strings need to outlive
  // function scope as they're stored in pathFilter_
  folly::split('/', path, tokens);

  // Remove empty tokens (from leading/trailing slashes)
  tokens.erase(
      std::remove_if(
          tokens.begin(),
          tokens.end(),
          [](const std::string& s) { return s.empty(); }),
      tokens.end());

  return tokens;
}

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
