// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/**
 * End-to-end tests for the lookup-class attribute:
 *   'fboss2-dev config interface <name> lookup-class <id>[,<id>...]'
 *   'fboss2-dev delete interface <name> lookup-class'
 *
 * Happy-path cases stage a set, commit it, and verify the port's
 * lookupClasses list in the agent's running config, then delete + commit and
 * verify the list is cleared again. The change is applied hitlessly (normal
 * port-settings delta): commit reloads the agent config without restarting
 * the agents.
 *
 * Requirements:
 * - FBOSS agent must be running with a valid configuration.
 */

#include <folly/dynamic.h>
#include <gtest/gtest.h>
#include <exception>
#include <string>
#include <vector>
#include "fboss/cli/fboss2/test/integration_test/Fboss2IntegrationTest.h"

using namespace facebook::fboss;

class ConfigInterfaceLookupClassTest : public Fboss2IntegrationTest {
 protected:
  void TearDown() override {
    // If the test failed after committing a lookup-class change but before
    // clearing it, best-effort restore the port so a shared DUT is left
    // clean. (A committed change survives discardSession(), which the base
    // SetUp already runs to clear any staged session before the next test.)
    if (!committedPort_.empty()) {
      try {
        if (!portLookupClasses(getRunningConfig(), committedPort_).empty()) {
          runCli({"delete", "interface", committedPort_, "lookup-class"});
          commitConfig();
        }
      } catch (const std::exception&) {
        // Best-effort; do not mask the test result.
      }
    }
    Fboss2IntegrationTest::TearDown();
  }

  // lookupClasses ids for portName in a running config (as returned by
  // getRunningConfig()); empty when the port or the field is absent.
  static std::vector<int> portLookupClasses(
      const folly::dynamic& config,
      const std::string& portName) {
    if (!config.isObject() || !config.count("sw")) {
      return {};
    }
    const auto& sw = config["sw"];
    if (!sw.isObject() || !sw.count("ports")) {
      return {};
    }
    for (const auto& port : sw["ports"]) {
      if (port.count("name") && port["name"].asString() == portName) {
        std::vector<int> ids;
        if (port.count("lookupClasses")) {
          for (const auto& c : port["lookupClasses"]) {
            ids.push_back(static_cast<int>(c.asInt()));
          }
        }
        return ids;
      }
    }
    return {};
  }

  // Port a test committed a lookup-class list on; cleared in TearDown if the
  // test did not restore it itself.
  std::string committedPort_;
};

// Stage the full queue-per-host class list on a port, commit, verify it in
// the agent's running config, then delete + commit and verify it is cleared.
TEST_F(ConfigInterfaceLookupClassTest, SetThenDeleteLookupClassList) {
  // Pick a port with an empty lookupClasses list so the set -> clear cycle
  // restores the device to its original state.
  auto config = getRunningConfig();
  std::string ifaceName;
  if (config.isObject() && config.count("sw") && config["sw"].count("ports")) {
    for (const auto& port : config["sw"]["ports"]) {
      if (port.count("name") &&
          portLookupClasses(config, port["name"].asString()).empty()) {
        ifaceName = port["name"].asString();
        break;
      }
    }
  }
  if (ifaceName.empty()) {
    GTEST_SKIP() << "Every port already has lookupClasses configured";
  }

  // Stage the class list, commit, and verify it in the agent running config.
  const std::vector<int> expected = {10, 11, 12, 13, 14};
  auto setResult = runCli(
      {"config", "interface", ifaceName, "lookup-class", "10,11,12,13,14"});
  ASSERT_EQ(setResult.exitCode, 0)
      << "Failed to stage lookup-class: " << setResult.stderr;
  committedPort_ = ifaceName;
  commitConfig();
  auto afterSet = waitForRunningConfig([&](const folly::dynamic& c) {
    return portLookupClasses(c, ifaceName) == expected;
  });
  EXPECT_EQ(portLookupClasses(afterSet, ifaceName), expected)
      << "Running config does not show lookupClasses on " << ifaceName;

  // Delete, commit, and verify the list is cleared.
  auto delResult = runCli({"delete", "interface", ifaceName, "lookup-class"});
  ASSERT_EQ(delResult.exitCode, 0)
      << "Failed to delete lookup-class: " << delResult.stderr;
  commitConfig();
  auto afterClear = waitForRunningConfig([&](const folly::dynamic& c) {
    return portLookupClasses(c, ifaceName).empty();
  });
  EXPECT_TRUE(portLookupClasses(afterClear, ifaceName).empty())
      << "Running config still shows lookupClasses on " << ifaceName;
}
