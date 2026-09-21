// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/watchdog_util/Readers/AristaScdWdReader.h"

#include <folly/logging/xlog.h>

#include "fboss/platform/watchdog_util/DevmemReader.h"

namespace facebook::fboss::platform::watchdog_util {

namespace {
constexpr uint32_t kEnableBit = (1u << 31);
constexpr uint32_t kActionMask = (0x3u << 29);
constexpr uint32_t kPreTimeoutMask = (0xFFFu << 16);
constexpr uint32_t kTimeoutMask = 0xFFFFu;
} // namespace

WatchdogInfo AristaScdWdReader::read(const ResolvedWatchdog& resolved) {
  WatchdogInfo info;
  info.pmUnitScopedName = resolved.pmUnitScopedName;
  info.watchdogDev = resolved.watchdogDev;
  info.watchdogPath = resolved.symlinkTarget;
  info.devmapPath = resolved.devmapPath;
  info.accessType = AccessType::DEVMEM;
  info.family = WatchdogFamily::ARISTA_SCD;
  info.typeStr = "devmem";
  info.csrOffset = resolved.csrOffset;
  info.kernelDeviceName = resolved.fpgaDeviceName;

  if (!resolved.pciInfo.has_value()) {
    info.error = "Missing PCI info for SCD watchdog";
    return info;
  }

  if (!resolved.csrOffset.has_value()) {
    info.error = "Missing csrOffset for SCD watchdog";
    return info;
  }

  const auto& pci = *resolved.pciInfo;
  uint64_t csrOffset = resolved.csrOffset.value();
  uint64_t bar0 = pci.bar0;

  if (bar0 == 0) {
    std::string err;
    auto sysfsPath = DevmemReader::findPciSysfsPath(
        pci.vendorId,
        pci.deviceId,
        pci.subSystemVendorId,
        pci.subSystemDeviceId,
        err);
    if (!sysfsPath) {
      info.error = fmt::format("Failed to find PCI sysfs for SCD: {}", err);
      return info;
    }
    auto bar0Opt = DevmemReader::getBar0FromSysfs(*sysfsPath, err);
    if (!bar0Opt) {
      info.error = fmt::format("Failed to get BAR0 for SCD: {}", err);
      return info;
    }
    bar0 = *bar0Opt;
    info.pciSysfsPath = *sysfsPath;
    info.pciBar0 = bar0;
  } else {
    info.pciBar0 = bar0;
    info.pciSysfsPath = pci.sysfsPath;
  }

  uint64_t phys = bar0 + csrOffset;

  std::string err;
  auto regOpt = DevmemReader::read32(phys, err);
  if (!regOpt) {
    info.error = fmt::format("Failed to read SCD WDT @0x{:x}: {}", phys, err);
    return info;
  }

  uint32_t reg = *regOpt;
  info.enabled = (reg & kEnableBit) != 0;
  info.timeout = reg & kTimeoutMask;
  uint32_t preTimeout = (reg & kPreTimeoutMask) >> 16;
  uint32_t action = (reg & kActionMask) >> 29;

  // SCD hardware does not expose timeleft in this register.
  // Best effort: if enabled, timeleft = timeout (or preTimeout if available),
  // else 0. Document limitation.
  if (info.enabled) {
    // If preTimeout is configured, it may represent warning timeout before
    // final timeout. Use timeout as timeleft for debug, as we can't read
    // countdown.
    info.timeleft = info.timeout;
  } else {
    info.timeleft = 0;
  }

  info.expired = false; // No hardware expired bit; driver disables WDT

  info.valid = true;

  XLOG(INFO) << fmt::format(
      "ARISTA_SCD {} ({}): enabled={} timeout={} preTimeout={} action={} timeleft={} (derived) expired={} reg=0x{:x} phys=0x{:x}",
      info.pmUnitScopedName,
      info.watchdogDev,
      info.enabled,
      info.timeout,
      preTimeout,
      action,
      info.timeleft,
      info.expired,
      reg,
      phys);

  return info;
}

} // namespace facebook::fboss::platform::watchdog_util
