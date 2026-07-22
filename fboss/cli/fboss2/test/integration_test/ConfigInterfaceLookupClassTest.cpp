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
    // Never leave a staged lookup-class change behind for the next test.
    discardSession();
    // A test that failed between its set-commit and clear-commit would leave
    // the committed class list on the port; clear it (best-effort) so the
    // device is restored even on failure.
    if (!committedPort_.empty()) {
      try {
        if (!portLookupClasses(getRunningConfig(), committedPort_).empty()) {
          runCli({"delete", "interface", committedPort_, "lookup-class"});
          commitConfig();
        }
      } catch (const std::exception&) {
        // Restoration is best-effort; do not mask the test result.
      }
      // A failed restore may itself have staged a delete; never leave a
      // half-staged session behind for whatever runs next.
      discardSession();
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

  // Stage lookup-class ids on the interface, commit, and wait until the
  // running config shows exactly the expected list.
  void setLookupClassAndVerify(
      const std::string& ifaceName,
      const std::string& ids,
      const std::vector<int>& expected) {
    auto result =
        runCli({"config", "interface", ifaceName, "lookup-class", ids});
    ASSERT_EQ(result.exitCode, 0)
        << "Failed to stage lookup-class " << ids << ": " << result.stderr;
    committedPort_ = ifaceName;
    commitConfig();
    auto config = waitForRunningConfig([&](const folly::dynamic& c) {
      return portLookupClasses(c, ifaceName) == expected;
    });
    EXPECT_EQ(portLookupClasses(config, ifaceName), expected)
        << "Running config does not show lookupClasses " << ids << " on "
        << ifaceName;
  }

  // Delete lookup-class on the interface, commit, and wait until the running
  // config shows an empty lookupClasses list.
  void clearLookupClassAndVerify(const std::string& ifaceName) {
    auto result = runCli({"delete", "interface", ifaceName, "lookup-class"});
    ASSERT_EQ(result.exitCode, 0)
        << "Failed to delete lookup-class: " << result.stderr;
    commitConfig();
    auto config = waitForRunningConfig([&](const folly::dynamic& c) {
      return portLookupClasses(c, ifaceName).empty();
    });
    EXPECT_TRUE(portLookupClasses(config, ifaceName).empty())
        << "Running config still shows lookupClasses on " << ifaceName;
  }

  // First eth port whose lookupClasses list is empty in the running config.
  // The set -> clear cycle needs an empty baseline so the device is restored
  // to its original state; scanning (instead of insisting on the first eth
  // port) keeps the test runnable on a device that already has lookup
  // classes on some ports. Returns an empty string when every eth port has
  // classes configured.
  std::string findEthPortWithEmptyLookupClasses() {
    auto config = getRunningConfig();
    if (!config.isObject() || !config.count("sw")) {
      return "";
    }
    const auto& sw = config["sw"];
    if (!sw.isObject() || !sw.count("ports")) {
      return "";
    }
    for (const auto& port : sw["ports"]) {
      if (!port.count("name")) {
        continue;
      }
      auto name = port["name"].asString();
      if (name.rfind("eth", 0) != 0) {
        continue;
      }
      if (portLookupClasses(config, name).empty()) {
        return name;
      }
    }
    return "";
  }

  // Port a test committed a lookup-class list on; cleared in TearDown if the
  // test did not restore it itself.
  std::string committedPort_;
};

// A non-numeric value is rejected before anything is staged; the running
// config is untouched.
TEST_F(ConfigInterfaceLookupClassTest, RejectsNonNumericLookupClass) {
  Interface iface = findFirstEthInterface();
  auto before = portLookupClasses(getRunningConfig(), iface.name);
  auto result =
      runCli({"config", "interface", iface.name, "lookup-class", "bogus"});
  EXPECT_NE(result.exitCode, 0)
      << "Expected non-zero exit for non-numeric lookup-class value";
  EXPECT_EQ(portLookupClasses(getRunningConfig(), iface.name), before)
      << "Rejected command must not change the running config";
}

// An integer that is not a valid AclLookupClass enum value is rejected; the
// running config is untouched.
TEST_F(ConfigInterfaceLookupClassTest, RejectsInvalidLookupClassId) {
  Interface iface = findFirstEthInterface();
  auto before = portLookupClasses(getRunningConfig(), iface.name);
  auto result =
      runCli({"config", "interface", iface.name, "lookup-class", "999"});
  EXPECT_NE(result.exitCode, 0)
      << "Expected non-zero exit for out-of-range lookup-class id 999";
  EXPECT_EQ(portLookupClasses(getRunningConfig(), iface.name), before)
      << "Rejected command must not change the running config";
}

// A list with an invalid id is rejected entirely; the running config is
// untouched.
TEST_F(ConfigInterfaceLookupClassTest, RejectsInvalidIdInList) {
  Interface iface = findFirstEthInterface();
  auto before = portLookupClasses(getRunningConfig(), iface.name);
  auto result =
      runCli({"config", "interface", iface.name, "lookup-class", "10,999"});
  EXPECT_NE(result.exitCode, 0)
      << "Expected non-zero exit for lookup-class list containing 999";
  EXPECT_EQ(portLookupClasses(getRunningConfig(), iface.name), before)
      << "Rejected command must not change the running config";
}

// Duplicate ids in the list are rejected; the running config is untouched.
TEST_F(ConfigInterfaceLookupClassTest, RejectsDuplicateIdsInList) {
  Interface iface = findFirstEthInterface();
  auto before = portLookupClasses(getRunningConfig(), iface.name);
  auto result =
      runCli({"config", "interface", iface.name, "lookup-class", "10,11,10"});
  EXPECT_NE(result.exitCode, 0)
      << "Expected non-zero exit for duplicate lookup-class ids";
  EXPECT_EQ(portLookupClasses(getRunningConfig(), iface.name), before)
      << "Rejected command must not change the running config";
}

// Set a single id, commit, verify it in the running config, then delete +
// commit and verify the running config is restored.
TEST_F(ConfigInterfaceLookupClassTest, SetThenDeleteLookupClass) {
  std::string ifaceName = findEthPortWithEmptyLookupClasses();
  if (ifaceName.empty()) {
    GTEST_SKIP() << "Every eth port already has lookupClasses configured";
  }

  setLookupClassAndVerify(ifaceName, "10", {10});
  clearLookupClassAndVerify(ifaceName);
}

// Set the full queue-per-host pool, commit, verify, then clear + verify.
TEST_F(ConfigInterfaceLookupClassTest, SetThenDeleteLookupClassList) {
  std::string ifaceName = findEthPortWithEmptyLookupClasses();
  if (ifaceName.empty()) {
    GTEST_SKIP() << "Every eth port already has lookupClasses configured";
  }

  setLookupClassAndVerify(ifaceName, "10,11,12,13,14", {10, 11, 12, 13, 14});
  clearLookupClassAndVerify(ifaceName);
}

// Delete on a port whose lookupClasses list is already empty succeeds and
// stages nothing, so the running config is unchanged. (No commit: an
// unchanged session has nothing to commit.)
TEST_F(ConfigInterfaceLookupClassTest, DeleteLookupClassIdempotent) {
  std::string ifaceName = findEthPortWithEmptyLookupClasses();
  if (ifaceName.empty()) {
    GTEST_SKIP() << "Every eth port already has lookupClasses configured";
  }

  auto delResult = runCli({"delete", "interface", ifaceName, "lookup-class"});
  EXPECT_EQ(delResult.exitCode, 0)
      << "Expected delete on empty lookupClasses to succeed: "
      << delResult.stderr;
  EXPECT_TRUE(portLookupClasses(getRunningConfig(), ifaceName).empty());
}
