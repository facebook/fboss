// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>

#include "fboss/platform/platform_manager/Utils.h"

namespace fs = std::filesystem;
using namespace ::testing;
using namespace facebook::fboss::platform::platform_manager;

namespace {
std::pair<std::string, std::string> makeDevicePathPair(
    std::string slotPath,
    std::string deviceName) {
  return std::make_pair(slotPath, deviceName);
}
}; // namespace

TEST(UtilsTest, ParseDevicePath) {
  EXPECT_EQ(
      makeDevicePathPair("/", "IDPROM"), Utils().parseDevicePath("/[IDPROM]"));
  EXPECT_EQ(
      makeDevicePathPair("/MCB_SLOT@0", "sensor"),
      Utils().parseDevicePath("/MCB_SLOT@0/[sensor]"));
  EXPECT_EQ(
      makeDevicePathPair("/MCB_SLOT@0/SMB_SLOT@11", "SMB_IOB_I2C_1"),
      Utils().parseDevicePath("/MCB_SLOT@0/SMB_SLOT@11/[SMB_IOB_I2C_1]"));
  EXPECT_NO_THROW(Utils().parseDevicePath("ABCDE/[abc]"));
  EXPECT_NO_THROW(Utils().parseDevicePath("/MCB_SLOT/[abc]"));
  EXPECT_NO_THROW(Utils().parseDevicePath("/MCB_SLOT@1/[]"));
}

TEST(UtilsTest, ComputeHexExpression) {
  Utils utils;

  // Test simple arithmetic expressions
  EXPECT_EQ("0xa", utils.computeHexExpression("5 + 5", 1, 2));
  EXPECT_EQ("0x14", utils.computeHexExpression("10 * 2", 1, 2));
  EXPECT_EQ("0x0", utils.computeHexExpression("10 - 10", 1, 2));
  EXPECT_EQ("0x5", utils.computeHexExpression("15 / 3", 1, 2));

  // Test with hex literals that get converted to decimal
  EXPECT_EQ("0x20", utils.computeHexExpression("0x10 + 0x10", 1, 2));
  EXPECT_EQ("0xff", utils.computeHexExpression("0xf0 + 0xf", 1, 2));
  EXPECT_EQ("0x100", utils.computeHexExpression("0xff + 1", 1, 2));

  // Test with port and led parameters, no startPort
  EXPECT_EQ("0x3", utils.computeHexExpression("{portNum} + {ledNum}", 1, 0, 2));
  EXPECT_EQ(
      "0x14", utils.computeHexExpression("{portNum} * {ledNum}", 4, 0, 5));
  EXPECT_EQ(
      "0x1a",
      utils.computeHexExpression("0x10 + {portNum} * {ledNum}", 2, 0, 5));

  // Test complex expressions
  EXPECT_EQ(
      "0x6c",
      utils.computeHexExpression("0x64 + {portNum} + {ledNum}", 4, 0, 4));

  // Test with zero values
  EXPECT_EQ("0x0", utils.computeHexExpression("{portNum} + {ledNum}", 0, 0, 0));

  // Test with multiple operations
  EXPECT_EQ(
      "0x40418",
      utils.computeHexExpression(
          "0x40410 + ({portNum} - {startPort})*0x8 + ({ledNum} - 1)*0x4",
          2,
          1,
          1));

  // Test with multiple operations
  EXPECT_EQ(
      "0x48474",
      utils.computeHexExpression(
          "0x48410 + ({portNum} - {startPort})*0x8 + ({ledNum} - 1)*0x4",
          45,
          33,
          2));

  // Test with multiple operations
  EXPECT_EQ(
      "0x65e0",
      utils.computeHexExpression(
          "0x65c0 + ({portNum} - {startPort})*0x10 + ({ledNum} - 1)*0x10",
          39,
          39,
          3));

  // Test with invalid expression - should throw
  EXPECT_THROW(
      utils.computeHexExpression("invalid_expression", 1, 2, 1),
      std::runtime_error);

  // Test with optional led parameter (led = std::nullopt)
  // Expression without {ledNum} placeholder
  EXPECT_EQ("0x64", utils.computeHexExpression("0x64", 1));
  EXPECT_EQ("0x69", utils.computeHexExpression("0x64 + {portNum}", 5));
  EXPECT_EQ("0x6e", utils.computeHexExpression("0x64 + {portNum} * 2", 5));

  // Test with startPort but no led
  EXPECT_EQ(
      "0x10",
      utils.computeHexExpression("0x10 + ({portNum} - {startPort})", 5, 5));
  EXPECT_EQ(
      "0x18",
      utils.computeHexExpression(
          "0x10 + ({portNum} - {startPort}) * 0x8", 2, 1));
  EXPECT_EQ(
      "0x40420",
      utils.computeHexExpression(
          "0x40410 + ({portNum} - {startPort}) * 0x8", 3, 1));

  // Test expressions that work with both led present and absent
  // With led
  EXPECT_EQ(
      "0x40418",
      utils.computeHexExpression(
          "0x40410 + ({portNum} - {startPort})*0x8 + ({ledNum} - 1)*0x4",
          2,
          1,
          1));
  // Without led (different expression)
  EXPECT_EQ(
      "0x40418",
      utils.computeHexExpression(
          "0x40410 + ({portNum} - {startPort})*0x8", 2, 1));

  // Test with zero port value and no led
  EXPECT_EQ("0x0", utils.computeHexExpression("{portNum}", 0));
  EXPECT_EQ("0xa", utils.computeHexExpression("{portNum}", 10));

  // Test complex expression without led
  EXPECT_EQ(
      "0x65d0",
      utils.computeHexExpression(
          "0x65c0 + ({portNum} - {startPort})*0x10", 40, 39));
}

TEST(UtilsTest, ConvertHexLiteralsToDecimal) {
  Utils utils;

  // Test basic hex to decimal conversion
  EXPECT_EQ("255", utils.convertHexLiteralsToDecimal("0xff"));
  EXPECT_EQ("16", utils.convertHexLiteralsToDecimal("0x10"));
  EXPECT_EQ("0", utils.convertHexLiteralsToDecimal("0x0"));
  EXPECT_EQ("4095", utils.convertHexLiteralsToDecimal("0xfff"));

  // Test multiple hex literals in expression
  EXPECT_EQ("255 + 16", utils.convertHexLiteralsToDecimal("0xff + 0x10"));
  EXPECT_EQ(
      "255 * 16 - 1", utils.convertHexLiteralsToDecimal("0xff * 0x10 - 0x1"));

  // Test mixed case hex literals
  EXPECT_EQ("255", utils.convertHexLiteralsToDecimal("0xFF"));
  EXPECT_EQ("255", utils.convertHexLiteralsToDecimal("0xFf"));
  EXPECT_EQ("171", utils.convertHexLiteralsToDecimal("0xAb"));

  // Test expressions without hex literals
  EXPECT_EQ("5 + 3", utils.convertHexLiteralsToDecimal("5 + 3"));
  EXPECT_EQ("test_string", utils.convertHexLiteralsToDecimal("test_string"));
  EXPECT_EQ("", utils.convertHexLiteralsToDecimal(""));

  // Test hex literals in complex expressions
  EXPECT_EQ(
      "255 + @port * 16",
      utils.convertHexLiteralsToDecimal("0xff + @port * 0x10"));
}
