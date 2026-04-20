// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/**
 * End-to-end tests for `fboss2-dev config acl` commands.
 *
 * Covers:
 *   - config acl table-group <group-name> stage <stage>
 *     -> AclTableGroup.stage
 *   - config acl table <group-name> <table-name> priority <value>
 *     -> AclTable.priority
 *
 * For each attribute the test:
 *   1. Reads the live running config to discover group/table names and current
 *      values (no hardcoded names — portable across DUTs).
 *   2. Sets a new value via `config acl ...` runCli().
 *   3. Commits the session (HITLESS — no agent restart).
 *   4. Verifies the running config reflects the new value.
 *   5. Restores the original value.
 *
 * Requirements:
 *   - FBOSS agent is running with a valid configuration that includes
 *     sw.aclTableGroups (field 56) with at least one group and one table.
 *   - Test is run as a non-root user with write access to /etc/coop.
 */

#include <folly/json/dynamic.h>
#include <folly/json/json.h>
#include <folly/logging/xlog.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <optional>
#include <string>
#include "fboss/agent/if/gen-cpp2/FbossCtrlAsyncClient.h"
#include "fboss/cli/fboss2/test/integration_test/Fboss2IntegrationTest.h"
#include "fboss/cli/fboss2/utils/CmdClientUtilsCommon.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

using namespace facebook::fboss;
using ::testing::HasSubstr;

class ConfigAclTest : public Fboss2IntegrationTest {
 protected:
  folly::dynamic getRunningConfig() const {
    HostInfo hostInfo("localhost");
    auto client =
        utils::createClient<apache::thrift::Client<FbossCtrl>>(hostInfo);
    std::string configStr;
    client->sync_getRunningConfig(configStr);
    return folly::parseJson(configStr);
  }

  // Returns the first AclTableGroup object from the running config, or
  // std::nullopt when the DUT has no sw.aclTableGroups (field 56).
  std::optional<folly::dynamic> getFirstAclTableGroup() const {
    auto config = getRunningConfig();
    if (!config.isObject() || !config.count("sw")) {
      return std::nullopt;
    }
    const auto& sw = config["sw"];
    if (!sw.count("aclTableGroups") || !sw["aclTableGroups"].isArray() ||
        sw["aclTableGroups"].empty()) {
      return std::nullopt;
    }
    return sw["aclTableGroups"][0];
  }

  // Returns the first AclTable from the first AclTableGroup, or
  // std::nullopt when the DUT has no groups or no tables.
  std::optional<folly::dynamic> getFirstAclTable() const {
    auto maybeGroup = getFirstAclTableGroup();
    if (!maybeGroup.has_value()) {
      return std::nullopt;
    }
    const auto& group = *maybeGroup;
    if (!group.count("aclTables") || !group["aclTables"].isArray() ||
        group["aclTables"].empty()) {
      return std::nullopt;
    }
    return group["aclTables"][0];
  }
};

TEST_F(ConfigAclTest, SetTableGroupStage) {
  XLOG(INFO) << "========================================";
  XLOG(INFO) << "  acl table-group stage";
  XLOG(INFO) << "========================================";

  auto maybeGroup = getFirstAclTableGroup();
  if (!maybeGroup.has_value()) {
    GTEST_SKIP() << "DUT has no sw.aclTableGroups (field 56) — skipping";
  }
  auto& group = *maybeGroup;
  std::string groupName = group["name"].asString();
  int originalStage = group.getDefault("stage", 0).asInt();
  // Choose a different stage (INGRESS_POST_LOOKUP=3) for the test; if the
  // current stage already is 3, fall back to INGRESS=0.
  int newStage = (originalStage == 3) ? 0 : 3;
  std::string newStageName =
      (newStage == 3) ? "ingress-post-lookup" : "ingress";
  std::string originalStageName = (originalStage == 3) ? "ingress-post-lookup"
      : (originalStage == 2)                           ? "egress-macsec"
      : (originalStage == 1)                           ? "ingress-macsec"
                                                       : "ingress";

  XLOG(INFO) << "[Step 1] group='" << groupName
             << "' current stage=" << originalStage;
  ASSERT_NE(originalStage, newStage);

  XLOG(INFO) << "[Step 2] Running: config acl table-group " << groupName
             << " stage " << newStageName;
  auto result = runCli(
      {"config", "acl", "table-group", groupName, "stage", newStageName});
  ASSERT_EQ(result.exitCode, 0)
      << "stdout=" << result.stdout << " stderr=" << result.stderr;
  EXPECT_THAT(result.stdout, HasSubstr("stage"));

  XLOG(INFO) << "[Step 3] Committing (HITLESS)...";
  commitConfig();

  XLOG(INFO) << "[Step 4] Verifying...";
  auto updatedGroup = getFirstAclTableGroup();
  int observedStage = updatedGroup->getDefault("stage", 0).asInt();
  EXPECT_EQ(observedStage, newStage)
      << "Expected stage=" << newStage << " got " << observedStage;

  XLOG(INFO) << "[Step 5] Restoring stage=" << originalStageName;
  result = runCli(
      {"config", "acl", "table-group", groupName, "stage", originalStageName});
  ASSERT_EQ(result.exitCode, 0) << result.stderr;
  commitConfig();
  EXPECT_EQ(
      getFirstAclTableGroup()->getDefault("stage", 0).asInt(), originalStage);

  XLOG(INFO) << "  PASSED: acl table-group stage";
}

TEST_F(ConfigAclTest, SetTablePriority) {
  XLOG(INFO) << "========================================";
  XLOG(INFO) << "  acl table priority";
  XLOG(INFO) << "========================================";

  auto maybeGroup = getFirstAclTableGroup();
  if (!maybeGroup.has_value()) {
    GTEST_SKIP() << "DUT has no sw.aclTableGroups (field 56) — skipping";
  }
  auto& group = *maybeGroup;
  std::string groupName = group["name"].asString();
  auto maybeTable = getFirstAclTable();
  if (!maybeTable.has_value()) {
    GTEST_SKIP() << "First AclTableGroup has no aclTables — skipping";
  }
  auto& table = *maybeTable;
  std::string tableName = table["name"].asString();
  int originalPriority = table.getDefault("priority", 0).asInt();
  // Use priority+1 as the new value so we always have something different.
  int newPriority = originalPriority + 1;

  XLOG(INFO) << "[Step 1] group='" << groupName << "' table='" << tableName
             << "' current priority=" << originalPriority;

  XLOG(INFO) << "[Step 2] Running: config acl table " << groupName << " "
             << tableName << " priority " << newPriority;
  auto result = runCli(
      {"config",
       "acl",
       "table",
       groupName,
       tableName,
       "priority",
       std::to_string(newPriority)});
  ASSERT_EQ(result.exitCode, 0)
      << "stdout=" << result.stdout << " stderr=" << result.stderr;
  EXPECT_THAT(result.stdout, HasSubstr("priority"));

  XLOG(INFO) << "[Step 3] Committing (HITLESS)...";
  commitConfig();

  XLOG(INFO) << "[Step 4] Verifying...";
  auto updatedTable = getFirstAclTable();
  int observedPriority = updatedTable->getDefault("priority", 0).asInt();
  EXPECT_EQ(observedPriority, newPriority)
      << "Expected priority=" << newPriority << " got " << observedPriority;

  XLOG(INFO) << "[Step 5] Restoring priority=" << originalPriority;
  result = runCli(
      {"config",
       "acl",
       "table",
       groupName,
       tableName,
       "priority",
       std::to_string(originalPriority)});
  ASSERT_EQ(result.exitCode, 0) << result.stderr;
  commitConfig();
  EXPECT_EQ(
      getFirstAclTable()->getDefault("priority", 0).asInt(), originalPriority);

  XLOG(INFO) << "  PASSED: acl table priority";
}
