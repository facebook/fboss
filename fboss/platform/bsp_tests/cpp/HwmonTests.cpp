#include <string>
#include <vector>

#include <fmt/format.h>
#include <folly/logging/xlog.h>
#include <gtest/gtest.h>

#include "fboss/platform/bsp_tests/cpp/BspTest.h"
#include "fboss/platform/bsp_tests/cpp/utils/HwmonUtils.h"
#include "fboss/platform/bsp_tests/cpp/utils/I2CUtils.h"
#include "fboss/platform/bsp_tests/gen-cpp2/bsp_tests_config_types.h"

namespace facebook::fboss::platform::bsp_tests::cpp {

class HwmonTest : public BspTest {
 protected:
  static void SetUpTestSuite() {
    BspTest::SetUpTestSuite();
    ASSERT_TRUE(HwmonUtils::ensureSensorsInstalled())
        << "Failed to ensure lm_sensors package is installed";
  }

  std::optional<HwmonTestData> getHwmonTestData(const I2CDevice& device) {
    auto testDataOpt = getDeviceTestData(device);
    if (testDataOpt.has_value() && testDataOpt->hwmonTestData().has_value()) {
      return testDataOpt->hwmonTestData().value();
    }
    return std::nullopt;
  }

  std::vector<I2CAdapter> getAllAdaptersWithHwmons() {
    std::vector<I2CAdapter> adapters;

    for (const auto& [pmName, adapter] : *GetRuntimeConfig().i2cAdapters()) {
      bool hasHwmonDevice = false;
      for (const auto& i2cDevice : *adapter.i2cDevices()) {
        if (getHwmonTestData(i2cDevice).has_value()) {
          hasHwmonDevice = true;
          break;
        }
      }
      if (hasHwmonDevice) {
        adapters.push_back(adapter);
      }
    }
    return adapters;
  }

  void registerAdapterForCleanup(const I2CAdapter& adapter, int id) {
    if (adapter.pciAdapterInfo().has_value()) {
      registerDeviceForCleanup(
          *adapter.pciAdapterInfo()->pciInfo(),
          *adapter.pciAdapterInfo()->auxData(),
          id);
    }
  }
};

// Test that hardware monitoring sensors are detected and have the expected
// features
TEST_F(HwmonTest, HwmonSensors) {
  int id = 1;
  for (const auto& adapter : getAllAdaptersWithHwmons()) {
    try {
      auto newBuses = I2CUtils::createI2CAdapter(adapter, id);
      registerAdapterForCleanup(adapter, id);
      id++;

      for (const auto& i2cDevice : *adapter.i2cDevices()) {
        auto hwmonDataOpt = getHwmonTestData(i2cDevice);
        if (!hwmonDataOpt.has_value()) {
          continue;
        }
        const auto& hwmonTestData = hwmonDataOpt.value();

        int busNum = newBuses.at(*i2cDevice.channel()).busNum;
        ASSERT_TRUE(I2CUtils::createI2CDevice(i2cDevice, busNum))
            << "Failed to create I2C device " << *i2cDevice.deviceName()
            << " on bus " << busNum;

        std::string expectedSensorName = fmt::format(
            "{}-i2c-{}-{}",
            *i2cDevice.deviceName(),
            busNum,
            *i2cDevice.address());

        auto foundSensor = HwmonUtils::getDetectedChips(expectedSensorName);

        std::vector<std::string> featureNames;
        featureNames.reserve(foundSensor.features.size());
        for (const auto& feature : foundSensor.features) {
          featureNames.push_back(feature.name);
        }

        // Check that all expected features are present
        for (const auto& feature : *hwmonTestData.expectedFeatures()) {
          ASSERT_TRUE(
              std::find(featureNames.begin(), featureNames.end(), feature) !=
              featureNames.end())
              << "Hwmon device " << busNum << "-"
              << i2cDevice.address()->substr(2)
              << " Expected to find sensor value: " << feature
              << ". Got: " << fmt::format("{}", fmt::join(featureNames, ", "));
        }
      }
    } catch (const std::exception& e) {
      FAIL() << "Exception during hwmon test: " << e.what();
    }
  }
}
} // namespace facebook::fboss::platform::bsp_tests::cpp
