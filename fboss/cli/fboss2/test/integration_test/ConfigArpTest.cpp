// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/**
 * End-to-end tests for `fboss2-dev config arp <attr> <value>` commands.
 *
 * Covers the 4 hitless ARP/NDP tunables:
 *   - timeout         -> arpTimeoutSeconds
 *   - age-interval    -> arpAgerInterval
 *   - max-probes      -> maxNeighborProbes
 *   - stale-interval  -> staleEntryInterval
 *
 * For each attribute, the test:
 *   1. Reads the current value from the agent's running config
 *   2. Sets a new value via `config arp <attr> <new>`
 *   3. Commits the session (HITLESS — no agent restart)
 *   4. Verifies the running config reflects the new value
 *   5. Restores the original value
 *
 * Requirements:
 *   - FBOSS agent is running with a valid configuration
 *   - Test is run as root (or with sudo) on a DUT
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

class ConfigArpTest : public Fboss2IntegrationTest {
 protected:
  folly::dynamic getRunningConfig() const {
    HostInfo hostInfo("localhost");
    auto client =
        utils::createClient<apache::thrift::Client<FbossCtrl>>(hostInfo);
    std::string configStr;
    client->sync_getRunningConfig(configStr);
    return folly::parseJson(configStr);
  }

  // Read a top-level int field from sw (the field is only present when set).
  int getSwField(const std::string& field) const {
    auto config = getRunningConfig();
    if (!config.isObject() || !config.count("sw")) {
      throw std::runtime_error("Running config missing 'sw' object");
    }
    const auto& sw = config["sw"];
    if (!sw.isObject() || !sw.count(field)) {
      throw std::runtime_error(
          fmt::format("Running config 'sw' missing field '{}'", field));
    }
    return sw[field].asInt();
  }

  void setAndVerify(
      const std::string& attr,
      const std::string& swField,
      int newValue) {
    XLOG(INFO) << "========================================";
    XLOG(INFO) << "  arp " << attr << " -> " << newValue;
    XLOG(INFO) << "========================================";

    int originalValue = getSwField(swField);
    XLOG(INFO) << "[Step 1] Original " << swField << " = " << originalValue;
    ASSERT_NE(originalValue, newValue)
        << "Test value " << newValue << " must differ from original "
        << originalValue;

    XLOG(INFO) << "[Step 2] Running: config arp " << attr << " " << newValue;
    auto result = runCli({"config", "arp", attr, std::to_string(newValue)});
    ASSERT_EQ(result.exitCode, 0)
        << "stdout=" << result.stdout << " stderr=" << result.stderr;
    EXPECT_THAT(result.stdout, HasSubstr(attr));

    XLOG(INFO) << "[Step 3] Committing (expect HITLESS, no restart)...";
    commitConfig();

    XLOG(INFO) << "[Step 4] Verifying running config...";
    int observed = getSwField(swField);
    EXPECT_EQ(observed, newValue) << "Expected running config " << swField
                                  << "=" << newValue << ", got " << observed;

    XLOG(INFO) << "[Step 5] Restoring original value " << originalValue;
    result = runCli({"config", "arp", attr, std::to_string(originalValue)});
    ASSERT_EQ(result.exitCode, 0) << result.stderr;
    commitConfig();
    EXPECT_EQ(getSwField(swField), originalValue);

    XLOG(INFO) << "  PASSED: " << attr;
  }
};

TEST_F(ConfigArpTest, SetTimeout) {
  setAndVerify("timeout", "arpTimeoutSeconds", 75);
}

TEST_F(ConfigArpTest, SetAgeInterval) {
  setAndVerify("age-interval", "arpAgerInterval", 8);
}

TEST_F(ConfigArpTest, SetMaxProbes) {
  setAndVerify("max-probes", "maxNeighborProbes", 250);
}

TEST_F(ConfigArpTest, SetStaleInterval) {
  setAndVerify("stale-interval", "staleEntryInterval", 15);
}
