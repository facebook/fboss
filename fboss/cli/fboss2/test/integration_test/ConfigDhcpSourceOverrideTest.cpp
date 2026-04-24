// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/**
 * End-to-end tests for `fboss2-dev config dhcp {relay,reply}-source-override
 * {ipv4,ipv6} <IP>` commands.
 *
 * Covers the 4 hitless DHCP source-override fields under SwitchConfig:
 *   - dhcpRelaySrcOverrideV4  (relay-source-override ipv4)
 *   - dhcpRelaySrcOverrideV6  (relay-source-override ipv6)
 *   - dhcpReplySrcOverrideV4  (reply-source-override ipv4)
 *   - dhcpReplySrcOverrideV6  (reply-source-override ipv6)
 *
 * For each (category, family) pair the test:
 *   1. Reads the current override value (if any) from the running config
 *   2. Sets a new value via the CLI
 *   3. Commits the session (HITLESS — no agent restart)
 *   4. Verifies the running config reflects the new value
 *   5. Restores the original value (if one was present)
 *
 * Reply-source-override note: the CLI validates that the IP matches a
 * configured interface IP (required by the agent at reply-processing time).
 * SetReplyIpv4 / SetReplyIpv6 therefore discover live interface IPs from the
 * running config and skip if none are available (e.g. on a bare test DUT with
 * no L3 configuration). Relay-source-override has no such constraint.
 */

#include <folly/IPAddressV6.h>
#include <folly/json/dynamic.h>
#include <folly/json/json.h>
#include <folly/logging/xlog.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <optional>
#include <string>
#include <vector>
#include "fboss/agent/if/gen-cpp2/FbossCtrlAsyncClient.h"
#include "fboss/cli/fboss2/test/integration_test/Fboss2IntegrationTest.h"
#include "fboss/cli/fboss2/utils/CmdClientUtilsCommon.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

using namespace facebook::fboss;
using ::testing::HasSubstr;

class ConfigDhcpSourceOverrideTest : public Fboss2IntegrationTest {
 protected:
  folly::dynamic getRunningConfig() const {
    HostInfo hostInfo("localhost");
    auto client =
        utils::createClient<apache::thrift::Client<FbossCtrl>>(hostInfo);
    std::string configStr;
    client->sync_getRunningConfig(configStr);
    return folly::parseJson(configStr);
  }

  std::optional<std::string> getSwOptionalString(
      const std::string& field) const {
    auto config = getRunningConfig();
    if (!config.isObject() || !config.count("sw")) {
      throw std::runtime_error("Running config missing 'sw' object");
    }
    const auto& sw = config["sw"];
    if (!sw.isObject() || !sw.count(field)) {
      return std::nullopt;
    }
    return sw[field].asString();
  }

  // Return all interface IPs of the requested family.  Excludes IPv6
  // link-local (fe80::/10) since those can't be used as reply-src targets.
  // Returned strings are bare host IPs, no CIDR suffix.
  std::vector<std::string> getInterfaceIps(bool wantV4) const {
    auto config = getRunningConfig();
    const auto& sw = config["sw"];
    if (!sw.isObject() || !sw.count("interfaces")) {
      return {};
    }
    std::vector<std::string> result;
    for (const auto& intf : sw["interfaces"]) {
      if (!intf.count("ipAddresses")) {
        continue;
      }
      for (const auto& cidr : intf["ipAddresses"]) {
        const auto cidrStr = cidr.asString();
        const auto slashPos = cidrStr.find('/');
        const auto hostStr = slashPos != std::string::npos
            ? cidrStr.substr(0, slashPos)
            : cidrStr;
        // IPv6 addresses contain ':', IPv4 do not.
        const bool isV4 = hostStr.find(':') == std::string::npos;
        const bool isLinkLocal =
            !isV4 && folly::IPAddressV6(hostStr).isLinkLocal();
        if ((wantV4 && isV4) || (!wantV4 && !isV4 && !isLinkLocal)) {
          result.push_back(hostStr);
        }
      }
    }
    return result;
  }

  std::string pickTestIp(
      const std::string& swField,
      const std::string& primary,
      const std::string& alternate) const {
    auto current = getSwOptionalString(swField);
    if (!current.has_value() || *current != primary) {
      return primary;
    }
    return alternate;
  }

  // Find an interface IP for the reply-source-override test that differs from
  // the current override. Returns empty string if no suitable IP exists.
  std::string pickReplyTestIp(const std::string& swField, bool wantV4) const {
    auto intfIps = getInterfaceIps(wantV4);
    auto current = getSwOptionalString(swField);
    for (const auto& ip : intfIps) {
      if (!current.has_value() || *current != ip) {
        return ip;
      }
    }
    return "";
  }

  void setAndVerify(
      const std::string& category,
      const std::string& family,
      const std::string& swField,
      const std::string& newValue) {
    XLOG(INFO) << "========================================";
    XLOG(INFO) << "  dhcp " << category << " " << family << " -> " << newValue;
    XLOG(INFO) << "========================================";

    auto originalValue = getSwOptionalString(swField);
    XLOG(INFO) << "[Step 1] Original " << swField << " = "
               << (originalValue.has_value() ? *originalValue : "<unset>");
    if (originalValue.has_value()) {
      ASSERT_NE(*originalValue, newValue)
          << "Test value " << newValue << " must differ from original "
          << *originalValue;
    }

    XLOG(INFO) << "[Step 2] Running: config dhcp " << category << " " << family
               << " " << newValue;
    auto result = runCli({"config", "dhcp", category, family, newValue});
    ASSERT_EQ(result.exitCode, 0)
        << "stdout=" << result.stdout << " stderr=" << result.stderr;
    EXPECT_THAT(result.stdout, HasSubstr(category));
    EXPECT_THAT(result.stdout, HasSubstr(family));
    EXPECT_THAT(result.stdout, HasSubstr(newValue));

    XLOG(INFO) << "[Step 3] Committing (expect HITLESS, no restart)...";
    commitConfig();

    XLOG(INFO) << "[Step 4] Verifying running config...";
    auto observed = getSwOptionalString(swField);
    ASSERT_TRUE(observed.has_value())
        << "Running config missing " << swField << " after commit";
    EXPECT_EQ(*observed, newValue) << "Expected running config " << swField
                                   << "=" << newValue << ", got " << *observed;

    if (originalValue.has_value()) {
      XLOG(INFO) << "[Step 5] Restoring original value " << *originalValue;
      result = runCli({"config", "dhcp", category, family, *originalValue});
      ASSERT_EQ(result.exitCode, 0) << result.stderr;
      commitConfig();
      auto restored = getSwOptionalString(swField);
      ASSERT_TRUE(restored.has_value());
      EXPECT_EQ(*restored, *originalValue);
    } else {
      XLOG(INFO)
          << "[Step 5] Original was unset; no CLI-driven restore available. "
             "Leaving "
          << swField << " set to " << newValue;
    }

    XLOG(INFO) << "  PASSED: " << category << " " << family;
  }
};

TEST_F(ConfigDhcpSourceOverrideTest, SetRelayIpv4) {
  auto ip = pickTestIp("dhcpRelaySrcOverrideV4", "192.0.2.1", "192.0.2.2");
  setAndVerify("relay-source-override", "ipv4", "dhcpRelaySrcOverrideV4", ip);
}

TEST_F(ConfigDhcpSourceOverrideTest, SetRelayIpv6) {
  auto ip = pickTestIp("dhcpRelaySrcOverrideV6", "2001:db8::1", "2001:db8::2");
  setAndVerify("relay-source-override", "ipv6", "dhcpRelaySrcOverrideV6", ip);
}

TEST_F(ConfigDhcpSourceOverrideTest, SetReplyIpv4) {
  auto ip = pickReplyTestIp("dhcpReplySrcOverrideV4", true);
  if (ip.empty()) {
    GTEST_SKIP()
        << "No IPv4 interface IPs configured on this DUT; "
           "reply-source-override requires an IP that matches an interface";
  }
  setAndVerify("reply-source-override", "ipv4", "dhcpReplySrcOverrideV4", ip);
}

TEST_F(ConfigDhcpSourceOverrideTest, SetReplyIpv6) {
  auto ip = pickReplyTestIp("dhcpReplySrcOverrideV6", false);
  if (ip.empty()) {
    GTEST_SKIP()
        << "No non-link-local IPv6 interface IPs configured on this DUT; "
           "reply-source-override requires an IP that matches an interface";
  }
  setAndVerify("reply-source-override", "ipv6", "dhcpReplySrcOverrideV6", ip);
}
