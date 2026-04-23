// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/**
 * End-to-end test for:
 *   fboss2-dev config qos buffer-pool <name> shared-bytes/headroom-bytes
 *   fboss2-dev config qos queuing-policy <name> queue-id <id> <attr> <value>...
 *   fboss2-dev config interface <name> queuing-policy <policy>
 *
 * Builds a buffer pool + queuing policy with five queues exercising:
 *   - WRR + SP scheduling
 *   - weight, shared-bytes, reserved-bytes
 *   - scaling-factor (MMUScalingFactor enum)
 *   - stream-type (MULTICAST vs default UNICAST)
 *   - buffer-pool-name back-reference
 *   - AQM with ECN behavior and linear detection profile
 * then assigns the policy to an interface and verifies the running config.
 */

#include <folly/json/dynamic.h>
#include <folly/logging/xlog.h>
#include <gtest/gtest.h>
#include <string>
#include <vector>
#include "fboss/cli/fboss2/test/integration_test/Fboss2IntegrationTest.h"

using namespace facebook::fboss;

namespace {
// Stable names (see matching rationale in ConfigPfcTest.cpp). Shares the
// buffer pool name with ConfigPfcTest because FBOSS only permits one
// bufferPool in the running config.
constexpr auto kBufferPoolName = "cli_e2e_test_buffer_pool";
constexpr auto kQueuingPolicyName = "cli_e2e_test_queue_policy";

// Enum values from switch_config.thrift
constexpr int kSchedWrr = 0;
constexpr int kSchedSp = 1;
constexpr int kScalingTwo = 9;
constexpr int kStreamUnicast = 0;
constexpr int kStreamMulticast = 1;
constexpr int kBehaviorEcn = 1;
} // namespace

class ConfigPortQueueConfigTest : public Fboss2IntegrationTest {
 protected:
  std::string bufferPoolName_ = kBufferPoolName;
  std::string policyName_ = kQueuingPolicyName;

  void SetUp() override {
    Fboss2IntegrationTest::SetUp();
    XLOG(INFO) << "Using buffer-pool: " << bufferPoolName_;
    XLOG(INFO) << "Using queuing-policy: " << policyName_;
  }

  void configureBufferPool(int sharedBytes, int headroomBytes) {
    ASSERT_EQ(
        runCli({"config",
                "qos",
                "buffer-pool",
                bufferPoolName_,
                "shared-bytes",
                std::to_string(sharedBytes)})
            .exitCode,
        0);
    ASSERT_EQ(
        runCli({"config",
                "qos",
                "buffer-pool",
                bufferPoolName_,
                "headroom-bytes",
                std::to_string(headroomBytes)})
            .exitCode,
        0);
  }

  // Thin wrapper — just builds the long argv and forwards to runCli.
  void configureQueue(int queueId, const std::vector<std::string>& attrs) {
    std::vector<std::string> cmd = {
        "config",
        "qos",
        "queuing-policy",
        policyName_,
        "queue-id",
        std::to_string(queueId)};
    cmd.insert(cmd.end(), attrs.begin(), attrs.end());
    auto result = runCli(cmd);
    ASSERT_EQ(result.exitCode, 0)
        << "queue-id " << queueId << " failed: " << result.stderr;
  }

  const folly::dynamic* findQueue(const folly::dynamic& list, int queueId)
      const {
    for (const auto& q : list) {
      if (q.isObject() && q.count("id") && q["id"].asInt() == queueId) {
        return &q;
      }
    }
    return nullptr;
  }
};

TEST_F(ConfigPortQueueConfigTest, BuildAndAssignPolicy) {
  Interface intf = findFirstEthInterface();
  XLOG(INFO) << "Using test interface " << intf.name;

  XLOG(INFO) << "[Step 1] Configuring buffer pool...";
  configureBufferPool(/*sharedBytes=*/78773528, /*headroomBytes=*/4405376);

  XLOG(INFO) << "[Step 2] Configuring queues...";
  configureQueue(
      2, {"scheduling", "SP", "shared-bytes", "83618832", "weight", "10"});
  configureQueue(
      6,
      {"scheduling",
       "SP",
       "shared-bytes",
       "83618832",
       "scaling-factor",
       "TWO"});
  configureQueue(
      7,
      {"scheduling",
       "WRR",
       "weight",
       "20",
       "reserved-bytes",
       "5000",
       "buffer-pool-name",
       bufferPoolName_});
  configureQueue(
      3, {"scheduling", "WRR", "weight", "15", "stream-type", "MULTICAST"});
  configureQueue(
      4,
      {"scheduling",
       "SP",
       "shared-bytes",
       "83618832",
       "active-queue-management",
       "congestion-behavior",
       "ECN",
       "detection",
       "linear",
       "minimum-length",
       "120000",
       "maximum-length",
       "120000",
       "probability",
       "100"});

  XLOG(INFO) << "[Step 3] Assigning queuing-policy to " << intf.name;
  ASSERT_EQ(
      runCli({"config", "interface", intf.name, "queuing-policy", policyName_})
          .exitCode,
      0);

  XLOG(INFO) << "[Step 4] Committing...";
  commitConfig();
  waitForAgentReady();

  XLOG(INFO) << "[Step 5] Verifying running config...";
  auto config = getRunningConfig();
  const auto& sw = config["sw"];

  // Buffer pool
  ASSERT_TRUE(sw.count("bufferPoolConfigs"));
  const auto& pools = sw["bufferPoolConfigs"];
  ASSERT_TRUE(pools.count(bufferPoolName_))
      << "buffer pool " << bufferPoolName_ << " missing";
  EXPECT_EQ(pools[bufferPoolName_]["sharedBytes"].asInt(), 78773528);
  EXPECT_EQ(pools[bufferPoolName_]["headroomBytes"].asInt(), 4405376);

  // Queuing policy
  ASSERT_TRUE(sw.count("portQueueConfigs"));
  const auto& policies = sw["portQueueConfigs"];
  ASSERT_TRUE(policies.count(policyName_))
      << "policy " << policyName_ << " missing";
  const auto& queues = policies[policyName_];

  // Queue 2: SP + shared + weight
  {
    const auto* q = findQueue(queues, 2);
    ASSERT_NE(q, nullptr);
    EXPECT_EQ((*q)["scheduling"].asInt(), kSchedSp);
    EXPECT_EQ((*q)["sharedBytes"].asInt(), 83618832);
    EXPECT_EQ((*q)["weight"].asInt(), 10);
    EXPECT_EQ(
        (*q).getDefault("streamType", kStreamUnicast).asInt(), kStreamUnicast);
  }
  // Queue 6: SP + scaling factor TWO
  {
    const auto* q = findQueue(queues, 6);
    ASSERT_NE(q, nullptr);
    EXPECT_EQ((*q)["scheduling"].asInt(), kSchedSp);
    EXPECT_EQ((*q)["scalingFactor"].asInt(), kScalingTwo);
  }
  // Queue 7: WRR + reserved + buffer-pool ref
  {
    const auto* q = findQueue(queues, 7);
    ASSERT_NE(q, nullptr);
    EXPECT_EQ((*q)["scheduling"].asInt(), kSchedWrr);
    EXPECT_EQ((*q)["weight"].asInt(), 20);
    EXPECT_EQ((*q)["reservedBytes"].asInt(), 5000);
    EXPECT_EQ((*q)["bufferPoolName"].asString(), bufferPoolName_);
  }
  // Queue 3: streamType MULTICAST
  {
    const auto* q = findQueue(queues, 3);
    ASSERT_NE(q, nullptr);
    EXPECT_EQ((*q)["streamType"].asInt(), kStreamMulticast);
    EXPECT_EQ((*q)["scheduling"].asInt(), kSchedWrr);
    EXPECT_EQ((*q)["weight"].asInt(), 15);
  }
  // Queue 4: AQM ECN w/ linear detection
  {
    const auto* q = findQueue(queues, 4);
    ASSERT_NE(q, nullptr);
    ASSERT_TRUE(q->count("aqms"));
    const auto& aqms = (*q)["aqms"];
    ASSERT_TRUE(aqms.isArray());
    ASSERT_FALSE(aqms.empty());
    EXPECT_EQ(aqms[0]["behavior"].asInt(), kBehaviorEcn);
    const auto& linear = aqms[0]["detection"]["linear"];
    EXPECT_EQ(linear["minimumLength"].asInt(), 120000);
    EXPECT_EQ(linear["maximumLength"].asInt(), 120000);
    EXPECT_EQ(linear["probability"].asInt(), 100);
  }

  // Interface assignment
  bool sawPort = false;
  for (const auto& port : sw["ports"]) {
    if (port.count("name") && port["name"].asString() == intf.name) {
      sawPort = true;
      EXPECT_EQ(
          port.getDefault("portQueueConfigName", "").asString(), policyName_);
      break;
    }
  }
  EXPECT_TRUE(sawPort) << "interface " << intf.name << " not in running config";

  XLOG(INFO) << "TEST PASSED";
}
