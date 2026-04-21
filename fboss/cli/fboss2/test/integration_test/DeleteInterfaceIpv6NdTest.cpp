// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/**
 * End-to-end tests for 'fboss2-dev delete interface <name> ipv6 nd ...'
 *
 * These tests:
 *  1. Pick an interface from the running system
 *  2. Set NDP attributes via the config CLI
 *  3. Delete (reset to default) those attributes via the delete CLI
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

class DeleteInterfaceIpv6NdTest : public Fboss2IntegrationTest {
 protected:
  // Set an NDP attribute via the config CLI before deleting it.
  void setNdAttr(
      const std::string& interfaceName,
      const std::string& attr,
      const std::string& value = "") {
    std::vector<std::string> args = {
        "config", "interface", interfaceName, "ipv6", "nd", attr};
    if (!value.empty()) {
      args.push_back(value);
    }
    auto result = runCli(args);
    ASSERT_EQ(result.exitCode, 0)
        << "Failed to set nd " << attr << ": " << result.stderr;
    commitConfig();
  }

  // Delete (reset) an NDP attribute via the delete CLI.
  void deleteNdAttr(const std::string& interfaceName, const std::string& attr) {
    auto result =
        runCli({"delete", "interface", interfaceName, "ipv6", "nd", attr});
    ASSERT_EQ(result.exitCode, 0)
        << "Failed to delete nd " << attr << ": " << result.stderr;
    commitConfig();
  }
};

// ---------------------------------------------------------------------------
// Test: set ra-interval then delete it
// ---------------------------------------------------------------------------

TEST_F(DeleteInterfaceIpv6NdTest, DeleteRaInterval) {
  XLOG(INFO) << "[Step 1] Finding an interface to test...";
  Interface iface = findFirstEthInterface();
  XLOG(INFO) << "  Using interface: " << iface.name;

  XLOG(INFO) << "[Step 2] Setting ra-interval=30...";
  setNdAttr(iface.name, "ra-interval", "30");

  XLOG(INFO) << "[Step 3] Deleting ra-interval (reset to default)...";
  deleteNdAttr(iface.name, "ra-interval");

  XLOG(INFO) << "  ra-interval deleted successfully (exit 0, config committed)";
  XLOG(INFO) << "TEST PASSED";
}

// ---------------------------------------------------------------------------
// Test: set hop-limit then delete it
// ---------------------------------------------------------------------------

TEST_F(DeleteInterfaceIpv6NdTest, DeleteHopLimit) {
  XLOG(INFO) << "[Step 1] Finding an interface to test...";
  Interface iface = findFirstEthInterface();
  XLOG(INFO) << "  Using interface: " << iface.name;

  XLOG(INFO) << "[Step 2] Setting hop-limit=64...";
  setNdAttr(iface.name, "hop-limit", "64");

  XLOG(INFO) << "[Step 3] Deleting hop-limit (reset to default 255)...";
  deleteNdAttr(iface.name, "hop-limit");

  XLOG(INFO) << "  hop-limit deleted successfully";
  XLOG(INFO) << "TEST PASSED";
}

// ---------------------------------------------------------------------------
// Test: set managed-config-flag then delete it
// ---------------------------------------------------------------------------

TEST_F(DeleteInterfaceIpv6NdTest, DeleteManagedConfigFlag) {
  XLOG(INFO) << "[Step 1] Finding an interface to test...";
  Interface iface = findFirstEthInterface();
  XLOG(INFO) << "  Using interface: " << iface.name;

  XLOG(INFO) << "[Step 2] Setting managed-config-flag...";
  setNdAttr(iface.name, "managed-config-flag");

  XLOG(INFO) << "[Step 3] Deleting managed-config-flag (reset to false)...";
  deleteNdAttr(iface.name, "managed-config-flag");

  XLOG(INFO) << "  managed-config-flag deleted successfully";
  XLOG(INFO) << "TEST PASSED";
}

// ---------------------------------------------------------------------------
// Test: set ra-address then delete it
// ---------------------------------------------------------------------------

TEST_F(DeleteInterfaceIpv6NdTest, DeleteRaAddress) {
  XLOG(INFO) << "[Step 1] Finding an interface to test...";
  Interface iface = findFirstEthInterface();
  XLOG(INFO) << "  Using interface: " << iface.name;

  const std::string raAddr = "2001:db8::1";

  XLOG(INFO) << "[Step 2] Setting ra-address to " << raAddr << "...";
  setNdAttr(iface.name, "ra-address", raAddr);

  XLOG(INFO) << "[Step 3] Deleting ra-address (clearing the optional)...";
  deleteNdAttr(iface.name, "ra-address");

  XLOG(INFO) << "  ra-address deleted successfully";
  XLOG(INFO) << "TEST PASSED";
}

// ---------------------------------------------------------------------------
// Test: unknown attribute is rejected with non-zero exit code
// ---------------------------------------------------------------------------

TEST_F(DeleteInterfaceIpv6NdTest, DeleteUnknownAttrRejected) {
  XLOG(INFO) << "[Step 1] Finding an interface to test...";
  Interface iface = findFirstEthInterface();
  XLOG(INFO) << "  Using interface: " << iface.name;

  XLOG(INFO)
      << "[Step 2] Attempting to delete unknown attribute 'bogus-attr'...";
  auto result =
      runCli({"delete", "interface", iface.name, "ipv6", "nd", "bogus-attr"});
  EXPECT_NE(result.exitCode, 0)
      << "Expected non-zero exit for unknown nd attribute";
  XLOG(INFO) << "  Correctly rejected with exit code " << result.exitCode;
  XLOG(INFO) << "TEST PASSED";
}
