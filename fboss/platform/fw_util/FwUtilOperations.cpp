#include <folly/logging/xlog.h>
#include <filesystem>
#include <fstream>
#include <fmt/format.h>
#include "fboss/platform/fw_util/Flags.h"
#include "fboss/platform/fw_util/FwUtilImpl.h"
#include "fboss/platform/fw_util/fw_util_helpers.h"
#include "fboss/platform/helpers/PlatformUtils.h"
#include "fboss/platform/platform_manager/I2cExplorer.h"

namespace facebook::fboss::platform::fw_util {

using namespace facebook::fboss::platform::platform_manager;

void FwUtilImpl::doJtagOperation(
    const JtagConfig& config,
    const std::string& fpd) {
  XLOG(INFO) << "enabling jtag register for " << fpd
             << " as preUpgrade operation";
  if (!config.path()->empty()) {
    // print debug statement of what's being writen to which path
    XLOG(INFO) << "writing: " << *config.value()
               << " to path: " << *config.path();
    std::ofstream outputFile(*config.path());
    outputFile << std::to_string(*config.value());
    outputFile.close();
  } else {
    XLOG(ERR) << "Invalid JTAG configuration";
  }
}

void FwUtilImpl::doGpiosetOperation(
    const GpiosetConfig& config,
    const std::string& fpd) {
  XLOG(INFO) << "setting gpio register for " << fpd
             << " as preUpgrade operation";
  std::string gpioChip = getGpioChip(*config.gpioChip());
  // Execute the gpioset command
  std::vector<std::string> gpiosetCmd = {
      "/usr/bin/gpioset", gpioChip, *config.gpioChipPin() + "=1"};
  auto [exitStatus, standardOut] = PlatformUtils().runCommand(gpiosetCmd);
  checkCmdStatus(gpiosetCmd, exitStatus);
}

void FwUtilImpl::doWriteToPortOperation(
    const WriteToPortConfig& args,
    const std::string& platformName) {
  XLOG(INFO) << "Writing specific bytes to I/O port for " << platformName
             << " preUpgrade";

  // Open the port file in binary mode
  std::ofstream port(
      args.portFile().value(), std::ios_base::out | std::ios_base::binary);
  if (!port.is_open()) {
    XLOG(ERR) << "Failed to open " << args.portFile().value();
    return;
  }

  // Seek to the specified offset
  port.seekp(
      static_cast<int>(strtol(args.hexOffset().value().c_str(), nullptr, 16)));
  if (!port.good()) {
    XLOG(ERR) << "Failed to seek to offset " << args.hexOffset().value();
    return;
  }

  // Write the byte value to the port file
  char data = static_cast<char>(
      strtol(args.hexByteValue().value().c_str(), nullptr, 16));
  port.write(&data, 1);
  if (!port.good()) {
    XLOG(ERR) << "Failed to write to " << args.portFile().value();
    return;
  }

  port.close();
}

// Besides getting GPIO value, the gpioget command is also used to reset the
// input/output line direction. The operation below will reset the direction.
void FwUtilImpl::doGpiogetOperation(
    const GpiogetConfig& config,
    const std::string& fpd) {
  XLOG(INFO) << "Getting GPIO value for " << fpd;
  std::string gpioChip = getGpioChip(*config.gpioChip());
  std::vector<std::string> gpiogetCmd = {
      "/usr/bin/gpioget", gpioChip, *config.gpioChipPin()};
  auto [exitStatus, standardOut] = PlatformUtils().runCommand(gpiogetCmd);
  checkCmdStatus(gpiogetCmd, exitStatus);
}

void FwUtilImpl::performJamUpgrade(const JamConfig& jamConfig) {
  // Check if the firmware binary file is present
  if (!std::filesystem::exists(fwBinaryFile_)) {
    throw std::runtime_error(
        "Firmware binary file not found: " + fwBinaryFile_);
  }

  // Determine the JAM binary location
  std::string jamBinaryPath = getUpgradeToolBinaryPath("jam");

  // Construct the JAM command with the provided configuration
  std::vector<std::string> jamCmd = {jamBinaryPath};
  if (jamConfig.jamExtraArgs().has_value()) {
    for (const auto& arg : *jamConfig.jamExtraArgs()) {
      jamCmd.push_back(arg);
    }
  }

  jamCmd.push_back(fwBinaryFile_);

  // Execute the JAM command
  auto [exitStatus, standardOut] = PlatformUtils().runCommand(jamCmd);

  // we want to see the output of the command first
  XLOG(INFO) << standardOut;
  // let's check the exist status which will throw an exception if the command
  // failed.
  checkCmdStatus(jamCmd, exitStatus);
}

void FwUtilImpl::performXappUpgrade(const XappConfig& xappConfig) {
  // Check if the firmware binary file is present
  if (!std::filesystem::exists(fwBinaryFile_)) {
    throw std::runtime_error(
        "Firmware binary file not found: " + fwBinaryFile_);
  }

  // Determine the XAPP binary location
  std::string xappBinaryPath = getUpgradeToolBinaryPath("xapp");

  // Construct the XAPP command with the provided configuration
  std::vector<std::string> xappCmd = {xappBinaryPath};
  if (xappConfig.xappExtraArgs().has_value()) {
    for (const auto& arg : *xappConfig.xappExtraArgs()) {
      xappCmd.push_back(arg);
    }
  }
  xappCmd.push_back(fwBinaryFile_);

  // Execute the XAPP command
  auto [exitStatus, standardOut] = PlatformUtils().runCommand(xappCmd);

  // we want to see the output of the command first
  XLOG(INFO) << standardOut;

  checkCmdStatus(xappCmd, exitStatus);
}

// Function to firwmare upgrade and read for I2cEeprom
void FwUtilImpl::performI2cEepromOperation(
    const I2cEepromConfig& config, const std::string& fpd) {
  std::string cmd, eepromPath;
  std::string driverName = *config.driverName();
  std::string deviceAddress = *config.deviceAddress();
  uint16_t i2cBusNum;
  I2cExplorer i2cExplorer;
  I2cAddr addr(deviceAddress);

  // 1.Getting the i2cbus number from config path
  i2cBusNum =
      i2cExplorer.extractBusNumFromPath(std::filesystem::read_symlink(*config.path()));

  // 2.Bind the driver and get the eeprom path
  i2cExplorer.createI2cDevice("i2cEeprom", driverName, i2cBusNum, addr);
  eepromPath = i2cExplorer.getI2cEepromPath(i2cBusNum, addr);

  // 3.Run the DD command for FW write or read
  cmd = fmt::format(
      "dd if={} of={}", FLAGS_fw_binary_file, eepromPath);
  auto [exitStatus, standardOut] = PlatformUtils().execCommand(cmd);
  if (exitStatus != 0) {
    throw std::runtime_error("Run " + cmd + " failed!");
  }
  XLOG(INFO) << standardOut;

  // 4.Unbind the driver
  i2cExplorer.deleteI2cDevice(i2cBusNum, addr);
}

} // namespace facebook::fboss::platform::fw_util
