// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/**
 * End-to-end tests for 'fboss2-dev config interface <name> ipv6 ndp ...'
 *
 * These tests:
 *  1. Pick an interface from the running system
 *  2. Set NDP attributes via the CLI
 *  3. Verify the config landed in the agent's running config (fetched via
 *     thrift getRunningConfig()), using wait-retry polling so the
 *     verification is robust to commit-triggered agent restarts.
 *
 * Requirements:
 *  - FBOSS agent must be running with a valid configuration
 *  - The test must be run as root (or with appropriate permissions)
 */

#include <fmt/format.h>
#include <folly/dynamic.h>
#include <folly/json/dynamic.h>
#include <folly/json/json.h>
#include <folly/logging/xlog.h>
#include <gtest/gtest.h>
#include <chrono>
#include <functional>
#include <string>
#include <utility>
#include <vector>
#include "fboss/cli/fboss2/test/integration_test/Fboss2IntegrationTest.h"

using namespace facebook::fboss;

class ConfigInterfaceIpv6NdpTest : public Fboss2IntegrationTest {
 protected:
  // Run `config interface <name> ipv6 ndp <attr> [<value>]` and commit.
  // commitConfig() tolerates the agent-restart-on-commit case; the test body
  // is expected to verify the change actually landed via expectNdpField().
  // Every set attribute is recorded and reset to its schema default in
  // TearDown so the shared DUT is left clean even when a test fails midway.
  void setNdpAttr(
      const std::string& interfaceName,
      const std::string& attr,
      const std::string& value = "") {
    touched_.emplace_back(interfaceName, attr);
    std::vector<std::string> args = {
        "config", "interface", interfaceName, "ipv6", "ndp", attr};
    if (!value.empty()) {
      args.push_back(value);
    }
    auto result = runCli(args);
    ASSERT_EQ(result.exitCode, 0)
        << "Failed to set ndp " << attr << ": " << result.stderr;
    commitConfig();
  }

  // Reset every NDP attribute this test set back to its schema default
  // (which serializes the same as "unset" on the wire) and commit once.
  void TearDown() override {
    if (!touched_.empty()) {
      bool reset = false;
      for (const auto& [interfaceName, attr] : touched_) {
        auto result =
            runCli({"delete", "interface", interfaceName, "ipv6", "ndp", attr});
        if (result.exitCode != 0) {
          XLOG(WARN) << "TearDown failed to reset ndp " << attr << " on "
                     << interfaceName << ": " << result.stderr;
        } else {
          reset = true;
        }
      }
      if (reset) {
        commitConfig();
      }
    }
    Fboss2IntegrationTest::TearDown();
  }

  // Poll the agent's running config until ndp[fieldName] for the L3
  // interface with the given intfID satisfies `predicate`. Fails the test if
  // the field never matches before the timeout, dumping the last observed
  // ndp object.
  void expectNdpField(
      int intfID,
      const std::string& fieldName,
      const std::function<bool(const folly::dynamic&)>& predicate,
      const std::string& description) {
    auto cfg = waitForRunningConfig(
        [&](const folly::dynamic& c) {
          auto ndp = getNdpConfig(c, intfID);
          return ndp.count(fieldName) && predicate(ndp[fieldName]);
        },
        std::chrono::seconds(30));
    auto ndp = getNdpConfig(cfg, intfID);
    ASSERT_TRUE(ndp.count(fieldName))
        << "intfID " << intfID << " ndp." << fieldName
        << " missing from running config (expected " << description
        << "). Observed ndp: " << folly::toJson(ndp);
    EXPECT_TRUE(predicate(ndp[fieldName]))
        << "intfID " << intfID << " ndp." << fieldName << " = "
        << folly::toJson(ndp[fieldName]) << ", expected " << description;
  }

 private:
  // (interfaceName, attr) pairs set by this test, reset in TearDown
  std::vector<std::pair<std::string, std::string>> touched_;
};

// ---------------------------------------------------------------------------
// Test: set ra-interval and verify via running config
// ---------------------------------------------------------------------------

TEST_F(ConfigInterfaceIpv6NdpTest, SetAndVerifyRaInterval) {
  XLOG(INFO) << "[Step 1] Finding an interface to test...";
  std::string ifName = getRandomInterfacePortName();
  XLOG(INFO) << "  Using interface: " << ifName;

  const int newInterval = 30;

  XLOG(INFO) << "[Step 2] Setting ra-interval to " << newInterval << "...";
  setNdpAttr(ifName, "ra-interval", std::to_string(newInterval));

  XLOG(INFO) << "[Step 3] Verifying running config via thrift...";
  expectNdpField(
      getInterfaceIdForPort(ifName),
      "routerAdvertisementSeconds",
      [&](const folly::dynamic& v) { return v.asInt() == newInterval; },
      fmt::format("== {}", newInterval));
  XLOG(INFO) << "TEST PASSED";
}

// ---------------------------------------------------------------------------
// Test: set hop-limit and verify via running config
// ---------------------------------------------------------------------------

TEST_F(ConfigInterfaceIpv6NdpTest, SetAndVerifyHopLimit) {
  XLOG(INFO) << "[Step 1] Finding an interface to test...";
  std::string ifName = getRandomInterfacePortName();
  XLOG(INFO) << "  Using interface: " << ifName;

  const int newHopLimit = 64;

  XLOG(INFO) << "[Step 2] Setting hop-limit to " << newHopLimit << "...";
  setNdpAttr(ifName, "hop-limit", std::to_string(newHopLimit));

  XLOG(INFO) << "[Step 3] Verifying running config via thrift...";
  expectNdpField(
      getInterfaceIdForPort(ifName),
      "curHopLimit",
      [&](const folly::dynamic& v) { return v.asInt() == newHopLimit; },
      fmt::format("== {}", newHopLimit));
  XLOG(INFO) << "TEST PASSED";
}

// ---------------------------------------------------------------------------
// Test: set managed-config-flag and verify via running config
// ---------------------------------------------------------------------------

TEST_F(ConfigInterfaceIpv6NdpTest, SetManagedConfigFlag) {
  XLOG(INFO) << "[Step 1] Finding an interface to test...";
  std::string ifName = getRandomInterfacePortName();
  XLOG(INFO) << "  Using interface: " << ifName;

  XLOG(INFO) << "[Step 2] Setting managed-config-flag...";
  setNdpAttr(ifName, "managed-config-flag");

  XLOG(INFO) << "[Step 3] Verifying running config via thrift...";
  expectNdpField(
      getInterfaceIdForPort(ifName),
      "routerAdvertisementManagedBit",
      [](const folly::dynamic& v) { return v.asBool(); },
      "== true");
  XLOG(INFO) << "TEST PASSED";
}

// ---------------------------------------------------------------------------
// Test: set ra-address and verify via running config
// ---------------------------------------------------------------------------

TEST_F(ConfigInterfaceIpv6NdpTest, SetRaAddress) {
  XLOG(INFO) << "[Step 1] Finding an interface with an IPv6 address...";
  Interface iface = getInterfaceInfo(getRandomInterfacePortName());
  XLOG(INFO) << "  Using interface: " << iface.name;

  // ra-address must be one of the interface's own addresses, so pick the
  // first IPv6 address configured on this interface.
  std::string raAddr;
  for (const auto& addr : iface.addresses) {
    if (addr.find(':') != std::string::npos) {
      raAddr = addr.substr(0, addr.find('/'));
      break;
    }
  }
  if (raAddr.empty()) {
    GTEST_SKIP() << "No IPv6 address on interface " << iface.name;
  }
  XLOG(INFO) << "  Using ra-address: " << raAddr;

  XLOG(INFO) << "[Step 2] Setting ra-address to " << raAddr << "...";
  setNdpAttr(iface.name, "ra-address", raAddr);

  XLOG(INFO) << "[Step 3] Verifying running config via thrift...";
  expectNdpField(
      getInterfaceIdForPort(iface.name),
      "routerAddress",
      [&](const folly::dynamic& v) { return v.asString() == raAddr; },
      fmt::format("== \"{}\"", raAddr));
  XLOG(INFO) << "TEST PASSED";
}

// ---------------------------------------------------------------------------
// Test: invalid hop-limit returns non-zero exit code
// ---------------------------------------------------------------------------

TEST_F(ConfigInterfaceIpv6NdpTest, InvalidHopLimitRejected) {
  XLOG(INFO) << "[Step 1] Finding an interface to test...";
  std::string ifName = getRandomInterfacePortName();
  XLOG(INFO) << "  Using interface: " << ifName;

  XLOG(INFO) << "[Step 2] Attempting to set hop-limit=999 (out of range)...";
  auto result = runCli(
      {"config", "interface", ifName, "ipv6", "ndp", "hop-limit", "999"});
  EXPECT_NE(result.exitCode, 0)
      << "Expected non-zero exit for out-of-range hop-limit";
  XLOG(INFO) << "  Correctly rejected with exit code " << result.exitCode;
  XLOG(INFO) << "TEST PASSED";
}
