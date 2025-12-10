// Copyright (c) Meta Platforms, Inc. and affiliates.

#include <folly/logging/xlog.h>
#include <gtest/gtest.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <ranges>

#include "fboss/platform/bsp_tests/RuntimeConfigBuilder.h"
#include "fboss/platform/config_lib/ConfigLib.h"

namespace facebook::fboss::platform::bsp_tests {

// Test subclass that exposes protected methods for testing
class TestableRuntimeConfigBuilder : public RuntimeConfigBuilder {
 public:
  using RuntimeConfigBuilder::getActualAdapter;
};

// Test fixture for RuntimeConfigBuilder tests
class RuntimeConfigBuilderTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Create a test RuntimeConfigBuilder instance
    builder_ = std::make_unique<TestableRuntimeConfigBuilder>();

    setupMockPlatformConfig();
  }

  void setupMockPlatformConfig() {
    // Load the platform configuration using ConfigLib
    try {
      std::string configJson = ConfigLib().getPlatformManagerConfig("sample");
      apache::thrift::SimpleJSONSerializer::deserialize<
          platform_manager::PlatformConfig>(configJson, pmConfig_);
    } catch (const std::exception& e) {
      FAIL() << "Failed to load platform config: " << e.what();
    }
  }

  // Helper method to test getActualAdapter
  void testGetActualAdapter(
      const std::string& sourceUnitName,
      const std::string& sourceBusName,
      const std::string& slotType,
      const std::string& expectedUnitName,
      const std::string& expectedBusName,
      int expectedChannel) {
    std::string actualUnitName, actualBusName;
    int actualChannel;

    std::tie(actualUnitName, actualBusName, actualChannel) =
        builder_->getActualAdapter(
            pmConfig_, sourceUnitName, sourceBusName, slotType);

    EXPECT_EQ(actualUnitName, expectedUnitName)
        << "Expected unit name: " << expectedUnitName
        << ", got: " << actualUnitName;

    EXPECT_EQ(actualBusName, expectedBusName)
        << "Expected bus name: " << expectedBusName
        << ", got: " << actualBusName;

    EXPECT_EQ(actualChannel, expectedChannel)
        << "Expected channel: " << expectedChannel
        << ", got: " << actualChannel;
  }

  std::unique_ptr<TestableRuntimeConfigBuilder> builder_;
  platform_manager::PlatformConfig pmConfig_;
};

// Test that getActualAdapter correctly resolves a direct bus (non-INCOMING)
TEST_F(RuntimeConfigBuilderTest, DirectBusResolution) {
  testGetActualAdapter("SCM", "CPU@0", "SCM_SLOT", "SCM", "CPU", 0);
}

// Test that getActualAdapter correctly resolves a single INCOMING step
TEST_F(RuntimeConfigBuilderTest, SingleIncomingStep) {
  testGetActualAdapter(
      "SMB", "INCOMING@0", "SMB_SLOT", "SCM", "SCM_IOB_I2C_0", 0);
}

// Test handling of invalid INCOMING bus
TEST_F(RuntimeConfigBuilderTest, InvalidIncomingBus) {
  // The getActualAdapter method should throw an exception for an invalid
  // INCOMING bus
  EXPECT_THROW(
      builder_->getActualAdapter(pmConfig_, "SMB", "INCOMING@99", "SMB_SLOT"),
      std::runtime_error)
      << "Expected exception when resolving invalid INCOMING bus";
}

TEST_F(RuntimeConfigBuilderTest, CpuAdaptersAdded) {
  // Create a minimal BspTestsConfig
  bsp_tests::BspTestsConfig testConfig;
  testConfig.testData() = std::map<std::string, DeviceTestData>();
  BspKmodsFile kmods;

  // Build the runtime config using the sample platform config
  auto runtimeConfig =
      builder_->buildRuntimeConfig(testConfig, pmConfig_, kmods, "sample");

  // get all cpu adapters
  std::vector<I2CAdapter> cpuAdapters;
  for (const auto& [pmName, adapter] : *runtimeConfig.i2cAdapters()) {
    if (*adapter.isCpuAdapter()) {
      cpuAdapters.push_back(adapter);
    }
  }

  // Verify that the runtime config contains cpu adapters
  ASSERT_TRUE(cpuAdapters.size() == 1);

  // Verify that the cpu adapters have the correct properties
  for (const auto& adapter : cpuAdapters) {
    EXPECT_EQ(*adapter.pmName(), "SCM.CPU");
    EXPECT_EQ(*adapter.busName(), "CPU");
  }
}

TEST_F(RuntimeConfigBuilderTest, MuxAdaptersAdded) {
  bsp_tests::BspTestsConfig testConfig;
  testConfig.testData() = std::map<std::string, DeviceTestData>();

  // Create empty kmods
  BspKmodsFile kmods;
  auto runtimeConfig =
      builder_->buildRuntimeConfig(testConfig, pmConfig_, kmods, "sample");

  // get all mux adapters
  std::vector<I2CAdapter> muxAdapters;
  for (const auto& [pmName, adapter] : *runtimeConfig.i2cAdapters()) {
    if (adapter.muxAdapterInfo().has_value()) {
      muxAdapters.push_back(adapter);
    }
  }

  EXPECT_EQ(muxAdapters.size(), 4);

  for (const auto& adapter : muxAdapters) {
    if (adapter.pmName() == "YOLO_MAX.YOLO_MUX1") {
      EXPECT_EQ(*adapter.isCpuAdapter(), false);
      EXPECT_EQ(*adapter.busName(), "YOLO_MUX1");
      EXPECT_EQ(*adapter.muxAdapterInfo()->deviceName(), "pca9x44");
      EXPECT_EQ(*adapter.muxAdapterInfo()->parentAdapterChannel(), 1);
      EXPECT_EQ(*adapter.muxAdapterInfo()->numOutgoingChannels(), 4);
      EXPECT_EQ(*adapter.muxAdapterInfo()->address(), "0x55");
      auto parent_adapter = *adapter.muxAdapterInfo()->parentAdapter();
      EXPECT_EQ(*parent_adapter.isCpuAdapter(), true);
      EXPECT_EQ(*parent_adapter.pmName(), "SCM.CPU");
    }
  }
}

// Test that i2cAdapters are added at the top level of RuntimeConfig correctly
TEST_F(RuntimeConfigBuilderTest, I2cAdaptersTopLevel) {
  // Create a minimal BspTestsConfig
  bsp_tests::BspTestsConfig testConfig;
  testConfig.testData() = std::map<std::string, DeviceTestData>();

  // Create empty kmods
  BspKmodsFile kmods;

  // Build the runtime config using the sample platform config
  auto runtimeConfig =
      builder_->buildRuntimeConfig(testConfig, pmConfig_, kmods, "sample");

  // Verify that the runtime config contains i2cAdapters at the top level
  ASSERT_GT(runtimeConfig.i2cAdapters()->size(), 0);

  // Look for a specific i2c adapter from the sample config
  bool foundAdapter = false;
  for (const auto& [pmName, adapter] : *runtimeConfig.i2cAdapters()) {
    if (*adapter.pmName() == "SCM.SCM_IOB_I2C_0") {
      foundAdapter = true;

      // Check that the adapter has PCI adapter info
      ASSERT_TRUE(adapter.pciAdapterInfo().has_value());
      const auto& pciInfo = *adapter.pciAdapterInfo();

      EXPECT_EQ(*pciInfo.pciInfo()->vendorId(), "0x83bf");
      EXPECT_EQ(*pciInfo.pciInfo()->deviceId(), "0xab87");

      const auto& auxData = pciInfo.auxData();
      EXPECT_EQ(*auxData->name(), "SCM_IOB_I2C_0");
      EXPECT_EQ(*auxData->id()->deviceName(), "i2c-smb");
      EXPECT_EQ(*auxData->iobufOffset(), "0x0023");

      break;
    }
  }

  EXPECT_TRUE(foundAdapter) << "Expected I2C adapter not found at top level";
}

// Test that PCI devices are added correctly with auxDevices
TEST_F(RuntimeConfigBuilderTest, PciDevicesWithAuxDevices) {
  // Create a minimal BspTestsConfig
  bsp_tests::BspTestsConfig testConfig;
  testConfig.testData() = std::map<std::string, DeviceTestData>();

  // Create empty kmods
  BspKmodsFile kmods;

  // Build the runtime config
  auto runtimeConfig =
      builder_->buildRuntimeConfig(testConfig, pmConfig_, kmods, "sample");

  // Verify that the runtime config contains PCI devices with auxDevices
  bool foundPciDevice = false;

  for (const auto& testDevice : *runtimeConfig.devices()) {
    if (*testDevice.pmName() == "SCM.SCM_IOB") {
      foundPciDevice = true;

      // Check that the PCI device has the correct properties
      EXPECT_EQ(*testDevice.pciInfo()->vendorId(), "0x83bf");
      EXPECT_EQ(*testDevice.pciInfo()->deviceId(), "0xab87");

      ASSERT_GT(testDevice.auxDevices()->size(), 0);

      // Look for the SPI aux device
      bool foundSpiAuxDevice = false;
      for (const auto& auxDevice : *testDevice.auxDevices()) {
        if (*auxDevice.name() == "SCM_IOB_SPI_0") {
          foundSpiAuxDevice = true;
          EXPECT_EQ(*auxDevice.id()->deviceName(), "spi");
          EXPECT_EQ(*auxDevice.iobufOffset(), "0x0034");
          break;
        }
      }
      EXPECT_TRUE(foundSpiAuxDevice)
          << "SPI aux device not found in PCI device";

      break;
    }
  }

  EXPECT_TRUE(foundPciDevice)
      << "Expected PCI device not found in runtime config";
}

// Test that RuntimeConfigBuilder can build configs for all real platforms
TEST_F(RuntimeConfigBuilderTest, BuildConfigsForAllRealPlatforms) {
  std::vector<std::string> realPlatforms = {
      "sample",
      "meru800bia",
      "meru800bfa",
      "montblanc",
      "morgan800cc",
      "janga800bic",
      "tahan800bc",
      "darwin",
      "minipack3n",
      "minipack3ba",
      "icecube",
      "darwin48v",
  };

  for (const auto& platformName : realPlatforms) {
    SCOPED_TRACE("Testing platform: " + platformName);

    // Load platform configuration for this platform
    platform_manager::PlatformConfig platformConfig;
    try {
      std::string configJson =
          ConfigLib().getPlatformManagerConfig(platformName);
      apache::thrift::SimpleJSONSerializer::deserialize<
          platform_manager::PlatformConfig>(configJson, platformConfig);
    } catch (const std::exception& e) {
      FAIL() << "Failed to load platform config for " << platformName << ": "
             << e.what();
    }

    // Create a minimal BspTestsConfig for this platform
    bsp_tests::BspTestsConfig testConfig;
    testConfig.testData() = std::map<std::string, DeviceTestData>();

    BspKmodsFile kmods;

    // Attempt to build the runtime config
    RuntimeConfig runtimeConfig;
    EXPECT_NO_THROW({
      runtimeConfig = builder_->buildRuntimeConfig(
          testConfig, platformConfig, kmods, platformName);
    }) << "Failed to build runtime config for platform: "
       << platformName;
  }
}

// Test that hyphens in kmod names are replaced with underscores
TEST_F(RuntimeConfigBuilderTest, KmodNamesHyphenReplacement) {
  // Create a minimal BspTestsConfig
  bsp_tests::BspTestsConfig testConfig;
  testConfig.testData() = std::map<std::string, DeviceTestData>();

  // Create kmods with hyphenated names
  BspKmodsFile kmods;
  std::vector<std::string> kmodNames = {
      "test-kmod-1", "another-hyphenated-kmod", "no_hyphens_here"};
  kmods.bspKmods() = kmodNames;

  // Store original kmod names for verification
  std::vector<std::string> originalKmodNames = kmodNames;

  // Build the runtime config
  auto runtimeConfig =
      builder_->buildRuntimeConfig(testConfig, pmConfig_, kmods, "sample");

  // Verify that hyphens in kmod names have been replaced with underscores
  // We need to check the kmods in the returned RuntimeConfig object
  const auto& modifiedKmods = *runtimeConfig.kmods()->bspKmods();
  ASSERT_EQ(modifiedKmods.size(), originalKmodNames.size());

  for (size_t i = 0; i < modifiedKmods.size(); i++) {
    std::string expected = originalKmodNames[i];
    std::replace(expected.begin(), expected.end(), '-', '_');
    EXPECT_EQ(modifiedKmods[i], expected)
        << "Expected kmod name " << expected << ", got: " << modifiedKmods[i];
  }
}

} // namespace facebook::fboss::platform::bsp_tests
