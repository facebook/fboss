// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "fboss/platform/platform_manager/PkgManager.h"

using namespace ::testing;
namespace facebook::fboss::platform::platform_manager {
class MockPkgManager : public PkgManager {
 public:
  explicit MockPkgManager(const PlatformConfig& config) : PkgManager(config) {}
  MOCK_METHOD(bool, processRpms, (), (const));
  MOCK_METHOD(void, processKmods, (), (const));
  MOCK_METHOD(void, processLocalRpms, (), (const));
  MOCK_METHOD(void, loadBSPKmods, (), (const));
  MOCK_METHOD(void, loadUpstreamKmods, (), (const));
};

class PkgManagerTest : public testing::Test {
 public:
  void SetUp() override {
    FLAGS_local_rpm_path = "";
  }
  PlatformConfig platformConfig_;
  MockPkgManager mockPkgManager_{platformConfig_};
};

TEST_F(PkgManagerTest, EnablePkgMgmnt) {
  FLAGS_enable_pkg_mgmnt = true;
  FLAGS_reload_kmods = false;

  EXPECT_CALL(mockPkgManager_, processLocalRpms()).Times(0);
  // Case 1: When new rpm installed
  EXPECT_CALL(mockPkgManager_, loadUpstreamKmods()).Times(1);
  EXPECT_CALL(mockPkgManager_, processRpms()).WillOnce(Return(true));
  EXPECT_CALL(mockPkgManager_, processKmods()).Times(1);
  EXPECT_CALL(mockPkgManager_, loadBSPKmods()).Times(0);
  EXPECT_NO_THROW(mockPkgManager_.processAll());
  // Case 2: When rpm is already installed
  EXPECT_CALL(mockPkgManager_, loadUpstreamKmods()).Times(1);
  EXPECT_CALL(mockPkgManager_, processRpms()).WillOnce(Return(false));
  EXPECT_CALL(mockPkgManager_, processKmods()).Times(0);
  EXPECT_CALL(mockPkgManager_, loadBSPKmods()).Times(1);
  EXPECT_NO_THROW(mockPkgManager_.processAll());
}

TEST_F(PkgManagerTest, EnablePkgMgmntWithReloadKmods) {
  FLAGS_enable_pkg_mgmnt = true;
  FLAGS_reload_kmods = true;

  EXPECT_CALL(mockPkgManager_, processLocalRpms()).Times(0);
  // Case 1: When new rpm installed and expect to reload kmods once.
  EXPECT_CALL(mockPkgManager_, loadUpstreamKmods()).Times(1);
  EXPECT_CALL(mockPkgManager_, processRpms()).WillOnce(Return(true));
  EXPECT_CALL(mockPkgManager_, processKmods()).Times(1);
  EXPECT_CALL(mockPkgManager_, loadBSPKmods()).Times(0);
  EXPECT_NO_THROW(mockPkgManager_.processAll());
  // Case 2: When rpm is already installed and still expect to reload kmods
  // once because of FLAGS_reload_kmods being true.
  EXPECT_CALL(mockPkgManager_, loadUpstreamKmods()).Times(1);
  EXPECT_CALL(mockPkgManager_, processRpms()).WillOnce(Return(false));
  EXPECT_CALL(mockPkgManager_, processKmods()).Times(1);
  EXPECT_CALL(mockPkgManager_, loadBSPKmods()).Times(0);
  EXPECT_NO_THROW(mockPkgManager_.processAll());
}

TEST_F(PkgManagerTest, DisablePkgMgmnt) {
  FLAGS_enable_pkg_mgmnt = false;
  FLAGS_reload_kmods = false;

  EXPECT_CALL(mockPkgManager_, loadUpstreamKmods()).Times(1);
  EXPECT_CALL(mockPkgManager_, processLocalRpms()).Times(0);
  EXPECT_CALL(mockPkgManager_, processRpms()).Times(0);
  EXPECT_CALL(mockPkgManager_, processKmods()).Times(0);
  EXPECT_CALL(mockPkgManager_, loadBSPKmods()).Times(1);
  EXPECT_NO_THROW(mockPkgManager_.processAll());
}
TEST_F(PkgManagerTest, DisablePkgMgmntWithReloadKmods) {
  FLAGS_enable_pkg_mgmnt = false;
  FLAGS_reload_kmods = true;

  EXPECT_CALL(mockPkgManager_, loadUpstreamKmods()).Times(1);
  EXPECT_CALL(mockPkgManager_, processLocalRpms()).Times(0);
  EXPECT_CALL(mockPkgManager_, processRpms()).Times(0);
  EXPECT_CALL(mockPkgManager_, processKmods()).Times(1);
  EXPECT_CALL(mockPkgManager_, loadBSPKmods()).Times(0);
  EXPECT_NO_THROW(mockPkgManager_.processAll());
}
}; // namespace facebook::fboss::platform::platform_manager
