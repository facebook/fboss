// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/**
 * End-to-end test for IP address add/remove on a VLAN SVI addressed as
 * "vlan<id>":
 *   create VLAN + SVI → add v4 + v6 → verify the SVI interface carries the
 *   address → delete both → verify gone → delete VLAN.
 *
 * Requirements:
 *   - FBOSS agent is running with a valid configuration
 *   - Test is run as root (or with sudo) on a DUT
 */

#include <folly/ScopeGuard.h>
#include <folly/logging/xlog.h>
#include <gtest/gtest.h>
#include <optional>
#include <set>
#include <string>
#include "fboss/cli/fboss2/test/integration_test/Fboss2IntegrationTest.h"

using namespace facebook::fboss;

class ConfigInterfaceVlanIpTest : public Fboss2IntegrationTest {
 protected:
  // The ipAddresses on VLAN `vlanId`'s SVI in the running config, or nullopt
  // if the SVI is missing.
  static std::optional<std::set<std::string>> sviAddresses(
      const folly::dynamic& config,
      int vlanId) {
    if (!config.isObject() || !config.count("sw") ||
        !config["sw"].count("interfaces")) {
      return std::nullopt;
    }
    for (const auto& i : config["sw"]["interfaces"]) {
      if (i.count("vlanID") && i["vlanID"].asInt() == vlanId) {
        std::set<std::string> addrs;
        if (i.count("ipAddresses")) {
          for (const auto& a : i["ipAddresses"]) {
            addrs.insert(a.asString());
          }
        }
        return addrs;
      }
    }
    return std::nullopt;
  }
};

TEST_F(ConfigInterfaceVlanIpTest, AddThenDeleteSviAddresses) {
  const int vlanId = pickUnusedVlanId();
  ASSERT_NE(vlanId, 0) << "no free VLAN ID on this switch";
  SCOPE_EXIT {
    deleteVlanIfPresent(vlanId);
  };
  const std::string sviName = "vlan" + std::to_string(vlanId);
  const std::string v4 = "10.254.254.1/24";
  const std::string v6 = "2001:db8:fefe::1/64";

  XLOG(INFO) << "[Step 1] Create VLAN " << vlanId << " + SVI and commit";
  ensureUnderlayIntfId(vlanId);
  commitConfig();

  XLOG(INFO) << "[Step 2] Add v4 + v6 to " << sviName << " and commit";
  auto addV4 = runCli({"config", "interface", sviName, "ip-address", v4});
  ASSERT_EQ(addV4.exitCode, 0)
      << "stdout=" << addV4.stdout << " stderr=" << addV4.stderr;
  auto addV6 = runCli({"config", "interface", sviName, "ipv6-address", v6});
  ASSERT_EQ(addV6.exitCode, 0)
      << "stdout=" << addV6.stdout << " stderr=" << addV6.stderr;
  commitConfig();

  XLOG(INFO) << "[Step 3] Verify both addresses are on the SVI";
  auto hasBoth = [&](const folly::dynamic& c) {
    auto addrs = sviAddresses(c, vlanId);
    return addrs && addrs->count(v4) && addrs->count(v6);
  };
  EXPECT_TRUE(hasBoth(waitForRunningConfig(hasBoth)))
      << v4 << " / " << v6 << " not on the SVI for VLAN " << vlanId;

  XLOG(INFO) << "[Step 4] Delete both addresses from " << sviName
             << " and commit";
  auto delV4 = runCli({"delete", "interface", sviName, "ip-address", v4});
  ASSERT_EQ(delV4.exitCode, 0)
      << "stdout=" << delV4.stdout << " stderr=" << delV4.stderr;
  auto delV6 = runCli({"delete", "interface", sviName, "ipv6-address", v6});
  ASSERT_EQ(delV6.exitCode, 0)
      << "stdout=" << delV6.stdout << " stderr=" << delV6.stderr;
  commitConfig();

  XLOG(INFO) << "[Step 5] Verify the addresses are gone and the SVI remains";
  auto hasNeither = [&](const folly::dynamic& c) {
    auto addrs = sviAddresses(c, vlanId);
    return addrs && !addrs->count(v4) && !addrs->count(v6);
  };
  EXPECT_TRUE(hasNeither(waitForRunningConfig(hasNeither)))
      << v4 << " / " << v6 << " still on the SVI for VLAN " << vlanId
      << ", or the SVI is gone";
}
