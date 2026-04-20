// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/**
 * End-to-end tests for the 'fboss2-dev config interface <name>
 * lldp-expected-* <value>' family of commands.
 *
 * Covered attributes and their LLDPTag mappings:
 *   lldp-expected-value       → PORT
 *   lldp-expected-chassis     → CHASSIS
 *   lldp-expected-ttl         → TTL
 *   lldp-expected-port-desc   → PORT_DESC
 *   lldp-expected-system-name → SYSTEM_NAME
 *   lldp-expected-system-desc → SYSTEM_DESC
 *
 * Each positive test:
 *   1. Picks an interface from the running system
 *   2. Sets the LLDP expected value via the CLI
 *   3. Verifies the CLI command succeeds and the config commits without error
 *   4. Overwrites with a different value to confirm idempotent behaviour
 *
 * Negative tests verify that an empty value is rejected (non-zero exit code).
 *
 * Note on restoration: There is no CLI command to delete an individual
 * lldp-expected-value entry once committed. Unit tests cover correctness of
 * value handling exhaustively. Integration tests focus on CLI acceptance and
 * commit success.
 */

#include <folly/logging/xlog.h>
#include <gtest/gtest.h>
#include <string>
#include <vector>
#include "fboss/cli/fboss2/test/integration_test/Fboss2IntegrationTest.h"

using namespace facebook::fboss;

class ConfigInterfaceLldpExpectedValueTest : public Fboss2IntegrationTest {
 protected:
  void setLldpExpectedValue(
      const std::string& interfaceName,
      const std::string& attr,
      const std::string& value) {
    auto result = runCli({"config", "interface", interfaceName, attr, value});
    ASSERT_EQ(result.exitCode, 0)
        << "Failed to set " << attr << "='" << value << "': " << result.stderr;
    commitConfig();
  }

  // Runs a test for a single lldp-expected-* attribute:
  //   sets a value, verifies success, then overwrites with an alt value.
  void runPositiveTest(
      const std::string& attr,
      const std::string& testValue,
      const std::string& altValue) {
    Interface iface = findFirstEthInterface();
    XLOG(INFO) << "[" << attr << "] Using interface: " << iface.name;

    XLOG(INFO) << "  Setting " << attr << "='" << testValue << "'";
    setLldpExpectedValue(iface.name, attr, testValue);
    XLOG(INFO) << "  Set OK";

    XLOG(INFO) << "  Overwriting with '" << altValue << "'";
    setLldpExpectedValue(iface.name, attr, altValue);
    XLOG(INFO) << "  Overwrite OK";

    XLOG(WARN) << "  Note: test artifact '" << altValue
               << "' remains in config (no delete command available)";
  }
};

// ─── Positive tests
// ───────────────────────────────────────────────────────────

TEST_F(ConfigInterfaceLldpExpectedValueTest, SetLldpExpectedPort) {
  runPositiveTest(
      "lldp-expected-value", "CLI_E2E_LLDP_PORT", "CLI_E2E_LLDP_PORT_ALT");
  XLOG(INFO) << "TEST PASSED";
}

TEST_F(ConfigInterfaceLldpExpectedValueTest, SetLldpExpectedChassis) {
  runPositiveTest(
      "lldp-expected-chassis",
      "CLI_E2E_LLDP_CHASSIS",
      "CLI_E2E_LLDP_CHASSIS_ALT");
  XLOG(INFO) << "TEST PASSED";
}

TEST_F(ConfigInterfaceLldpExpectedValueTest, SetLldpExpectedTtl) {
  runPositiveTest("lldp-expected-ttl", "120", "180");
  XLOG(INFO) << "TEST PASSED";
}

TEST_F(ConfigInterfaceLldpExpectedValueTest, SetLldpExpectedPortDesc) {
  runPositiveTest(
      "lldp-expected-port-desc",
      "CLI_E2E_LLDP_PORT_DESC",
      "CLI_E2E_LLDP_PORT_DESC_ALT");
  XLOG(INFO) << "TEST PASSED";
}

TEST_F(ConfigInterfaceLldpExpectedValueTest, SetLldpExpectedSystemName) {
  runPositiveTest(
      "lldp-expected-system-name",
      "CLI_E2E_LLDP_SYS_NAME",
      "CLI_E2E_LLDP_SYS_NAME_ALT");
  XLOG(INFO) << "TEST PASSED";
}

TEST_F(ConfigInterfaceLldpExpectedValueTest, SetLldpExpectedSystemDesc) {
  runPositiveTest(
      "lldp-expected-system-desc",
      "CLI_E2E_LLDP_SYS_DESC",
      "CLI_E2E_LLDP_SYS_DESC_ALT");
  XLOG(INFO) << "TEST PASSED";
}

// ─── Negative tests
// ───────────────────────────────────────────────────────────

TEST_F(ConfigInterfaceLldpExpectedValueTest, EmptyPortValueIsRejected) {
  Interface iface = findFirstEthInterface();
  auto result =
      runCli({"config", "interface", iface.name, "lldp-expected-value", ""});
  EXPECT_NE(result.exitCode, 0)
      << "Expected non-zero exit for empty lldp-expected-value";
}

TEST_F(ConfigInterfaceLldpExpectedValueTest, EmptyChassisValueIsRejected) {
  Interface iface = findFirstEthInterface();
  auto result =
      runCli({"config", "interface", iface.name, "lldp-expected-chassis", ""});
  EXPECT_NE(result.exitCode, 0)
      << "Expected non-zero exit for empty lldp-expected-chassis";
}

TEST_F(ConfigInterfaceLldpExpectedValueTest, EmptySystemNameValueIsRejected) {
  Interface iface = findFirstEthInterface();
  auto result = runCli(
      {"config", "interface", iface.name, "lldp-expected-system-name", ""});
  EXPECT_NE(result.exitCode, 0)
      << "Expected non-zero exit for empty lldp-expected-system-name";
}
