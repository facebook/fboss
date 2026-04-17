// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/**
 * End-to-end tests for `fboss2-dev config arp <attr> <value>` commands.
 *
 * Covers the 4 hitless ARP/NDP tunables. Each `<attr>` maps to one
 * `cfg::SwitchConfig.sw.<field>` and one `switchSettings.<field>`:
 *   - timeout        -> arpTimeoutSeconds  / arpTimeout
 *   - age-interval   -> arpAgerInterval    / arpAgerInterval
 *   - max-probes     -> maxNeighborProbes  / maxNeighborProbes
 *   - stale-interval -> staleEntryInterval / staleEntryInterval
 *
 * For each attribute, the test:
 *   1. Reads the current value from the agent's running config + switch state
 *   2. Sets a new value via `config arp <attr> <new>`
 *   3. Commits the session (HITLESS — no agent restart)
 *   4. Verifies the running config reflects the new value
 *   5. Verifies the agent's programmed switch state reflects the new value
 *   6. Restores the original value
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

  // Fetch the agent's programmed switchSettings from the live switch state.
  // Defined in switch_state.thrift::SwitchSettingsFields.
  folly::dynamic getSwitchSettings() const {
    HostInfo hostInfo("localhost");
    auto client =
        utils::createClient<apache::thrift::Client<FbossCtrl>>(hostInfo);
    std::string settingsJson;
    client->sync_getCurrentStateJSON(settingsJson, "switchSettings");
    return folly::parseJson(settingsJson);
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

  // Read a field from switchSettings in the agent's switch state. The field is
  // only present in the JSON once it has been programmed by the agent.
  int getSwitchSettingsField(const std::string& field) const {
    auto settings = getSwitchSettings();
    if (!settings.isObject() || !settings.count(field)) {
      throw std::runtime_error(
          fmt::format("switchSettings missing field '{}'", field));
    }
    return settings[field].asInt();
  }

  void setAndVerify(
      const std::string& attr,
      const std::string& swField,
      const std::string& switchStateField,
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

    XLOG(INFO) << "[Step 5] Verifying switch state switchSettings."
               << switchStateField;
    int programmed = getSwitchSettingsField(switchStateField);
    EXPECT_EQ(programmed, newValue)
        << "Expected switchSettings." << switchStateField << "=" << newValue
        << ", got " << programmed;

    XLOG(INFO) << "[Step 6] Restoring original value " << originalValue;
    result = runCli({"config", "arp", attr, std::to_string(originalValue)});
    ASSERT_EQ(result.exitCode, 0) << result.stderr;
    commitConfig();
    EXPECT_EQ(getSwField(swField), originalValue);
    EXPECT_EQ(getSwitchSettingsField(switchStateField), originalValue);

    XLOG(INFO) << "  PASSED: " << attr;
  }
};

TEST_F(ConfigArpTest, SetTimeout) {
  setAndVerify("timeout", "arpTimeoutSeconds", "arpTimeout", 75);
}

TEST_F(ConfigArpTest, SetAgeInterval) {
  setAndVerify("age-interval", "arpAgerInterval", "arpAgerInterval", 8);
}

TEST_F(ConfigArpTest, SetMaxProbes) {
  setAndVerify("max-probes", "maxNeighborProbes", "maxNeighborProbes", 250);
}

TEST_F(ConfigArpTest, SetStaleInterval) {
  setAndVerify(
      "stale-interval", "staleEntryInterval", "staleEntryInterval", 15);
}
