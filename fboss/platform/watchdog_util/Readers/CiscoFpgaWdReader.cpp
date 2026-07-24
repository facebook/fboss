// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/watchdog_util/Readers/CiscoFpgaWdReader.h"

#include <folly/logging/xlog.h>

#include "fboss/platform/watchdog_util/DevmemReader.h"

namespace facebook::fboss::platform::watchdog_util {

namespace {
constexpr uint64_t kStg1CtlOffset = 0x34;
constexpr uint64_t kStatOffset = 0x40;

constexpr uint32_t kStgCtlEnbMask = 0x1;
constexpr uint32_t kStgCtlTimeoutMask = 0xFFFF0000;
constexpr int kStgCtlTimeoutShift = 16;

constexpr uint32_t kStatTimervalMask = 0xFFFF;
constexpr uint32_t kStatStage1Enabled = 0x10000;
constexpr uint32_t kStatStage1Expired = 0x20000;
} // namespace

WatchdogInfo CiscoFpgaWdReader::read(const ResolvedWatchdog& resolved) {
  WatchdogInfo info;
  info.pmUnitScopedName = resolved.pmUnitScopedName;
  info.watchdogDev = resolved.watchdogDev;
  info.watchdogPath = resolved.symlinkTarget;
  info.devmapPath = resolved.devmapPath;
  info.accessType = AccessType::DEVMEM;
  info.family = WatchdogFamily::CISCO_FPGA;
  info.typeStr = "devmem";
  info.csrOffset = resolved.csrOffset;
  info.iobufOffset = resolved.iobufOffset;
  info.kernelDeviceName = resolved.fpgaDeviceName;

  if (!resolved.pciInfo.has_value()) {
    info.error = "Missing PCI info for devmem watchdog";
    return info;
  }

  if (!resolved.csrOffset.has_value()) {
    info.error = "Missing csrOffset for Cisco FPGA watchdog";
    return info;
  }

  const auto& pci = *resolved.pciInfo;
  uint64_t csrBase = resolved.csrOffset.value();
  uint64_t bar0 = pci.bar0;

  if (bar0 == 0) {
    // Try to resolve BAR0 if not already resolved
    std::string err;
    auto sysfsPath = DevmemReader::findPciSysfsPath(
        pci.vendorId,
        pci.deviceId,
        pci.subSystemVendorId,
        pci.subSystemDeviceId,
        err);
    if (!sysfsPath) {
      info.error = fmt::format("Failed to find PCI sysfs: {}", err);
      return info;
    }
    auto bar0Opt = DevmemReader::getBar0FromSysfs(*sysfsPath, err);
    if (!bar0Opt) {
      info.error = fmt::format("Failed to get BAR0: {}", err);
      return info;
    }
    bar0 = *bar0Opt;
    info.pciSysfsPath = *sysfsPath;
    info.pciBar0 = bar0;
  } else {
    info.pciBar0 = bar0;
    info.pciSysfsPath = pci.sysfsPath;
  }

  uint64_t stg1CtlPhys = bar0 + csrBase + kStg1CtlOffset;
  uint64_t statPhys = bar0 + csrBase + kStatOffset;

  std::string err;
  auto stg1CtlOpt = DevmemReader::read32(stg1CtlPhys, err);
  if (!stg1CtlOpt) {
    info.error =
        fmt::format("Failed to read stg1Ctl @0x{:x}: {}", stg1CtlPhys, err);
    return info;
  }

  auto statOpt = DevmemReader::read32(statPhys, err);
  if (!statOpt) {
    info.error = fmt::format("Failed to read stat @0x{:x}: {}", statPhys, err);
    return info;
  }

  uint32_t stg1Ctl = *stg1CtlOpt;
  uint32_t stat = *statOpt;

  bool enabledByCtl = (stg1Ctl & kStgCtlEnbMask) != 0;
  bool enabledByStat = (stat & kStatStage1Enabled) != 0;
  info.enabled = enabledByCtl || enabledByStat;

  info.timeout = (stg1Ctl & kStgCtlTimeoutMask) >> kStgCtlTimeoutShift;
  info.timeleft = stat & kStatTimervalMask;
  info.expired = (stat & kStatStage1Expired) != 0;

  // If enabled with no time left, the watchdog has effectively expired even if
  // the hardware alarm bit is not (yet) set. Report it as expired for
  // consistency with the CPLD readers, which derive expired the same way.
  if (info.enabled && info.timeleft == 0 && info.timeout != 0 &&
      !info.expired) {
    info.expired = true;
    XLOG(DBG1) << fmt::format(
        "Cisco watchdog {} timeleft=0 while enabled; deriving expired=true",
        info.pmUnitScopedName);
  }

  if (!info.enabled) {
    info.timeleft = 0;
    // Don't clear expired if hardware says expired even when disabled? Keep as
    // is
  }

  info.valid = true;

  XLOG(INFO) << fmt::format(
      "CISCO_FPGA {} ({}): enabled={} (ctl=0x{:x} stat=0x{:x}) timeout={} timeleft={} expired={} phys stg1Ctl=0x{:x} stat=0x{:x}",
      info.pmUnitScopedName,
      info.watchdogDev,
      info.enabled,
      stg1Ctl,
      stat,
      info.timeout,
      info.timeleft,
      info.expired,
      stg1CtlPhys,
      statPhys);

  return info;
}

} // namespace facebook::fboss::platform::watchdog_util
