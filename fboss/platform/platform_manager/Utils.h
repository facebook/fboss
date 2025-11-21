// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <chrono>
#include <functional>
#include <optional>
#include <string>
#include "fboss/platform/platform_manager/gen-cpp2/platform_manager_config_types.h"

namespace facebook::fboss::platform::platform_manager {

class Utils {
 public:
  virtual ~Utils() = default;
  // Extract (SlotPath, DeviceName) from DevicePath.
  // Returns a pair of (SlotPath, DeviceName). Throws if DevicePath is invalid.
  // Eg: /MCB_SLOT@0/[IDPROM] will return std::pair("/MCB_SLOT@0", "IDPROM")
  std::pair<std::string, std::string> parseDevicePath(
      const std::string& devicePath);

  // Construct and Return a DevicePath from given SlotPath and DeviceName
  // Eg: SlotPath:"/MCB_SLOT@0", DeviceName:"IDPROM" will return
  // /MCB_SLOT@0/[IDPROM]
  std::string createDevicePath(
      const std::string& slotPath,
      const std::string& deviceName);

  // Explore and resolve GpioChip's CharDevicePath for given SysfsPath.
  // Throws an exception when it fails to resolve CharDevicePath
  std::string resolveGpioChipCharDevPath(const std::string& sysfsPath);

  // Explore and resolve Watchdogs's CharDevicePath for given SysfsPath.
  // Throws an exception when it fails to resolve CharDevicePath
  std::string resolveWatchdogCharDevPath(const std::string& sysfsPath);

  // Explore and resolve MdioBus's CharDevicePath for given SysfsPath.
  // Throws an exception when it fails to resolve CharDevicePath
  std::string resolveMdioBusCharDevPath(uint32_t instanceId);

  bool checkDeviceReadiness(
      std::function<bool()>&& isDeviceReadyFunc,
      const std::string& onWaitMsg,
      std::chrono::seconds maxWaitSecs);

  virtual int getGpioLineValue(const std::string& charDevPath, int lineIndex)
      const;

  // Format the expression by substituting port, startPort, and led parameters
  std::string formatExpression(
      const std::string& expression,
      int port,
      int startPort,
      std::optional<int> led);

  // Evaluate a mathematical expression and return the result as a hex string
  std::string evaluateExpression(const std::string& expression);

  // Compute the expression and return the result as a string.
  std::string computeHexExpression(
      const std::string& expression,
      int port,
      int startPort = 1,
      std::optional<int> led = std::nullopt);

  // Replace hex literals with decimal values in expression string
  std::string convertHexLiteralsToDecimal(const std::string& expression);

  // Create the XCVR Controller Config block based on the given xcvrCtrlConfig
  // residing at the given PciDevice. Throw std::runtime_error on failure.
  static std::vector<XcvrCtrlConfig> createXcvrCtrlConfigs(
      const PciDeviceConfig& pciDeviceConfig);

  // Create the LED Controller Config block based on the given ledCtrlConfig
  // residing at the given PciDevice. Throw std::runtime_error on failure.
  static std::vector<LedCtrlConfig> createLedCtrlConfigs(
      const PciDeviceConfig& pciDeviceConfig);
};

} // namespace facebook::fboss::platform::platform_manager
