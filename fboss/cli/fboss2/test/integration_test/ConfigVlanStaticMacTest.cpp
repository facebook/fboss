// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/**
 * End-to-end test for:
 *   fboss2-dev config vlan <vlan-id> static-mac add <mac> <port>
 *   fboss2-dev config vlan <vlan-id> static-mac delete <mac>
 *
 * Adds a static MAC entry on a live VLAN, verifies it appears in
 * 'show mac details', deletes it, and confirms it's gone. Pre-cleans
 * any stale entry from a prior failed run.
 */

#include <folly/json/dynamic.h>
#include <folly/logging/xlog.h>
#include <gtest/gtest.h>
#include <algorithm>
#include <cctype>
#include <optional>
#include <string>
#include "fboss/cli/fboss2/test/integration_test/Fboss2IntegrationTest.h"

using namespace facebook::fboss;

namespace {
// Locally-administered unicast MAC (second hex digit is 2/6/A/E).
constexpr auto kTestMac = "02:00:00:E2:E2:01";

std::string toUpper(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(), ::toupper);
  return s;
}
} // namespace

class ConfigVlanStaticMacTest : public Fboss2IntegrationTest {
 protected:
  std::optional<folly::dynamic> findMacEntry(const std::string& mac, int vlanId)
      const {
    auto json = runCliJson({"show", "mac", "details"});
    auto target = toUpper(mac);
    for (const auto& [host, hostData] : json.items()) {
      if (!hostData.isObject() || !hostData.count("l2Entries")) {
        continue;
      }
      for (const auto& entry : hostData["l2Entries"]) {
        if (!entry.isObject()) {
          continue;
        }
        auto entryMac = toUpper(entry.getDefault("mac", "").asString());
        if (entryMac == target && entry.count("vlanID") &&
            entry["vlanID"].asInt() == vlanId) {
          return entry;
        }
      }
    }
    return std::nullopt;
  }

  void
  addStaticMac(int vlanId, const std::string& mac, const std::string& port) {
    auto result = runCli(
        {"config",
         "vlan",
         std::to_string(vlanId),
         "static-mac",
         "add",
         mac,
         port});
    ASSERT_EQ(result.exitCode, 0) << "add static-mac failed: " << result.stderr;
    commitConfig();
  }

  void deleteStaticMac(int vlanId, const std::string& mac) {
    auto result = runCli(
        {"config",
         "vlan",
         std::to_string(vlanId),
         "static-mac",
         "delete",
         mac});
    ASSERT_EQ(result.exitCode, 0)
        << "delete static-mac failed: " << result.stderr;
    commitConfig();
  }
};

TEST_F(ConfigVlanStaticMacTest, AddAndDelete) {
  XLOG(INFO) << "[Step 1] Finding a (VLAN, port) pair with membership...";
  auto vp = findConfiguredVlanPort();
  if (!vp.has_value()) {
    GTEST_SKIP()
        << "This DUT has no port that is a member of a configured VLAN via "
           "sw.vlanPorts; 'config vlan <id> static-mac' requires that.";
  }
  int vlanId = vp->first;
  Interface intf;
  intf.name = vp->second;
  intf.vlan = vlanId;
  XLOG(INFO) << "  Using " << intf.name << " on VLAN " << vlanId;

  // Pre-clean: remove any leftover test entry from a prior failed run.
  if (findMacEntry(kTestMac, vlanId).has_value()) {
    XLOG(INFO) << "[Pre-clean] Removing stale test MAC...";
    deleteStaticMac(vlanId, kTestMac);
  }

  XLOG(INFO) << "[Step 2] Adding static MAC " << kTestMac << " on VLAN "
             << vlanId << " port " << intf.name;
  addStaticMac(vlanId, kTestMac, intf.name);

  XLOG(INFO) << "[Step 3] Verifying MAC entry via 'show mac details'...";
  auto entry = findMacEntry(kTestMac, vlanId);
  ASSERT_TRUE(entry.has_value())
      << "MAC entry not found for " << kTestMac << " on VLAN " << vlanId;
  EXPECT_EQ(toUpper((*entry)["mac"].asString()), toUpper(kTestMac));
  EXPECT_EQ((*entry)["vlanID"].asInt(), vlanId);

  XLOG(INFO) << "[Step 4] Deleting static MAC...";
  deleteStaticMac(vlanId, kTestMac);

  XLOG(INFO) << "[Step 5] Verifying MAC entry is gone...";
  EXPECT_FALSE(findMacEntry(kTestMac, vlanId).has_value());

  XLOG(INFO) << "TEST PASSED";
}
