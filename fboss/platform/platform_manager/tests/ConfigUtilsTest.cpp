// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>

#include <fmt/format.h>
#include <folly/hash/Hash.h>
#include <folly/testing/TestUtil.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

#include "fboss/platform/config_lib/MockConfigLib.h"
#include "fboss/platform/helpers/Init.h"
#include "fboss/platform/helpers/MockPlatformFsUtils.h"
#include "fboss/platform/helpers/PlatformNameLib.h"
#include "fboss/platform/platform_manager/ConfigUtils.h"

using namespace ::testing;

namespace facebook::fboss::platform::platform_manager {

namespace {
// Valid config JSON that passes ConfigValidator::isValid()
const std::string kTestPlatformName = "TESTPLATFORM";
const std::string kTestConfigJson = R"({
  "platformName": "TESTPLATFORM",
  "rootSlotType": "SCM_SLOT",
  "bspKmodsRpmName": "test_bsp_kmods",
  "bspKmodsRpmVersion": "1.0.0-1",
  "slotTypeConfigs": {
    "SCM_SLOT": {
      "pmUnitName": "SCM",
      "idpromConfig": {
        "address": "0x14"
      }
    }
  },
  "pmUnitConfigs": {
    "SCM": {
      "pluggedInSlotType": "SCM_SLOT",
      "i2cDeviceConfigs": [
        {
          "pmUnitScopedName": "CHASSIS_EEPROM",
          "address": "0x50"
        }
      ]
    }
  },
  "chassisEepromDevicePath": "/[CHASSIS_EEPROM]"
})";

std::string computeExpectedHash() {
  PlatformConfig config;
  apache::thrift::SimpleJSONSerializer::deserialize<PlatformConfig>(
      kTestConfigJson, config);
  auto configJson =
      apache::thrift::SimpleJSONSerializer::serialize<std::string>(config);
  return fmt::format("{:016x}", folly::hasher<std::string>{}(configJson));
}
} // namespace

class MockPlatformNameLib : public helpers::PlatformNameLib {
 public:
  MockPlatformNameLib() : helpers::PlatformNameLib(nullptr, nullptr) {}
  MOCK_METHOD(std::string, getPlatformNameFromBios, (bool), (const));
};

class ConfigUtilsTest : public ::testing::Test {
 protected:
  void SetUp() override {
    mockConfigLib_ = std::make_shared<MockConfigLib>();
    mockPlatformNameLib_ = std::make_shared<MockPlatformNameLib>();
    mockPlatformFsUtils_ = std::make_shared<MockPlatformFsUtils>();
  }

  void setupMocksForGetConfig() {
    ON_CALL(*mockPlatformNameLib_, getPlatformNameFromBios(_))
        .WillByDefault(Return(kTestPlatformName));
    ON_CALL(*mockConfigLib_, getPlatformManagerConfig(_))
        .WillByDefault(Return(kTestConfigJson));
  }

  std::shared_ptr<MockConfigLib> mockConfigLib_;
  std::shared_ptr<MockPlatformNameLib> mockPlatformNameLib_;
  std::shared_ptr<MockPlatformFsUtils> mockPlatformFsUtils_;
};

TEST_F(ConfigUtilsTest, GetConfigReturnsValidConfig) {
  setupMocksForGetConfig();
  ConfigUtils configUtils(
      mockConfigLib_, mockPlatformNameLib_, mockPlatformFsUtils_);

  auto config = configUtils.getConfig();

  EXPECT_EQ(*config.platformName(), kTestPlatformName);
  EXPECT_EQ(*config.rootSlotType(), "SCM_SLOT");
}

TEST_F(ConfigUtilsTest, GetConfigCachesResult) {
  ConfigUtils configUtils(
      mockConfigLib_, mockPlatformNameLib_, mockPlatformFsUtils_);

  // Expect the mocks to be called only once due to caching
  EXPECT_CALL(*mockPlatformNameLib_, getPlatformNameFromBios(_))
      .WillOnce(Return(kTestPlatformName));
  EXPECT_CALL(*mockConfigLib_, getPlatformManagerConfig(_))
      .WillOnce(Return(kTestConfigJson));

  // Call getConfig multiple times
  auto config1 = configUtils.getConfig();
  auto config2 = configUtils.getConfig();
  auto config3 = configUtils.getConfig();

  // All should return the same config
  EXPECT_EQ(*config1.platformName(), kTestPlatformName);
  EXPECT_EQ(*config2.platformName(), kTestPlatformName);
  EXPECT_EQ(*config3.platformName(), kTestPlatformName);
}

TEST_F(ConfigUtilsTest, GetConfigVerifiesPlatformName) {
  // Success case: platform names match
  ON_CALL(*mockPlatformNameLib_, getPlatformNameFromBios(_))
      .WillByDefault(Return(kTestPlatformName));
  ON_CALL(*mockConfigLib_, getPlatformManagerConfig(_))
      .WillByDefault(Return(kTestConfigJson));

  ConfigUtils configUtils(
      mockConfigLib_, mockPlatformNameLib_, mockPlatformFsUtils_);
  EXPECT_NO_THROW(configUtils.getConfig());

  // Failure case: platform names mismatch
  auto mockPlatformNameLib2 = std::make_shared<MockPlatformNameLib>();
  auto mockConfigLib2 = std::make_shared<MockConfigLib>();
  ON_CALL(*mockPlatformNameLib2, getPlatformNameFromBios(_))
      .WillByDefault(Return("DIFFERENTPLATFORM"));
  ON_CALL(*mockConfigLib2, getPlatformManagerConfig(_))
      .WillByDefault(Return(kTestConfigJson));

  ConfigUtils configUtils2(
      mockConfigLib2, mockPlatformNameLib2, mockPlatformFsUtils_);
  EXPECT_THROW(configUtils2.getConfig(), std::runtime_error);
}

// NOTE: This test verifies temporary backward-compatibility behavior.
// When no stored config hash exists (first run after this change), we return
// false to avoid triggering kmod reloads. This aligns with existing service
// behavior. Once all units have the config hash file, this behavior will change
// to trigger kmod reloading when the config hash file is missing.
// See ConfigUtils.cpp hasConfigChanged() for implementation details.
TEST_F(ConfigUtilsTest, HasConfigChangedReturnsFalseWhenNoStoredHash) {
  setupMocksForGetConfig();
  ON_CALL(*mockPlatformFsUtils_, getStringFileContent(_))
      .WillByDefault(Return(std::nullopt));

  ConfigUtils configUtils(
      mockConfigLib_, mockPlatformNameLib_, mockPlatformFsUtils_);

  EXPECT_FALSE(configUtils.hasConfigChanged());
}

TEST_F(ConfigUtilsTest, HasConfigChangedReturnsFalseWhenHashMatches) {
  setupMocksForGetConfig();

  // Mock getStringFileContent with path-specific matchers
  ON_CALL(
      *mockPlatformFsUtils_,
      getStringFileContent(std::filesystem::path(ConfigUtils::kConfigHashFile)))
      .WillByDefault(Return(computeExpectedHash()));

  ConfigUtils configUtils(
      mockConfigLib_, mockPlatformNameLib_, mockPlatformFsUtils_);

  EXPECT_FALSE(configUtils.hasConfigChanged());
}

TEST_F(ConfigUtilsTest, HasConfigChangedReturnsTrueWhenHashDiffers) {
  setupMocksForGetConfig();
  ON_CALL(
      *mockPlatformFsUtils_,
      getStringFileContent(std::filesystem::path(ConfigUtils::kConfigHashFile)))
      .WillByDefault(Return("differenthash12345"));
  ON_CALL(
      *mockPlatformFsUtils_,
      getStringFileContent(std::filesystem::path(ConfigUtils::kBuildInfoFile)))
      .WillByDefault(Return("old_build_info"));

  ConfigUtils configUtils(
      mockConfigLib_, mockPlatformNameLib_, mockPlatformFsUtils_);

  EXPECT_TRUE(configUtils.hasConfigChanged());
}

TEST_F(ConfigUtilsTest, StoreConfigHashWritesFiles) {
  setupMocksForGetConfig();
  ON_CALL(*mockPlatformFsUtils_, createDirectories(_))
      .WillByDefault(Return(true));
  ON_CALL(*mockPlatformFsUtils_, writeStringToFile(_, _, _, _))
      .WillByDefault(Return(true));

  // Capture the content written to each file
  std::string writtenHash;
  std::string writtenBuildInfo;
  EXPECT_CALL(
      *mockPlatformFsUtils_,
      writeStringToFile(
          _, std::filesystem::path(ConfigUtils::kConfigHashFile), _, _))
      .WillOnce(DoAll(SaveArg<0>(&writtenHash), Return(true)));
  EXPECT_CALL(
      *mockPlatformFsUtils_,
      writeStringToFile(
          _, std::filesystem::path(ConfigUtils::kBuildInfoFile), _, _))
      .WillOnce(DoAll(SaveArg<0>(&writtenBuildInfo), Return(true)));

  ConfigUtils configUtils(
      mockConfigLib_, mockPlatformNameLib_, mockPlatformFsUtils_);
  configUtils.storeConfigHash();

  // Verify hash matches exactly (with trailing newline)
  EXPECT_EQ(writtenHash, computeExpectedHash() + "\n");

  // Verify build info matches exactly (with trailing newline)
  EXPECT_EQ(writtenBuildInfo, helpers::getBuildSummary() + "\n");
}

TEST_F(ConfigUtilsTest, StoreConfigHashHandlesDirectoryCreationFailure) {
  setupMocksForGetConfig();
  ON_CALL(*mockPlatformFsUtils_, createDirectories(_))
      .WillByDefault(Return(false));

  EXPECT_CALL(*mockPlatformFsUtils_, writeStringToFile(_, _, _, _)).Times(0);

  ConfigUtils configUtils(
      mockConfigLib_, mockPlatformNameLib_, mockPlatformFsUtils_);

  // Verify the function handles failure gracefully without throwing
  EXPECT_NO_THROW(configUtils.storeConfigHash());
}

} // namespace facebook::fboss::platform::platform_manager
