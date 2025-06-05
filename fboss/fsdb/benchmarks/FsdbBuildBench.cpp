// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <folly/Benchmark.h>
#include <folly/Format.h>
#include <folly/Subprocess.h>
#include <folly/logging/xlog.h>
#include <gflags/gflags.h>

#include <chrono>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

DEFINE_int32(build_iterations, 5, "Number of times to run each build");
DEFINE_string(
    target,
    "//fboss/fsdb/server:fsdb",
    "Specific target to benchmark. If empty, runs the fsdb binary benchmark");

namespace facebook::fboss::fsdb::test {

/**
 * Helper function to format milliseconds as minutes and seconds.
 */
std::string formatTimeMinSec(int64_t milliseconds) {
  int64_t totalSeconds = milliseconds / 1000;
  int64_t minutes = totalSeconds / 60;
  int64_t seconds = totalSeconds % 60;
  int64_t remainingMs = milliseconds % 1000;

  std::stringstream ss;
  ss << minutes << "m " << seconds << "s " << remainingMs << "ms";
  return ss.str();
}

/**
 * Helper function to run a buck2 build command and measure its execution time.
 * Returns the execution time in milliseconds.
 */
int64_t runBuildCommand(const std::string& target) {
  auto startTime = std::chrono::steady_clock::now();

  std::string command = folly::sformat(
      "buck2 clean && buck2 build --no-remote-cache @//mode/opt {}", target);

  XLOG(INFO) << "Running command: " << command;

  folly::Subprocess process({"/bin/sh", "-c", command});
  auto exitCode = process.wait().exitStatus();

  auto endTime = std::chrono::steady_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
      endTime - startTime);

  int64_t durationMs = duration.count();

  if (exitCode != 0) {
    XLOG(ERR) << "Build failed with exit code: " << exitCode;
    return -1;
  }

  std::cout << "Build completed in: " << formatTimeMinSec(durationMs) << " ("
            << durationMs << "ms)" << std::endl;

  return durationMs;
}

/**
 * Helper function to run a build benchmark for a specific target.
 * Runs the build multiple times and returns the average execution time.
 */
int64_t runBuildBenchmark(const std::string& target) {
  XLOG(INFO) << "Running build benchmark for target: " << target;

  std::vector<int64_t> buildTimes;
  int64_t totalTime = 0;
  int successfulBuilds = 0;

  for (int i = 0; i < FLAGS_build_iterations; i++) {
    XLOG(INFO) << "Build iteration " << (i + 1) << " of "
               << FLAGS_build_iterations;

    auto buildTime = runBuildCommand(target);
    if (buildTime >= 0) {
      buildTimes.push_back(buildTime);
      totalTime += buildTime;
      successfulBuilds++;
    }
  }

  if (successfulBuilds == 0) {
    XLOG(ERR) << "All builds failed for target: " << target;
    return -1;
  }

  int64_t averageTime = totalTime / successfulBuilds;

  std::cout << "\nBuild benchmark results for target: " << target << std::endl;
  std::cout << "  Number of successful builds: " << successfulBuilds
            << std::endl;
  std::cout << "  Average build time: " << formatTimeMinSec(averageTime) << " ("
            << averageTime << "ms)" << std::endl;

  for (int i = 0; i < buildTimes.size(); i++) {
    std::cout << "  Build " << (i + 1)
              << " time: " << formatTimeMinSec(buildTimes[i]) << " ("
              << buildTimes[i] << "ms)" << std::endl;
  }
  std::cout << std::endl;

  return averageTime;
}

// Generic benchmark function for any target
BENCHMARK(FsdbBuildCustomTarget) {
  folly::BenchmarkSuspender suspender;

  runBuildBenchmark(FLAGS_target);
  return;
}
} // namespace facebook::fboss::fsdb::test
