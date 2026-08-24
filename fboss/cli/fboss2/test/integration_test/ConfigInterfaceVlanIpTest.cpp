// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/**
 * End-to-end test for IP address add/remove on a VLAN SVI addressed as
 * "vlan<id>":
 *   create VLAN + SVI → add v4 + v6 → verify the SVI interface carries the
 *   address → delete both → verify gone → delete VLAN.
 *
 * Refusal edge cases are covered by CmdConfigInterfaceVlanIp unit tests.
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

class ConfigInterfaceVlanIpTest : public Fboss2IntegrationTest {
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

  static bool listHasAddress(
      const folly::dynamic& obj,
      const std::string& addr) {
    if (!obj.count("ipAddresses")) {
      return false;
    }
    for (const auto& a : obj["ipAddresses"]) {
      if (a.asString() == addr) {
        return true;
      }
    }
    return false;
  }

  // Whether the address appears on the SVI's interface entry — the field the
  // agent programs addresses from.
  static bool addressPresent(
      const folly::dynamic& config,
      int vlanId,
      const std::string& addr,
      bool onInterface) {
    if (!config.isObject() || !config.count("sw")) {
      return false;
    }
    const auto& sw = config["sw"];
    bool intfHas = false;
    if (sw.count("interfaces")) {
      for (const auto& i : sw["interfaces"]) {
        if (i.count("vlanID") && i["vlanID"].asInt() == vlanId) {
          intfHas = listHasAddress(i, addr);
        }
      }
    }
    return intfHas == onInterface;
  }
};

TEST_F(ConfigInterfaceVlanIpTest, AddThenDeleteSviAddresses) {
  const int vlanId = pickUnusedVlanId();
  ASSERT_NE(vlanId, 0) << "no free VLAN ID in [2, 4094]";
  const std::string sviName = "vlan" + std::to_string(vlanId);
  const std::string v4 = "10.254.254.1/24";
  const std::string v6 = "2001:db8:fefe::1/64";

  XLOG(INFO) << "[Step 1] Create VLAN " << vlanId << " + SVI and commit";
  ensureUnderlayIntfId(vlanId);
  commitConfig();
  waitForAgentReady();

  XLOG(INFO) << "[Step 2] Add v4 + v6 to " << sviName;
  auto addV4 = runCli({"config", "interface", sviName, "ip-address", v4});
  auto addV6 = runCli({"config", "interface", sviName, "ipv6-address", v6});
  if (addV4.exitCode != 0 || addV6.exitCode != 0) {
    discardSession();
  }
  ASSERT_EQ(addV4.exitCode, 0)
      << "stdout=" << addV4.stdout << " stderr=" << addV4.stderr;
  ASSERT_EQ(addV6.exitCode, 0)
      << "stdout=" << addV6.stdout << " stderr=" << addV6.stderr;
  commitConfig();
  waitForAgentReady();

  XLOG(INFO) << "[Step 3] Verify addresses on the SVI interface";
  {
    auto config = waitForRunningConfig([&](const folly::dynamic& c) {
      return addressPresent(c, vlanId, v4, true) &&
          addressPresent(c, vlanId, v6, true);
    });
    EXPECT_TRUE(addressPresent(config, vlanId, v4, true))
        << v4 << " not on the SVI interface for VLAN " << vlanId;
    EXPECT_TRUE(addressPresent(config, vlanId, v6, true))
        << v6 << " not on the SVI interface for VLAN " << vlanId;
  }

  XLOG(INFO) << "[Step 4] Delete both addresses from " << sviName;
  auto delV4 = runCli({"delete", "interface", sviName, "ip-address", v4});
  auto delV6 = runCli({"delete", "interface", sviName, "ipv6-address", v6});
  if (delV4.exitCode != 0 || delV6.exitCode != 0) {
    discardSession();
  }
  ASSERT_EQ(delV4.exitCode, 0)
      << "stdout=" << delV4.stdout << " stderr=" << delV4.stderr;
  ASSERT_EQ(delV6.exitCode, 0)
      << "stdout=" << delV6.stdout << " stderr=" << delV6.stderr;
  commitConfig();
  waitForAgentReady();

  XLOG(INFO) << "[Step 5] Verify addresses are gone";
  {
    auto config = waitForRunningConfig([&](const folly::dynamic& c) {
      return addressPresent(c, vlanId, v4, false) &&
          addressPresent(c, vlanId, v6, false);
    });
    EXPECT_TRUE(addressPresent(config, vlanId, v4, false))
        << v4 << " still present for VLAN " << vlanId;
    EXPECT_TRUE(addressPresent(config, vlanId, v6, false))
        << v6 << " still present for VLAN " << vlanId;
  }

  XLOG(INFO) << "[Step 6] Clean up: delete VLAN " << vlanId;
  auto delVlan = runCli({"delete", "vlan", std::to_string(vlanId)});
  if (delVlan.exitCode != 0) {
    discardSession();
  }
  ASSERT_EQ(delVlan.exitCode, 0)
      << "stdout=" << delVlan.stdout << " stderr=" << delVlan.stderr;
  commitConfig();
  waitForAgentReady();
}

TEST_F(ConfigInterfaceVlanIpTest, UnknownVlanSviRejected) {
  const int vlanId = pickUnusedVlanId();
  ASSERT_NE(vlanId, 0) << "no free VLAN ID in [2, 4094]";
  const std::string sviName = "vlan" + std::to_string(vlanId);

  auto result =
      runCli({"config", "interface", sviName, "ip-address", "10.9.9.1/24"});
  discardSession();
  EXPECT_NE(result.exitCode, 0)
      << "adding an address to nonexistent " << sviName << " must fail";
}
