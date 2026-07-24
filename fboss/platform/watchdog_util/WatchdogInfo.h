// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace facebook::fboss::platform::watchdog_util {

enum class AccessType {
  DEVMEM,
  I2C,
  UNKNOWN,
};

enum class WatchdogFamily {
  CISCO_FPGA, // Cisco FPGA watchdog (block id 33, wd_regs_v4_t)
  ARISTA_SCD, // Arista SCD watchdog_darwin @0x120/@0x304
  FB_FAN_CPLD, // fbfancpld / mp3_fancpld / w800_fancpld / icecube / etc
               // (0x1F/0x20/0x21)
  ARISTA_FAN_CPLD, // Arista fan-cpld-i2c (0x06/0x08/0x0c)
  GENERIC_I2C,
  GENERIC_DEVMEM,
  UNKNOWN,
};

struct WatchdogInfo {
  std::string pmUnitScopedName; // e.g. MCB_FAN_WATCHDOG or FAN_WATCHDOG
  std::string watchdogDev; // e.g. watchdog1
  std::string watchdogPath; // e.g. /dev/watchdog1
  std::string devmapPath; // e.g. /run/devmap/watchdogs/MCB_FAN_WATCHDOG
  AccessType accessType{AccessType::UNKNOWN};
  WatchdogFamily family{WatchdogFamily::UNKNOWN};

  // Resolved hardware details
  bool enabled{false};
  uint32_t timeleft{0}; // seconds
  uint32_t timeout{0}; // seconds
  bool expired{false};

  // Debug / metadata
  std::string typeStr; // "devmem" / "i2c"
  std::optional<int> i2cBus;
  std::optional<int> i2cAddr; // 7-bit address, e.g. 0x33
  std::optional<uint64_t> pciBar0;
  std::optional<uint64_t> csrOffset;
  std::optional<uint64_t> iobufOffset;
  std::string pciSysfsPath;
  std::string kernelDeviceName;
  std::string error;
  bool valid{false};
};

struct ResolvedWatchdog {
  std::string pmUnitScopedName; // symlink filename
  std::string watchdogDev; // watchdogN
  std::string devmapPath;
  std::string symlinkTarget; // /dev/watchdogN
  std::string
      devicePath; // from symbolicLinkToDevicePath, e.g. "/[MCB_FAN_CPLD]"
  std::string devicePmName; // extracted from devicePath, e.g. MCB_FAN_CPLD

  // Config matches
  AccessType accessType{AccessType::UNKNOWN};
  WatchdogFamily family{WatchdogFamily::UNKNOWN};

  // For devmem
  struct PciInfo {
    std::string pmUnitScopedName;
    std::string vendorId;
    std::string deviceId;
    std::string subSystemVendorId;
    std::string subSystemDeviceId;
    std::string sysfsPath;
    uint64_t bar0{0};
  };
  std::optional<PciInfo> pciInfo;
  std::optional<uint64_t> csrOffset;
  std::optional<uint64_t> iobufOffset;
  std::string fpgaDeviceName; // e.g. watchdog_fan, watchdog_darwin

  // For i2c
  std::optional<int> i2cBus; // from realpath
  std::optional<int> i2cAddr; // from realpath, 7-bit
  std::string i2cBusName; // from config, e.g. MCB_IOB_I2C_MASTER_14
  std::string i2cAddressStr; // from config, e.g. 0x33
  std::string kernelDeviceName;
  bool isFanCpldConfig{false};
  bool hasExpiredBit{false}; // mp3_fancpld has BIT6 expired
};

} // namespace facebook::fboss::platform::watchdog_util
