// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/qsfp_service/test/hal_test/HalTestUtils.h"

#include <algorithm>
#include <atomic>
#include <optional>
#include <set>
#include <thread>

#include <folly/Conv.h>
#include <folly/FileUtil.h>
#include <folly/Format.h>
#include <folly/logging/xlog.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

#include "fboss/agent/FbossError.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/lib/bsp/BspPlatformMapping.h"
#include "fboss/lib/firmware_storage/FbossFwStorage.h"
#include "fboss/platform/helpers/PlatformNameLib.h"
#include "fboss/qsfp_service/if/gen-cpp2/qsfp_service_config_types.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"

#include "fboss/qsfp_service/module/properties/TransceiverPropertiesManager.h"

namespace facebook::fboss::hal_test {

namespace {
// Number of BSP override paths (i2cDevicePath / presentPath / resetPath) the
// entry sets.
int numBspPathOverrides(const HalTestTransceiverEntry& entry) {
  return (entry.i2cDevicePath().has_value() ? 1 : 0) +
      (entry.presentPath().has_value() ? 1 : 0) +
      (entry.resetPath().has_value() ? 1 : 0);
}

// True only if the entry fully specifies BSP override paths (all three). This
// is the mode for running on a non-FBOSS platform, where there is no
// BspPlatformMapping and the user supplies the IO / present / reset sysfs paths
// directly. Partial overrides are rejected by validateBspPathOverrides, so
// detection (all three) and validation (all three) agree.
bool hasBspPathOverrides(const HalTestTransceiverEntry& entry) {
  return numBspPathOverrides(entry) == 3;
}

// Override mode is all-or-nothing: an entry that sets some but not all of the
// three paths is a misconfiguration (there is no platform mapping to fill the
// rest on a non-FBOSS platform). Reject it with a clear error rather than
// silently falling back to the platform mapping.
void validateBspPathOverrides(const HalTestTransceiverEntry& entry) {
  int numOverrides = numBspPathOverrides(entry);
  if (numOverrides != 0 && numOverrides != 3) {
    throw FbossError(
        "Transceiver ",
        *entry.id(),
        ": BSP path override mode requires i2cDevicePath, presentPath, and "
        "resetPath to all be set (or none, to use the platform mapping)");
  }
}

// Build a BspTransceiverMapping from the user-provided paths in the config.
// Callers must ensure hasBspPathOverrides(entry) is true (all three paths set).
BspTransceiverMapping buildBspTransceiverMappingFromOverrides(
    const HalTestTransceiverEntry& entry) {
  int id = *entry.id();

  BspTransceiverMapping mapping;
  mapping.tcvrId() = id;

  BspTransceiverIOControllerInfo io;
  io.controllerId() = folly::to<std::string>(id);
  io.type() = TransceiverIOType::I2C;
  io.devicePath() = *entry.i2cDevicePath();
  mapping.io() = io;

  BspTransceiverAccessControllerInfo ac;
  ac.controllerId() = folly::to<std::string>(id);
  ac.type() = ResetAndPresenceAccessType::CPLD;

  BspPresencePinInfo presence;
  presence.sysfsPath() = *entry.presentPath();
  presence.mask() = 1;
  presence.presentHoldHi() = 0;
  ac.presence() = presence;

  BspResetPinInfo reset;
  reset.sysfsPath() = *entry.resetPath();
  reset.mask() = 1;
  reset.resetHoldHi() = 1;
  ac.reset() = reset;

  mapping.accessControl() = ac;
  return mapping;
}
} // namespace

std::unique_ptr<BspTransceiverImpl> createBspTransceiverImpl(
    const HalTestTransceiverEntry& entry,
    const BspPlatformMapping* bspMapping) {
  validateBspPathOverrides(entry);

  // Prefer user-provided paths (non-FBOSS mode).
  if (hasBspPathOverrides(entry)) {
    return std::make_unique<BspTransceiverImpl>(
        *entry.id(),
        *entry.name(),
        buildBspTransceiverMappingFromOverrides(entry));
  }

  // Otherwise use the platform's BspPlatformMapping (derived from XcvrLib /
  // platform_manager.json).
  if (bspMapping == nullptr) {
    throw FbossError(
        "Transceiver ",
        *entry.id(),
        ": no BSP path overrides provided and no platform BspPlatformMapping "
        "available");
  }
  if (!bspMapping->hasTcvrMapping(*entry.id())) {
    throw FbossError(
        "Transceiver ",
        *entry.id(),
        " not found in the platform BspPlatformMapping. Provide "
        "i2cDevicePath/presentPath/resetPath overrides in the HAL test config "
        "for it instead.");
  }
  return std::make_unique<BspTransceiverImpl>(
      *entry.id(), *entry.name(), bspMapping->getTcvrMapping(*entry.id()));
}

HalTestModule createQsfpModule(
    const HalTestTransceiverEntry& entry,
    const BspPlatformMapping* bspMapping) {
  HalTestModule result;
  result.impl = createBspTransceiverImpl(entry, bspMapping);

  std::set<std::string> portNames;
  portNames.insert(*entry.name());

  auto cfg = std::make_shared<const TransceiverConfig>(TransceiverOverrides{});

  result.module = std::make_unique<CmisModule>(
      std::move(portNames),
      result.impl.get(),
      std::move(cfg),
      false, // supportRemediate
      *entry.name());

  return result;
}

std::map<int, HalTestModule> createAllQsfpModules(const HalTestConfig& config) {
  // Only build the platform BspPlatformMapping if some entry relies on it (i.e.
  // has no path overrides). On a non-FBOSS platform every entry must supply its
  // own BSP paths, and we never touch PlatformNameLib / platform_manager.json.
  bool needPlatformMapping = false;
  for (const auto& entry : *config.transceivers()) {
    validateBspPathOverrides(entry);
    if (!hasBspPathOverrides(entry)) {
      needPlatformMapping = true;
    }
  }

  std::unique_ptr<BspPlatformMapping> bspMapping;
  if (needPlatformMapping) {
    // Derive the per-transceiver IO / reset / presence mapping from the running
    // platform (XcvrLib + platform_manager.json), auto-detected from the
    // device.
    auto platformName = platform::helpers::PlatformNameLib().getPlatformName();
    if (!platformName.has_value()) {
      throw FbossError(
          "Could not auto-detect platform name for BSP transceiver mapping. "
          "On a non-FBOSS platform, provide i2cDevicePath/presentPath/resetPath "
          "for every transceiver in the HAL test config instead.");
    }
    bspMapping = std::make_unique<BspPlatformMapping>(*platformName);
  }

  std::map<int, HalTestModule> modules;
  for (const auto& entry : *config.transceivers()) {
    int id = *entry.id();
    modules[id] = createQsfpModule(entry, bspMapping.get());
  }
  return modules;
}

HalTestConfig loadHalTestConfig(const std::string& configPath) {
  std::string contents;
  if (!folly::readFile(configPath.c_str(), contents)) {
    throw FbossError("Failed to read HAL test config file: ", configPath);
  }
  return apache::thrift::SimpleJSONSerializer::deserialize<HalTestConfig>(
      contents);
}

ProgramTransceiverState createProgramTransceiverState(
    const SpeedCombination& combo) {
  ProgramTransceiverState state;
  for (const auto& port : *combo.ports()) {
    TransceiverPortState portState;
    auto startHostLane = static_cast<uint8_t>(*port.hostLanes()->start());
    portState.portName = fmt::format("dummyPort/{:d}", startHostLane);
    portState.startHostLane = startHostLane;
    portState.speed = *port.speed();
    portState.numHostLanes = static_cast<uint8_t>(*port.hostLanes()->count());
    state.ports.emplace(portState.portName, portState);
  }
  return state;
}

std::vector<MediaInterfaceCode> getExpectedMediaInterfaceCodes(
    const std::string& comboDescription,
    const SpeedCombination& combo) {
  std::vector<MediaInterfaceCode> result(8, MediaInterfaceCode::UNKNOWN);
  int totalMediaLanes = 0;
  for (const auto& port : *combo.ports()) {
    auto mediaLaneCodeValue =
        extractFromMediaInterfaceUnion<SMFMediaInterfaceCode>(
            *port.mediaLaneCode());
    auto mediaInterfaceCode =
        TransceiverPropertiesManager::mediaLaneCodeToMediaInterfaceCode(
            static_cast<uint8_t>(mediaLaneCodeValue));
    if (mediaInterfaceCode == MediaInterfaceCode::UNKNOWN) {
      throw FbossError(
          "Unknown SMF media interface code 0x",
          fmt::format("{:02x}", static_cast<int>(mediaLaneCodeValue)),
          " in speed combination ",
          comboDescription);
    }
    auto start = *port.mediaLanes()->start();
    auto count = *port.mediaLanes()->count();
    for (int i = 0; i < count; i++) {
      result[start + i] = mediaInterfaceCode;
    }
    totalMediaLanes = std::max(totalMediaLanes, start + count);
  }
  result.resize(totalMediaLanes);
  return result;
}

std::vector<std::string> getAllSpeedCombinationDescriptions() {
  std::set<std::string> seen;
  std::vector<std::string> result;
  for (auto code : TransceiverPropertiesManager::getKnownCodes()) {
    const auto& props = TransceiverPropertiesManager::getProperties(code);
    for (const auto& combo : *props.supportedSpeedCombinations()) {
      const auto& desc = *combo.combinationName();
      if (seen.insert(desc).second) {
        result.push_back(desc);
      }
    }
  }
  return result;
}

std::vector<std::pair<std::string, std::string>>
getAllSpeedChangeTransitions() {
  std::set<std::pair<std::string, std::string>> seen;
  std::vector<std::pair<std::string, std::string>> result;
  for (auto code : TransceiverPropertiesManager::getKnownCodes()) {
    const auto& props = TransceiverPropertiesManager::getProperties(code);
    for (const auto& transition : *props.speedChangeTransitions()) {
      if (transition.size() == 2) {
        auto pair = std::make_pair(transition[0], transition[1]);
        if (seen.insert(pair).second) {
          result.push_back(std::move(pair));
        }
      }
    }
  }
  return result;
}

namespace {

struct ModuleFwVersions {
  std::string appVer; // application "major.minor"
  std::string dspVer; // DSP "major.minor"
  std::optional<int> appBuildNumber; // CDB build number, when reported
};

// Extract APP/DSP firmware versions (and the app build number) from a module's
// transceiver info.
ModuleFwVersions readFirmwareVersions(QsfpModule* module) {
  auto info = module->getTransceiverInfo();
  const auto& tcvrState = *info.tcvrState();

  ModuleFwVersions versions;
  if (tcvrState.status().has_value()) {
    const auto& status = *tcvrState.status();
    if (status.fwStatus().has_value()) {
      const auto& fwStatus = *status.fwStatus();
      if (fwStatus.version().has_value()) {
        versions.appVer = *fwStatus.version();
      }
      if (fwStatus.dspFwVer().has_value()) {
        versions.dspVer = *fwStatus.dspFwVer();
      }
      if (fwStatus.buildNumber().has_value()) {
        versions.appBuildNumber = *fwStatus.buildNumber();
      }
    }
  }
  return versions;
}

// The module reports the application version as a 2-tuple "major.minor", with
// the build number in a separate field. ZR optics use 3-tuple config versions
// like "1.1.8192". When the config version is a 3-tuple, append the module's
// build number so the comparison is apples-to-apples (mirrors
// TransceiverManager::getFirmwareUpgradeData).
std::string appVersionMatchingConfig(
    const ModuleFwVersions& versions,
    const std::string& configVersion) {
  if (std::count(configVersion.begin(), configVersion.end(), '.') >= 2 &&
      versions.appBuildNumber.has_value()) {
    return fmt::format("{}.{}", versions.appVer, *versions.appBuildNumber);
  }
  return versions.appVer;
}

} // namespace

void ensureModuleReady(QsfpModule* module) {
  module->detectPresence();
  auto* cmis = dynamic_cast<CmisModule*>(module);
  if (cmis == nullptr) {
    return;
  }
  // releaseModuleLowPowerModeLocked / moduleReadyStatePoll are safe to call
  // without the module mutex here: this runs single threaded before any upgrade
  // worker starts. Swallow transient early-boot I2C errors so callers (e.g.
  // SetUp) don't abort on a module that just needs another moment.
  try {
    cmis->releaseModuleLowPowerModeLocked();
    if (!cmis->moduleReadyStatePoll()) {
      XLOG(WARNING) << "Module " << module->getID()
                    << " did not reach ready state";
    }
  } catch (const std::exception& e) {
    XLOG(WARNING) << "Module " << module->getID()
                  << " readiness prep failed: " << e.what();
  }
}

bool upgradeFirmware(QsfpModule* module, const cfg::Firmware& desiredFw) {
  auto tcvrId = module->getID();

  module->detectPresence();
  module->refresh();

  auto current = readFirmwareVersions(module);

  // Check if upgrade is needed by comparing current vs desired versions
  bool needsUpgrade = false;
  for (const auto& fwVersion : *desiredFw.versions()) {
    auto desiredVer = *fwVersion.version();
    if (*fwVersion.fwType() == cfg::FirmwareType::APPLICATION) {
      auto currentAppVer = appVersionMatchingConfig(current, desiredVer);
      if (currentAppVer != desiredVer) {
        needsUpgrade = true;
        XLOG(INFO) << "Transceiver " << tcvrId << " APP firmware mismatch: "
                   << "current=" << currentAppVer << " desired=" << desiredVer;
      }
    } else if (*fwVersion.fwType() == cfg::FirmwareType::DSP) {
      if (current.dspVer != desiredVer) {
        needsUpgrade = true;
        XLOG(INFO) << "Transceiver " << tcvrId << " DSP firmware mismatch: "
                   << "current=" << current.dspVer << " desired=" << desiredVer;
      }
    }
  }

  if (!needsUpgrade) {
    XLOG(INFO) << "Transceiver " << tcvrId
               << " firmware already at desired version";
    return false;
  }

  // Resolve firmware images and perform upgrade
  auto fwStorage = FbossFwStorage::initStorage();
  auto fwStorageHandle = module->getFwStorageHandle();

  std::vector<std::unique_ptr<FbossFirmware>> fwList;
  for (const auto& fwVersion : *desiredFw.versions()) {
    fwList.push_back(
        fwStorage.getFirmware(fwStorageHandle, *fwVersion.version()));
  }

  XLOG(INFO) << "Upgrading firmware on transceiver " << tcvrId;
  module->upgradeFirmware(fwList);

  // Re-detect and refresh after upgrade
  module->detectPresence();
  module->refresh();

  // Verify post-upgrade firmware versions match desired versions
  auto postVersions = readFirmwareVersions(module);
  for (const auto& fwVersion : *desiredFw.versions()) {
    auto desiredVer = *fwVersion.version();
    if (*fwVersion.fwType() == cfg::FirmwareType::APPLICATION) {
      auto postAppVer = appVersionMatchingConfig(postVersions, desiredVer);
      if (postAppVer != desiredVer) {
        throw FbossError(
            "Transceiver ",
            tcvrId,
            " APP firmware version mismatch after upgrade: expected=",
            desiredVer,
            " actual=",
            postAppVer);
      }
    }
    if (*fwVersion.fwType() == cfg::FirmwareType::DSP &&
        postVersions.dspVer != desiredVer) {
      throw FbossError(
          "Transceiver ",
          tcvrId,
          " DSP firmware version mismatch after upgrade: expected=",
          desiredVer,
          " actual=",
          postVersions.dspVer);
    }
  }

  XLOG(INFO) << "Firmware upgrade complete for transceiver " << tcvrId;
  return true;
}

int applyStartupFirmwareUpgrades(
    const HalTestConfig& config,
    std::map<int, HalTestModule>& modules) {
  std::atomic<int> upgraded{0};

  // Collect entries that are eligible for upgrade
  struct UpgradeTask {
    int id;
    const HalTestTransceiverEntry* entry;
    HalTestModule* halModule;
  };
  std::vector<UpgradeTask> tasks;

  for (const auto& entry : *config.transceivers()) {
    int id = *entry.id();
    auto it = modules.find(id);
    if (it == modules.end()) {
      XLOG(WARNING) << "Transceiver " << id
                    << " has startup config but no module";
      continue;
    }

    auto& halModule = it->second;
    if (!halModule.impl->detectTransceiver()) {
      XLOG(WARNING) << "Transceiver " << id
                    << " has startup config but is not present";
      continue;
    }

    tasks.push_back({id, &entry, &halModule});
  }

  // Run upgrades in parallel
  std::vector<std::thread> threads;
  threads.reserve(tasks.size());
  for (const auto& task : tasks) {
    threads.emplace_back([&upgraded, &task]() {
      if (upgradeFirmware(
              task.halModule->module.get(),
              *task.entry->startupConfig()->firmware())) {
        ++upgraded;
      }
    });
  }
  for (auto& t : threads) {
    t.join();
  }

  return upgraded.load();
}

} // namespace facebook::fboss::hal_test
