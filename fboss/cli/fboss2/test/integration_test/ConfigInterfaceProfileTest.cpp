// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/**
 * End-to-end tests for 'fboss2-dev config interface <name> profile <P>'.
 *
 * Mirrors ConfigInterfaceSpeedTest: supported profiles are discovered
 * dynamically from the CLI error message, a different profile is picked
 * to create a real before/after delta, and the agent is verified via a
 * direct getAllPortInfo() thrift call with a wait-retry loop.
 *
 * Requirements:
 * - FBOSS agent must be running with a valid configuration
 * - The test must be run as root (or with appropriate permissions)
 */

#include <folly/String.h>
#include <folly/logging/xlog.h>
#include <gtest/gtest.h>
#include <string>
#include <vector>
#include "fboss/cli/fboss2/test/integration_test/Fboss2IntegrationTest.h"

using namespace facebook::fboss;

class ConfigInterfaceProfileTest : public Fboss2IntegrationTest {
 protected:
  /**
   * Set a profile on an interface and commit the config.
   * Only commits if the CLI set succeeds (mirrors setInterfaceSpeed).
   */
  Result setInterfaceProfile(
      const std::string& interfaceName,
      const std::string& profile) {
    auto result =
        runCli({"config", "interface", interfaceName, "profile", profile});
    if (result.exitCode == 0) {
      commitConfig();
    }
    return result;
  }

  /**
   * Extract the list of supported profiles from a CLI error message.
   * Error format: "Supported profiles: [PROFILE_A, PROFILE_B, ...]"
   */
  std::vector<std::string> extractSupportedProfiles(
      const std::string& errorMessage) const {
    std::vector<std::string> profiles;
    auto pos = errorMessage.find("Supported profiles: [");
    if (pos == std::string::npos) {
      return profiles;
    }
    pos += 21; // skip "Supported profiles: ["
    auto end = errorMessage.find(']', pos);
    if (end == std::string::npos) {
      return profiles;
    }
    auto listStr = errorMessage.substr(pos, end - pos);
    folly::splitTo<std::string>(", ", listStr, std::back_inserter(profiles));
    return profiles;
  }
};

// Test 1: Set a valid (different) profile and verify via direct thrift call.
// Discovers supported profiles from the per-port validation error message,
// then picks a profile different from the current one.
// Skips when the port supports only one profile (no delta possible).
TEST_F(ConfigInterfaceProfileTest, SetValidProfileVerifiedViaThrift) {
  XLOG(INFO) << "[Step 1] Finding an interface to test...";
  Interface iface = findFirstEthInterface();
  XLOG(INFO) << "  Using interface: " << iface.name;

  // Read current profile from agent
  XLOG(INFO) << "[Step 2] Reading current profile/speed from agent...";
  auto original = getPortRunningInfo(iface.name);
  XLOG(INFO) << "  profile=" << original.profileId
             << "  speed=" << original.speedMbps << " Mbps";

  // Probe with a valid enum that is likely unsupported per-port (10G on a
  // high-speed port) to trigger per-port validateProfile(), which emits:
  //   "Supported profiles: [PROFILE_A, PROFILE_B, ...]"
  // We cannot use a bogus string because parseProfile() rejects it before
  // per-port validation runs.
  XLOG(INFO) << "[Step 3] Discovering supported profiles via per-port probe...";
  const std::string kProbeA = "PROFILE_10G_1_NRZ_NOFEC";
  const std::string kProbeB = "PROFILE_100G_4_NRZ_RS528_COPPER";
  const std::string probeProfile =
      (original.profileId == kProbeA) ? kProbeB : kProbeA;

  auto probeResult = setInterfaceProfile(iface.name, probeProfile);
  std::vector<std::string> supported;
  if (probeResult.exitCode != 0) {
    // Expected: per-port validation rejected it → extract supported list
    supported = extractSupportedProfiles(probeResult.stderr);
    ASSERT_FALSE(supported.empty())
        << "Could not extract supported profiles from probe error: "
        << probeResult.stderr;
  } else {
    // Probe succeeded: port supports both the current and probe profiles
    setInterfaceProfile(iface.name, original.profileId); // restore
    supported = {original.profileId, probeProfile};
  }
  XLOG(INFO) << "  Supported: " << folly::join(", ", supported);

  // Pick a profile different from the current one
  std::string testProfile;
  for (const auto& p : supported) {
    if (p != original.profileId) {
      testProfile = p;
      break;
    }
  }
  if (testProfile.empty()) {
    GTEST_SKIP() << "Port supports only one profile (" << original.profileId
                 << "); cannot test a profile change";
  }
  XLOG(INFO) << "  Will test changing to: " << testProfile;

  // Set the new profile
  XLOG(INFO) << "[Step 4] Setting profile to " << testProfile << "...";
  auto result = setInterfaceProfile(iface.name, testProfile);
  ASSERT_EQ(result.exitCode, 0)
      << "Failed to set profile to " << testProfile << ": " << result.stderr;

  // Verify via direct getAllPortInfo() call with wait-retry
  XLOG(INFO) << "[Step 5] Verifying profile via direct agent thrift call...";
  auto after =
      waitForPortRunningInfo(iface.name, [&testProfile](const auto& info) {
        return info.profileId == testProfile;
      });
  XLOG(INFO) << "  After: profile=" << after.profileId
             << "  speed=" << after.speedMbps << " Mbps";
  EXPECT_EQ(after.profileId, testProfile)
      << "Agent should report " << testProfile << " after CLI set";

  // Restore original profile
  XLOG(INFO) << "[Step 6] Restoring original profile (" << original.profileId
             << ")...";
  auto restore = setInterfaceProfile(iface.name, original.profileId);
  ASSERT_EQ(restore.exitCode, 0) << restore.stderr;
  XLOG(INFO) << "TEST PASSED";
}

// Test 2: Setting a completely invalid profile string is rejected before
// commit.
TEST_F(ConfigInterfaceProfileTest, SetInvalidProfileFails) {
  XLOG(INFO) << "[Step 1] Finding an interface to test...";
  Interface iface = findFirstEthInterface();
  XLOG(INFO) << "  Using interface: " << iface.name;

  XLOG(INFO)
      << "[Step 2] Setting invalid profile 'PROFILE_COMPLETELY_BOGUS_9999'...";
  auto result =
      setInterfaceProfile(iface.name, "PROFILE_COMPLETELY_BOGUS_9999");
  XLOG(INFO) << "  Exit code: " << result.exitCode;

  EXPECT_NE(result.exitCode, 0)
      << "Expected non-zero exit code for invalid profile";
  EXPECT_NE(result.stderr.find("not a valid PortProfileID"), std::string::npos)
      << "Expected 'not a valid PortProfileID' in stderr, got: "
      << result.stderr;

  XLOG(INFO) << "TEST PASSED";
}

// Test 3: The command accepts multiple space-separated port names.
TEST_F(ConfigInterfaceProfileTest, SetProfileMultiInterface) {
  XLOG(INFO) << "[Step 1] Finding an interface to test...";
  Interface iface = findFirstEthInterface();
  XLOG(INFO) << "  Using interface: " << iface.name;

  XLOG(INFO) << "[Step 2] Reading current profile from agent...";
  auto info = getPortRunningInfo(iface.name);
  XLOG(INFO) << "  Current profile: " << info.profileId;

  if (info.profileId == "PROFILE_DEFAULT") {
    GTEST_SKIP() << "Port has PROFILE_DEFAULT; no specific profile to set";
  }

  // Re-apply the current profile on a multi-port list (same port listed twice,
  // passed as separate CLI tokens so InterfacesConfig resolves each one).
  XLOG(INFO) << "[Step 3] Setting " << info.profileId
             << " on two-port list: " << iface.name << " " << iface.name;
  auto result = runCli(
      {"config",
       "interface",
       iface.name,
       iface.name,
       "profile",
       info.profileId});
  ASSERT_EQ(result.exitCode, 0) << result.stderr;
  commitConfig();

  // Verify via direct thrift with wait-retry
  XLOG(INFO) << "[Step 4] Verifying via agent thrift...";
  auto after = waitForPortRunningInfo(iface.name, [&info](const auto& i) {
    return i.profileId == info.profileId;
  });
  EXPECT_EQ(after.profileId, info.profileId)
      << "Agent should report " << info.profileId
      << " after multi-interface set";

  XLOG(INFO) << "TEST PASSED";
}
