// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/**
 * End-to-end test for `fboss2-dev delete vlan <id>`:
 *   program an unreferenced VLAN → check present → delete → check gone.
 *
 * Refusal / cascade edge cases are covered by CmdDeleteVlan unit tests.
 *
 * Requirements:
 *   - FBOSS agent is running with a valid configuration
 *   - Test is run as root (or with sudo) on a DUT
 */

#include <folly/ScopeGuard.h>
#include <folly/logging/xlog.h>
#include <gtest/gtest.h>
#include <string>
#include "fboss/cli/fboss2/test/integration_test/Fboss2IntegrationTest.h"

using namespace facebook::fboss;

class DeleteVlanTest : public Fboss2IntegrationTest {
 protected:
  static bool hasVlan(const folly::dynamic& config, int vlanId) {
    if (!config.isObject() || !config.count("sw") ||
        !config["sw"].count("vlans")) {
      return false;
    }
    for (const auto& v : config["sw"]["vlans"]) {
      if (v.count("id") && v["id"].asInt() == vlanId) {
        return true;
      }
    }
    return false;
  }

  static bool hasInterfaceForVlan(const folly::dynamic& config, int vlanId) {
    if (!config.isObject() || !config.count("sw") ||
        !config["sw"].count("interfaces")) {
      return false;
    }
    for (const auto& i : config["sw"]["interfaces"]) {
      if (i.count("vlanID") && i["vlanID"].asInt() == vlanId) {
        return true;
      }
    }
    return false;
  }

  static bool vlanPresent(const folly::dynamic& config, int vlanId) {
    return hasVlan(config, vlanId) && hasInterfaceForVlan(config, vlanId);
  }

  static bool vlanGone(const folly::dynamic& config, int vlanId) {
    return !hasVlan(config, vlanId) && !hasInterfaceForVlan(config, vlanId);
  }
};

TEST_F(DeleteVlanTest, ProgramThenDeleteVlan) {
  const int vlanId = pickUnusedVlanId();
  if (vlanId == 0) {
    GTEST_SKIP() << "No VLAN ID free in [" << kTestVlanMin << ", "
                 << kTestVlanMax << "] on this switch";
  }
  // Step 3 deletes the VLAN on the happy path; this guards the case where the
  // test aborts earlier and would otherwise leak the interface's TUN device.
  SCOPE_EXIT {
    deleteVlanIfPresent(vlanId);
  };
  const std::string id = std::to_string(vlanId);

  XLOG(INFO) << "[Step 1] Program unreferenced VLAN " << id << " and commit";
  // VLAN + barebone interface only (no static MAC).
  ensureUnderlayIntfId(vlanId);
  commitConfig();
  waitForAgentReady();

  XLOG(INFO) << "[Step 2] Check VLAN " << id << " is present";
  {
    auto config = waitForRunningConfig(
        [&](const folly::dynamic& c) { return vlanPresent(c, vlanId); });
    ASSERT_TRUE(vlanPresent(config, vlanId))
        << "VLAN " << id << " (and barebone interface) not in running config";
  }

  XLOG(INFO) << "[Step 3] Delete VLAN " << id << " and commit";
  auto result = runCli({"delete", "vlan", id});
  if (result.exitCode != 0) {
    discardSession();
  }
  ASSERT_EQ(result.exitCode, 0)
      << "stdout=" << result.stdout << " stderr=" << result.stderr;
  commitConfig();
  waitForAgentReady();

  XLOG(INFO) << "[Step 4] Check VLAN " << id << " is gone";
  auto config = waitForRunningConfig(
      [&](const folly::dynamic& c) { return vlanGone(c, vlanId); });
  EXPECT_TRUE(vlanGone(config, vlanId))
      << "VLAN " << id << " (or its interface) still in running config";
}
