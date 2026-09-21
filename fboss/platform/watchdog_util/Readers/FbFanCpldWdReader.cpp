// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/watchdog_util/Readers/FbFanCpldWdReader.h"

#include <filesystem>

#include <folly/logging/xlog.h>

#include "fboss/platform/watchdog_util/I2cReader.h"

namespace facebook::fboss::platform::watchdog_util {

namespace {
constexpr uint8_t kCtrl0Reg = 0x1F;
constexpr uint8_t kCtrl1Reg = 0x20;
constexpr uint8_t kStaReg = 0x21;

constexpr uint8_t kEnableBit = (1 << 4);
constexpr uint8_t kExpiredBit = (1 << 6); // mp3_fancpld only, but safe to check
} // namespace

WatchdogInfo FbFanCpldWdReader::read(const ResolvedWatchdog& resolved) {
  WatchdogInfo info;
  info.pmUnitScopedName = resolved.pmUnitScopedName;
  info.watchdogDev = resolved.watchdogDev;
  info.watchdogPath = resolved.symlinkTarget;
  info.devmapPath = resolved.devmapPath;
  info.accessType = AccessType::I2C;
  info.family = WatchdogFamily::FB_FAN_CPLD;
  info.typeStr = "i2c";
  info.i2cBus = resolved.i2cBus;
  info.i2cAddr = resolved.i2cAddr;
  info.kernelDeviceName = resolved.kernelDeviceName;

  if (!resolved.i2cAddr.has_value()) {
    info.error = "Missing i2c address";
    return info;
  }

  int addr = *resolved.i2cAddr;
  std::vector<int> busesToTry;
  const bool fallbackScan = !resolved.i2cBus.has_value();
  if (resolved.i2cBus.has_value()) {
    busesToTry.push_back(*resolved.i2cBus);
  } else {
    // No bus from sysfs/config mapping – discover by scanning the i2c buses
    // that actually exist. Only probe buses whose /dev/i2c-<n> node is present,
    // to avoid spawning an i2cget subprocess for every number in 0..32.
    for (int b = 0; b <= 32; ++b) {
      if (std::filesystem::exists(fmt::format("/dev/i2c-{}", b))) {
        busesToTry.push_back(b);
      }
    }
    XLOG(WARN) << fmt::format(
        "No i2c bus for {} addr=0x{:x}, scanning {} available buses",
        resolved.pmUnitScopedName,
        addr,
        busesToTry.size());
  }

  std::string err;
  std::optional<uint8_t> ctrl0, ctrl1, sta;

  for (int bus : busesToTry) {
    std::string busErr;
    auto c0 = I2cReader::read(bus, addr, kCtrl0Reg, busErr);
    if (!c0) {
      XLOG(DBG2) << fmt::format(
          "Bus {} addr 0x{:x} reg 0x{:x} failed: {}",
          bus,
          addr,
          kCtrl0Reg,
          busErr);
      err = busErr;
      continue;
    }
    auto c1 = I2cReader::read(bus, addr, kCtrl1Reg, busErr);
    if (!c1) {
      XLOG(DBG2) << fmt::format(
          "Bus {} addr 0x{:x} reg 0x{:x} failed: {}",
          bus,
          addr,
          kCtrl1Reg,
          busErr);
      err = busErr;
      continue;
    }
    // During a fallback scan there is no authoritative bus, so require a
    // non-zero timeout (CTRL1) before accepting a bus. This avoids latching
    // onto an unrelated device that merely ACKs a read at the same 7-bit
    // address; an in-use fan-CPLD watchdog has a non-zero timeout configured.
    if (fallbackScan && *c1 == 0) {
      XLOG(WARN) << fmt::format(
          "Skipping bus {} addr=0x{:x} during fallback scan: timeout register 0x{:x} is 0",
          bus,
          addr,
          kCtrl1Reg);
      continue;
    }
    auto s = I2cReader::read(bus, addr, kStaReg, busErr);
    if (!s) {
      // CTRL0/CTRL1 read fine, so this is the right bus/device, but we cannot
      // read the counter. Coercing to 0 would make timeleft == timeout and
      // expired == false, i.e. report a healthy watchdog we never actually
      // read. Surface it as an error so the caller can tell "watchdog fine"
      // apart from "counter unreadable".
      info.error = fmt::format(
          "Failed to read STA reg 0x{:x} on bus {} addr=0x{:x}: {}",
          kStaReg,
          bus,
          addr,
          busErr);
      return info;
    }
    ctrl0 = c0;
    ctrl1 = c1;
    sta = s;
    info.i2cBus = bus;
    break;
  }

  if (!ctrl0 || !ctrl1) {
    info.error = fmt::format(
        "Failed to read CTRL0/CTRL1 from any bus (tried {} buses) addr=0x{:x}: {}",
        busesToTry.size(),
        addr,
        err);
    return info;
  }

  uint8_t ctrl0Val = *ctrl0;
  uint8_t timeoutReg = *ctrl1;
  uint8_t counter = *sta;

  info.enabled = (ctrl0Val & kEnableBit) != 0;
  info.timeout = static_cast<uint32_t>(timeoutReg) * 5;

  // timeleft = (max_timeout - counter) * 5 per fbfancpld_i2c.c
  // max_timeout is timeoutReg, counter increments towards max
  if (timeoutReg >= counter) {
    info.timeleft = static_cast<uint32_t>(timeoutReg - counter) * 5;
  } else {
    // Counter wrapped or invalid, assume 0
    info.timeleft = 0;
  }

  // Expired detection:
  // - mp3_fancpld has BIT6 as expired/timeout flag
  // - generic fbfancpld has no explicit bit, derive from timeleft==0 && enabled
  bool expiredBit = (ctrl0Val & kExpiredBit) != 0;
  if (resolved.hasExpiredBit || resolved.kernelDeviceName == "mp3_fancpld") {
    info.expired = expiredBit;
  } else {
    // If BIT6 is set, treat as expired even for generic, as it may indicate
    // timeout on some hardware. Otherwise derive.
    if (expiredBit) {
      info.expired = true;
    } else {
      info.expired =
          info.enabled && (info.timeleft == 0) && (info.timeout != 0);
    }
  }

  // If disabled, timeleft should be 0 and expired false
  if (!info.enabled) {
    info.timeleft = 0;
    info.expired = false;
  }

  info.valid = true;
  XLOG(INFO) << fmt::format(
      "FB_FAN_CPLD {} ({}): enabled={} timeout={} timeleft={} expired={} ctrl0=0x{:x} timeoutReg=0x{:x} counter=0x{:x}",
      info.pmUnitScopedName,
      info.watchdogDev,
      info.enabled,
      info.timeout,
      info.timeleft,
      info.expired,
      ctrl0Val,
      timeoutReg,
      counter);

  return info;
}

} // namespace facebook::fboss::platform::watchdog_util
