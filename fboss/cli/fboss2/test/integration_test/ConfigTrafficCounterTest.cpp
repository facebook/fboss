// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/**
 * End-to-end test for:
 *   fboss2-dev config traffic-counter <name> PACKETS,BYTES
 *   fboss2-dev delete traffic-counter <name>
 *
 * Programs a named traffic counter, verifies it round-trips through the
 * agent's running config, deletes it, and verifies the running config
 * returns to its original set. Both operations are hitless (no coldboot),
 * so no agent-restart wait is needed between steps. Unit tests cover the
 * in-memory mutation and the referenced-counter delete refusal; this test
 * asserts the value survives a real hitless commit through the agent.
 */

#include <folly/json/dynamic.h>
#include <folly/logging/xlog.h>
#include <gtest/gtest.h>
#include <algorithm>
#include <optional>
#include <string>
#include <vector>
#include "fboss/cli/fboss2/test/integration_test/Fboss2IntegrationTest.h"

using namespace facebook::fboss;

namespace {
// CounterType enum values from switch_config.thrift
constexpr int kPackets = 0;
constexpr int kBytes = 1;
// A name unlikely to collide with any production counter.
const std::string kTestCounter = "fboss2-it-traffic-counter";
} // namespace

class ConfigTrafficCounterTest : public Fboss2IntegrationTest {
 protected:
  // Returns the counter's (sorted) type list if present in the running config.
  std::optional<std::vector<int>> getCounterTypes(
      const std::string& name) const {
    auto config = getRunningConfig();
    const auto& sw = config["sw"];
    if (!sw.count("trafficCounters")) {
      return std::nullopt;
    }
    for (const auto& counter : sw["trafficCounters"]) {
      if (counter["name"].asString() == name) {
        std::vector<int> types;
        if (counter.count("types")) {
          for (const auto& t : counter["types"]) {
            types.push_back(t.asInt());
          }
        }
        std::sort(types.begin(), types.end());
        return types;
      }
    }
    return std::nullopt;
  }

  size_t counterCount() const {
    auto config = getRunningConfig();
    const auto& sw = config["sw"];
    return sw.count("trafficCounters") ? sw["trafficCounters"].size() : 0;
  }

  // An earlier run that died between create and delete would leave the test
  // counter behind, and every later run would then exercise the update path
  // instead of the create path. Reset the config rather than refuse to run.
  // Nothing else may reference kTestCounter - a traffic policy pointing at a
  // counter this deletes would make the next commit invalid.
  void SetUp() override {
    Fboss2IntegrationTest::SetUp();
    if (!getCounterTypes(kTestCounter).has_value()) {
      return;
    }
    XLOG(INFO) << "Leftover counter '" << kTestCounter << "'; deleting it";
    auto del = runCli({"delete", "traffic-counter", kTestCounter});
    ASSERT_EQ(del.exitCode, 0) << "cleanup delete failed: " << del.stderr;
    commitConfig();
    ASSERT_FALSE(getCounterTypes(kTestCounter).has_value())
        << "cleanup delete left '" << kTestCounter << "' in the running config";
  }
};

TEST_F(ConfigTrafficCounterTest, CreateThenDelete) {
  XLOG(INFO) << "[Step 1] Reading baseline traffic counters...";
  size_t baseline = counterCount();

  XLOG(INFO) << "[Step 2] Creating counter '" << kTestCounter
             << "' (PACKETS,BYTES)...";
  auto create =
      runCli({"config", "traffic-counter", kTestCounter, "PACKETS,BYTES"});
  ASSERT_EQ(create.exitCode, 0)
      << "config traffic-counter failed: " << create.stderr;
  commitConfig();

  auto types = getCounterTypes(kTestCounter);
  ASSERT_TRUE(types.has_value()) << "counter not found in running config";
  EXPECT_EQ(*types, (std::vector<int>{kPackets, kBytes}));
  EXPECT_EQ(counterCount(), baseline + 1);

  XLOG(INFO) << "[Step 3] Deleting counter '" << kTestCounter << "'...";
  auto del = runCli({"delete", "traffic-counter", kTestCounter});
  ASSERT_EQ(del.exitCode, 0) << "delete traffic-counter failed: " << del.stderr;
  commitConfig();

  EXPECT_FALSE(getCounterTypes(kTestCounter).has_value())
      << "counter still present after delete";
  EXPECT_EQ(counterCount(), baseline);

  XLOG(INFO) << "TEST PASSED";
}
