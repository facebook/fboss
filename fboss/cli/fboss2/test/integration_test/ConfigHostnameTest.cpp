// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

/**
 * End-to-end test for 'fboss2-dev config hostname <name>'
 *
 * This test:
 * 1. Reads the current hostname from the running config
 * 2. Sets a new hostname
 * 3. Verifies the hostname was applied to the running config
 * 4. Restores the original hostname
 *
 * Requirements:
 * - FBOSS agent must be running with a valid configuration
 * - The test must be run as root (or with appropriate permissions)
 */

#include <folly/logging/xlog.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string>
#include "fboss/cli/fboss2/test/integration_test/Fboss2IntegrationTest.h"

using namespace facebook::fboss;

class ConfigHostnameTest : public Fboss2IntegrationTest {
 protected:
  std::string getHostname() const {
    // hostname is an optional thrift field; on platforms (e.g. sim) where it is
    // unset, it is omitted from the running config. Default to "" so SetUp's
    // initial read does not throw before any hostname has been set.
    return getSwConfigField<std::string>("hostname", "");
  }

  void setHostname(const std::string& name) {
    auto result = runCli({"config", "hostname", name});
    ASSERT_EQ(result.exitCode, 0)
        << "Failed to set hostname: " << result.stderr;
    commitConfig();
  }

  void SetUp() override {
    Fboss2IntegrationTest::SetUp();
    originalHostname_ = getHostname();
  }

  void TearDown() override {
    // Restore original hostname on test exit (even if test failed
    // mid-execution)
    if (!originalHostname_.empty()) {
      try {
        setHostname(originalHostname_);
      } catch (const std::exception& e) {
        XLOG(ERR) << "Failed to restore hostname in TearDown: " << e.what();
      }
    }
    Fboss2IntegrationTest::TearDown();
  }

  std::string originalHostname_;
};

TEST_F(ConfigHostnameTest, SetAndVerifyHostname) {
  XLOG(INFO) << "[Step 1] Reading current hostname...";
  XLOG(INFO) << "  Current hostname: '" << originalHostname_ << "'";

  std::string testHostname = "cli-e2e-test-hostname";
  if (originalHostname_ == testHostname) {
    testHostname = "cli-e2e-test-hostname-alt";
  }

  XLOG(INFO) << "[Step 2] Setting hostname to '" << testHostname << "'...";
  setHostname(testHostname);
  EXPECT_EQ(getHostname(), testHostname);
  XLOG(INFO) << "  Verified: hostname is '" << testHostname << "'";

  XLOG(INFO) << "TEST PASSED (TearDown will restore original hostname)";
}

TEST_F(ConfigHostnameTest, IdempotentSet) {
  std::string testHostname = "cli-e2e-idempotent-hostname";
  if (originalHostname_ == testHostname) {
    testHostname = "cli-e2e-idempotent-hostname-alt";
  }

  // Set to new value
  setHostname(testHostname);
  EXPECT_EQ(getHostname(), testHostname);

  // Set same value again — should return "already set" message
  auto result = runCli({"config", "hostname", testHostname});
  EXPECT_EQ(result.exitCode, 0) << result.stderr;
  EXPECT_THAT(result.stdout, ::testing::HasSubstr("already"));

  XLOG(INFO) << "TEST PASSED (TearDown will restore original hostname)";
}
