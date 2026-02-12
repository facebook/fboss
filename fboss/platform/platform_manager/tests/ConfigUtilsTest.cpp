// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>

#include <folly/testing/TestUtil.h>

#include "fboss/platform/config_lib/MockConfigLib.h"
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
  }

  void setupMocksForGetConfig() {
    ON_CALL(*mockPlatformNameLib_, getPlatformNameFromBios(_))
        .WillByDefault(Return(kTestPlatformName));
    ON_CALL(*mockConfigLib_, getPlatformManagerConfig(_))
        .WillByDefault(Return(kTestConfigJson));
  }

  std::shared_ptr<MockConfigLib> mockConfigLib_;
  std::shared_ptr<MockPlatformNameLib> mockPlatformNameLib_;
};

TEST_F(ConfigUtilsTest, GetConfigReturnsValidConfig) {
  setupMocksForGetConfig();
  ConfigUtils configUtils(mockConfigLib_, mockPlatformNameLib_);

  auto config = configUtils.getConfig();

  EXPECT_EQ(*config.platformName(), kTestPlatformName);
  EXPECT_EQ(*config.rootSlotType(), "SCM_SLOT");
}

TEST_F(ConfigUtilsTest, GetConfigCachesResult) {
  ConfigUtils configUtils(mockConfigLib_, mockPlatformNameLib_);

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

  ConfigUtils configUtils(mockConfigLib_, mockPlatformNameLib_);
  EXPECT_NO_THROW(configUtils.getConfig());

  // Failure case: platform names mismatch
  auto mockPlatformNameLib2 = std::make_shared<MockPlatformNameLib>();
  auto mockConfigLib2 = std::make_shared<MockConfigLib>();
  ON_CALL(*mockPlatformNameLib2, getPlatformNameFromBios(_))
      .WillByDefault(Return("DIFFERENTPLATFORM"));
  ON_CALL(*mockConfigLib2, getPlatformManagerConfig(_))
      .WillByDefault(Return(kTestConfigJson));

  ConfigUtils configUtils2(mockConfigLib2, mockPlatformNameLib2);
  EXPECT_THROW(configUtils2.getConfig(), std::runtime_error);
}

} // namespace facebook::fboss::platform::platform_manager
