// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/**
 * End-to-end tests for `fboss2-dev config copp ...` commands.
 *
 * Covers the two families that mutate control-plane policing state:
 *
 *   - cpu-queue <id> [name <string> | rate-limit kbps <max> | rate-limit pps
 *     <max>] — hitless edits to entries in sw.cpuQueues[].
 *   - reason <reason-name> queue <id> — hitless upsert into
 *     sw.cpuTrafficPolicy.rxReasonToQueueOrderedList[].
 *
 * For each attribute the test:
 *   1. Reads the current state from the agent's running config
 *   2. Applies the CLI change
 *   3. Commits the session (HITLESS — no agent restart)
 *   4. Verifies the running config reflects the new value
 *   5. Restores the original value (to leave the DUT unchanged)
 *
 * Requirements:
 *   - FBOSS agent is running with a valid configuration that already contains
 *     cpuQueues and cpuTrafficPolicy entries (true on every platform that
 *     supports the CPU port, which is every fboss DUT we ship).
 */

#include <folly/json/dynamic.h>
#include <folly/json/json.h>
#include <folly/logging/xlog.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <optional>
#include <stdexcept>
#include <string>

#include "fboss/agent/if/gen-cpp2/FbossCtrlAsyncClient.h"
#include "fboss/cli/fboss2/test/integration_test/Fboss2IntegrationTest.h"
#include "fboss/cli/fboss2/utils/CmdClientUtilsCommon.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

using namespace facebook::fboss;
using ::testing::HasSubstr;

class ConfigCoppTest : public Fboss2IntegrationTest {
 protected:
  folly::dynamic getRunningConfig() const {
    HostInfo hostInfo("localhost");
    auto client =
        utils::createClient<apache::thrift::Client<FbossCtrl>>(hostInfo);
    std::string configStr;
    client->sync_getRunningConfig(configStr);
    return folly::parseJson(configStr);
  }

  // Find the cpuQueues entry with the given id, or return a null dynamic.
  folly::dynamic findCpuQueue(int id) const {
    auto config = getRunningConfig();
    const auto& sw = config["sw"];
    if (!sw.count("cpuQueues") || !sw["cpuQueues"].isArray()) {
      throw std::runtime_error("Running config missing 'sw.cpuQueues' array");
    }
    for (const auto& q : sw["cpuQueues"]) {
      if (q.count("id") && q["id"].asInt() == id) {
        return q;
      }
    }
    return {nullptr};
  }

  // Return the first cpu queue id that exists in the running config.
  int getFirstExistingQueueId() const {
    auto config = getRunningConfig();
    const auto& queues = config["sw"]["cpuQueues"];
    if (!queues.isArray() || queues.empty()) {
      throw std::runtime_error("Running config has no cpuQueues entries");
    }
    return queues[0]["id"].asInt();
  }

  // Look up the rxReason mapping for a numeric reason id in the running
  // config. Returns std::nullopt if not present.
  std::optional<int> findReasonQueueId(int rxReason) const {
    auto config = getRunningConfig();
    const auto& sw = config["sw"];
    if (!sw.count("cpuTrafficPolicy")) {
      return std::nullopt;
    }
    const auto& policy = sw["cpuTrafficPolicy"];
    if (!policy.count("rxReasonToQueueOrderedList")) {
      return std::nullopt;
    }
    for (const auto& entry : policy["rxReasonToQueueOrderedList"]) {
      if (entry["rxReason"].asInt() == rxReason) {
        return static_cast<int>(entry["queueId"].asInt());
      }
    }
    return std::nullopt;
  }
};

// cpu-queue <id>: when the id already exists in the running config, the
// command must be a no-op. We exercise the code path and verify the queue is
// still present (and unchanged) after commit.
TEST_F(ConfigCoppTest, CpuQueueEnsureExistsIsNoOp) {
  int id = getFirstExistingQueueId();
  XLOG(INFO) << "cpu-queue " << id << " (ensure-exists no-op)";

  auto before = findCpuQueue(id);
  ASSERT_FALSE(before.isNull());
  std::string originalName = before.count("name") && before["name"].isString()
      ? before["name"].asString()
      : "";

  auto result = runCli({"config", "copp", "cpu-queue", std::to_string(id)});
  ASSERT_EQ(result.exitCode, 0)
      << "stdout=" << result.stdout << " stderr=" << result.stderr;
  EXPECT_THAT(result.stdout, HasSubstr(std::to_string(id)));
  commitConfig();

  auto after = findCpuQueue(id);
  ASSERT_FALSE(after.isNull());
  std::string afterName = after.count("name") && after["name"].isString()
      ? after["name"].asString()
      : "";
  EXPECT_EQ(afterName, originalName);
}

TEST_F(ConfigCoppTest, CpuQueueSetName) {
  int id = getFirstExistingQueueId();
  auto before = findCpuQueue(id);
  ASSERT_FALSE(before.isNull());
  std::string originalName = before.count("name") && before["name"].isString()
      ? before["name"].asString()
      : "";
  const std::string newName = "cpuQueue-test-nos6185";
  ASSERT_NE(originalName, newName);

  XLOG(INFO) << "cpu-queue " << id << " name " << newName;
  auto result = runCli(
      {"config", "copp", "cpu-queue", std::to_string(id), "name", newName});
  ASSERT_EQ(result.exitCode, 0)
      << "stdout=" << result.stdout << " stderr=" << result.stderr;
  EXPECT_THAT(result.stdout, HasSubstr(newName));
  commitConfig();

  auto after = findCpuQueue(id);
  ASSERT_FALSE(after.isNull());
  ASSERT_TRUE(after["name"].isString());
  EXPECT_EQ(after["name"].asString(), newName);

  XLOG(INFO) << "Restoring name to '" << originalName << "'";
  if (!originalName.empty()) {
    result = runCli(
        {"config",
         "copp",
         "cpu-queue",
         std::to_string(id),
         "name",
         originalName});
    ASSERT_EQ(result.exitCode, 0) << result.stderr;
    commitConfig();
    EXPECT_EQ(findCpuQueue(id)["name"].asString(), originalName);
  }
}

TEST_F(ConfigCoppTest, CpuQueueSetRateLimitPps) {
  // Low-priority queues (id 0 and 1) are typically pps-rate-limited in
  // production configs — find one that has an existing pps cap to minimize
  // disruption. Fall back to id 0 if none is found.
  int targetId = 0;
  std::optional<int> originalPpsMax;
  auto config = getRunningConfig();
  for (const auto& q : config["sw"]["cpuQueues"]) {
    if (q.count("portQueueRate") && q["portQueueRate"].count("pktsPerSec") &&
        q["portQueueRate"]["pktsPerSec"].count("maximum")) {
      targetId = q["id"].asInt();
      originalPpsMax = q["portQueueRate"]["pktsPerSec"]["maximum"].asInt();
      break;
    }
  }
  XLOG(INFO) << "Using queue id=" << targetId
             << ", originalPpsMax=" << originalPpsMax.value_or(-1);

  const int newPps = 1234;
  XLOG(INFO) << "cpu-queue " << targetId << " rate-limit pps " << newPps;
  auto result = runCli(
      {"config",
       "copp",
       "cpu-queue",
       std::to_string(targetId),
       "rate-limit",
       "pps",
       std::to_string(newPps)});
  ASSERT_EQ(result.exitCode, 0)
      << "stdout=" << result.stdout << " stderr=" << result.stderr;
  commitConfig();

  auto after = findCpuQueue(targetId);
  ASSERT_FALSE(after.isNull());
  ASSERT_TRUE(after.count("portQueueRate"));
  ASSERT_TRUE(after["portQueueRate"].count("pktsPerSec"));
  EXPECT_EQ(after["portQueueRate"]["pktsPerSec"]["maximum"].asInt(), newPps);

  if (originalPpsMax.has_value()) {
    XLOG(INFO) << "Restoring pps max to " << *originalPpsMax;
    result = runCli(
        {"config",
         "copp",
         "cpu-queue",
         std::to_string(targetId),
         "rate-limit",
         "pps",
         std::to_string(*originalPpsMax)});
    ASSERT_EQ(result.exitCode, 0) << result.stderr;
    commitConfig();
    EXPECT_EQ(
        findCpuQueue(targetId)["portQueueRate"]["pktsPerSec"]["maximum"]
            .asInt(),
        *originalPpsMax);
  }
}

// Reason -> queue mapping: update an existing entry. We pick ARP because
// every production CoPP policy maps ARP, and we can read its current queue
// id to compute a distinct new value and then restore.
TEST_F(ConfigCoppTest, ReasonToQueueUpdate) {
  constexpr int kArpReason = 1; // cfg::PacketRxReason::ARP

  auto originalQueueId = findReasonQueueId(kArpReason);
  ASSERT_TRUE(originalQueueId.has_value())
      << "Running config has no arp reason mapping — test needs a populated "
         "rxReasonToQueueOrderedList";
  // Pick a target that differs from the original. If the original is 0, move
  // to 1, otherwise move to 0 — both are valid CPU queue ids on every
  // platform we ship.
  const int newQueueId = (*originalQueueId == 0) ? 1 : 0;

  XLOG(INFO) << "reason arp queue " << newQueueId << " (was "
             << *originalQueueId << ")";
  auto result = runCli(
      {"config", "copp", "reason", "arp", "queue", std::to_string(newQueueId)});
  ASSERT_EQ(result.exitCode, 0)
      << "stdout=" << result.stdout << " stderr=" << result.stderr;
  EXPECT_THAT(result.stdout, HasSubstr("ARP"));
  commitConfig();

  auto after = findReasonQueueId(kArpReason);
  ASSERT_TRUE(after.has_value());
  EXPECT_EQ(*after, newQueueId);

  XLOG(INFO) << "Restoring arp -> queue " << *originalQueueId;
  result = runCli(
      {"config",
       "copp",
       "reason",
       "arp",
       "queue",
       std::to_string(*originalQueueId)});
  ASSERT_EQ(result.exitCode, 0) << result.stderr;
  commitConfig();
  EXPECT_EQ(findReasonQueueId(kArpReason), originalQueueId);
}
