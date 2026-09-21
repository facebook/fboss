// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/**
 * End-to-end tests for 'fboss2-dev delete interface <name> ipv6 ndp ...'
 *
 * These tests:
 *  1. Pick an interface from the running system
 *  2. Set NDP attributes via the config CLI
 *  3. Delete (reset to default) those attributes via the delete CLI
 *  4. Verify the field reverted to its default in the agent's running
 *     config (fetched via thrift), using wait-retry polling so verification
 *     is robust to commit-triggered agent restarts.
 *
 * Requirements:
 *  - FBOSS agent must be running with a valid configuration
 *  - The test must be run as root (or with appropriate permissions)
 */

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

class DeleteInterfaceIpv6NdpTest : public Fboss2IntegrationTest {
 protected:
  // Set an NDP attribute via the config CLI before deleting it. Every set
  // attribute is recorded and reset to its schema default in TearDown so a
  // test that fails between set and delete still leaves the shared DUT clean.
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

  // Delete (reset) an NDP attribute via the delete CLI.
  void deleteNdpAttr(
      const std::string& interfaceName,
      const std::string& attr) {
    auto result =
        runCli({"delete", "interface", interfaceName, "ipv6", "ndp", attr});
    ASSERT_EQ(result.exitCode, 0)
        << "Failed to delete ndp " << attr << ": " << result.stderr;
    commitConfig();
  }

  // Reset every NDP attribute this test set back to its schema default and
  // commit once. The reset is idempotent, so attributes the test body
  // already deleted are harmless to reset again.
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

  // Poll the agent's running config until ndp[fieldName] either matches the
  // schema default or is absent (thrift omits default-valued fields, so
  // "missing" is equivalent to "default" on the wire).
  void expectNdpFieldDefault(
      int intfID,
      const std::string& fieldName,
      const std::function<bool(const folly::dynamic&)>& isDefault,
      const std::string& description) {
    auto cfg = waitForRunningConfig(
        [&](const folly::dynamic& c) {
          auto ndp = getNdpConfig(c, intfID);
          if (!ndp.count(fieldName)) {
            return true; // omitted == default
          }
          return isDefault(ndp[fieldName]);
        },
        std::chrono::seconds(30));
    auto ndp = getNdpConfig(cfg, intfID);
    if (ndp.count(fieldName)) {
      EXPECT_TRUE(isDefault(ndp[fieldName]))
          << "intfID " << intfID << " ndp." << fieldName << " = "
          << folly::toJson(ndp[fieldName]) << " after delete, expected "
          << description << " or absent. Observed ndp: " << folly::toJson(ndp);
    }
  }

 private:
  // (interfaceName, attr) pairs set by this test, reset in TearDown
  std::vector<std::pair<std::string, std::string>> touched_;
};

// ---------------------------------------------------------------------------
// Test: set ra-interval then delete it
// ---------------------------------------------------------------------------

TEST_F(DeleteInterfaceIpv6NdpTest, DeleteRaInterval) {
  XLOG(INFO) << "[Step 1] Finding an interface to test...";
  std::string ifName = getRandomInterfacePortName();
  XLOG(INFO) << "  Using interface: " << ifName;

  XLOG(INFO) << "[Step 2] Setting ra-interval=30...";
  setNdpAttr(ifName, "ra-interval", "30");

  XLOG(INFO) << "[Step 3] Deleting ra-interval (reset to default 0)...";
  deleteNdpAttr(ifName, "ra-interval");

  XLOG(INFO) << "[Step 4] Verifying running config via thrift...";
  expectNdpFieldDefault(
      getInterfaceIdForPort(ifName),
      "routerAdvertisementSeconds",
      [](const folly::dynamic& v) { return v.asInt() == 0; },
      "== 0 (default)");
  XLOG(INFO) << "TEST PASSED";
}

// ---------------------------------------------------------------------------
// Test: set hop-limit then delete it
// ---------------------------------------------------------------------------

TEST_F(DeleteInterfaceIpv6NdpTest, DeleteHopLimit) {
  XLOG(INFO) << "[Step 1] Finding an interface to test...";
  std::string ifName = getRandomInterfacePortName();
  XLOG(INFO) << "  Using interface: " << ifName;

  XLOG(INFO) << "[Step 2] Setting hop-limit=64...";
  setNdpAttr(ifName, "hop-limit", "64");

  XLOG(INFO) << "[Step 3] Deleting hop-limit (reset to default 255)...";
  deleteNdpAttr(ifName, "hop-limit");

  XLOG(INFO) << "[Step 4] Verifying running config via thrift...";
  expectNdpFieldDefault(
      getInterfaceIdForPort(ifName),
      "curHopLimit",
      [](const folly::dynamic& v) { return v.asInt() == 255; },
      "== 255 (default)");
  XLOG(INFO) << "TEST PASSED";
}

// ---------------------------------------------------------------------------
// Test: set managed-config-flag then delete it
// ---------------------------------------------------------------------------

TEST_F(DeleteInterfaceIpv6NdpTest, DeleteManagedConfigFlag) {
  XLOG(INFO) << "[Step 1] Finding an interface to test...";
  std::string ifName = getRandomInterfacePortName();
  XLOG(INFO) << "  Using interface: " << ifName;

  XLOG(INFO) << "[Step 2] Setting managed-config-flag...";
  setNdpAttr(ifName, "managed-config-flag");

  XLOG(INFO) << "[Step 3] Deleting managed-config-flag (reset to false)...";
  deleteNdpAttr(ifName, "managed-config-flag");

  XLOG(INFO) << "[Step 4] Verifying running config via thrift...";
  expectNdpFieldDefault(
      getInterfaceIdForPort(ifName),
      "routerAdvertisementManagedBit",
      [](const folly::dynamic& v) { return !v.asBool(); },
      "== false (default)");
  XLOG(INFO) << "TEST PASSED";
}

// ---------------------------------------------------------------------------
// Test: set ra-address then delete it
// ---------------------------------------------------------------------------

TEST_F(DeleteInterfaceIpv6NdpTest, DeleteRaAddress) {
  XLOG(INFO) << "[Step 1] Finding an interface to test...";
  std::string ifName = getRandomInterfacePortName();
  XLOG(INFO) << "  Using interface: " << ifName;

  const std::string raAddr = "2001:db8::1";

  XLOG(INFO) << "[Step 2] Setting ra-address to " << raAddr << "...";
  setNdpAttr(ifName, "ra-address", raAddr);

  XLOG(INFO) << "[Step 3] Deleting ra-address (clearing the optional)...";
  deleteNdpAttr(ifName, "ra-address");

  XLOG(INFO) << "[Step 4] Verifying running config via thrift...";
  // routerAddress is optional with no schema default — after delete it must
  // be absent from the running config.
  int intfID = getInterfaceIdForPort(ifName);
  auto cfg = waitForRunningConfig(
      [&](const folly::dynamic& c) {
        auto ndp = getNdpConfig(c, intfID);
        return !ndp.count("routerAddress");
      },
      std::chrono::seconds(30));
  auto ndp = getNdpConfig(cfg, intfID);
  EXPECT_FALSE(ndp.count("routerAddress"))
      << "ra-address still present after delete: "
      << folly::toJson(ndp["routerAddress"]);
  XLOG(INFO) << "TEST PASSED";
}

// ---------------------------------------------------------------------------
// Test: unknown attribute is rejected with non-zero exit code
// ---------------------------------------------------------------------------

TEST_F(DeleteInterfaceIpv6NdpTest, DeleteUnknownAttrRejected) {
  XLOG(INFO) << "[Step 1] Finding an interface to test...";
  std::string ifName = getRandomInterfacePortName();
  XLOG(INFO) << "  Using interface: " << ifName;

  XLOG(INFO)
      << "[Step 2] Attempting to delete unknown attribute 'bogus-attr'...";
  auto result =
      runCli({"delete", "interface", ifName, "ipv6", "ndp", "bogus-attr"});
  EXPECT_NE(result.exitCode, 0)
      << "Expected non-zero exit for unknown ndp attribute";
  XLOG(INFO) << "  Correctly rejected with exit code " << result.exitCode;
  XLOG(INFO) << "TEST PASSED";
}
