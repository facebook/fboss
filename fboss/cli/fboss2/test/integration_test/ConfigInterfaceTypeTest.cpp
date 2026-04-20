// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/**
 * End-to-end test for 'fboss2-dev config interface <name> type <value>'
 *
 * This test:
 * 1. Picks an interface from the running system
 * 2. Sets the interface type to routed-port (the standard default)
 * 3. Verifies the CLI command succeeds and the config commits without error
 * 4. Restores the type to routed-port (no-op since it is the default, but the
 *    restore step is included for consistency with other tests)
 *
 * Requirements:
 * - FBOSS agent must be running with a valid configuration
 * - The test must be run as root (or with appropriate permissions)
 *
 * Note: The Interface struct does not expose a portType field, so verification
 * is performed by checking the CLI exit code and commit success. Unit tests
 * cover correctness of the type values exhaustively.
 */

#include <folly/logging/xlog.h>
#include <gtest/gtest.h>
#include <string>
#include "fboss/cli/fboss2/test/integration_test/Fboss2IntegrationTest.h"

using namespace facebook::fboss;

class ConfigInterfaceTypeTest : public Fboss2IntegrationTest {
 protected:
  void setInterfaceType(
      const std::string& interfaceName,
      const std::string& type) {
    auto result = runCli({"config", "interface", interfaceName, "type", type});
    ASSERT_EQ(result.exitCode, 0) << "Failed to set interface type to '" << type
                                  << "': " << result.stderr;
    commitConfig();
  }
};

TEST_F(ConfigInterfaceTypeTest, InvalidTypeIsRejected) {
  Interface iface = findFirstEthInterface();
  auto result =
      runCli({"config", "interface", iface.name, "type", "fabric-port"});
  EXPECT_NE(result.exitCode, 0)
      << "Expected non-zero exit for invalid type 'fabric-port'";
}

TEST_F(ConfigInterfaceTypeTest, EmptyTypeIsRejected) {
  Interface iface = findFirstEthInterface();
  auto result = runCli({"config", "interface", iface.name, "type", ""});
  EXPECT_NE(result.exitCode, 0)
      << "Expected non-zero exit for empty type value";
}

TEST_F(ConfigInterfaceTypeTest, SetAndVerifyTypeRoutedPort) {
  // Step 1: Find a switched interface to test with (needs VLAN to restore
  // later)
  XLOG(INFO) << "[Step 1] Finding a switched interface to test...";
  Interface interface = findFirstEthInterface();
  ASSERT_TRUE(interface.vlan.has_value())
      << "Expected interface with a VLAN for proper restore";
  int originalVlan = *interface.vlan;
  XLOG(INFO) << "  Using interface: " << interface.name
             << " (VLAN: " << originalVlan << ")";

  // Step 2: Set the interface type to routed-port
  XLOG(INFO) << "[Step 2] Setting interface type to 'routed-port'...";
  setInterfaceType(interface.name, "routed-port");
  XLOG(INFO) << "  Interface type set to 'routed-port'";

  // Step 3: Restore to switchport with the original VLAN.
  // 'type routed-port' removes the VlanPort entry and clears ingressVlan,
  // so simply re-applying 'type routed-port' is a no-op and leaves the
  // interface as routed. Restoring via 'switchport access vlan' correctly
  // re-adds the VlanPort entry and resets ingressVlan, leaving the agent
  // in the same state as before this test ran.
  XLOG(INFO) << "[Step 3] Restoring interface to switchport (VLAN "
             << originalVlan << ")...";
  auto result = runCli(
      {"config",
       "interface",
       interface.name,
       "switchport",
       "access",
       "vlan",
       std::to_string(originalVlan)});
  ASSERT_EQ(result.exitCode, 0)
      << "Failed to restore switchport: " << result.stderr;
  commitConfig();
  XLOG(INFO) << "  Interface restored to switchport (VLAN " << originalVlan
             << ")";

  XLOG(INFO) << "TEST PASSED";
}
