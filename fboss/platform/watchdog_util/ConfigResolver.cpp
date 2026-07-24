// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/watchdog_util/ConfigResolver.h"

#include <filesystem>
#include <fstream>

#include <folly/FileUtil.h>
#include <folly/String.h>
#include <folly/logging/xlog.h>
#include <re2/re2.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

#include "fboss/platform/config_lib/ConfigLib.h"
#include "fboss/platform/platform_manager/gen-cpp2/platform_manager_config_types.h"
#include "fboss/platform/watchdog_util/DevmemReader.h"

namespace fs = std::filesystem;
using namespace facebook::fboss::platform::platform_manager;

namespace facebook::fboss::platform::watchdog_util {

ConfigResolver::ConfigResolver(
    const std::string& devmapDir,
    const std::string& configFile)
    : devmapDir_(devmapDir), configFile_(configFile) {
  configHolder_ = std::make_unique<PlatformConfigHolder>();
}

ConfigResolver::~ConfigResolver() = default;

std::vector<ResolvedWatchdog> ConfigResolver::enumerateDevmap() {
  std::vector<ResolvedWatchdog> result;

  if (!fs::exists(devmapDir_)) {
    XLOG(WARN) << "Devmap dir does not exist: " << devmapDir_;
    // Per requirement, exit after this error – do not fallback to /dev
    return result;
  }

  try {
    for (const auto& entry : fs::directory_iterator(devmapDir_)) {
      auto pmName = entry.path().filename().string();
      std::string symlinkTarget;
      try {
        if (fs::is_symlink(entry.path())) {
          symlinkTarget = fs::read_symlink(entry.path()).string();
        } else {
          // Might be regular file (some platforms create file instead of
          // symlink)
          symlinkTarget = entry.path().string();
        }
      } catch (const std::exception& ex) {
        XLOG(WARN) << "Failed to read symlink " << entry.path() << ": "
                   << ex.what();
        continue;
      }

      // Extract watchdogN from symlink target like /dev/watchdog1. Require at
      // least one digit so a target ending in a bare "watchdog" falls back to
      // pmName instead of yielding the ambiguous device name "watchdog".
      static const re2::RE2 kWdRe(R"(.*/(watchdog\d+))");
      std::string wdMatch;
      ResolvedWatchdog rw;
      if (re2::RE2::FullMatch(symlinkTarget, kWdRe, &wdMatch)) {
        rw.watchdogDev = std::move(wdMatch);
      } else {
        // Symlink target has no watchdog\d* component; fall back to pmName
        rw.watchdogDev = pmName;
      }

      rw.pmUnitScopedName = std::move(pmName);
      rw.devmapPath = entry.path().string();
      rw.symlinkTarget = symlinkTarget;
      result.push_back(std::move(rw));
    }
  } catch (const std::exception& ex) {
    XLOG(ERR) << "Failed to enumerate " << devmapDir_ << ": " << ex.what();
  }

  return result;
}

void ConfigResolver::loadPlatformConfig() {
  std::string jsonStr;
  try {
    if (!configFile_.empty()) {
      if (!folly::readFile(configFile_.c_str(), jsonStr)) {
        throw std::runtime_error("Failed to read config file: " + configFile_);
      }
    } else {
      jsonStr =
          facebook::fboss::platform::ConfigLib().getPlatformManagerConfig();
    }

    apache::thrift::SimpleJSONSerializer::deserialize<PlatformConfig>(
        jsonStr, configHolder_->config);
    configHolder_->loaded = true;
    XLOG(INFO) << "Loaded platform config for "
               << *configHolder_->config.platformName();
  } catch (const std::exception& ex) {
    XLOG(WARN) << "Failed to load platform config: " << ex.what()
               << " - continuing with devmap-only resolution";
    configHolder_->loaded = false;
  }
}

void ConfigResolver::resolveDevicePath(ResolvedWatchdog& rw) {
  if (!configHolder_->loaded) {
    return;
  }

  const auto& symMap = *configHolder_->config.symbolicLinkToDevicePath();
  std::string key =
      fmt::format("/run/devmap/watchdogs/{}", rw.pmUnitScopedName);
  auto it = symMap.find(key);
  if (it == symMap.end()) {
    // Try alternative key without /run prefix? Some configs use
    // /run/devmap/watchdogs but we already use that. If not found, try to find
    // by value containing pmName
    XLOG(DBG2) << "No symbolicLinkToDevicePath entry for " << key;
    return;
  }

  rw.devicePath = it->second;

  // Extract devicePmName from inside brackets: "/[MCB_FAN_CPLD]" ->
  // MCB_FAN_CPLD Also handles "/PDB_SLOT@0/[PDB_FAN_CPLD]" -> PDB_FAN_CPLD
  static const re2::RE2 kBracketRe(R"(\[([^\]]+)\])");
  std::string pmName;
  // Find last occurrence of bracketed name
  std::string_view view = rw.devicePath;
  re2::StringPiece input(view);
  re2::StringPiece match;
  std::string lastMatch;
  while (re2::RE2::FindAndConsume(&input, kBracketRe, &match)) {
    lastMatch = std::string(match);
  }
  if (!lastMatch.empty()) {
    rw.devicePmName = lastMatch;
  } else {
    // Fallback: use whole devicePath
    rw.devicePmName = rw.devicePath;
  }

  XLOG(DBG1) << fmt::format(
      "Resolved {} -> devicePath={} devicePmName={}",
      rw.pmUnitScopedName,
      rw.devicePath,
      rw.devicePmName);
}

static std::optional<uint64_t> parseHexOffset(const std::string& str) {
  auto trimmed = folly::trimWhitespace(str);
  if (trimmed.empty()) {
    return std::nullopt;
  }
  try {
    return std::stoull(std::string(trimmed), nullptr, 0);
  } catch (const std::exception& ex) {
    XLOG(DBG1) << fmt::format(
        "Failed to parse hex offset '{}': {}", str, ex.what());
    return std::nullopt;
  }
}

template <typename T>
static std::optional<uint64_t> parseHexOffsetField(const T& fieldRef) {
  try {
    // fieldRef is a thrift field_ref, dereference to get the string
    const std::string& s = *fieldRef;
    return parseHexOffset(s);
  } catch (...) {
    return std::nullopt;
  }
}

void ConfigResolver::resolveHardwareDetails(ResolvedWatchdog& rw) {
  // First, try to get bus and addr from sysfs realpath
  // /sys/class/watchdog/watchdogN -> .../i2c-15/15-0033/...  (I2C)
  // or ->
  // .../pci0000:00/0000:00:0b.0/0000:01:00.0/cisco_meta.watchdog_fan.1017/...
  // (devmem)
  std::string sysfsRoot = DevmemReader::getSysfsRoot();
  std::string sysfsPath =
      fmt::format("{}/class/watchdog/{}", sysfsRoot, rw.watchdogDev);
  std::optional<std::string> pciBdfFromRealpath;
  try {
    if (fs::exists(sysfsPath)) {
      auto canon = fs::canonical(sysfsPath);
      std::string canonStr = canon.string();
      XLOG(DBG2) << "realpath " << sysfsPath << " -> " << canonStr;

      // Regex for i2c bus and address
      static const re2::RE2 kI2cRe(
          R"(i2c-(\d+).*?/(\d+)-([0-9a-fA-F]+)/watchdog)");
      int bus = -1;
      int bus2 = -1;
      std::string addrStr;
      if (re2::RE2::PartialMatch(canonStr, kI2cRe, &bus, &bus2, &addrStr)) {
        rw.i2cBus = bus;
        try {
          int addr = std::stoi(addrStr, nullptr, 16);
          rw.i2cAddr = addr;
        } catch (const std::exception& ex) {
          try {
            rw.i2cAddr = std::stoi(addrStr, nullptr, 0);
          } catch (const std::exception& ex2) {
            XLOG(DBG1) << fmt::format(
                "Failed to parse i2c address '{}' from sysfs: {} / {}",
                addrStr,
                ex.what(),
                ex2.what());
          }
        }
        XLOG(DBG1) << fmt::format(
            "Parsed i2c bus={} addr=0x{:x} from {}",
            bus,
            rw.i2cAddr.value_or(0),
            canonStr);
      } else {
        static const re2::RE2 kBusRe(R"(i2c-(\d+))");
        int b = -1;
        if (re2::RE2::PartialMatch(canonStr, kBusRe, &b)) {
          rw.i2cBus = b;
        }
      }

      // Extract PCI BDF for devmem watchdogs
      // Example:
      // /sys/devices/pci0000:00/0000:00:0b.0/0000:01:00.0/cisco_meta.watchdog_fan.1017/...
      // We want the last PCI BDF in the path, which is the parent PCI device.
      static const re2::RE2 kPciBdfRe(
          R"(([0-9a-fA-F]{4}:[0-9a-fA-F]{2}:[0-9a-fA-F]{2}\.[0-9]))");
      std::string bdf;
      re2::StringPiece input(canonStr);
      std::string lastBdf;
      re2::StringPiece match;
      while (re2::RE2::FindAndConsume(&input, kPciBdfRe, &match)) {
        lastBdf = std::string(match);
      }
      if (!lastBdf.empty()) {
        pciBdfFromRealpath = lastBdf;
        XLOG(DBG1) << fmt::format(
            "Parsed PCI BDF {} from {}", lastBdf, canonStr);
        // Try to resolve sysfs path and BAR0 immediately
        std::string pciSysfs =
            fmt::format("{}/bus/pci/devices/{}", sysfsRoot, lastBdf);
        if (fs::exists(pciSysfs)) {
          std::string err;
          auto bar0 = DevmemReader::getBar0FromSysfs(pciSysfs, err);
          if (bar0) {
            // Create or update pciInfo with BDF-derived info
            if (!rw.pciInfo.has_value()) {
              rw.pciInfo = ResolvedWatchdog::PciInfo();
            }
            rw.pciInfo->sysfsPath = pciSysfs;
            rw.pciInfo->bar0 = *bar0;
            XLOG(INFO) << fmt::format(
                "Resolved PCI BAR0 0x{:x} for {} via BDF {}",
                *bar0,
                rw.pmUnitScopedName,
                lastBdf);
          } else {
            XLOG(WARN) << fmt::format(
                "Failed to get BAR0 from {}: {}", pciSysfs, err);
          }
        } else {
          XLOG(WARN) << fmt::format(
              "PCI sysfs path {} does not exist for BDF {}", pciSysfs, lastBdf);
        }
      }
    }
  } catch (const std::exception& ex) {
    XLOG(DBG1) << "Failed to resolve sysfs for " << rw.watchdogDev << ": "
               << ex.what();
  }

  if (!configHolder_->loaded) {
    // Without config, infer from bus presence: if i2cBus present => I2C, else
    // DEVMEM
    if (rw.i2cBus.has_value()) {
      rw.accessType = AccessType::I2C;
      rw.family = WatchdogFamily::GENERIC_I2C;
    } else {
      rw.accessType = AccessType::DEVMEM;
      rw.family = WatchdogFamily::GENERIC_DEVMEM;
    }
    return;
  }

  const auto& cfg = configHolder_->config;

  // Search pmUnitConfigs and versionedPmUnitConfigs for matching device
  auto searchPmUnit = [&](const PmUnitConfig& pmCfg,
                          const std::string& pmUnitNameForLog) -> bool {
    // Check watchdogConfigs (devmem)
    for (const auto& pciDev : *pmCfg.pciDeviceConfigs()) {
      // watchdogConfigs
      {
        for (const auto& wdCfg : *pciDev.watchdogConfigs()) {
          if (*wdCfg.pmUnitScopedName() == rw.devicePmName ||
              *wdCfg.pmUnitScopedName() == rw.pmUnitScopedName) {
            rw.accessType = AccessType::DEVMEM;
            rw.family = WatchdogFamily::CISCO_FPGA;
            rw.csrOffset = parseHexOffsetField(wdCfg.csrOffset());
            rw.iobufOffset = parseHexOffsetField(wdCfg.iobufOffset());
            rw.fpgaDeviceName = *wdCfg.deviceName();
            rw.kernelDeviceName = *wdCfg.deviceName();

            // Fill PCI info
            ResolvedWatchdog::PciInfo pciInfo;
            pciInfo.pmUnitScopedName = *pciDev.pmUnitScopedName();
            pciInfo.vendorId = *pciDev.vendorId();
            pciInfo.deviceId = *pciDev.deviceId();
            pciInfo.subSystemVendorId = *pciDev.subSystemVendorId();
            pciInfo.subSystemDeviceId = *pciDev.subSystemDeviceId();

            // Prefer BDF from realpath if available (more robust than vendor ID
            // matching)
            bool bar0Resolved = false;
            if (pciBdfFromRealpath.has_value()) {
              std::string pciSysfs = fmt::format(
                  "{}/bus/pci/devices/{}", sysfsRoot, *pciBdfFromRealpath);
              if (fs::exists(pciSysfs)) {
                std::string err;
                auto bar0 = DevmemReader::getBar0FromSysfs(pciSysfs, err);
                if (bar0) {
                  pciInfo.sysfsPath = pciSysfs;
                  pciInfo.bar0 = *bar0;
                  bar0Resolved = true;
                  XLOG(INFO) << fmt::format(
                      "Resolved PCI via BDF {} BAR0 0x{:x} for {}",
                      *pciBdfFromRealpath,
                      *bar0,
                      rw.pmUnitScopedName);
                }
              }
            }

            // If we already have BAR0 from earlier realpath parsing (stored in
            // rw.pciInfo), preserve it
            if (!bar0Resolved && rw.pciInfo.has_value() &&
                rw.pciInfo->bar0 != 0) {
              pciInfo.sysfsPath = rw.pciInfo->sysfsPath;
              pciInfo.bar0 = rw.pciInfo->bar0;
              bar0Resolved = true;
            }

            // Fallback to vendor/device ID search
            if (!bar0Resolved) {
              std::string err;
              auto sysfs = DevmemReader::findPciSysfsPath(
                  pciInfo.vendorId,
                  pciInfo.deviceId,
                  pciInfo.subSystemVendorId,
                  pciInfo.subSystemDeviceId,
                  err);
              if (sysfs) {
                pciInfo.sysfsPath = *sysfs;
                auto bar0 = DevmemReader::getBar0FromSysfs(*sysfs, err);
                if (bar0) {
                  pciInfo.bar0 = *bar0;
                }
              } else {
                XLOG(DBG1) << "PCI sysfs not found for "
                           << pciInfo.pmUnitScopedName << ": " << err;
                // If BDF was available but vendor search failed, keep BDF sysfs
                // path even without BAR0
                if (pciBdfFromRealpath.has_value()) {
                  pciInfo.sysfsPath = fmt::format(
                      "{}/bus/pci/devices/{}", sysfsRoot, *pciBdfFromRealpath);
                }
              }
            }

            rw.pciInfo = pciInfo;
            XLOG(INFO) << fmt::format(
                "Matched DEVMEM watchdog {} as {} in {} PCI {} csr=0x{:x}",
                rw.pmUnitScopedName,
                *wdCfg.pmUnitScopedName(),
                pmUnitNameForLog,
                pciInfo.pmUnitScopedName,
                rw.csrOffset.value_or(0));
            return true;
          }
        }
      }

      // miscCtrlConfigs with watchdog_darwin
      {
        for (const auto& misc : *pciDev.miscCtrlConfigs()) {
          if (misc.deviceName()->find("watchdog") != std::string::npos) {
            // Match the pmUnitScopedName as a bracketed path segment (e.g.
            // "[MCB_FAN_CPLD]") rather than an unbounded substring, so a short
            // common name doesn't accidentally match an unrelated devicePath.
            std::string bracketedName =
                fmt::format("[{}]", *misc.pmUnitScopedName());
            if (*misc.pmUnitScopedName() == rw.devicePmName ||
                *misc.pmUnitScopedName() == rw.pmUnitScopedName ||
                rw.devicePath.find(bracketedName) != std::string::npos) {
              rw.accessType = AccessType::DEVMEM;
              // Determine family by deviceName
              if (*misc.deviceName() == "watchdog_darwin") {
                rw.family = WatchdogFamily::ARISTA_SCD;
              } else if (*misc.deviceName() == "watchdog_fan") {
                rw.family = WatchdogFamily::CISCO_FPGA;
              } else {
                rw.family = WatchdogFamily::GENERIC_DEVMEM;
              }
              rw.csrOffset = parseHexOffsetField(misc.csrOffset());
              rw.iobufOffset = parseHexOffsetField(misc.iobufOffset());
              rw.fpgaDeviceName = *misc.deviceName();

              ResolvedWatchdog::PciInfo pciInfo;
              pciInfo.pmUnitScopedName = *pciDev.pmUnitScopedName();
              pciInfo.vendorId = *pciDev.vendorId();
              pciInfo.deviceId = *pciDev.deviceId();
              pciInfo.subSystemVendorId = *pciDev.subSystemVendorId();
              pciInfo.subSystemDeviceId = *pciDev.subSystemDeviceId();

              bool bar0Resolved = false;
              if (pciBdfFromRealpath.has_value()) {
                std::string pciSysfs = fmt::format(
                    "{}/bus/pci/devices/{}", sysfsRoot, *pciBdfFromRealpath);
                if (fs::exists(pciSysfs)) {
                  std::string err;
                  auto bar0 = DevmemReader::getBar0FromSysfs(pciSysfs, err);
                  if (bar0) {
                    pciInfo.sysfsPath = pciSysfs;
                    pciInfo.bar0 = *bar0;
                    bar0Resolved = true;
                  }
                }
              }
              if (!bar0Resolved && rw.pciInfo.has_value() &&
                  rw.pciInfo->bar0 != 0) {
                pciInfo.sysfsPath = rw.pciInfo->sysfsPath;
                pciInfo.bar0 = rw.pciInfo->bar0;
                bar0Resolved = true;
              }
              if (!bar0Resolved) {
                std::string err;
                auto sysfs = DevmemReader::findPciSysfsPath(
                    pciInfo.vendorId,
                    pciInfo.deviceId,
                    pciInfo.subSystemVendorId,
                    pciInfo.subSystemDeviceId,
                    err);
                if (sysfs) {
                  pciInfo.sysfsPath = *sysfs;
                  auto bar0 = DevmemReader::getBar0FromSysfs(*sysfs, err);
                  if (bar0) {
                    pciInfo.bar0 = *bar0;
                  }
                } else if (pciBdfFromRealpath.has_value()) {
                  pciInfo.sysfsPath = fmt::format(
                      "{}/bus/pci/devices/{}", sysfsRoot, *pciBdfFromRealpath);
                }
              }

              rw.pciInfo = pciInfo;
              XLOG(INFO) << fmt::format(
                  "Matched DEVMEM misc watchdog {} as {} in {} PCI {} csr=0x{:x}",
                  rw.pmUnitScopedName,
                  *misc.pmUnitScopedName(),
                  pmUnitNameForLog,
                  pciInfo.pmUnitScopedName,
                  rw.csrOffset.value_or(0));
              return true;
            }
          }
        }
      }
    }

    // Check i2cDeviceConfigs
    for (const auto& i2cDev : *pmCfg.i2cDeviceConfigs()) {
      if (*i2cDev.pmUnitScopedName() == rw.devicePmName ||
          *i2cDev.pmUnitScopedName() == rw.pmUnitScopedName) {
        if (*i2cDev.isWatchdog()) {
          rw.accessType = AccessType::I2C;
          rw.i2cBusName = *i2cDev.busName();
          rw.i2cAddressStr = *i2cDev.address();
          rw.kernelDeviceName = *i2cDev.kernelDeviceName();

          // Determine family
          auto kname = rw.kernelDeviceName;
          if (kname == "mp3_fancpld") {
            rw.family = WatchdogFamily::FB_FAN_CPLD;
            rw.hasExpiredBit = true;
          } else if (
              kname == "fbfancpld" || kname == "w800_fancpld" ||
              kname == "icecube_fancpld" || kname == "anacapa_fancpld" ||
              kname == "tahansb_fancpld" || kname == "leh800b_pdbcpld" ||
              kname.find("fancpld") != std::string::npos) {
            rw.family = WatchdogFamily::FB_FAN_CPLD;
            rw.hasExpiredBit = false;
          } else if (
              kname.find("fan") != std::string::npos &&
              kname.find("cpld") != std::string::npos) {
            // Arista fan CPLD or similar
            // Check if it uses 0x06 layout by kernel name pattern
            // For safety, default to FB_FAN_CPLD which covers 0x1F layout,
            // but if bus is Arista and address is not 0x33, try ARISTA_FAN_CPLD
            rw.family = WatchdogFamily::FB_FAN_CPLD;
          } else {
            rw.family = WatchdogFamily::GENERIC_I2C;
          }

          if (i2cDev.fanCpldConfig().has_value()) {
            rw.isFanCpldConfig = true;
          }

          // Validate address matches realpath if we have it
          if (rw.i2cAddr.has_value()) {
            try {
              int cfgAddr = std::stoi(rw.i2cAddressStr, nullptr, 0);
              if (cfgAddr != *rw.i2cAddr) {
                XLOG(WARN) << fmt::format(
                    "Address mismatch for {}: config {} vs sysfs 0x{:x}",
                    rw.pmUnitScopedName,
                    rw.i2cAddressStr,
                    *rw.i2cAddr);
              }
            } catch (const std::exception& ex) {
              XLOG(DBG1) << fmt::format(
                  "Failed to parse config i2c address '{}' for {}: {}",
                  rw.i2cAddressStr,
                  rw.pmUnitScopedName,
                  ex.what());
            }
          } else {
            // No sysfs addr, parse from config
            try {
              rw.i2cAddr = std::stoi(rw.i2cAddressStr, nullptr, 0);
            } catch (const std::exception& ex) {
              XLOG(DBG1) << fmt::format(
                  "Failed to parse config i2c address '{}' for {}: {}",
                  rw.i2cAddressStr,
                  rw.pmUnitScopedName,
                  ex.what());
            }
          }

          XLOG(INFO) << fmt::format(
              "Matched I2C watchdog {} as {} in {} bus={} addr={} kname={}",
              rw.pmUnitScopedName,
              *i2cDev.pmUnitScopedName(),
              pmUnitNameForLog,
              rw.i2cBusName,
              rw.i2cAddressStr,
              rw.kernelDeviceName);
          return true;
        }
      }
    }

    return false;
  };

  bool found = false;
  for (const auto& [pmName, pmCfg] : *cfg.pmUnitConfigs()) {
    if (searchPmUnit(pmCfg, pmName)) {
      found = true;
      break;
    }
  }

  if (!found) {
    for (const auto& [pmName, verList] : *cfg.versionedPmUnitConfigs()) {
      for (const auto& verCfg : verList) {
        if (searchPmUnit(
                *verCfg.pmUnitConfig(), fmt::format("{} versioned", pmName))) {
          found = true;
          break;
        }
      }
      if (found) {
        break;
      }
    }
  }

  if (!found) {
    XLOG(WARN) << fmt::format(
        "No config match for watchdog {} devicePmName={} devicePath={}",
        rw.pmUnitScopedName,
        rw.devicePmName,
        rw.devicePath);
    // Infer from bus presence
    if (rw.i2cBus.has_value()) {
      rw.accessType = AccessType::I2C;
      rw.family = WatchdogFamily::FB_FAN_CPLD;
    } else {
      rw.accessType = AccessType::DEVMEM;
      rw.family = WatchdogFamily::GENERIC_DEVMEM;
    }
  }

  // Second fallback: if we have I2C address from config but still missing bus,
  // scan /sys/bus/i2c/devices again (now that we have address from config)
  if (!rw.i2cBus.has_value() && rw.accessType == AccessType::I2C &&
      !rw.i2cAddressStr.empty()) {
    int cfgAddr = -1;
    try {
      cfgAddr = std::stoi(rw.i2cAddressStr, nullptr, 0);
    } catch (const std::exception& ex) {
      XLOG(DBG1) << fmt::format(
          "Failed to parse config i2c address '{}' for {}: {}",
          rw.i2cAddressStr,
          rw.pmUnitScopedName,
          ex.what());
    }
    if (cfgAddr >= 0) {
      try {
        std::string addrHex4 = fmt::format("{:04x}", cfgAddr);
        std::string i2cDevicesPath = sysfsRoot + "/bus/i2c/devices";
        for (const auto& entry : fs::directory_iterator(i2cDevicesPath)) {
          std::string fname = entry.path().filename().string();
          static const re2::RE2 kDevRe(R"((\d+)-([0-9a-fA-F]{4}))");
          int busNum = -1;
          std::string devAddr;
          if (re2::RE2::FullMatch(fname, kDevRe, &busNum, &devAddr)) {
            std::string lower = devAddr;
            std::transform(
                lower.begin(), lower.end(), lower.begin(), ::tolower);
            if (lower == addrHex4) {
              rw.i2cBus = busNum;
              if (!rw.i2cAddr.has_value()) {
                rw.i2cAddr = cfgAddr;
              }
              XLOG(INFO) << fmt::format(
                  "Fallback resolved bus={} for {} addr=0x{:x} via {}",
                  busNum,
                  rw.pmUnitScopedName,
                  cfgAddr,
                  fname);
              break;
            }
          }
        }
      } catch (const std::exception& ex) {
        XLOG(DBG1) << "Fallback scan failed: " << ex.what();
      }
    }
  }
}

std::vector<ResolvedWatchdog> ConfigResolver::resolveAll() {
  auto watchdogs = enumerateDevmap();
  loadPlatformConfig();

  for (auto& rw : watchdogs) {
    resolveDevicePath(rw);
    resolveHardwareDetails(rw);
  }

  return watchdogs;
}

} // namespace facebook::fboss::platform::watchdog_util
