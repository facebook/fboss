// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/**
 * End-to-end test for:
 *   fboss2-dev config qos default-queue-config <queue-id> <attr> <value> ...
 *   fboss2-dev delete qos default-queue-config <queue-id>
 *
 * One happy path, covering only what cannot be checked off-device: that a
 * committed config session survives the agent restart and lands in the running
 * config, and that delete takes it back out.
 *
 * Everything else already has coverage a layer down. Argument parsing,
 * attribute handling and rejection of malformed input are unit tested in
 * fboss/cli/fboss2/test/config/CmdConfigQosDefaultQueueConfigTest.cpp against a
 * seeded ConfigSession; whether the agent accepts a given cfg::PortQueue is
 * covered by fboss/agent/state/tests/PortQueueTests.cpp against the real
 * ThriftConfigApplier. Neither needs a device.
 *
 * Both commands commit at ConfigActionLevel::AGENT_COLDBOOT, so each commit
 * restarts the agent. Keep the test to the two restarts it already needs.
 */

#include <folly/json/dynamic.h>
#include <folly/logging/xlog.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <set>
#include <string>
#include "fboss/cli/fboss2/test/integration_test/Fboss2IntegrationTest.h"

using namespace facebook::fboss;

class ConfigQosDefaultQueueConfigTest : public Fboss2IntegrationTest {
 protected:
  void SetUp() override {
    Fboss2IntegrationTest::SetUp();
    discardSession();
  }

  void TearDown() override {
    discardSession();
    Fboss2IntegrationTest::TearDown();
  }

  /**
   * Return the defaultPortQueues entry with the given queue id as a
   * folly::dynamic object, or nullptr if no such entry exists.
   */
  const folly::dynamic* findQueueById(const folly::dynamic& config, int queueId)
      const {
    if (!config.isObject() || !config.count("sw") ||
        !config["sw"].count("defaultPortQueues")) {
      return nullptr;
    }
    const auto& queues = config["sw"]["defaultPortQueues"];
    if (!queues.isArray()) {
      return nullptr;
    }
    for (const auto& q : queues) {
      if (q.count("id") && q["id"].asInt() == queueId) {
        return &q;
      }
    }
    return nullptr;
  }

  /**
   * Return the set of queue ids present in the running config's
   * defaultPortQueues, or an empty set if the list is absent.
   */
  std::set<int> defaultQueueIds(const folly::dynamic& config) const {
    std::set<int> ids;
    if (!config.isObject() || !config.count("sw") ||
        !config["sw"].count("defaultPortQueues")) {
      return ids;
    }
    const auto& queues = config["sw"]["defaultPortQueues"];
    if (!queues.isArray()) {
      return ids;
    }
    for (const auto& q : queues) {
      if (q.count("id")) {
        ids.insert(q["id"].asInt());
      }
    }
    return ids;
  }
};

// Create a queue entry, commit, verify the agent's running config, then delete
// the entry and verify it is gone. Operating on an unused queue id keeps the
// test independent of how many queues the ASIC already has configured,
// including none at all.
TEST_F(ConfigQosDefaultQueueConfigTest, CreateThenDeleteQueueEntry) {
  auto config = getRunningConfig();
  auto ids = defaultQueueIds(config);

  int candidateId = 0;
  while (ids.count(candidateId)) {
    candidateId++;
  }
  XLOG(INFO) << "using candidate queue-id " << candidateId;

  ASSERT_EQ(
      runCli({"config",
              "qos",
              "default-queue-config",
              std::to_string(candidateId),
              "weight",
              "2"})
          .exitCode,
      0);

  XLOG(INFO) << "Committing create...";
  commitConfig();
  waitForAgentReady();

  auto afterCreate = getRunningConfig();
  const auto* q = findQueueById(afterCreate, candidateId);
  ASSERT_NE(q, nullptr) << "created queue-id " << candidateId
                        << " not in running config";
  ASSERT_TRUE(q->count("weight"));
  EXPECT_EQ((*q)["weight"].asInt(), 2);
  XLOG(INFO) << "Create verified.";

  discardSession();
  auto del = runCli(
      {"delete", "qos", "default-queue-config", std::to_string(candidateId)});
  ASSERT_EQ(del.exitCode, 0) << "CLI failed: " << del.stderr;
  EXPECT_THAT(del.stdout, ::testing::HasSubstr("Successfully deleted"));

  XLOG(INFO) << "Committing delete...";
  commitConfig();
  waitForAgentReady();

  EXPECT_EQ(defaultQueueIds(getRunningConfig()), ids)
      << "defaultPortQueues ids differ from original after delete";
  XLOG(INFO) << "Delete verified.";
}
