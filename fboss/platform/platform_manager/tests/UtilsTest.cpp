// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <chrono>

#include <gtest/gtest.h>

#include "fboss/platform/platform_manager/Utils.h"

using namespace facebook::fboss::platform::platform_manager;

namespace {
std::pair<std::string, std::string> makeDevicePathPair(
    std::string slotPath,
    std::string deviceName) {
  return std::make_pair(slotPath, deviceName);
}
} // namespace

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

TEST(UtilsTest, CreateDevicePath) {
  Utils utils;
  EXPECT_EQ("/[IDPROM]", utils.createDevicePath("/", "IDPROM"));
  EXPECT_EQ(
      "/MCB_SLOT@0/[sensor]", utils.createDevicePath("/MCB_SLOT@0", "sensor"));
  EXPECT_EQ(
      "/MCB_SLOT@0/SMB_SLOT@11/[SMB_IOB_I2C_1]",
      utils.createDevicePath("/MCB_SLOT@0/SMB_SLOT@11", "SMB_IOB_I2C_1"));
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

TEST(UtilsTest, FormatExpression) {
  Utils utils;
  EXPECT_EQ(
      "0x1000 + (1 - 1)*0x4",
      utils.formatExpression(
          "0x1000 + ({portNum} - {startPort})*0x4", 1, 1, std::nullopt));
  EXPECT_EQ(
      "0x1000 + (2 - 1)*0x8 + (2 - 1)*0x4",
      utils.formatExpression(
          "0x1000 + ({portNum} - {startPort})*0x8 + ({ledNum} - 1)*0x4",
          2,
          1,
          2));
  EXPECT_EQ("5", utils.formatExpression("{portNum}", 5, 0, std::nullopt));
  EXPECT_EQ("3", utils.formatExpression("{ledNum}", 0, 0, 3));
}

TEST(UtilsTest, CheckDeviceReadiness) {
  Utils utils;

  EXPECT_TRUE(utils.checkDeviceReadiness(
      []() { return true; }, "ready", std::chrono::seconds(1)));
  EXPECT_FALSE(utils.checkDeviceReadiness(
      []() { return false; }, "not ready", std::chrono::seconds(0)));

  int callCount = 0;
  EXPECT_TRUE(utils.checkDeviceReadiness(
      [&callCount]() { return ++callCount >= 3; },
      "waiting",
      std::chrono::seconds(5)));
  EXPECT_GE(callCount, 3);
}

TEST(UtilsTest, CreateXcvrCtrlConfigs) {
  PciDeviceConfig pciDeviceConfig;
  pciDeviceConfig.xcvrCtrlBlockConfigs() = {};
  EXPECT_TRUE(Utils::createXcvrCtrlConfigs(pciDeviceConfig).empty());

  XcvrCtrlBlockConfig xcvrBlock;
  xcvrBlock.pmUnitScopedNamePrefix() = "SMB_XCVR";
  xcvrBlock.deviceName() = "fbiob-xcvr";
  xcvrBlock.csrOffsetCalc() = "0x1000 + ({portNum} - {startPort})*0x4";
  xcvrBlock.numPorts() = 2;
  xcvrBlock.startPort() = 1;
  xcvrBlock.iobufOffsetCalc() = "0x2000 + ({portNum} - {startPort})*0x4";
  pciDeviceConfig.xcvrCtrlBlockConfigs() = {xcvrBlock};

  auto configs = Utils::createXcvrCtrlConfigs(pciDeviceConfig);
  EXPECT_EQ(2, configs.size());
  EXPECT_EQ(
      "SMB_XCVR_XCVR_CTRL_PORT_1",
      *configs[0].fpgaIpBlockConfig()->pmUnitScopedName());
  EXPECT_EQ("0x1000", *configs[0].fpgaIpBlockConfig()->csrOffset());
  EXPECT_EQ("0x2000", *configs[0].fpgaIpBlockConfig()->iobufOffset());
  EXPECT_EQ(1, *configs[0].portNumber());
  EXPECT_EQ("0x1004", *configs[1].fpgaIpBlockConfig()->csrOffset());
  EXPECT_EQ(2, *configs[1].portNumber());

  xcvrBlock.iobufOffsetCalc() = "";
  pciDeviceConfig.xcvrCtrlBlockConfigs() = {xcvrBlock};
  configs = Utils::createXcvrCtrlConfigs(pciDeviceConfig);
  EXPECT_TRUE(configs[0].fpgaIpBlockConfig()->iobufOffset()->empty());
}

TEST(UtilsTest, CreateLedCtrlConfigs) {
  PciDeviceConfig pciDeviceConfig;
  pciDeviceConfig.ledCtrlBlockConfigs() = {};
  EXPECT_TRUE(Utils::createLedCtrlConfigs(pciDeviceConfig).empty());

  LedCtrlBlockConfig ledBlock;
  ledBlock.pmUnitScopedNamePrefix() = "LED_CTRL";
  ledBlock.deviceName() = "fbiob-led";
  ledBlock.csrOffsetCalc() =
      "0x1000 + ({portNum} - {startPort})*0x8 + ({ledNum} - 1)*0x4";
  ledBlock.numPorts() = 2;
  ledBlock.ledPerPort() = 2;
  ledBlock.startPort() = 1;
  ledBlock.iobufOffsetCalc() =
      "0x2000 + ({portNum} - {startPort})*0x8 + ({ledNum} - 1)*0x4";
  pciDeviceConfig.ledCtrlBlockConfigs() = {ledBlock};

  auto configs = Utils::createLedCtrlConfigs(pciDeviceConfig);
  EXPECT_EQ(4, configs.size());
  EXPECT_EQ(
      "LED_CTRL_PORT_1_LED_1",
      *configs[0].fpgaIpBlockConfig()->pmUnitScopedName());
  EXPECT_EQ("0x1000", *configs[0].fpgaIpBlockConfig()->csrOffset());
  EXPECT_EQ("0x2000", *configs[0].fpgaIpBlockConfig()->iobufOffset());
  EXPECT_EQ(1, *configs[0].portNumber());
  EXPECT_EQ(1, *configs[0].ledId());
  EXPECT_EQ("0x100c", *configs[3].fpgaIpBlockConfig()->csrOffset());
  EXPECT_EQ(2, *configs[3].portNumber());
  EXPECT_EQ(2, *configs[3].ledId());

  ledBlock.iobufOffsetCalc() = "";
  pciDeviceConfig.ledCtrlBlockConfigs() = {ledBlock};
  configs = Utils::createLedCtrlConfigs(pciDeviceConfig);
  EXPECT_TRUE(configs[0].fpgaIpBlockConfig()->iobufOffset()->empty());
}

TEST(UtilsTest, CreateMdioBusConfigs) {
  PciDeviceConfig pciDeviceConfig;
  pciDeviceConfig.mdioBusBlockConfigs() = {};
  EXPECT_TRUE(Utils::createMdioBusConfigs(pciDeviceConfig).empty());

  MdioBusBlockConfig mdioBlock;
  mdioBlock.pmUnitScopedNamePrefix() = "MDIO_BUS";
  mdioBlock.deviceName() = "fbiob-mdio";
  mdioBlock.csrOffsetCalc() = "0x200 + {busIndex}*0x20";
  mdioBlock.numBuses() = 2;
  mdioBlock.iobufOffsetCalc() = "0x100 + {busIndex}*0x4";
  pciDeviceConfig.mdioBusBlockConfigs() = {mdioBlock};

  auto configs = Utils::createMdioBusConfigs(pciDeviceConfig);
  EXPECT_EQ(2, configs.size());
  EXPECT_EQ("MDIO_BUS_1", *configs[0].pmUnitScopedName());
  EXPECT_EQ("fbiob-mdio", *configs[0].deviceName());
  EXPECT_EQ("0x200", *configs[0].csrOffset());
  EXPECT_EQ("0x100", *configs[0].iobufOffset());
  EXPECT_EQ("MDIO_BUS_2", *configs[1].pmUnitScopedName());
  EXPECT_EQ("0x220", *configs[1].csrOffset());
  EXPECT_EQ("0x104", *configs[1].iobufOffset());
}
