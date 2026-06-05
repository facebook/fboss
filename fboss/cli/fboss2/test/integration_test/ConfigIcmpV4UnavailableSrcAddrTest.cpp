// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/**
 * End-to-end test for:
 *   fboss2-dev config icmpv4-unavailable-src-addr <ipv4-address>
 *
 * Reads the current icmpV4UnavailableSrcAddress, changes it, verifies the new
 * value round-trips through the agent's running config, then restores the
 * original. The change is HITLESS so no agent restart is needed between steps.
 */

#include <folly/logging/xlog.h>
#include <gtest/gtest.h>
#include <string>
#include "fboss/cli/fboss2/test/integration_test/Fboss2IntegrationTest.h"

using namespace facebook::fboss;

namespace {
// RFC7600 IANA dummy IPv4 address — agent default when field is absent
constexpr std::string_view kDefaultIcmpV4SrcAddr = "192.0.0.8";
} // namespace

class ConfigIcmpV4UnavailableSrcAddrTest : public Fboss2IntegrationTest {
 protected:
  std::string getIcmpV4SrcAddr() const {
    return getSwConfigField<std::string>(
        "icmpV4UnavailableSrcAddress", std::string(kDefaultIcmpV4SrcAddr));
  }

  void setIcmpV4SrcAddr(const std::string& addr) {
    auto result = runCli({"config", "icmpv4-unavailable-src-addr", addr});
    ASSERT_EQ(result.exitCode, 0)
        << "icmpv4-unavailable-src-addr CLI failed: " << result.stderr;
    commitConfig();
  }
};

TEST_F(ConfigIcmpV4UnavailableSrcAddrTest, SetAndRestoreAddress) {
  XLOG(INFO) << "[Step 1] Reading current ICMPv4 unavailable source address...";
  std::string originalAddr = getIcmpV4SrcAddr();
  XLOG(INFO) << "  Current: " << originalAddr;

  // Pick a target value different from current
  std::string newAddr = (originalAddr == "10.0.0.1") ? "10.0.0.2" : "10.0.0.1";

  XLOG(INFO) << "[Step 2] Setting address to " << newAddr << "...";
  setIcmpV4SrcAddr(newAddr);
  EXPECT_EQ(getIcmpV4SrcAddr(), newAddr);

  XLOG(INFO) << "[Step 3] Restoring original address " << originalAddr << "...";
  setIcmpV4SrcAddr(originalAddr);
  EXPECT_EQ(getIcmpV4SrcAddr(), originalAddr);

  XLOG(INFO) << "TEST PASSED";
}
