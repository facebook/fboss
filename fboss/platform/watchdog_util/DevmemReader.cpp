// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/watchdog_util/DevmemReader.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <filesystem>
#include <fstream>
#include <string>

#include <folly/FileUtil.h>
#include <folly/String.h>
#include <folly/logging/xlog.h>

namespace fs = std::filesystem;

namespace facebook::fboss::platform::watchdog_util {

std::string DevmemReader::getSysfsRoot() {
  if (const char* env = std::getenv("WATCHDOG_UTIL_SYSFS_ROOT")) {
    std::string root = env;
    // Trim trailing slash
    if (!root.empty() && root.back() == '/') {
      root.pop_back();
    }
    return root;
  }
  return "/sys";
}

std::string DevmemReader::normalizeId(const std::string& id) {
  // Trim whitespace and lowercase
  std::string s = folly::trimWhitespace(id).str();
  std::transform(s.begin(), s.end(), s.begin(), ::tolower);
  // Ensure 0x prefix for comparison with sysfs which has 0x
  if (!s.starts_with("0x")) {
    s = "0x" + s;
  }
  return s;
}

std::optional<std::string> DevmemReader::findPciSysfsPath(
    const std::string& vendorId,
    const std::string& deviceId,
    const std::string& subSystemVendorId,
    const std::string& subSystemDeviceId,
    std::string& err) {
  auto normVendor = normalizeId(vendorId);
  auto normDevice = normalizeId(deviceId);
  auto normSubVendor = normalizeId(subSystemVendorId);
  auto normSubDevice = normalizeId(subSystemDeviceId);

  std::string sysfsRoot = getSysfsRoot();
  std::string pciDevicesPath = sysfsRoot + "/bus/pci/devices";

  try {
    for (const auto& entry : fs::directory_iterator(pciDevicesPath)) {
      std::string vendor, device, subVendor, subDevice;
      auto vendorPath = entry.path() / "vendor";
      auto devicePath = entry.path() / "device";
      auto subVendorPath = entry.path() / "subsystem_vendor";
      auto subDevicePath = entry.path() / "subsystem_device";

      if (!folly::readFile(vendorPath.c_str(), vendor) ||
          !folly::readFile(devicePath.c_str(), device) ||
          !folly::readFile(subVendorPath.c_str(), subVendor) ||
          !folly::readFile(subDevicePath.c_str(), subDevice)) {
        continue;
      }

      vendor = folly::trimWhitespace(vendor).str();
      device = folly::trimWhitespace(device).str();
      subVendor = folly::trimWhitespace(subVendor).str();
      subDevice = folly::trimWhitespace(subDevice).str();

      std::transform(vendor.begin(), vendor.end(), vendor.begin(), ::tolower);
      std::transform(device.begin(), device.end(), device.begin(), ::tolower);
      std::transform(
          subVendor.begin(), subVendor.end(), subVendor.begin(), ::tolower);
      std::transform(
          subDevice.begin(), subDevice.end(), subDevice.begin(), ::tolower);

      if (vendor == normVendor && device == normDevice &&
          subVendor == normSubVendor && subDevice == normSubDevice) {
        XLOG(DBG2) << "Found PCI device at " << entry.path().string() << " for "
                   << normVendor << ":" << normDevice;
        return entry.path().string();
      }
    }
  } catch (const std::exception& ex) {
    err = fmt::format("Exception scanning PCI devices: {}", ex.what());
    return std::nullopt;
  }

  err = fmt::format(
      "No PCI sysfs path found for vendor={} device={} subVendor={} subDevice={}",
      normVendor,
      normDevice,
      normSubVendor,
      normSubDevice);
  return std::nullopt;
}

std::optional<uint64_t> DevmemReader::getBar0FromSysfs(
    const std::string& sysfsPath,
    std::string& err) {
  auto resourcePath = fs::path(sysfsPath) / "resource";
  std::string content;
  if (!folly::readFile(resourcePath.c_str(), content)) {
    err = fmt::format("Failed to read {}", resourcePath.string());
    return std::nullopt;
  }

  // resource file: each line is "start end flags"
  // First line is BAR0
  std::istringstream iss(content);
  std::string line;
  if (!std::getline(iss, line)) {
    err = fmt::format("Empty resource file {}", resourcePath.string());
    return std::nullopt;
  }

  line = folly::trimWhitespace(line).str();
  if (line.empty()) {
    err = "Empty BAR0 line";
    return std::nullopt;
  }

  std::vector<std::string> parts;
  folly::split(' ', line, parts, /*ignoreEmpty=*/true);
  if (parts.empty()) {
    err = fmt::format("Failed to parse resource line: {}", line);
    return std::nullopt;
  }

  try {
    uint64_t bar0 = std::stoull(parts[0], nullptr, 16);
    if (bar0 == 0) {
      err = fmt::format("BAR0 is 0 for {}", sysfsPath);
      return std::nullopt;
    }
    XLOG(DBG2) << "BAR0 for " << sysfsPath << " = 0x" << std::hex << bar0;
    return bar0;
  } catch (const std::exception& ex) {
    err = fmt::format("Failed to parse BAR0 {}: {}", parts[0], ex.what());
    return std::nullopt;
  }
}

std::optional<uint32_t> DevmemReader::read32(
    uint64_t physAddr,
    std::string& err) {
  // read32 only reads, so open and map read-only to reduce blast radius and
  // to work on systems where /dev/mem is not writable by the caller.
  int fd = open("/dev/mem", O_RDONLY | O_SYNC);
  if (fd < 0) {
    err = fmt::format(
        "Failed to open /dev/mem: {} (need root?)", folly::errnoStr(errno));
    return std::nullopt;
  }

  long pageSize = sysconf(_SC_PAGE_SIZE);
  if (pageSize <= 0) {
    pageSize = 4096;
  }
  uint64_t pageMask = ~(static_cast<uint64_t>(pageSize - 1));
  uint64_t mapBase = physAddr & pageMask;
  uint64_t offset = physAddr & ~pageMask;

  void* map = mmap(
      nullptr,
      pageSize,
      PROT_READ,
      MAP_SHARED,
      fd,
      static_cast<off_t>(mapBase));
  if (map == MAP_FAILED) {
    err = fmt::format(
        "mmap failed for phys 0x{:x} (mapBase 0x{:x}): {}",
        physAddr,
        mapBase,
        folly::errnoStr(errno));
    close(fd);
    return std::nullopt;
  }

  volatile uint32_t* virt = reinterpret_cast<volatile uint32_t*>(
      reinterpret_cast<char*>(map) + offset);
  uint32_t val = *virt;

  if (munmap(map, pageSize) != 0) {
    XLOG(WARN) << "munmap failed: " << folly::errnoStr(errno);
  }
  close(fd);

  XLOG(DBG2) << fmt::format(
      "devmem read 0x{:x} => 0x{:x} ({})", physAddr, val, val);
  return val;
}

} // namespace facebook::fboss::platform::watchdog_util
