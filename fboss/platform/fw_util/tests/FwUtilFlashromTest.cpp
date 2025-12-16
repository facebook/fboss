// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/fw_util/FwUtilImpl.h"

#include <gtest/gtest.h>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

namespace facebook::fboss::platform::fw_util {

namespace {

// Helper function to create a minimal valid config file for testing
std::string createMinimalConfigFile(const std::string& tempDir) {
  std::string configPath = tempDir + "/fw_util.json";
  std::ofstream configFile(configPath);
  configFile << R"({
  "fwConfigs": {
    "test_bios": {
      "version": {
        "versionType": "sysfs",
        "path": "/sys/devices/virtual/dmi/id/bios_version"
      },
      "priority": 1,
      "desiredVersion": "1.0.0",
      "sha1sum": "0000000000000000000000000000000000000000"
    }
  }
})";
  configFile.close();
  return configPath;
}

} // namespace

class FwUtilFlashromTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Create a unique temporary directory for test files
    tempDir_ = std::filesystem::temp_directory_path() /
        ("fw_util_flashrom_test_" +
         std::to_string(
             std::chrono::system_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(tempDir_);

    // Create minimal config and binary files
    configFilePath_ = createMinimalConfigFile(tempDir_.string());
    binaryFilePath_ = tempDir_ / "test_binary.bin";

    // Create binary file with some content
    std::ofstream binaryFile(binaryFilePath_, std::ios::binary);
    // Create a 1KB file for testing
    std::string content(1024, 'A');
    binaryFile << content;
    binaryFile.close();
  }

  void TearDown() override {
    // Clean up temporary directory
    if (std::filesystem::exists(tempDir_)) {
      std::filesystem::remove_all(tempDir_);
    }
  }

  std::filesystem::path tempDir_;
  std::string configFilePath_;
  std::filesystem::path binaryFilePath_;
};

// ============================================================================
// detectFlashromChip() Tests
// ============================================================================

TEST_F(FwUtilFlashromTest, DetectFlashromChipFound) {
  // Create FwUtilImpl instance
  FwUtilImpl fwUtil(
      binaryFilePath_.string(),
      configFilePath_,
      false, // verifySha1sum
      false // dryRun
  );

  // Set up spiChip_ map with test chips
  std::string fpd = "test_fpd";
  fwUtil.spiChip_[fpd] = {"MX25L12835F", "W25Q128.V", "N25Q128..3E"};

  // Create FlashromConfig with programmer_type
  FlashromConfig flashromConfig;
  flashromConfig.programmer_type() = "internal";

  // Test method returns empty string when flashrom command fails
  std::string detectedChip = fwUtil.detectFlashromChip(flashromConfig, fpd);
  EXPECT_EQ(detectedChip, "");
}

TEST_F(FwUtilFlashromTest, DetectFlashromChipNotFound) {
  // Create FwUtilImpl instance
  FwUtilImpl fwUtil(
      binaryFilePath_.string(),
      configFilePath_,
      false, // verifySha1sum
      false // dryRun
  );

  // Set up spiChip_ map with test chips
  std::string fpd = "test_fpd";
  fwUtil.spiChip_[fpd] = {"NonExistentChip1", "NonExistentChip2"};

  // Create FlashromConfig with programmer_type
  FlashromConfig flashromConfig;
  flashromConfig.programmer_type() = "internal";

  // Test returns empty string when no chip matches
  std::string detectedChip = fwUtil.detectFlashromChip(flashromConfig, fpd);
  EXPECT_EQ(detectedChip, "");
}

TEST_F(FwUtilFlashromTest, DetectFlashromChipMultiple) {
  // Create FwUtilImpl instance
  FwUtilImpl fwUtil(
      binaryFilePath_.string(),
      configFilePath_,
      false, // verifySha1sum
      false // dryRun
  );

  // Set up spiChip_ map with multiple test chips
  std::string fpd = "test_fpd";
  fwUtil.spiChip_[fpd] = {"FirstChip", "SecondChip", "ThirdChip"};

  // Create FlashromConfig with programmer_type
  FlashromConfig flashromConfig;
  flashromConfig.programmer_type() = "internal";

  // Test method handles multiple chips without crashing
  std::string detectedChip = fwUtil.detectFlashromChip(flashromConfig, fpd);
  EXPECT_EQ(detectedChip, "");
}

// ============================================================================
// createCustomContentFile() Tests
// ============================================================================

TEST_F(FwUtilFlashromTest, CreateCustomContentFileSuccess) {
  // Create FwUtilImpl instance
  FwUtilImpl fwUtil(
      binaryFilePath_.string(),
      configFilePath_,
      false, // verifySha1sum
      false // dryRun
  );

  // Prepare test data
  std::string customContent = "CUSTOM_DATA";
  int customContentOffset = 100;
  std::string customContentFileName =
      (tempDir_ / "custom_content.bin").string();

  // Call createCustomContentFile
  bool result = fwUtil.createCustomContentFile(
      customContent, customContentOffset, customContentFileName);

  // Verify success
  EXPECT_TRUE(result);

  // Verify file was created
  EXPECT_TRUE(std::filesystem::exists(customContentFileName));

  // Verify file size matches binary file size
  auto binarySize = std::filesystem::file_size(binaryFilePath_);
  auto customSize = std::filesystem::file_size(customContentFileName);
  EXPECT_EQ(customSize, binarySize);

  // Verify custom content was written at correct offset
  std::ifstream customFile(customContentFileName, std::ios::binary);
  customFile.seekg(customContentOffset);
  std::string readContent(customContent.length(), '\0');
  customFile.read(&readContent[0], customContent.length());
  customFile.close();

  EXPECT_EQ(readContent, customContent);
}

TEST_F(FwUtilFlashromTest, CreateCustomContentFileFailure) {
  // Create FwUtilImpl instance with non-existent binary file
  std::string nonExistentBinary = (tempDir_ / "nonexistent.bin").string();

  FwUtilImpl fwUtil(
      nonExistentBinary,
      configFilePath_,
      false, // verifySha1sum
      false // dryRun
  );

  // Prepare test data
  std::string customContent = "CUSTOM_DATA";
  int customContentOffset = 100;
  std::string customContentFileName =
      (tempDir_ / "custom_content.bin").string();

  // Call createCustomContentFile with non-existent source binary
  bool result = fwUtil.createCustomContentFile(
      customContent, customContentOffset, customContentFileName);

  // Should return false when source binary doesn't exist
  EXPECT_FALSE(result);
}

} // namespace facebook::fboss::platform::fw_util
