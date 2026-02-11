// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/**
 * End-to-end test for 'fboss2-dev config interface <name> description <string>'
 *
 * This test:
 * 1. Picks an interface from the running system
 * 2. Gets the current description
 * 3. Sets a new description
 * 4. Verifies the description was set correctly via 'fboss2-dev show interface'
 * 5. Restores the original description
 *
 * Requirements:
 * - FBOSS agent must be running with a valid configuration
 * - The test must be run as root (or with appropriate permissions)
 */

#include <folly/logging/xlog.h>
#include <gtest/gtest.h>
#include <string>
#include "fboss/cli/test/CliTest.h"

using namespace facebook::fboss;

class ConfigInterfaceDescriptionTest : public CliTest {
 protected:
  void setInterfaceDescription(
      const std::string& interfaceName,
      const std::string& description) {
    auto result = runCli(
        {"config", "interface", interfaceName, "description", description});
    ASSERT_EQ(result.exitCode, 0)
        << "Failed to set description: " << result.stderr;
    commitConfig();
  }
};

TEST_F(ConfigInterfaceDescriptionTest, SetAndVerifyDescription) {
  // Step 1: Find an interface to test with
  XLOG(INFO) << "[Step 1] Finding an interface to test...";
  Interface interface = findFirstEthInterface();
  XLOG(INFO) << "  Using interface: " << interface.name << " (VLAN: "
             << (interface.vlan.has_value() ? std::to_string(*interface.vlan)
                                            : "none")
             << ")";

  // Step 2: Get the current description
  XLOG(INFO) << "[Step 2] Getting current description...";
  std::string originalDescription = interface.description;
  XLOG(INFO) << "  Current description: '" << originalDescription << "'";

  // Step 3: Set a new description
  std::string testDescription = "CLI_E2E_TEST_DESCRIPTION";
  if (originalDescription == testDescription) {
    testDescription = "CLI_E2E_TEST_DESCRIPTION_ALT";
  }
  XLOG(INFO) << "[Step 3] Setting description to '" << testDescription
             << "'...";
  setInterfaceDescription(interface.name, testDescription);
  XLOG(INFO) << "  Description set to '" << testDescription << "'";

  // Step 4: Verify description via 'show interface'
  XLOG(INFO) << "[Step 4] Verifying description via 'show interface'...";
  Interface updatedInterface = getInterfaceInfo(interface.name);
  EXPECT_EQ(updatedInterface.description, testDescription)
      << "Expected description '" << testDescription << "', got '"
      << updatedInterface.description << "'";
  XLOG(INFO) << "  Verified: Description is '" << updatedInterface.description
             << "'";

  // Step 5: Restore original description
  XLOG(INFO) << "[Step 5] Restoring original description ('"
             << originalDescription << "')...";
  setInterfaceDescription(interface.name, originalDescription);
  XLOG(INFO) << "  Restored description to '" << originalDescription << "'";

  // Verify restoration
  Interface restoredInterface = getInterfaceInfo(interface.name);
  if (restoredInterface.description != originalDescription) {
    XLOG(WARN) << "  WARNING: Restoration may have failed. Current: '"
               << restoredInterface.description << "'";
  }

  XLOG(INFO) << "TEST PASSED";
}
