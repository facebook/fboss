// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <folly/memory/MallctlHelper.h>
#include <gtest/gtest.h>

#include "fboss/agent/test/AgentHwTest.h"

using namespace ::testing;

DEFINE_int32(
    memory_test_stats_iterations,
    1000,
    "Number of iterations for the test");

namespace facebook::fboss {

constexpr int kWarmupIterations = 60;
constexpr size_t kMemoryHeadroom = 10 << 20; // 10MB

class AgentMemoryUsageTest : public AgentHwTest {
 public:
  std::vector<production_features::ProductionFeature>
  getProductionFeaturesVerified() const override {
    return {};
  }

 protected:
  void setCmdLineFlagOverrides() const override {
    AgentHwTest::setCmdLineFlagOverrides();
    // Disable stats update to improve performance
    FLAGS_enable_stats_update_thread = false;
    // enable running on fab switches as well
    FLAGS_hide_fabric_ports = false;
  }

  void setupJeMalloc() {
    // Disable caching to make stats more accurate.
    folly::mallctlWrite("thread.tcache.enabled", false);
    folly::mallctlWrite("arenas.dirty_decay_ms", 100); // milliseconds
  }

  size_t getMemoryUsage() {
    // Update the statistics cached by mallctl.
    uint64_t epoch = 1;
    folly::mallctlWrite("epoch", epoch);

    size_t usage = 0;
    folly::mallctlRead("stats.allocated", &usage);
    return usage;
  }
};

TEST_F(AgentMemoryUsageTest, MeasureStatsCollection) {
  if (!folly::usingJEMalloc()) {
    // Don't use GTEST_SKIP() because it messes up our stats.
    XLOG(INFO) << "Skipping test because jemalloc is not enabled";
    return;
  }

  size_t baseline = 0;

  // The first ~60 updateStats() calls perform some allocation, so measure the
  // baseline memory usage after that.
  for (int i = 0; i < kWarmupIterations; ++i) {
    getAgentEnsemble()->updateStats();

    size_t current = getMemoryUsage();
    if (current > baseline) {
      baseline = current;
    }
    XLOG(DBG2) << "baseline stats.allocated[" << i << "] = " << baseline;
  }

  // Make sure stats collection don't grow memory. We still allow for some
  // headroom due to memory usage from other threads, vector resizing, etc.
  for (int i = 0; i < FLAGS_memory_test_stats_iterations; i++) {
    getAgentEnsemble()->updateStats();

    size_t current = getMemoryUsage();
    XLOG(DBG2) << "stats.allocated[" << i << "] = " << current;
    EXPECT_LE(current, baseline + kMemoryHeadroom);
  }
}

} // namespace facebook::fboss
