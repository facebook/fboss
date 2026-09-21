// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/**
 * End-to-end test for `fboss2-dev delete dhcp relay-source-override ipv4`:
 * set an override, commit, delete it, commit, verify it is gone from the
 * running config. Per-field behaviour and argument validation are covered
 * by the CmdDeleteDhcpTest unit tests.
 */

#include <folly/logging/xlog.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string>
#include "fboss/cli/fboss2/test/integration_test/Fboss2IntegrationTest.h"

using namespace facebook::fboss;
using ::testing::HasSubstr;

class DeleteDhcpSourceOverrideTest : public Fboss2IntegrationTest {
 protected:
  void setOverride(
      const std::string& category,
      const std::string& family,
      const std::string& value) {
    auto result = runCli({"config", "dhcp", category, family, value});
    ASSERT_EQ(result.exitCode, 0)
        << "stdout=" << result.stdout << " stderr=" << result.stderr;
    commitConfig();
  }
};

TEST_F(DeleteDhcpSourceOverrideTest, DeleteRelayIpv4) {
  const std::string category = "relay-source-override";
  const std::string family = "ipv4";
  const std::string swField = "dhcpRelaySrcOverrideV4";
  const std::string testValue = "192.0.2.1";

  // The override fields hold IP addresses, so an empty string means unset.
  auto originalValue = getSwConfigField<std::string>(swField, "");
  XLOG(INFO) << "[Step 1] Original " << swField << " = "
             << (originalValue.empty() ? "<unset>" : originalValue)
             << "; programming test value " << testValue;
  setOverride(category, family, testValue);
  ASSERT_EQ(getSwConfigField<std::string>(swField, ""), testValue);

  XLOG(INFO) << "[Step 2] Running: delete dhcp " << category << " " << family;
  auto result = runCli({"delete", "dhcp", category, family});
  ASSERT_EQ(result.exitCode, 0)
      << "stdout=" << result.stdout << " stderr=" << result.stderr;
  EXPECT_THAT(result.stdout, HasSubstr(category));
  EXPECT_THAT(result.stdout, HasSubstr(family));
  EXPECT_THAT(result.stdout, HasSubstr(testValue));

  XLOG(INFO) << "[Step 3] Committing (expect HITLESS, no restart)...";
  commitConfig();

  XLOG(INFO) << "[Step 4] Verifying override is gone from running config";
  EXPECT_EQ(getSwConfigField<std::string>(swField, ""), "")
      << "Expected " << swField << " unset after delete";

  if (!originalValue.empty()) {
    XLOG(INFO) << "[Step 5] Restoring original value " << originalValue;
    setOverride(category, family, originalValue);
    EXPECT_EQ(getSwConfigField<std::string>(swField, ""), originalValue);
  }
}
