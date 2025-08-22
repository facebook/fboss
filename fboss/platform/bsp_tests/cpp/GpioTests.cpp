#include <string>
#include <vector>

#include <fmt/format.h>
#include <gtest/gtest.h>

#include "fboss/platform/bsp_tests/cpp/BspTest.h"
#include "fboss/platform/bsp_tests/cpp/utils/GpioUtils.h"
#include "fboss/platform/bsp_tests/cpp/utils/I2CUtils.h"
#include "fboss/platform/bsp_tests/cpp/utils/KmodUtils.h"
#include "fboss/platform/bsp_tests/gen-cpp2/bsp_tests_config_types.h"

namespace facebook::fboss::platform::bsp_tests::cpp {

// Test fixture for GPIO tests
class GpioTest : public BspTest {
 protected:
  std::vector<I2CAdapter> getAllAdaptersWithGpios() {
    std::vector<I2CAdapter> adapters;

    const auto& runtimeConfig = this->GetRuntimeConfig();
    // Get I2C adapters from the runtime config
    for (const auto& [pmName, adapter] : *runtimeConfig.i2cAdapters()) {
      bool hasGpioDevice = false;
      for (const auto& i2cDevice : *adapter.i2cDevices()) {
        if (*i2cDevice.isGpioChip()) {
          hasGpioDevice = true;
          break;
        }
      }
      if (hasGpioDevice) {
        adapters.emplace_back(adapter);
      }
    }
    return adapters;
  }

  std::optional<GpioTestData> getGpioTestData(const I2CDevice& device) {
    auto testDataOpt = getDeviceTestData(device);
    if (testDataOpt.has_value() && testDataOpt->gpioTestData().has_value()) {
      return testDataOpt->gpioTestData().value();
    }
    return std::nullopt;
  }

  // Format GPIO label: busNum-address (with address zero-padded to 4 digits)
  std::string formatGpioLabel(int busNum, const std::string& address) {
    std::string formattedAddress = address.substr(2); // Remove "0x" prefix
    formattedAddress.insert(
        0, 4 - formattedAddress.length(), '0'); // Zero-pad to 4 digits
    return fmt::format("{}-{}", busNum, formattedAddress);
  }

  // Helper struct to hold GPIO device information
  struct GpioDeviceInfo {
    std::string gpioName;
    std::optional<GpioTestData> testData;
    int busNum;
  };

  // Setup GPIO devices, returning a vector of GPIO device info
  std::vector<GpioDeviceInfo> setupGpioDevices(
      const I2CAdapter& adapter,
      int adapterBaseBusNum) {
    std::vector<GpioDeviceInfo> gpioDevices;

    for (const auto& i2cDevice : *adapter.i2cDevices()) {
      // Skip devices that are not GPIO chips
      if (!*i2cDevice.isGpioChip()) {
        continue;
      }

      // Get GPIO test data if available
      auto testDataOpt = getGpioTestData(i2cDevice);

      int busNum = adapterBaseBusNum + *i2cDevice.channel();
      EXPECT_TRUE(I2CUtils::createI2CDevice(i2cDevice, busNum))
          << "Failed to create I2C device " << *i2cDevice.deviceName();

      std::string expectedLabel = formatGpioLabel(busNum, *i2cDevice.address());

      // Check that the GPIO device is detected
      auto gpios = GpioUtils::gpiodetect(expectedLabel);
      EXPECT_EQ(gpios.size(), 1)
          << "Expected GPIO not detected: " << expectedLabel;

      gpioDevices.push_back({expectedLabel, testDataOpt, busNum});
    }

    return gpioDevices;
  }

  // Helper function to run a common GPIO test pattern
  void runGpioTest(
      const std::string& testName,
      const std::function<void(const std::string&, GpioTestData)>&
          deviceTestFn) {
    const auto adapters = getAllAdaptersWithGpios();
    if (adapters.empty()) {
      GTEST_SKIP() << "No GPIOs found in test config";
    }

    int id = 1;
    for (const auto& adapter : adapters) {
      try {
        auto newBuses = I2CUtils::createI2CAdapter(adapter, id);
        if (adapter.pciAdapterInfo().has_value()) {
          registerDeviceForCleanup(
              *adapter.pciAdapterInfo()->pciInfo(),
              *adapter.pciAdapterInfo()->auxData(),
              id);
        }
        id++;

        auto gpioDevices =
            setupGpioDevices(adapter, newBuses.begin()->second.busNum);

        // Run the test-specific function for each GPIO device
        for (const auto& gpioDevice : gpioDevices) {
          if (!gpioDevice.testData.has_value()) {
            XLOG(INFO) << "Skipping " << testName << " test for "
                       << gpioDevice.gpioName
                       << " because no test data is provided";
            continue;
          }
          deviceTestFn(gpioDevice.gpioName, gpioDevice.testData.value());
        }
      } catch (const std::exception& e) {
        FAIL() << "Exception during " << testName << " test: " << e.what();
      }
    }
  }
};

// Test that GPIO devices are detectable
TEST_F(GpioTest, GpioIsDetectable) {
  runGpioTest(
      "GPIO detection", [](const std::string& gpioName, GpioTestData testData) {
        // Verify the number of lines
        auto detectedGpios = GpioUtils::gpiodetect(gpioName);
        ASSERT_EQ(detectedGpios.size(), 1);
        ASSERT_EQ(detectedGpios[0].lines, *testData.numLines())
            << "Expected " << *testData.numLines()
            << " lines, got: " << GpioUtils::gpiodetect(gpioName)[0].lines;
      });
}

// Test GPIO info
TEST_F(GpioTest, GpioInfo) {
  runGpioTest(
      "GPIO info", [](const std::string& gpioName, GpioTestData testData) {
        // Get GPIO info
        auto info = GpioUtils::gpioinfo(gpioName);
        ASSERT_EQ(info.size(), *testData.numLines());

        // Check each line's info
        for (const auto& expectedLine : *testData.lines()) {
          int idx = *expectedLine.index();
          EXPECT_EQ(info[idx].name, *expectedLine.name())
              << "Line " << idx
              << " name mismatch: expected=" << *expectedLine.name()
              << ", got=" << info[idx].name;

          EXPECT_EQ(info[idx].direction, *expectedLine.direction())
              << "Line " << idx
              << " direction mismatch: expected=" << *expectedLine.direction()
              << ", got=" << info[idx].direction;
        }
      });
}

// Test GPIO get
TEST_F(GpioTest, GpioGet) {
  runGpioTest(
      "GPIO get", [](const std::string& gpioName, GpioTestData testData) {
        // Check each line's value
        for (size_t i = 0; i < testData.lines()->size(); ++i) {
          const auto& line = (*testData.lines())[i];
          if (!line.getValue().has_value()) {
            continue;
          }

          try {
            int val = GpioUtils::gpioget(gpioName, *line.index());
            int expectedValue = *line.getValue();
            EXPECT_EQ(val, expectedValue)
                << "Expected " << expectedValue << " for " << gpioName
                << " on line " << i << ", got: " << val;
          } catch (const std::exception& e) {
            FAIL() << "Failed to get GPIO value for " << gpioName << ": "
                   << e.what();
          }
        }
      });
}

// Test driver unloads while GPIO devices are created
TEST_F(GpioTest, DriverUnload) {
  int id = 1;
  for (const auto& adapter : getAllAdaptersWithGpios()) {
    try {
      auto newBuses = I2CUtils::createI2CAdapter(adapter, id);
      id++;

      for (const auto& i2cDevice : *adapter.i2cDevices()) {
        // Only create devices that are GPIO chips
        if (*i2cDevice.isGpioChip()) {
          int busNum = newBuses.at(*i2cDevice.channel()).busNum;
          ASSERT_TRUE(I2CUtils::createI2CDevice(i2cDevice, busNum))
              << "Failed to create I2C device " << *i2cDevice.deviceName();
        }
      }
    } catch (const std::exception& e) {
      FAIL() << "Failed to create GPIO devices: " << e.what();
    }
  }

  // Unload kernel modules
  try {
    KmodUtils::unloadKmods(*GetRuntimeConfig().kmods());
  } catch (const std::exception& e) {
    FAIL() << "Failed to unload kmods with GPIO devices open: " << e.what();
  }
}
} // namespace facebook::fboss::platform::bsp_tests::cpp
