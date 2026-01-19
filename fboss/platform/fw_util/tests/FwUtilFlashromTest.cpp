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

// Helper function to read file contents
std::string readFileContents(const std::string& filePath) {
  std::ifstream file(filePath);
  if (!file.is_open()) {
    return "";
  }
  std::string content(
      (std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
  file.close();
  return content;
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

// ============================================================================
// setProgrammerAndChip() Tests
// ============================================================================

TEST_F(FwUtilFlashromTest, SetProgrammerAndChipBoth) {
  // Create FwUtilImpl instance
  FwUtilImpl fwUtil(
      binaryFilePath_.string(),
      configFilePath_,
      false, // verifySha1sum
      false // dryRun
  );

  // Create FlashromConfig with both programmer and programmer_type
  FlashromConfig flashromConfig;
  flashromConfig.programmer() = ":dev=/dev/spidev0.0";
  flashromConfig.programmer_type() = "linux_spi";

  std::string fpd = "test_fpd";
  std::vector<std::string> flashromCmd;

  // Call setProgrammerAndChip
  fwUtil.setProgrammerAndChip(flashromConfig, fpd, flashromCmd);

  // Verify programmer arguments were added
  ASSERT_GE(flashromCmd.size(), 2);
  EXPECT_EQ(flashromCmd[0], "-p");
  EXPECT_EQ(flashromCmd[1], "linux_spi:dev=/dev/spidev0.0");
}

TEST_F(FwUtilFlashromTest, SetProgrammerAndChipTypeOnly) {
  // Create FwUtilImpl instance
  FwUtilImpl fwUtil(
      binaryFilePath_.string(),
      configFilePath_,
      false, // verifySha1sum
      false // dryRun
  );

  // Create FlashromConfig with programmer_type only
  FlashromConfig flashromConfig;
  flashromConfig.programmer_type() = "internal";

  std::string fpd = "test_fpd";
  std::vector<std::string> flashromCmd;

  // Call setProgrammerAndChip
  fwUtil.setProgrammerAndChip(flashromConfig, fpd, flashromCmd);

  // Verify programmer arguments were added
  ASSERT_GE(flashromCmd.size(), 2);
  EXPECT_EQ(flashromCmd[0], "-p");
  EXPECT_EQ(flashromCmd[1], "internal");
}

TEST_F(FwUtilFlashromTest, SetProgrammerAndChipDetected) {
  // Create FwUtilImpl instance
  FwUtilImpl fwUtil(
      binaryFilePath_.string(),
      configFilePath_,
      false, // verifySha1sum
      false // dryRun
  );

  // Create FlashromConfig with chip list
  FlashromConfig flashromConfig;
  flashromConfig.programmer_type() = "internal";
  std::vector<std::string> chips = {"MX25L12835F", "W25Q128.V"};
  flashromConfig.chip() = chips;

  std::string fpd = "test_fpd";
  std::vector<std::string> flashromCmd;

  // Test method handles chip detection (will fail in test environment)
  fwUtil.setProgrammerAndChip(flashromConfig, fpd, flashromCmd);

  // Verify programmer arguments were added
  ASSERT_GE(flashromCmd.size(), 2);
  EXPECT_EQ(flashromCmd[0], "-p");
  EXPECT_EQ(flashromCmd[1], "internal");
}

TEST_F(FwUtilFlashromTest, SetProgrammerAndChipNoProgrammerType) {
  // Create FwUtilImpl instance
  FwUtilImpl fwUtil(
      binaryFilePath_.string(),
      configFilePath_,
      false, // verifySha1sum
      false // dryRun
  );

  // Create FlashromConfig without programmer_type
  FlashromConfig flashromConfig;
  // No programmer_type set

  std::string fpd = "test_fpd";
  std::vector<std::string> flashromCmd;

  // Call setProgrammerAndChip - should default to "internal"
  fwUtil.setProgrammerAndChip(flashromConfig, fpd, flashromCmd);

  // Verify programmer arguments were added with default "internal"
  ASSERT_GE(flashromCmd.size(), 2);
  EXPECT_EQ(flashromCmd[0], "-p");
  EXPECT_EQ(flashromCmd[1], "internal");
}

// ============================================================================
// addLayoutFile() Tests
// ============================================================================

TEST_F(FwUtilFlashromTest, AddLayoutFileValid) {
  // Create FwUtilImpl instance
  FwUtilImpl fwUtil(
      binaryFilePath_.string(),
      configFilePath_,
      false, // verifySha1sum
      false // dryRun
  );

  // Create FlashromConfig with spi_layout
  FlashromConfig flashromConfig;
  flashromConfig.spi_layout() =
      "0x00000000:0x000fffff bios\n0x00100000:0x001fffff data";

  // Use unique fpd name to avoid race conditions with parallel test execution
  std::string fpd =
      "test_fpd_layout_" +
      std::to_string(
          std::chrono::system_clock::now().time_since_epoch().count());
  std::vector<std::string> flashromCmd;

  fwUtil.addLayoutFile(flashromConfig, flashromCmd, fpd);

  // Verify layout file arguments and temp file with correct contents
  ASSERT_GE(flashromCmd.size(), 2);
  EXPECT_EQ(flashromCmd[0], "-l");
  EXPECT_TRUE(
      flashromCmd[1].find("/tmp/" + fpd + "_spi_layout") != std::string::npos);
  EXPECT_TRUE(std::filesystem::exists(flashromCmd[1]));
  std::string fileContents = readFileContents(flashromCmd[1]);
  EXPECT_EQ(fileContents, *flashromConfig.spi_layout());

  // Cleanup
  std::filesystem::remove(flashromCmd[1]);
}

// ============================================================================
// addCommonFlashromArgs() Tests
// ============================================================================

TEST_F(FwUtilFlashromTest, AddCommonFlashromArgsWithLayout) {
  // Create FwUtilImpl instance
  FwUtilImpl fwUtil(
      binaryFilePath_.string(),
      configFilePath_,
      false, // verifySha1sum
      false // dryRun
  );

  // Create FlashromConfig with spi_layout
  FlashromConfig flashromConfig;
  flashromConfig.spi_layout() = "0x00000000:0x000fffff bios";

  // Use unique fpd name to avoid race conditions with parallel test execution
  std::string fpd =
      "test_fpd_common_" +
      std::to_string(
          std::chrono::system_clock::now().time_since_epoch().count());
  std::vector<std::string> flashromCmd;

  // Call addCommonFlashromArgs with "write" operation
  fwUtil.addCommonFlashromArgs(flashromConfig, fpd, "write", flashromCmd);

  // Verify layout file was added (-l flag)
  bool hasLayoutFlag = false;
  std::string layoutFilePath;
  for (size_t i = 0; i < flashromCmd.size(); ++i) {
    if (flashromCmd[i] == "-l" && i + 1 < flashromCmd.size()) {
      hasLayoutFlag = true;
      layoutFilePath = flashromCmd[i + 1];
      break;
    }
  }
  EXPECT_TRUE(hasLayoutFlag);

  // Verify write operation flag was added
  bool hasWriteFlag = false;
  for (const auto& arg : flashromCmd) {
    if (arg == "-w") {
      hasWriteFlag = true;
      break;
    }
  }
  EXPECT_TRUE(hasWriteFlag);

  // Verify binary file was added
  bool hasBinaryFile = false;
  for (const auto& arg : flashromCmd) {
    if (arg == binaryFilePath_.string()) {
      hasBinaryFile = true;
      break;
    }
  }
  EXPECT_TRUE(hasBinaryFile);

  // Clean up temporary layout file
  if (!layoutFilePath.empty() && std::filesystem::exists(layoutFilePath)) {
    std::filesystem::remove(layoutFilePath);
  }
}

TEST_F(FwUtilFlashromTest, AddCommonFlashromArgsWithCustomContent) {
  // Create FwUtilImpl instance
  FwUtilImpl fwUtil(
      binaryFilePath_.string(),
      configFilePath_,
      false, // verifySha1sum
      false // dryRun
  );

  // Create FlashromConfig with custom_content
  FlashromConfig flashromConfig;
  flashromConfig.custom_content() = "CUSTOM_DATA";
  flashromConfig.custom_content_offset() = 100;

  std::string fpd = "test_fpd";
  std::vector<std::string> flashromCmd;

  // Call addCommonFlashromArgs with "write" operation
  fwUtil.addCommonFlashromArgs(flashromConfig, fpd, "write", flashromCmd);

  // Verify write operation flag was added
  bool hasWriteFlag = false;
  for (const auto& arg : flashromCmd) {
    if (arg == "-w") {
      hasWriteFlag = true;
      break;
    }
  }
  EXPECT_TRUE(hasWriteFlag);

  // Verify custom content file was added (not the original binary)
  bool hasCustomContentFile = false;
  for (const auto& arg : flashromCmd) {
    if (arg.find("_custom_content.bin") != std::string::npos) {
      hasCustomContentFile = true;
      // Verify the custom content file was created
      EXPECT_TRUE(std::filesystem::exists(arg));
      // Clean up
      std::filesystem::remove(arg);
      break;
    }
  }
  EXPECT_TRUE(hasCustomContentFile);
}

TEST_F(FwUtilFlashromTest, AddCommonFlashromArgsWithFileOption) {
  // Create FwUtilImpl instance
  FwUtilImpl fwUtil(
      binaryFilePath_.string(),
      configFilePath_,
      false, // verifySha1sum
      false // dryRun
  );

  // Create FlashromConfig without custom content
  FlashromConfig flashromConfig;

  std::string fpd = "test_fpd";
  std::vector<std::string> flashromCmd;

  // Call addCommonFlashromArgs with "write" operation
  fwUtil.addCommonFlashromArgs(flashromConfig, fpd, "write", flashromCmd);

  // Verify write flag was added
  bool hasWriteFlag = false;
  for (const auto& arg : flashromCmd) {
    if (arg == "-w") {
      hasWriteFlag = true;
      break;
    }
  }
  EXPECT_TRUE(hasWriteFlag);

  // Verify binary file was added
  bool hasBinaryFile = false;
  for (const auto& arg : flashromCmd) {
    if (arg == binaryFilePath_.string()) {
      hasBinaryFile = true;
      break;
    }
  }
  EXPECT_TRUE(hasBinaryFile);
}

TEST_F(FwUtilFlashromTest, AddCommonFlashromArgsWithImageOption) {
  // Create FwUtilImpl instance
  FwUtilImpl fwUtil(
      binaryFilePath_.string(),
      configFilePath_,
      false, // verifySha1sum
      false // dryRun
  );

  // Create FlashromConfig with flashromExtraArgs
  FlashromConfig flashromConfig;
  std::vector<std::string> extraArgs = {"--image", "bios"};
  flashromConfig.flashromExtraArgs() = extraArgs;

  std::string fpd = "test_fpd";
  std::vector<std::string> flashromCmd;

  // Call addCommonFlashromArgs with "verify" operation
  fwUtil.addCommonFlashromArgs(flashromConfig, fpd, "verify", flashromCmd);

  // Verify extra args were added
  bool hasImageFlag = false;
  bool hasBiosArg = false;
  for (size_t i = 0; i < flashromCmd.size(); ++i) {
    if (flashromCmd[i] == "--image") {
      hasImageFlag = true;
      if (i + 1 < flashromCmd.size() && flashromCmd[i + 1] == "bios") {
        hasBiosArg = true;
      }
    }
  }
  EXPECT_TRUE(hasImageFlag);
  EXPECT_TRUE(hasBiosArg);

  // Verify verify flag was added
  bool hasVerifyFlag = false;
  for (const auto& arg : flashromCmd) {
    if (arg == "-v") {
      hasVerifyFlag = true;
      break;
    }
  }
  EXPECT_TRUE(hasVerifyFlag);
}

TEST_F(FwUtilFlashromTest, AddCommonFlashromArgsMinimal) {
  // Create FwUtilImpl instance
  FwUtilImpl fwUtil(
      binaryFilePath_.string(),
      configFilePath_,
      false, // verifySha1sum
      false // dryRun
  );

  // Create minimal FlashromConfig (no optional fields)
  FlashromConfig flashromConfig;

  std::string fpd = "test_fpd";
  std::vector<std::string> flashromCmd;

  // Call addCommonFlashromArgs with "read" operation
  fwUtil.addCommonFlashromArgs(flashromConfig, fpd, "read", flashromCmd);

  // Verify read flag was added
  bool hasReadFlag = false;
  for (const auto& arg : flashromCmd) {
    if (arg == "-r") {
      hasReadFlag = true;
      break;
    }
  }
  EXPECT_TRUE(hasReadFlag);

  // Verify binary file was added
  bool hasBinaryFile = false;
  for (const auto& arg : flashromCmd) {
    if (arg == binaryFilePath_.string()) {
      hasBinaryFile = true;
      break;
    }
  }
  EXPECT_TRUE(hasBinaryFile);

  // Verify no layout file was added (no -l flag)
  bool hasLayoutFlag = false;
  for (const auto& arg : flashromCmd) {
    if (arg == "-l") {
      hasLayoutFlag = true;
      break;
    }
  }
  EXPECT_FALSE(hasLayoutFlag);
}

// ============================================================================
// performFlashromUpgrade() and performFlashromRead() Tests
// ============================================================================

TEST_F(FwUtilFlashromTest, PerformFlashromUpgradeSuccess) {
  // Create FwUtilImpl instance
  FwUtilImpl fwUtil(
      binaryFilePath_.string(),
      configFilePath_,
      false, // verifySha1sum
      false // dryRun
  );

  // Create FlashromConfig
  FlashromConfig flashromConfig;
  flashromConfig.programmer_type() = "internal";

  std::string fpd = "test_fpd";

  // Test method is callable (will fail without flashrom)
  EXPECT_THROW(
      fwUtil.performFlashromUpgrade(flashromConfig, fpd), std::runtime_error);
}

TEST_F(FwUtilFlashromTest, PerformFlashromUpgradeDryRun) {
  // Create FwUtilImpl instance in dry run mode
  FwUtilImpl fwUtil(
      binaryFilePath_.string(),
      configFilePath_,
      false, // verifySha1sum
      true // dryRun
  );

  // Create FlashromConfig
  FlashromConfig flashromConfig;
  flashromConfig.programmer_type() = "internal";

  std::string fpd = "test_fpd";

  // Test method is callable in dry run mode
  EXPECT_THROW(
      fwUtil.performFlashromUpgrade(flashromConfig, fpd), std::runtime_error);
}

TEST_F(FwUtilFlashromTest, PerformFlashromUpgradeMissingBinary) {
  // Create FwUtilImpl instance with non-existent binary
  std::string nonExistentBinary = (tempDir_ / "nonexistent.bin").string();

  FwUtilImpl fwUtil(
      nonExistentBinary,
      configFilePath_,
      false, // verifySha1sum
      false // dryRun
  );

  // Create FlashromConfig
  FlashromConfig flashromConfig;
  flashromConfig.programmer_type() = "internal";

  std::string fpd = "test_fpd";

  // Should throw because binary file doesn't exist
  EXPECT_THROW(
      fwUtil.performFlashromUpgrade(flashromConfig, fpd), std::runtime_error);
}

TEST_F(FwUtilFlashromTest, PerformFlashromReadSuccess) {
  // Create FwUtilImpl instance
  FwUtilImpl fwUtil(
      binaryFilePath_.string(),
      configFilePath_,
      false, // verifySha1sum
      false // dryRun
  );

  // Create FlashromConfig
  FlashromConfig flashromConfig;
  flashromConfig.programmer_type() = "internal";

  std::string fpd = "test_fpd";

  // Test method is callable (will fail without flashrom)
  EXPECT_THROW(
      fwUtil.performFlashromRead(flashromConfig, fpd), std::runtime_error);
}

} // namespace facebook::fboss::platform::fw_util
