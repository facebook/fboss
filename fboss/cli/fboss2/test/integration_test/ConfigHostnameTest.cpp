// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <folly/logging/xlog.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string>
#include "fboss/cli/fboss2/test/integration_test/Fboss2IntegrationTest.h"

using namespace facebook::fboss;

class ConfigHostnameTest : public Fboss2IntegrationTest {
 protected:
  std::string getHostname() const {
    try {
      auto hostnames = getSwitchSettingsField<std::string>("hostname");
      return hostnames.begin()->second;
    } catch (const std::runtime_error&) {
      return "";
    }
  }

  void setHostname(const std::string& name) {
    auto result = runCli({"config", "switch", "hostname", name});
    ASSERT_EQ(result.exitCode, 0)
        << "Failed to set hostname: " << result.stderr;
    commitConfig();
  }

  void SetUp() override {
    Fboss2IntegrationTest::SetUp();
    originalHostname_ = getHostname();
  }

  void TearDown() override {
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
  verifySwitchSettingsField<std::string>("hostname", testHostname);
  XLOG(INFO) << "  Verified: hostname is '" << testHostname << "'";

  XLOG(INFO) << "TEST PASSED (TearDown will restore original hostname)";
}

TEST_F(ConfigHostnameTest, IdempotentSet) {
  std::string testHostname = "cli-e2e-idempotent-hostname";
  if (originalHostname_ == testHostname) {
    testHostname = "cli-e2e-idempotent-hostname-alt";
  }

  setHostname(testHostname);
  verifySwitchSettingsField<std::string>("hostname", testHostname);

  // Set same value again — should return "already set" message
  auto result = runCli({"config", "switch", "hostname", testHostname});
  EXPECT_EQ(result.exitCode, 0) << result.stderr;
  EXPECT_THAT(result.stdout, ::testing::HasSubstr("already"));

  XLOG(INFO) << "TEST PASSED (TearDown will restore original hostname)";
}
