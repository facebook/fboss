// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/**
 * End-to-end tests for 'fboss2-dev delete interface <name> <attr> [<attr>...]'
 *
 * These tests:
 *  1. Pick an interface from the running system
 *  2. Set one or more attributes via the config CLI
 *  3. Delete (reset to defaults) via the delete CLI
 *  4. Verify the command succeeds (exit code 0)
 *
 * Requirements:
 *  - FBOSS agent must be running with a valid configuration
 *  - The test must be run as root (or with appropriate permissions)
 */

#include <folly/logging/xlog.h>
#include <gtest/gtest.h>
#include <string>
#include "fboss/cli/fboss2/test/integration_test/Fboss2IntegrationTest.h"

using namespace facebook::fboss;

class DeleteInterfaceTest : public Fboss2IntegrationTest {
 protected:
  void setLoopbackMode(
      const std::string& interfaceName,
      const std::string& mode) {
    auto result =
        runCli({"config", "interface", interfaceName, "loopback-mode", mode});
    ASSERT_EQ(result.exitCode, 0)
        << "Failed to set loopback-mode=" << mode << ": " << result.stderr;
    commitConfig();
  }

  void setLldpExpectedValue(
      const std::string& interfaceName,
      const std::string& value) {
    auto result = runCli(
        {"config", "interface", interfaceName, "lldp-expected-value", value});
    ASSERT_EQ(result.exitCode, 0)
        << "Failed to set lldp-expected-value=" << value << ": "
        << result.stderr;
    commitConfig();
  }

  void deleteInterfaceAttrs(
      const std::string& interfaceName,
      const std::vector<std::string>& attrs) {
    std::vector<std::string> args = {"delete", "interface", interfaceName};
    for (const auto& attr : attrs) {
      args.push_back(attr);
    }
    auto result = runCli(args);
    ASSERT_EQ(result.exitCode, 0)
        << "Failed to delete interface attrs: " << result.stderr;
    commitConfig();
  }
};

// ---------------------------------------------------------------------------
// Test: set loopback-mode=PHY then delete it (reset to NONE)
// ---------------------------------------------------------------------------

TEST_F(DeleteInterfaceTest, DeleteLoopbackModePHY) {
  XLOG(INFO) << "[Step 1] Finding an interface to test...";
  Interface iface = findFirstEthInterface();
  XLOG(INFO) << "  Using interface: " << iface.name;

  XLOG(INFO) << "[Step 2] Setting loopback-mode=PHY...";
  setLoopbackMode(iface.name, "PHY");

  XLOG(INFO) << "[Step 3] Deleting loopback-mode (reset to NONE)...";
  deleteInterfaceAttrs(iface.name, {"loopback-mode"});

  XLOG(INFO)
      << "  loopback-mode deleted successfully (exit 0, config committed)";
  XLOG(INFO) << "TEST PASSED";
}

// ---------------------------------------------------------------------------
// Test: delete loopback-mode is idempotent (no loopback-mode set beforehand)
// ---------------------------------------------------------------------------

TEST_F(DeleteInterfaceTest, DeleteLoopbackModeIdempotent) {
  XLOG(INFO) << "[Step 1] Finding an interface to test...";
  Interface iface = findFirstEthInterface();
  XLOG(INFO) << "  Using interface: " << iface.name;

  XLOG(INFO) << "[Step 2] Deleting loopback-mode without setting it first...";
  deleteInterfaceAttrs(iface.name, {"loopback-mode"});

  XLOG(INFO) << "  Idempotent delete succeeded (exit 0)";
  XLOG(INFO) << "TEST PASSED";
}

// ---------------------------------------------------------------------------
// Test: set lldp-expected-value then delete it
// ---------------------------------------------------------------------------

TEST_F(DeleteInterfaceTest, DeleteLldpExpectedValue) {
  XLOG(INFO) << "[Step 1] Finding an interface to test...";
  Interface iface = findFirstEthInterface();
  XLOG(INFO) << "  Using interface: " << iface.name;

  XLOG(INFO) << "[Step 2] Setting lldp-expected-value=ge-0/0/0...";
  setLldpExpectedValue(iface.name, "ge-0/0/0");

  XLOG(INFO)
      << "[Step 3] Deleting lldp-expected-value (removing PORT entry)...";
  deleteInterfaceAttrs(iface.name, {"lldp-expected-value"});

  XLOG(INFO)
      << "  lldp-expected-value deleted successfully (exit 0, config committed)";
  XLOG(INFO) << "TEST PASSED";
}

// ---------------------------------------------------------------------------
// Test: delete lldp-expected-value is idempotent
// ---------------------------------------------------------------------------

TEST_F(DeleteInterfaceTest, DeleteLldpExpectedValueIdempotent) {
  XLOG(INFO) << "[Step 1] Finding an interface to test...";
  Interface iface = findFirstEthInterface();
  XLOG(INFO) << "  Using interface: " << iface.name;

  XLOG(INFO)
      << "[Step 2] Deleting lldp-expected-value without setting it first...";
  deleteInterfaceAttrs(iface.name, {"lldp-expected-value"});

  XLOG(INFO) << "  Idempotent delete succeeded (exit 0)";
  XLOG(INFO) << "TEST PASSED";
}

// ---------------------------------------------------------------------------
// Test: delete both loopback-mode and lldp-expected-value in one call
// ---------------------------------------------------------------------------

TEST_F(DeleteInterfaceTest, DeleteLoopbackModeAndLldpExpectedValue) {
  XLOG(INFO) << "[Step 1] Finding an interface to test...";
  Interface iface = findFirstEthInterface();
  XLOG(INFO) << "  Using interface: " << iface.name;

  XLOG(INFO) << "[Step 2] Setting loopback-mode=PHY and lldp-expected-value...";
  setLoopbackMode(iface.name, "PHY");
  setLldpExpectedValue(iface.name, "ge-0/0/0");

  XLOG(INFO)
      << "[Step 3] Deleting both loopback-mode and lldp-expected-value in one call...";
  deleteInterfaceAttrs(iface.name, {"loopback-mode", "lldp-expected-value"});

  XLOG(INFO) << "  Combined delete succeeded (exit 0, config committed)";
  XLOG(INFO) << "TEST PASSED";
}
