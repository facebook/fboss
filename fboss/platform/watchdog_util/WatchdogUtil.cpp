// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/watchdog_util/WatchdogUtil.h"

#include <memory>

#include <folly/json.h>
#include <folly/logging/xlog.h>

#include "fboss/platform/watchdog_util/ConfigResolver.h"
#include "fboss/platform/watchdog_util/Readers/AristaFanCpldWdReader.h"
#include "fboss/platform/watchdog_util/Readers/AristaScdWdReader.h"
#include "fboss/platform/watchdog_util/Readers/CiscoFpgaWdReader.h"
#include "fboss/platform/watchdog_util/Readers/FbFanCpldWdReader.h"
#include "fboss/platform/watchdog_util/WatchdogReader.h"

namespace facebook::fboss::platform::watchdog_util {

WatchdogUtil::WatchdogUtil(
    const std::string& devmapDir,
    const std::string& configFile)
    : devmapDir_(devmapDir), configFile_(configFile) {}

std::vector<WatchdogInfo> WatchdogUtil::getAllWatchdogInfo() {
  ConfigResolver resolver(devmapDir_, configFile_);
  auto resolvedList = resolver.resolveAll();

  std::vector<WatchdogInfo> results;
  results.reserve(resolvedList.size());

  for (const auto& resolved : resolvedList) {
    std::unique_ptr<WatchdogReader> reader;

    switch (resolved.family) {
      case WatchdogFamily::CISCO_FPGA:
        reader = std::make_unique<CiscoFpgaWdReader>();
        break;
      case WatchdogFamily::ARISTA_SCD:
        reader = std::make_unique<AristaScdWdReader>();
        break;
      case WatchdogFamily::FB_FAN_CPLD:
        reader = std::make_unique<FbFanCpldWdReader>();
        break;
      case WatchdogFamily::ARISTA_FAN_CPLD:
        reader = std::make_unique<AristaFanCpldWdReader>();
        break;
      case WatchdogFamily::GENERIC_I2C:
        // Default generic I2C to FB_FAN_CPLD layout (0x1F/0x20/0x21)
        // which covers most Meta platforms
        reader = std::make_unique<FbFanCpldWdReader>();
        break;
      case WatchdogFamily::GENERIC_DEVMEM:
        // Default generic devmem to Cisco layout, as it's the most common
        // FPGA watchdog layout. Fall back to the SCD reader when csrOffset
        // matches a known SCD offset.
        if (resolved.csrOffset.has_value() &&
            isScdCsrOffset(*resolved.csrOffset)) {
          XLOG(WARN) << fmt::format(
              "Using SCD reader for {} based on csrOffset=0x{:x} heuristic; family was not explicitly resolved",
              resolved.pmUnitScopedName,
              *resolved.csrOffset);
          reader = std::make_unique<AristaScdWdReader>();
        } else {
          reader = std::make_unique<CiscoFpgaWdReader>();
        }
        break;
      case WatchdogFamily::UNKNOWN:
      default:
        // Infer from access type
        if (resolved.accessType == AccessType::I2C) {
          reader = std::make_unique<FbFanCpldWdReader>();
        } else if (
            resolved.csrOffset.has_value() &&
            isScdCsrOffset(*resolved.csrOffset)) {
          XLOG(WARN) << fmt::format(
              "Using SCD reader for {} based on csrOffset=0x{:x} heuristic; family was not explicitly resolved",
              resolved.pmUnitScopedName,
              *resolved.csrOffset);
          reader = std::make_unique<AristaScdWdReader>();
        } else {
          reader = std::make_unique<CiscoFpgaWdReader>();
        }
        break;
    }

    try {
      auto info = reader->read(resolved);
      // Fill in missing fields from resolved if reader didn't
      if (info.pmUnitScopedName.empty()) {
        info.pmUnitScopedName = resolved.pmUnitScopedName;
      }
      if (info.watchdogDev.empty()) {
        info.watchdogDev = resolved.watchdogDev;
      }
      results.push_back(std::move(info));
    } catch (const std::exception& ex) {
      WatchdogInfo errInfo;
      errInfo.pmUnitScopedName = resolved.pmUnitScopedName;
      errInfo.watchdogDev = resolved.watchdogDev;
      errInfo.watchdogPath = resolved.symlinkTarget;
      errInfo.devmapPath = resolved.devmapPath;
      errInfo.error = fmt::format("Exception: {}", ex.what());
      errInfo.valid = false;
      results.push_back(std::move(errInfo));
      XLOG(ERR) << "Failed to read watchdog " << resolved.pmUnitScopedName
                << ": " << ex.what();
    }
  }

  return results;
}

void WatchdogUtil::printHuman(
    const std::vector<WatchdogInfo>& infos,
    const std::string& devmapDir) {
  if (infos.empty()) {
    printf("No watchdogs found in %s\n", devmapDir.c_str());
    return;
  }

  for (const auto& info : infos) {
    if (!info.valid) {
      printf(
          "%s (%s): ERROR: %s\n",
          info.pmUnitScopedName.c_str(),
          info.watchdogDev.c_str(),
          info.error.c_str());
      continue;
    }

    // Output format per spec: pmUnitScopedName (watchdogN), Enabled, Timeleft,
    // Timeout, Expired - No redundant bracketed (142/300)
    printf(
        "%s (%s):\n", info.pmUnitScopedName.c_str(), info.watchdogDev.c_str());
    printf("  Enabled: %s\n", info.enabled ? "true" : "false");
    printf("  Timeout: %us\n", info.timeout);
    printf("  Timeleft: %us\n", info.timeleft);
    printf("  Expired: %s\n", info.expired ? "true" : "false");
    printf("\n");
  }
}

void WatchdogUtil::printJson(const std::vector<WatchdogInfo>& infos) {
  folly::dynamic arr = folly::dynamic::array();

  for (const auto& info : infos) {
    folly::dynamic obj = folly::dynamic::object();
    obj["pmUnitScopedName"] = info.pmUnitScopedName;
    obj["watchdog"] = info.watchdogDev;
    obj["watchdogPath"] = info.watchdogPath;
    obj["devmapPath"] = info.devmapPath;
    obj["enabled"] = info.enabled;
    obj["timeout"] = info.timeout;
    obj["timeleft"] = info.timeleft;
    obj["expired"] = info.expired;
    obj["type"] = info.typeStr;
    obj["valid"] = info.valid;
    if (!info.error.empty()) {
      obj["error"] = info.error;
    }
    if (info.i2cBus.has_value()) {
      obj["i2cBus"] = *info.i2cBus;
    }
    if (info.i2cAddr.has_value()) {
      obj["i2cAddr"] = *info.i2cAddr;
    }
    if (info.pciBar0.has_value()) {
      obj["pciBar0"] = *info.pciBar0;
    }
    if (info.csrOffset.has_value()) {
      obj["csrOffset"] = *info.csrOffset;
    }
    if (!info.kernelDeviceName.empty()) {
      obj["kernelDeviceName"] = info.kernelDeviceName;
    }
    arr.push_back(std::move(obj));
  }

  std::string jsonStr = folly::toJson(arr);
  printf("%s\n", jsonStr.c_str());
}

} // namespace facebook::fboss::platform::watchdog_util
