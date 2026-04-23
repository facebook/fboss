// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/**
 * End-to-end test for PFC-related CLI commands:
 *   fboss2-dev config qos buffer-pool <name> shared-bytes/headroom-bytes
 *   fboss2-dev config qos priority-group-policy <name> group-id <id> <attrs>
 *   fboss2-dev config interface <port> pfc-config <attrs>
 *
 * Exercises both the single-attribute and multi-attribute forms of the
 * priority-group-policy and pfc-config commands, then verifies the agent's
 * running config.
 */

#include <fmt/format.h>
#include <folly/json/dynamic.h>
#include <folly/logging/xlog.h>
#include <gtest/gtest.h>
#include <cstdint>
#include <initializer_list>
#include <string>
#include <vector>
#include "fboss/cli/fboss2/test/integration_test/Fboss2IntegrationTest.h"

using namespace facebook::fboss;

namespace {
// Use stable (non-unique) names because the agent only supports a single
// bufferPool in the running config ("Only one bufferPool supported!").
// Unique-per-run names would accumulate pools across runs and wedge the
// agent on next restart. With stable names each run overwrites the prior
// definition. Shared with ConfigPortQueueConfigTest for the same reason.
constexpr auto kBufferPoolName = "cli_e2e_test_buffer_pool";
constexpr auto kPolicyName = "cli_e2e_test_pg_policy";

// MMUScalingFactor enum values from switch_config.thrift
constexpr int kOneHalf = 8;
constexpr int kTwo = 9;
// PfcWatchdogRecoveryAction enum: NO_DROP=0, DROP=1
constexpr int kRecoveryNoDrop = 0;

struct PgSpec {
  int id;
  const char* scalingFactorName; // "ONE_HALF" / "TWO"
  int scalingFactorInt;
  int64_t minLimitBytes;
  int64_t headroomLimitBytes;
  int64_t resumeOffsetBytes;
};
} // namespace

class ConfigPfcTest : public Fboss2IntegrationTest {
 protected:
  std::string bufferPoolName_ = kBufferPoolName;
  std::string policyName_ = kPolicyName;

  void SetUp() override {
    Fboss2IntegrationTest::SetUp();
    XLOG(INFO) << "Using buffer-pool: " << bufferPoolName_;
    XLOG(INFO) << "Using pg-policy:  " << policyName_;
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

  // Configure a priority group by issuing one attribute per CLI call.
  void configurePgSingleAttr(const PgSpec& pg) {
    std::vector<std::string> base = {
        "config",
        "qos",
        "priority-group-policy",
        policyName_,
        "group-id",
        std::to_string(pg.id)};
    auto run = [&](std::initializer_list<std::string> extras) {
      std::vector<std::string> cmd = base;
      cmd.insert(cmd.end(), extras);
      auto r = runCli(cmd);
      ASSERT_EQ(r.exitCode, 0) << r.stderr;
    };
    run({"min-limit-bytes", std::to_string(pg.minLimitBytes)});
    run({"headroom-limit-bytes", std::to_string(pg.headroomLimitBytes)});
    run({"resume-offset-bytes", std::to_string(pg.resumeOffsetBytes)});
    run({"scaling-factor", pg.scalingFactorName});
    run({"buffer-pool-name", bufferPoolName_});
  }

  // Configure a priority group in one CLI call with all attributes.
  void configurePgMultiAttr(const PgSpec& pg) {
    auto r = runCli(
        {"config",
         "qos",
         "priority-group-policy",
         policyName_,
         "group-id",
         std::to_string(pg.id),
         "min-limit-bytes",
         std::to_string(pg.minLimitBytes),
         "headroom-limit-bytes",
         std::to_string(pg.headroomLimitBytes),
         "resume-offset-bytes",
         std::to_string(pg.resumeOffsetBytes),
         "scaling-factor",
         pg.scalingFactorName,
         "buffer-pool-name",
         bufferPoolName_});
    ASSERT_EQ(r.exitCode, 0) << r.stderr;
  }

  const folly::dynamic* findPg(const folly::dynamic& list, int id) const {
    for (const auto& pg : list) {
      if (pg.isObject() && pg.count("id") && pg["id"].asInt() == id) {
        return &pg;
      }
    }
    return nullptr;
  }

  void expectPgMatch(const folly::dynamic& actual, const PgSpec& expected) {
    EXPECT_EQ(actual["sramScalingFactor"].asInt(), expected.scalingFactorInt);
    EXPECT_EQ(actual["minLimitBytes"].asInt(), expected.minLimitBytes);
    EXPECT_EQ(
        actual["headroomLimitBytes"].asInt(), expected.headroomLimitBytes);
    EXPECT_EQ(actual["resumeOffsetBytes"].asInt(), expected.resumeOffsetBytes);
    EXPECT_EQ(actual["bufferPoolName"].asString(), bufferPoolName_);
    EXPECT_EQ(actual["name"].asString(), fmt::format("pg{}", expected.id));
  }
};

TEST_F(ConfigPfcTest, BufferPoolPriorityGroupAndPortPfc) {
  Interface intf = findFirstEthInterface();
  XLOG(INFO) << "Using test interface " << intf.name;

  XLOG(INFO) << "[Step 1] Configuring buffer pool...";
  configureBufferPool(/*sharedBytes=*/78773528, /*headroomBytes=*/4405376);

  const std::vector<PgSpec> pgs = {
      {2, "ONE_HALF", kOneHalf, 14478, 726440, 9652},
      {6, "ONE_HALF", kOneHalf, 4826, 726440, 9652},
      {0, "TWO", kTwo, 4826, 0, 9652},
      {7, "TWO", kTwo, 4826, 0, 9652},
  };

  XLOG(INFO) << "[Step 2a] Configuring PGs with single-attribute commands...";
  configurePgSingleAttr(pgs[0]);
  configurePgSingleAttr(pgs[1]);
  XLOG(INFO) << "[Step 2b] Configuring PGs with multi-attribute commands...";
  configurePgMultiAttr(pgs[2]);
  configurePgMultiAttr(pgs[3]);

  XLOG(INFO) << "[Step 3] Configuring port PFC on " << intf.name;
  // Single-attribute form for tx / rx / pg-policy binding.
  ASSERT_EQ(
      runCli({"config", "interface", intf.name, "pfc-config", "tx", "enabled"})
          .exitCode,
      0);
  ASSERT_EQ(
      runCli({"config", "interface", intf.name, "pfc-config", "rx", "enabled"})
          .exitCode,
      0);
  ASSERT_EQ(
      runCli({"config",
              "interface",
              intf.name,
              "pfc-config",
              "priority-group-policy",
              policyName_})
          .exitCode,
      0);
  // Multi-attribute form for the watchdog bundle.
  ASSERT_EQ(
      runCli({"config",
              "interface",
              intf.name,
              "pfc-config",
              "watchdog-detection-time",
              "150",
              "watchdog-recovery-time",
              "1000",
              "watchdog-recovery-action",
              "no-drop"})
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
  ASSERT_TRUE(pools.count(bufferPoolName_));
  EXPECT_EQ(pools[bufferPoolName_]["sharedBytes"].asInt(), 78773528);
  EXPECT_EQ(pools[bufferPoolName_]["headroomBytes"].asInt(), 4405376);

  // Priority group policy
  ASSERT_TRUE(sw.count("portPgConfigs"));
  const auto& pgConfigs = sw["portPgConfigs"];
  ASSERT_TRUE(pgConfigs.count(policyName_));
  const auto& pgList = pgConfigs[policyName_];
  for (const auto& spec : pgs) {
    const auto* actual = findPg(pgList, spec.id);
    ASSERT_NE(actual, nullptr)
        << "PG id " << spec.id << " missing in running config";
    expectPgMatch(*actual, spec);
  }

  // Per-port PFC
  bool sawPort = false;
  for (const auto& port : sw["ports"]) {
    if (!port.count("name") || port["name"].asString() != intf.name) {
      continue;
    }
    sawPort = true;
    ASSERT_TRUE(port.count("pfc")) << "pfc missing on " << intf.name;
    const auto& pfc = port["pfc"];
    EXPECT_TRUE(pfc["tx"].asBool());
    EXPECT_TRUE(pfc["rx"].asBool());
    EXPECT_EQ(pfc["portPgConfigName"].asString(), policyName_);
    ASSERT_TRUE(pfc.count("watchdog"));
    const auto& wd = pfc["watchdog"];
    EXPECT_EQ(wd["detectionTimeMsecs"].asInt(), 150);
    EXPECT_EQ(wd["recoveryTimeMsecs"].asInt(), 1000);
    EXPECT_EQ(wd["recoveryAction"].asInt(), kRecoveryNoDrop);
    break;
  }
  EXPECT_TRUE(sawPort) << "interface " << intf.name << " not in running config";

  XLOG(INFO) << "TEST PASSED";
}
