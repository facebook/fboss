// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/watchdog_util/Readers/AristaFanCpldWdReader.h"

#include <filesystem>

#include <folly/logging/xlog.h>

#include "fboss/platform/watchdog_util/I2cReader.h"

namespace facebook::fboss::platform::watchdog_util {

namespace {
constexpr uint8_t kEnableReg = 0x06;
constexpr uint8_t kCounterReg = 0x08;
constexpr uint8_t kTimeoutReg = 0x0c;
} // namespace

WatchdogInfo AristaFanCpldWdReader::read(const ResolvedWatchdog& resolved) {
  WatchdogInfo info;
  info.pmUnitScopedName = resolved.pmUnitScopedName;
  info.watchdogDev = resolved.watchdogDev;
  info.watchdogPath = resolved.symlinkTarget;
  info.devmapPath = resolved.devmapPath;
  info.accessType = AccessType::I2C;
  info.family = WatchdogFamily::ARISTA_FAN_CPLD;
  info.typeStr = "i2c";
  info.i2cBus = resolved.i2cBus;
  info.i2cAddr = resolved.i2cAddr;
  info.kernelDeviceName = resolved.kernelDeviceName;

  if (!resolved.i2cAddr.has_value()) {
    info.error = "Missing i2c address for Arista fan CPLD";
    return info;
  }

  int addr = *resolved.i2cAddr;
  std::vector<int> busesToTry;
  const bool fallbackScan = !resolved.i2cBus.has_value();
  if (resolved.i2cBus.has_value()) {
    busesToTry.push_back(*resolved.i2cBus);
  } else {
    // No bus mapping – only probe i2c buses whose /dev/i2c-<n> node exists, to
    // avoid spawning an i2cget subprocess for every number in 0..32.
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
  std::optional<uint8_t> enableOpt, timeoutOpt, counterOpt;

  for (int bus : busesToTry) {
    std::string busErr;
    auto e = I2cReader::read(bus, addr, kEnableReg, busErr);
    if (!e) {
      err = busErr;
      continue;
    }
    auto t = I2cReader::read(bus, addr, kTimeoutReg, busErr);
    auto c = I2cReader::read(bus, addr, kCounterReg, busErr);

    // During a fallback scan there is no authoritative bus, so only accept a
    // bus where all three registers read back and WDT_TIMEOUT is non-zero.
    // This avoids latching onto an unrelated device that merely ACKs a read of
    // ENABLE (0x06) at the same 7-bit address. A genuine, in-use fan-CPLD
    // watchdog has a non-zero timeout configured.
    if (fallbackScan && (!t || !c || *t == 0)) {
      XLOG(WARN) << fmt::format(
          "Skipping bus {} addr=0x{:x} during fallback scan: failed plausibility check (timeout/counter unreadable or timeout=0)",
          bus,
          addr);
      err = busErr;
      continue;
    }

    enableOpt = e;
    timeoutOpt = t;
    counterOpt = c;
    info.i2cBus = bus;
    break;
  }

  if (!enableOpt) {
    info.error = fmt::format(
        "Failed to read ENABLE from any bus (tried {} buses) addr=0x{:x}: {}",
        busesToTry.size(),
        addr,
        err);
    return info;
  }

  // With an explicit bus mapping, a failure to read TIMEOUT/COUNTER is a real
  // bus error. Surface it instead of coercing to 0, which would be misreported
  // downstream as an expired watchdog.
  if (!timeoutOpt || !counterOpt) {
    info.error = fmt::format(
        "Read ENABLE but failed to read TIMEOUT/COUNTER on bus {} addr=0x{:x}: {}",
        info.i2cBus.value_or(-1),
        addr,
        err);
    return info;
  }

  uint8_t enableVal = *enableOpt;
  uint8_t timeoutReg = *timeoutOpt;
  uint8_t counter = *counterOpt;

  info.enabled = enableVal != 0;

  // WDT_TIMEOUT comment says multiples of 5 seconds. Use *5 to match other
  // CPLDs. If timeoutReg is 0, fallback to counter value if enabled.
  if (timeoutReg != 0) {
    info.timeout = static_cast<uint32_t>(timeoutReg) * 5;
  } else if (info.enabled) {
    info.timeout = static_cast<uint32_t>(counter);
  } else {
    info.timeout = 0;
  }

  // Counter is timeleft. For Arista, start writes timeout to counter, so
  // counter should decrement. It may be in seconds or in 5-sec units. Try to
  // infer: If timeout is *5 and counter <= timeout/5, counter is likely in same
  // units as timeoutReg (not multiplied). So we report counter*5 if timeout was
  // *5, else raw.
  if (timeoutReg != 0) {
    // timeout is timeoutReg*5, counter likely raw seconds or same unit?
    // From driver: fan_wdt_start writes wdd->timeout to COUNTER directly,
    // and wdd->timeout is in seconds (not *5). But WDT_TIMEOUT reg comment says
    // multiples of 5. So COUNTER may be seconds, TIMEOUT may be *5? Ambiguous.
    // Best effort: if counter <= timeoutReg, assume counter is in timeoutReg
    // units, so timeleft = counter*5. Else timeleft = counter.
    if (counter <= timeoutReg) {
      info.timeleft = static_cast<uint32_t>(counter) * 5;
    } else {
      info.timeleft = static_cast<uint32_t>(counter);
    }
  } else {
    info.timeleft = static_cast<uint32_t>(counter);
  }

  if (!info.enabled) {
    info.timeleft = 0;
  }

  // No explicit expired bit, derive
  info.expired = info.enabled && (info.timeleft == 0) && (info.timeout != 0);

  info.valid = true;

  XLOG(INFO) << fmt::format(
      "ARISTA_FAN_CPLD {} ({}): enabled={} timeout={} (reg=0x{:x}) timeleft={} (counter=0x{:x}) expired={}",
      info.pmUnitScopedName,
      info.watchdogDev,
      info.enabled,
      info.timeout,
      timeoutReg,
      info.timeleft,
      counter,
      info.expired);

  return info;
}

} // namespace facebook::fboss::platform::watchdog_util
