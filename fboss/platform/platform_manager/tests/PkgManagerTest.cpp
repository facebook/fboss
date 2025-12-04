// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <fb303/ServiceData.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/concat.hpp>
#include <thrift/lib/cpp2/protocol/Serializer.h>

#include "fboss/platform/platform_manager/PkgManager.h"

using namespace ::testing;
namespace facebook::fboss::platform::platform_manager {
class MockSystemInterface : public package_manager::SystemInterface {
 public:
  explicit MockSystemInterface() : package_manager::SystemInterface() {}
  MOCK_METHOD(bool, isRpmInstalled, (const std::string&), (const));
  MOCK_METHOD(
      std::vector<std::string>,
      getInstalledRpms,
      (const std::string&),
      (const));
  MOCK_METHOD(int, removeRpms, (const std::vector<std::string>&), (const));
  MOCK_METHOD(int, installRpm, (const std::string&), (const));
  MOCK_METHOD(int, depmod, (), (const));
  MOCK_METHOD(std::set<std::string>, lsmod, (), (const));
  MOCK_METHOD(bool, unloadKmod, (const std::string&), (const));
  MOCK_METHOD(bool, loadKmod, (const std::string&), (const));
  MOCK_METHOD(std::string, getHostKernelVersion, (), (const));
};
class MockPkgManager : public PkgManager {
 public:
  explicit MockPkgManager(
      const PlatformConfig& config,
      const std::shared_ptr<package_manager::SystemInterface>& systemInterface)
      : PkgManager(config, systemInterface) {}
  MOCK_METHOD(void, processRpms, (), (const));
  MOCK_METHOD(void, processLocalRpms, (), (const));
  MOCK_METHOD(void, unloadBspKmods, (), (const));
  MOCK_METHOD(void, loadRequiredKmods, (), (const));
};
class MockPlatformFsUtils : public PlatformFsUtils {
 public:
  MOCK_METHOD(
      std::optional<std::string>,
      getStringFileContent,
      (const std::filesystem::path& path),
      (const));
};

class PkgManagerTest : public testing::Test {
 public:
  void SetUp() override {
    FLAGS_local_rpm_path = "";
    platformConfig_.bspKmodsRpmName() = "fboss_bsp_kmods";
    platformConfig_.bspKmodsRpmVersion() = "11.44.63-14";
    platformConfig_.requiredKmodsToLoad() = {"fboss_iob_pci", "spidev"};

    bspKmodsFile_.bspKmods() = {"fboss_iob_pci", "fboss_iob_spi"};
    bspKmodsFile_.sharedKmods() = {"scd"};
    jsonBspKmodsFile_ =
        apache::thrift::SimpleJSONSerializer::serialize<std::string>(
            bspKmodsFile_);
  }
  std::string jsonBspKmodsFile_;
  BspKmodsFile bspKmodsFile_;
  PlatformConfig platformConfig_;
  std::shared_ptr<MockSystemInterface> mockSystemInterface_{
      std::make_shared<MockSystemInterface>()};
  std::shared_ptr<MockPlatformFsUtils> mockPlatformFsUtils_{
      std::make_shared<MockPlatformFsUtils>()};
  // For testing high level flow in PkgManager::processAll
  MockPkgManager mockPkgManager_{platformConfig_, mockSystemInterface_};
  // For testing individual member functions such as PkgManager::unloadBspKmods
  PkgManager pkgManager_{
      platformConfig_,
      mockSystemInterface_,
      mockPlatformFsUtils_};
};

TEST_F(PkgManagerTest, EnablePkgMgmnt) {
  FLAGS_enable_pkg_mgmnt = true;
  FLAGS_reload_kmods = false;

  EXPECT_CALL(mockPkgManager_, processLocalRpms()).Times(0);
  // Case 1: When new rpm installed
  {
    InSequence seq;
    EXPECT_CALL(*mockSystemInterface_, isRpmInstalled(_))
        .WillOnce(Return(false));
    EXPECT_CALL(mockPkgManager_, unloadBspKmods()).Times(1);
    EXPECT_CALL(mockPkgManager_, processRpms()).Times(1);
    EXPECT_CALL(mockPkgManager_, unloadBspKmods()).Times(1);
    EXPECT_CALL(mockPkgManager_, loadRequiredKmods()).Times(1);
  }
  EXPECT_NO_THROW(mockPkgManager_.processAll());
  // Case 2: When rpm is already installed
  {
    InSequence seq;
    EXPECT_CALL(*mockSystemInterface_, isRpmInstalled(_))
        .WillOnce(Return(true));
    EXPECT_CALL(mockPkgManager_, loadRequiredKmods()).Times(1);
  }
  EXPECT_CALL(mockPkgManager_, unloadBspKmods()).Times(0);
  EXPECT_CALL(mockPkgManager_, processRpms()).Times(0);
  EXPECT_NO_THROW(mockPkgManager_.processAll());
}

TEST_F(PkgManagerTest, EnablePkgMgmntWithReloadKmods) {
  FLAGS_enable_pkg_mgmnt = true;
  FLAGS_reload_kmods = true;

  EXPECT_CALL(mockPkgManager_, processLocalRpms()).Times(0);
  // Case 1: When new rpm installed and expect to reload kmods twice.
  {
    InSequence seq;
    EXPECT_CALL(*mockSystemInterface_, isRpmInstalled(_))
        .WillOnce(Return(false));
    EXPECT_CALL(mockPkgManager_, unloadBspKmods()).Times(1);
    EXPECT_CALL(mockPkgManager_, processRpms()).Times(1);
    EXPECT_CALL(mockPkgManager_, unloadBspKmods()).Times(1);
    EXPECT_CALL(mockPkgManager_, loadRequiredKmods()).Times(1);
  }
  EXPECT_NO_THROW(mockPkgManager_.processAll());
  // Case 2: When rpm is already installed and still expect to unload kmods
  // once because of FLAGS_reload_kmods being true.
  {
    InSequence seq;
    EXPECT_CALL(*mockSystemInterface_, isRpmInstalled(_))
        .WillOnce(Return(true));
    EXPECT_CALL(mockPkgManager_, unloadBspKmods()).Times(1);
    EXPECT_CALL(mockPkgManager_, loadRequiredKmods()).Times(1);
  }
  EXPECT_CALL(mockPkgManager_, processRpms()).Times(0);
  EXPECT_NO_THROW(mockPkgManager_.processAll());
}

TEST_F(PkgManagerTest, DisablePkgMgmnt) {
  FLAGS_enable_pkg_mgmnt = false;
  FLAGS_reload_kmods = false;

  EXPECT_CALL(mockPkgManager_, processLocalRpms()).Times(0);
  EXPECT_CALL(*mockSystemInterface_, isRpmInstalled(_)).Times(0);
  EXPECT_CALL(mockPkgManager_, unloadBspKmods()).Times(0);
  EXPECT_CALL(mockPkgManager_, processRpms()).Times(0);
  EXPECT_CALL(mockPkgManager_, loadRequiredKmods()).Times(1);
  EXPECT_NO_THROW(mockPkgManager_.processAll());
}

TEST_F(PkgManagerTest, DisablePkgMgmntWithReloadKmods) {
  FLAGS_enable_pkg_mgmnt = false;
  FLAGS_reload_kmods = true;

  EXPECT_CALL(mockPkgManager_, processLocalRpms()).Times(0);
  EXPECT_CALL(*mockSystemInterface_, isRpmInstalled(_)).Times(0);
  EXPECT_CALL(mockPkgManager_, processRpms()).Times(0);
  {
    InSequence seq;
    EXPECT_CALL(mockPkgManager_, unloadBspKmods()).Times(1);
    EXPECT_CALL(mockPkgManager_, loadRequiredKmods()).Times(1);
  }
  EXPECT_NO_THROW(mockPkgManager_.processAll());
}

TEST_F(PkgManagerTest, processRpms) {
  EXPECT_CALL(*mockSystemInterface_, getHostKernelVersion())
      .WillRepeatedly(Return("6.4.3-0_fbk1_755_ga25447393a1d"));
  // No installed rpms
  EXPECT_CALL(
      *mockSystemInterface_,
      getInstalledRpms(
          fmt::format(
              "{}-6.4.3-0_fbk1_755_ga25447393a1d",
              *platformConfig_.bspKmodsRpmName())))
      .WillOnce(Return(std::vector<std::string>{}));
  EXPECT_CALL(*mockSystemInterface_, removeRpms(_)).Times(0);
  EXPECT_CALL(
      *mockSystemInterface_,
      installRpm(
          fmt::format(
              "{}-6.4.3-0_fbk1_755_ga25447393a1d-{}",
              *platformConfig_.bspKmodsRpmName(),
              *platformConfig_.bspKmodsRpmVersion())))
      .WillOnce(Return(0));
  EXPECT_CALL(*mockSystemInterface_, depmod()).WillOnce(Return(0));
  EXPECT_NO_THROW(pkgManager_.processRpms());
  EXPECT_EQ(
      facebook::fb303::fbData->getCounter(PkgManager::kProcessRpmFailure), 0);
  // Installed rpms
  EXPECT_CALL(
      *mockSystemInterface_,
      getInstalledRpms(
          fmt::format(
              "{}-6.4.3-0_fbk1_755_ga25447393a1d",
              *platformConfig_.bspKmodsRpmName())))
      .WillOnce(Return(
          std::vector<std::string>{
              "fboss_bsp_kmods-6.4.3-0_fbk1_755_ga25447393a1d-2.4.0-1"}));
  EXPECT_CALL(
      *mockSystemInterface_,
      removeRpms(
          std::vector<std::string>{
              "fboss_bsp_kmods-6.4.3-0_fbk1_755_ga25447393a1d-2.4.0-1"}))
      .WillOnce(Return(0));
  EXPECT_CALL(
      *mockSystemInterface_,
      installRpm(
          fmt::format(
              "{}-6.4.3-0_fbk1_755_ga25447393a1d-{}",
              *platformConfig_.bspKmodsRpmName(),
              *platformConfig_.bspKmodsRpmVersion())))
      .WillOnce(Return(0));
  EXPECT_CALL(*mockSystemInterface_, depmod()).WillOnce(Return(0));
  EXPECT_NO_THROW(pkgManager_.processRpms());
  EXPECT_EQ(
      facebook::fb303::fbData->getCounter(PkgManager::kProcessRpmFailure), 0);
  // Remove installed rpms failed.
  EXPECT_CALL(
      *mockSystemInterface_,
      getInstalledRpms(
          fmt::format(
              "{}-6.4.3-0_fbk1_755_ga25447393a1d",
              *platformConfig_.bspKmodsRpmName())))
      .WillOnce(Return(
          std::vector<std::string>{
              "fboss_bsp_kmods-6.4.3-0_fbk1_755_ga25447393a1d-2.4.0-1"}));
  EXPECT_CALL(
      *mockSystemInterface_,
      removeRpms(
          std::vector<std::string>{
              "fboss_bsp_kmods-6.4.3-0_fbk1_755_ga25447393a1d-2.4.0-1"}))
      .WillOnce(Return(1));
  EXPECT_THROW(pkgManager_.processRpms(), std::runtime_error);
  EXPECT_EQ(
      facebook::fb303::fbData->getCounter(PkgManager::kProcessRpmFailure), 1);
  // depmod failed
  EXPECT_CALL(
      *mockSystemInterface_,
      getInstalledRpms(
          fmt::format(
              "{}-6.4.3-0_fbk1_755_ga25447393a1d",
              *platformConfig_.bspKmodsRpmName())))
      .WillOnce(Return(std::vector<std::string>{}));
  EXPECT_CALL(
      *mockSystemInterface_,
      removeRpms(
          std::vector<std::string>{
              "fboss_bsp_kmods-6.4.3-0_fbk1_755_ga25447393a1d-2.4.0-1"}))
      .Times(0);
  EXPECT_CALL(
      *mockSystemInterface_,
      installRpm(
          fmt::format(
              "{}-6.4.3-0_fbk1_755_ga25447393a1d-{}",
              *platformConfig_.bspKmodsRpmName(),
              *platformConfig_.bspKmodsRpmVersion())))
      .WillOnce(Return(0));
  EXPECT_CALL(*mockSystemInterface_, depmod()).WillOnce(Return(1));
  EXPECT_NO_THROW(pkgManager_.processRpms());
  EXPECT_EQ(
      facebook::fb303::fbData->getCounter(PkgManager::kProcessRpmFailure), 0);
  // Rpm install failed
  EXPECT_CALL(
      *mockSystemInterface_,
      getInstalledRpms(
          fmt::format(
              "{}-6.4.3-0_fbk1_755_ga25447393a1d",
              *platformConfig_.bspKmodsRpmName())))
      .WillOnce(Return(std::vector<std::string>{}));
  EXPECT_CALL(
      *mockSystemInterface_,
      removeRpms(
          std::vector<std::string>{
              "fboss_bsp_kmods-6.4.3-0_fbk1_755_ga25447393a1d-2.4.0-1"}))
      .Times(0);
  EXPECT_CALL(
      *mockSystemInterface_,
      installRpm(
          fmt::format(
              "{}-6.4.3-0_fbk1_755_ga25447393a1d-{}",
              *platformConfig_.bspKmodsRpmName(),
              *platformConfig_.bspKmodsRpmVersion())))
      .Times(3)
      .WillRepeatedly(Return(1));
  EXPECT_THROW(pkgManager_.processRpms(), std::runtime_error);
  EXPECT_EQ(
      facebook::fb303::fbData->getCounter(PkgManager::kProcessRpmFailure), 1);
}

TEST_F(PkgManagerTest, unloadBspKmods) {
  EXPECT_CALL(*mockSystemInterface_, getHostKernelVersion())
      .WillRepeatedly(Return("6.4.3-0_fbk1_755_ga25447393a1d"));
  // No kmods.json and no old rpms
  EXPECT_CALL(*mockPlatformFsUtils_, getStringFileContent(_))
      .WillOnce(Return(std::nullopt));
  EXPECT_CALL(
      *mockSystemInterface_,
      getInstalledRpms(
          fmt::format(
              "{}-6.4.3-0_fbk1_755_ga25447393a1d",
              *platformConfig_.bspKmodsRpmName())))
      .WillOnce(Return(std::vector<std::string>{}));
  EXPECT_CALL(*mockSystemInterface_, lsmod()).Times(0);
  EXPECT_NO_THROW(pkgManager_.unloadBspKmods());
  EXPECT_EQ(
      facebook::fb303::fbData->getCounter(PkgManager::kUnloadKmodsFailure), 0);
  // No kmods.json when it should exist
  EXPECT_CALL(*mockPlatformFsUtils_, getStringFileContent(_))
      .WillOnce(Return(std::nullopt));
  EXPECT_CALL(
      *mockSystemInterface_,
      getInstalledRpms(
          fmt::format(
              "{}-6.4.3-0_fbk1_755_ga25447393a1d",
              *platformConfig_.bspKmodsRpmName())))
      .WillOnce(Return(
          std::vector<std::string>{
              "fboss_bsp_kmods-6.4.3-0_fbk1_755_ga25447393a1d-2.4.0-1"}));
  EXPECT_THROW(pkgManager_.unloadBspKmods(), std::runtime_error);
  EXPECT_EQ(
      facebook::fb303::fbData->getCounter(PkgManager::kUnloadKmodsFailure), 1);
  // kmods.json exist and all kmods are loaded
  EXPECT_CALL(*mockPlatformFsUtils_, getStringFileContent(_))
      .WillOnce(Return(jsonBspKmodsFile_));
  EXPECT_CALL(*mockSystemInterface_, lsmod())
      .WillOnce(Return(
          ranges::views::concat(
              *bspKmodsFile_.bspKmods(), *bspKmodsFile_.sharedKmods()) |
          ranges::to<std::set<std::string>>));
  EXPECT_CALL(*mockSystemInterface_, unloadKmod(_))
      .Times(
          bspKmodsFile_.sharedKmods()->size() +
          bspKmodsFile_.bspKmods()->size())
      .WillRepeatedly(Return(true));
  EXPECT_NO_THROW(pkgManager_.unloadBspKmods());
  EXPECT_EQ(
      facebook::fb303::fbData->getCounter(PkgManager::kUnloadKmodsFailure), 0);
  // kmods.json exists but unload fails
  EXPECT_CALL(*mockPlatformFsUtils_, getStringFileContent(_))
      .WillOnce(Return(jsonBspKmodsFile_));
  EXPECT_CALL(*mockSystemInterface_, lsmod())
      .WillOnce(Return(
          ranges::views::concat(
              *bspKmodsFile_.bspKmods(), *bspKmodsFile_.sharedKmods()) |
          ranges::to<std::set<std::string>>));
  EXPECT_CALL(*mockSystemInterface_, unloadKmod(_)).WillOnce(Return(false));
  EXPECT_THROW(pkgManager_.unloadBspKmods(), std::runtime_error);
  EXPECT_EQ(
      facebook::fb303::fbData->getCounter(PkgManager::kUnloadKmodsFailure), 1);
  // kmods.json exist and all kmods aren't loaded
  EXPECT_CALL(*mockPlatformFsUtils_, getStringFileContent(_))
      .WillOnce(Return(jsonBspKmodsFile_));
  EXPECT_CALL(*mockSystemInterface_, lsmod())
      .WillOnce(Return(std::set<std::string>{}));
  EXPECT_CALL(*mockSystemInterface_, unloadKmod(_)).Times(0);
  EXPECT_NO_THROW(pkgManager_.unloadBspKmods());
  EXPECT_EQ(
      facebook::fb303::fbData->getCounter(PkgManager::kUnloadKmodsFailure), 0);
}

TEST_F(PkgManagerTest, loadRequiredKmods) {
  // Load kmods from PlatformManagerConfig::loadRequiredKmods
  EXPECT_CALL(*mockSystemInterface_, loadKmod(_))
      .Times(platformConfig_.requiredKmodsToLoad()->size())
      .WillRepeatedly(Return(true));
  EXPECT_NO_THROW(pkgManager_.loadRequiredKmods());
  EXPECT_EQ(
      facebook::fb303::fbData->getCounter(PkgManager::kLoadKmodsFailure), 0);
  // Load kmods fail
  EXPECT_CALL(*mockSystemInterface_, loadKmod(_)).WillOnce(Return(false));
  EXPECT_THROW(pkgManager_.loadRequiredKmods(), std::runtime_error);
  EXPECT_EQ(
      facebook::fb303::fbData->getCounter(PkgManager::kLoadKmodsFailure), 1);
}

TEST_F(PkgManagerTest, ReadKmodsFileSuccess) {
  const std::string kernelVersion = "6.4.3-0_fbk1_755_ga25447393a1d";
  const std::filesystem::path expectedFilePath =
      "/usr/local/fboss_bsp/6.4.3-0_fbk1_755_ga25447393a1d/kmods.json";

  EXPECT_CALL(*mockSystemInterface_, getHostKernelVersion())
      .WillOnce(Return(kernelVersion));

  EXPECT_CALL(*mockPlatformFsUtils_, getStringFileContent(expectedFilePath))
      .WillOnce(Return(jsonBspKmodsFile_));

  BspKmodsFile result = pkgManager_.readKmodsFile();

  EXPECT_EQ(result.bspKmods()->size(), bspKmodsFile_.bspKmods()->size());
  EXPECT_EQ(result.sharedKmods()->size(), bspKmodsFile_.sharedKmods()->size());

  for (size_t i = 0; i < result.bspKmods()->size(); i++) {
    EXPECT_EQ((*result.bspKmods())[i], (*bspKmodsFile_.bspKmods())[i]);
  }

  for (size_t i = 0; i < result.sharedKmods()->size(); i++) {
    EXPECT_EQ((*result.sharedKmods())[i], (*bspKmodsFile_.sharedKmods())[i]);
  }
}

TEST_F(PkgManagerTest, ReadKmodsFileFailure) {
  const std::string kernelVersion = "6.4.3-0_fbk1_755_ga25447393a1d";
  const std::filesystem::path expectedFilePath =
      "/usr/local/fboss_bsp/6.4.3-0_fbk1_755_ga25447393a1d/kmods.json";

  EXPECT_CALL(*mockSystemInterface_, getHostKernelVersion())
      .WillOnce(Return(kernelVersion));

  EXPECT_CALL(*mockPlatformFsUtils_, getStringFileContent(expectedFilePath))
      .WillOnce(Return(std::nullopt));

  EXPECT_THROW(pkgManager_.readKmodsFile(), std::runtime_error);
}

TEST_F(PkgManagerTest, isValidRpm) {
  EXPECT_CALL(*mockSystemInterface_, getHostKernelVersion())
      .WillRepeatedly(Return("6.11.1-0_fbk3_647_gc1af76fcc8cb"));

  // RPM with valide kernel version
  FLAGS_local_rpm_path =
      "/path/to/fboss_bsp_kmods-6.11.1-0_fbk3_647_gc1af76fcc8cb-3.3.0-1.x86_64.rpm";
  EXPECT_TRUE(mockPkgManager_.isValidRpm());

  // RPM with invalid kernel version
  FLAGS_local_rpm_path =
      "/path/to/fboss_bsp_kmods-6.11.2-0_fbk3_647_gc1af76fcc8cb-3.3.0-1.x86_64.rpm";
  EXPECT_FALSE(mockPkgManager_.isValidRpm());

  // Invalid RPM file
  FLAGS_local_rpm_path =
      "/path/to/fboss_bsp_kmods-6.11.1-0_fbk3_647_gc1af76fcc8cb-3.3.0-2.x86_64";
  EXPECT_FALSE(mockPkgManager_.isValidRpm());

  // Invalid file name
  FLAGS_local_rpm_path = "/path/to/invalid.txt";
  EXPECT_FALSE(mockPkgManager_.isValidRpm());

  // Empty file name
  FLAGS_local_rpm_path = "";
  EXPECT_FALSE(mockPkgManager_.isValidRpm());
}
}; // namespace facebook::fboss::platform::platform_manager
