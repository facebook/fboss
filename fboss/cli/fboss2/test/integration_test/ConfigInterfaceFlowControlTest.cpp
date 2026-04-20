// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/**
 * End-to-end tests for:
 *   'fboss2-dev config interface <name> flow-control-rx <value>'
 *   'fboss2-dev config interface <name> flow-control-tx <value>'
 *
 * These tests:
 * 1. Pick an interface from the running system
 * 2. Enable flow-control-rx / flow-control-tx
 * 3. Verify the CLI command succeeds and the config commits without error
 * 4. Restore flow-control to disable
 *
 * Requirements:
 * - FBOSS agent must be running with a valid configuration
 * - The test must be run as root (or with appropriate permissions)
 *
 * Note: The Interface struct does not expose pause/flow-control fields, so
 * verification is performed by checking the CLI exit code and commit success.
 * Unit tests cover correctness of the flow-control values exhaustively.
 */

#include <folly/logging/xlog.h>
#include <gtest/gtest.h>
#include <string>
#include "fboss/cli/fboss2/test/integration_test/Fboss2IntegrationTest.h"

using namespace facebook::fboss;

class ConfigInterfaceFlowControlTest : public Fboss2IntegrationTest {
 protected:
  void setFlowControlRx(
      const std::string& interfaceName,
      const std::string& value) {
    auto result = runCli(
        {"config", "interface", interfaceName, "flow-control-rx", value});
    ASSERT_EQ(result.exitCode, 0) << "Failed to set flow-control-rx to '"
                                  << value << "': " << result.stderr;
    commitConfig();
  }

  void setFlowControlTx(
      const std::string& interfaceName,
      const std::string& value) {
    auto result = runCli(
        {"config", "interface", interfaceName, "flow-control-tx", value});
    ASSERT_EQ(result.exitCode, 0) << "Failed to set flow-control-tx to '"
                                  << value << "': " << result.stderr;
    commitConfig();
  }
};

TEST_F(ConfigInterfaceFlowControlTest, SetAndVerifyFlowControlRx) {
  // Step 1: Find an interface to test with
  XLOG(INFO) << "[Step 1] Finding an interface to test...";
  Interface interface = findFirstEthInterface();
  XLOG(INFO) << "  Using interface: " << interface.name << " (VLAN: "
             << (interface.vlan.has_value() ? std::to_string(*interface.vlan)
                                            : "none")
             << ")";

  // Step 2: Enable flow-control-rx
  XLOG(INFO) << "[Step 2] Setting flow-control-rx to 'enable'...";
  setFlowControlRx(interface.name, "enable");
  XLOG(INFO) << "  flow-control-rx set to 'enable'";

  // Step 3: Restore flow-control-rx to disable
  // Note: The Interface struct does not expose pause/flow-control fields, so
  // deep state verification is not performed here. The primary goal is
  // confirming the CLI command and commit succeed (exit code 0).
  XLOG(INFO) << "[Step 3] Restoring flow-control-rx to 'disable'...";
  setFlowControlRx(interface.name, "disable");
  XLOG(INFO) << "  flow-control-rx restored to 'disable'";

  XLOG(INFO) << "TEST PASSED";
}

TEST_F(ConfigInterfaceFlowControlTest, InvalidFlowControlRxValueIsRejected) {
  Interface iface = findFirstEthInterface();
  auto result =
      runCli({"config", "interface", iface.name, "flow-control-rx", "yes"});
  EXPECT_NE(result.exitCode, 0)
      << "Expected non-zero exit for invalid flow-control-rx value 'yes'";
}

TEST_F(ConfigInterfaceFlowControlTest, InvalidFlowControlTxValueIsRejected) {
  Interface iface = findFirstEthInterface();
  auto result =
      runCli({"config", "interface", iface.name, "flow-control-tx", "on"});
  EXPECT_NE(result.exitCode, 0)
      << "Expected non-zero exit for invalid flow-control-tx value 'on'";
}

TEST_F(ConfigInterfaceFlowControlTest, SetAndVerifyFlowControlTx) {
  // Step 1: Find an interface to test with
  XLOG(INFO) << "[Step 1] Finding an interface to test...";
  Interface interface = findFirstEthInterface();
  XLOG(INFO) << "  Using interface: " << interface.name << " (VLAN: "
             << (interface.vlan.has_value() ? std::to_string(*interface.vlan)
                                            : "none")
             << ")";

  // Step 2: Enable flow-control-tx
  XLOG(INFO) << "[Step 2] Setting flow-control-tx to 'enable'...";
  setFlowControlTx(interface.name, "enable");
  XLOG(INFO) << "  flow-control-tx set to 'enable'";

  // Step 3: Restore flow-control-tx to disable
  // Note: The Interface struct does not expose pause/flow-control fields, so
  // deep state verification is not performed here. The primary goal is
  // confirming the CLI command and commit succeed (exit code 0).
  XLOG(INFO) << "[Step 3] Restoring flow-control-tx to 'disable'...";
  setFlowControlTx(interface.name, "disable");
  XLOG(INFO) << "  flow-control-tx restored to 'disable'";

  XLOG(INFO) << "TEST PASSED";
}
