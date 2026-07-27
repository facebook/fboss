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

#include <folly/logging/xlog.h>
#include <gtest/gtest.h>
#include <set>
#include <string>
#include "fboss/cli/fboss2/test/integration_test/Fboss2IntegrationTest.h"

using namespace facebook::fboss;

class DeleteVlanTest : public Fboss2IntegrationTest {
 protected:
  // First VLAN ID in [2, 4094] unused in the running config.
  int pickUnusedVlanId() {
    auto config = getRunningConfig();
    std::set<int> used;
    if (config.isObject() && config.count("sw")) {
      const auto& sw = config["sw"];
      if (sw.count("defaultVlan")) {
        used.insert(sw["defaultVlan"].asInt());
      }
      if (sw.count("ports")) {
        for (const auto& p : sw["ports"]) {
          if (p.count("ingressVlan")) {
            used.insert(p["ingressVlan"].asInt());
          }
        }
      }
      if (sw.count("vlanPorts")) {
        for (const auto& vp : sw["vlanPorts"]) {
          if (vp.count("vlanID")) {
            used.insert(vp["vlanID"].asInt());
          }
        }
      }
      if (sw.count("interfaces")) {
        for (const auto& i : sw["interfaces"]) {
          if (i.count("vlanID")) {
            used.insert(i["vlanID"].asInt());
          }
        }
      }
      if (sw.count("vlans")) {
        for (const auto& v : sw["vlans"]) {
          if (v.count("id")) {
            used.insert(v["id"].asInt());
          }
        }
      }
    }
    for (int id = 2; id <= 4094; ++id) {
      if (!used.count(id)) {
        return id;
      }
    }
    return 0;
  }

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
  ASSERT_NE(vlanId, 0) << "no free VLAN ID in [2, 4094]";
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
