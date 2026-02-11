// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/**
 * End-to-end test for the 'fboss2-dev config interface <name> mtu <N>' command.
 *
 * This test:
 * 1. Picks an interface from the running system
 * 2. Gets the current MTU value
 * 3. Sets a new MTU value
 * 4. Verifies the MTU was set correctly via 'fboss2-dev show interface'
 * 5. Verifies the MTU on the kernel interface via 'ip link'
 * 6. Restores the original MTU
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

class ConfigInterfaceMtuTest : public CliTest {
 protected:
  void setInterfaceMtu(const std::string& interfaceName, int mtu) {
    auto result = runCli(
        {"config", "interface", interfaceName, "mtu", std::to_string(mtu)});
    ASSERT_EQ(result.exitCode, 0) << "Failed to set MTU: " << result.stderr;
    commitConfig();
  }
};

TEST_F(ConfigInterfaceMtuTest, SetAndVerifyMtu) {
  // Step 1: Find an interface to test with
  XLOG(INFO) << "[Step 1] Finding an interface to test...";
  Interface interface = findFirstEthInterface();
  XLOG(INFO) << "  Using interface: " << interface.name << " (VLAN: "
             << (interface.vlan.has_value() ? std::to_string(*interface.vlan)
                                            : "none")
             << ")";

  // Step 2: Get the current MTU
  XLOG(INFO) << "[Step 2] Getting current MTU...";
  int originalMtu = interface.mtu;
  XLOG(INFO) << "  Current MTU: " << originalMtu;

  // Step 3: Set a new MTU (toggle between 1500 and 9000)
  int newMtu = (originalMtu != 9000) ? 9000 : 1500;
  XLOG(INFO) << "[Step 3] Setting MTU to " << newMtu << "...";
  setInterfaceMtu(interface.name, newMtu);
  XLOG(INFO) << "  MTU set to " << newMtu;

  // Step 4: Verify MTU via 'show interface'
  XLOG(INFO) << "[Step 4] Verifying MTU via 'show interface'...";
  Interface updatedInterface = getInterfaceInfo(interface.name);
  EXPECT_EQ(updatedInterface.mtu, newMtu)
      << "Expected MTU " << newMtu << ", got " << updatedInterface.mtu;
  XLOG(INFO) << "  Verified: MTU is " << updatedInterface.mtu;

  // Step 5: Verify kernel interface MTU
  XLOG(INFO) << "[Step 5] Verifying kernel interface MTU...";
  ASSERT_TRUE(interface.vlan.has_value());
  int kernelMtu = getKernelInterfaceMtu(*interface.vlan);
  if (kernelMtu > 0) {
    EXPECT_EQ(kernelMtu, newMtu)
        << "Kernel MTU is " << kernelMtu << ", expected " << newMtu;
    XLOG(INFO) << "  Verified: Kernel interface fboss" << *interface.vlan
               << " has MTU " << kernelMtu;
  } else {
    XLOG(INFO) << "  Skipped: Kernel interface fboss" << *interface.vlan
               << " not found";
  }

  // Step 6: Restore original MTU
  XLOG(INFO) << "[Step 6] Restoring original MTU (" << originalMtu << ")...";
  setInterfaceMtu(interface.name, originalMtu);
  XLOG(INFO) << "  Restored MTU to " << originalMtu;

  XLOG(INFO) << "TEST PASSED";
}
