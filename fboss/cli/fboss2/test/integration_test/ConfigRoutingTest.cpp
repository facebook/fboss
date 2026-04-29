// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/**
 * End-to-end tests for `fboss2-dev config routing max-route-counter-ids`
 * command.
 *
 * The command sets sw.switchSettings.maxRouteCounterIDs — the maximum number
 * of distinct hardware counter objects the agent may allocate for per-route
 * traffic tracking. 0 disables the feature; FSWs typically use 13.
 *
 * This command is AGENT_WARMBOOT: the agent restarts (without clearing ASIC
 * state) to re-process routes and allocate/free counter objects consistently
 * with the new cap.
 *
 * For each test:
 *   1. Read current maxRouteCounterIDs from the running config
 *   2. Apply a new value via `config routing max-route-counter-ids <N>`
 *   3. Commit (triggers agent warmboot restart)
 *   4. Wait for agent to come back up
 *   5. Verify running config reflects the new value
 *   6. Restore and verify
 *
 * Requirements:
 *   - FBOSS agent is running with a valid configuration
 *   - sw.switchSettings.maxRouteCounterIDs is present in the running config
 */

#include <fmt/format.h>
#include <folly/json/dynamic.h>
#include <folly/json/json.h>
#include <folly/logging/xlog.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <stdexcept>
#include <string>

#include "fboss/agent/if/gen-cpp2/FbossCtrlAsyncClient.h"
#include "fboss/cli/fboss2/test/integration_test/Fboss2IntegrationTest.h"
#include "fboss/cli/fboss2/utils/CmdClientUtilsCommon.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

using namespace facebook::fboss;
using ::testing::HasSubstr;

class ConfigRoutingTest : public Fboss2IntegrationTest {
 protected:
  folly::dynamic getRunningConfig() const {
    HostInfo hostInfo("localhost");
    auto client =
        utils::createClient<apache::thrift::Client<FbossCtrl>>(hostInfo);
    std::string configStr;
    client->sync_getRunningConfig(configStr);
    return folly::parseJson(configStr);
  }

  // Read sw.switchSettings.<field> as int from the running config.
  int getSwitchSettingsField(const std::string& field) const {
    auto config = getRunningConfig();
    const auto& sw = config["sw"];
    if (!sw.count("switchSettings")) {
      throw std::runtime_error(
          "Running config missing 'sw.switchSettings' object");
    }
    const auto& ss = sw["switchSettings"];
    if (!ss.count(field)) {
      throw std::runtime_error(
          fmt::format("sw.switchSettings missing field '{}'", field));
    }
    return ss[field].asInt();
  }
};

TEST_F(ConfigRoutingTest, SetMaxRouteCounterIds) {
  int originalValue = getSwitchSettingsField("maxRouteCounterIDs");
  XLOG(INFO) << "Original maxRouteCounterIDs = " << originalValue;

  // Pick a value distinct from the original.
  const int newValue = (originalValue == 0) ? 5 : 0;
  XLOG(INFO) << "Setting maxRouteCounterIDs to " << newValue;

  auto result = runCli(
      {"config", "routing", "max-route-counter-ids", std::to_string(newValue)});
  ASSERT_EQ(result.exitCode, 0)
      << "stdout=" << result.stdout << " stderr=" << result.stderr;
  EXPECT_THAT(result.stdout, HasSubstr("max-route-counter-ids"));

  XLOG(INFO) << "Committing (AGENT_WARMBOOT — agent will restart)...";
  commitConfig();

  XLOG(INFO) << "Waiting for agent to come back up...";
  waitForAgentReady();

  XLOG(INFO) << "Verifying running config...";
  int observed = getSwitchSettingsField("maxRouteCounterIDs");
  EXPECT_EQ(observed, newValue)
      << "Expected maxRouteCounterIDs=" << newValue << ", got " << observed;

  XLOG(INFO) << "Restoring maxRouteCounterIDs to " << originalValue;
  result = runCli(
      {"config",
       "routing",
       "max-route-counter-ids",
       std::to_string(originalValue)});
  ASSERT_EQ(result.exitCode, 0) << result.stderr;
  commitConfig();
  waitForAgentReady();
  EXPECT_EQ(getSwitchSettingsField("maxRouteCounterIDs"), originalValue);
}
