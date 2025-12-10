// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/fw_util/FwUtilImpl.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <chrono>
#include <filesystem>
#include <fstream>

namespace facebook::fboss::platform::fw_util {

class FwUtilPreUpgradeTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Create unique temp directory for this test
    tempDir_ = std::filesystem::temp_directory_path() /
        ("fw_util_preupgrade_test_" +
         std::to_string(
             std::chrono::system_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(tempDir_);

    // Create test binary file
    binaryFile_ = (tempDir_ / "test_binary.bin").string();
    std::ofstream binFile(binaryFile_);
    binFile << "test binary content";
    binFile.close();

    // Create platform name file if it doesn't exist
    std::filesystem::path platformPath = "/var/facebook/fboss/platform_name";
    if (!std::filesystem::exists(platformPath)) {
      std::ofstream platformFile(platformPath);
      if (platformFile.is_open()) {
        platformFile << "test_platform";
        platformFile.close();
        createdPlatformFile_ = true;
      }
    }
  }

  void TearDown() override {
    // Clean up temp directory
    if (std::filesystem::exists(tempDir_)) {
      std::filesystem::remove_all(tempDir_);
    }

    // Clean up platform file if we created it
    if (createdPlatformFile_) {
      std::filesystem::remove("/var/facebook/fboss/platform_name");
    }
  }

  void createMinimalConfig(const std::string& configFile) {
    std::ofstream file(configFile);
    file << R"({
      "fwConfigs": {
        "test_device": {
          "version": {
            "versionType": "sysfs",
            "path": "/run/devmap/sensors/test_device/version"
          },
          "priority": 1
        }
      }
    })";
    file.close();
  }

  std::filesystem::path tempDir_;
  std::string binaryFile_;
  bool createdPlatformFile_ = false;
};

// Test doPreUpgradeOperation with empty command type
TEST_F(FwUtilPreUpgradeTest, DoPreUpgradeOperationEmptyCommandType) {
  std::string configFile = (tempDir_ / "test_config.json").string();
  createMinimalConfig(configFile);

  FwUtilImpl fwUtil(binaryFile_, configFile, false, false);

  PreFirmwareOperationConfig operation;
  operation.commandType() = "";

  // Should return early without throwing
  EXPECT_NO_THROW(fwUtil.doPreUpgradeOperation(operation, "test_device"));
}

// Test doPreUpgradeOperation with jtag and args
TEST_F(FwUtilPreUpgradeTest, DoPreUpgradeOperationJtagWithArgs) {
  std::string configFile = (tempDir_ / "test_config.json").string();
  createMinimalConfig(configFile);

  FwUtilImpl fwUtil(binaryFile_, configFile, false, false);

  PreFirmwareOperationConfig operation;
  operation.commandType() = "jtag";

  JtagConfig jtagConfig;
  jtagConfig.path() = (tempDir_ / "jtag_test.txt").string();
  jtagConfig.value() = 1;
  operation.jtagArgs() = jtagConfig;

  // Should call doJtagOperation and create file
  EXPECT_NO_THROW(fwUtil.doPreUpgradeOperation(operation, "test_device"));

  // Verify file was created
  EXPECT_TRUE(std::filesystem::exists(jtagConfig.path().value()));
}

// Test doPreUpgradeOperation with invalid configs logs error
TEST_F(FwUtilPreUpgradeTest, DoPreUpgradeOperationInvalidConfigs) {
  std::string configFile = (tempDir_ / "test_config.json").string();
  createMinimalConfig(configFile);

  FwUtilImpl fwUtil(binaryFile_, configFile, false, false);

  // Test 1: Missing args (command type matches but no args)
  PreFirmwareOperationConfig operation1;
  operation1.commandType() = "jtag";
  EXPECT_NO_THROW(fwUtil.doPreUpgradeOperation(operation1, "test_device"));

  // Test 2: Unsupported command type
  PreFirmwareOperationConfig operation2;
  operation2.commandType() = "unsupported_type";
  EXPECT_NO_THROW(fwUtil.doPreUpgradeOperation(operation2, "test_device"));
}

} // namespace facebook::fboss::platform::fw_util
