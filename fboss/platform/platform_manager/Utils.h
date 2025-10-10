// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <chrono>
#include <functional>
#include <string>

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

  bool checkDeviceReadiness(
      std::function<bool()>&& isDeviceReadyFunc,
      const std::string& onWaitMsg,
      std::chrono::seconds maxWaitSecs);

  virtual int getGpioLineValue(const std::string& charDevPath, int lineIndex)
      const;

  // Compute the expression and return the result as a string.
  std::string computeHexExpression(
      const std::string& expression,
      int port,
      int led,
      int startPort = 1);

  // Replace hex literals with decimal values in expression string
  std::string convertHexLiteralsToDecimal(const std::string& expression);
};

} // namespace facebook::fboss::platform::platform_manager
