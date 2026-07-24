// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace facebook::fboss::platform::watchdog_util {

class DevmemReader {
 public:
  // Read 32-bit value from physical address via /dev/mem
  static std::optional<uint32_t> read32(uint64_t physAddr, std::string& err);

  // Helper to get BAR0 physical address from PCI sysfs path
  static std::optional<uint64_t> getBar0FromSysfs(
      const std::string& sysfsPath,
      std::string& err);

  // Find PCI device sysfs path by vendor/device/subsystem IDs
  static std::optional<std::string> findPciSysfsPath(
      const std::string& vendorId,
      const std::string& deviceId,
      const std::string& subSystemVendorId,
      const std::string& subSystemDeviceId,
      std::string& err);

  // Get sysfs root, overridable via env var for testing
  static std::string getSysfsRoot();

 private:
  static std::string normalizeId(const std::string& id);
};

} // namespace facebook::fboss::platform::watchdog_util
