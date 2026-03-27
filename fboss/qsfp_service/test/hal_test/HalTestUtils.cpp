// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/qsfp_service/test/hal_test/HalTestUtils.h"

#include <atomic>
#include <set>
#include <thread>

#include <folly/Conv.h>
#include <folly/FileUtil.h>
#include <folly/Format.h>
#include <folly/logging/xlog.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

#include "fboss/agent/FbossError.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/lib/firmware_storage/FbossFwStorage.h"
#include "fboss/qsfp_service/if/gen-cpp2/qsfp_service_config_types.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"

#include "fboss/qsfp_service/module/properties/TransceiverPropertiesManager.h"

namespace facebook::fboss::hal_test {

BspTransceiverMapping buildBspTransceiverMapping(
    const HalTestTransceiverEntry& entry) {
  int id = *entry.id();

  BspTransceiverMapping mapping;
  mapping.tcvrId() = id;

  // IO
  BspTransceiverIOControllerInfo io;
  io.controllerId() = folly::to<std::string>(id);
  io.type() = TransceiverIOType::I2C;
  if (entry.i2cDevicePath().has_value()) {
    io.devicePath() = *entry.i2cDevicePath();
  } else {
    io.devicePath() = fmt::format("/run/devmap/xcvrs/xcvr_io_{}", id);
  }
  mapping.io() = io;

  // Access control
  BspTransceiverAccessControllerInfo ac;
  ac.controllerId() = folly::to<std::string>(id);
  ac.type() = ResetAndPresenceAccessType::CPLD;

  BspPresencePinInfo presence;
  if (entry.presentPath().has_value()) {
    presence.sysfsPath() = *entry.presentPath();
  } else {
    presence.sysfsPath() =
        fmt::format("/run/devmap/xcvrs/xcvr_ctrl_{}/xcvr_present_{}", id, id);
  }
  presence.mask() = 1;
  presence.presentHoldHi() = 0;
  ac.presence() = presence;

  BspResetPinInfo reset;
  if (entry.resetPath().has_value()) {
    reset.sysfsPath() = *entry.resetPath();
  } else {
    reset.sysfsPath() =
        fmt::format("/run/devmap/xcvrs/xcvr_ctrl_{}/xcvr_reset_{}", id, id);
  }
  reset.mask() = 1;
  reset.resetHoldHi() = 1;
  ac.reset() = reset;

  mapping.accessControl() = ac;
  return mapping;
}

std::unique_ptr<BspTransceiverImpl> createBspTransceiverImpl(
    const HalTestTransceiverEntry& entry) {
  auto mapping = buildBspTransceiverMapping(entry);
  return std::make_unique<BspTransceiverImpl>(
      *entry.id(), *entry.name(), std::move(mapping));
}

HalTestModule createQsfpModule(const HalTestTransceiverEntry& entry) {
  HalTestModule result;
  result.impl = createBspTransceiverImpl(entry);

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
  std::map<int, HalTestModule> modules;
  for (const auto& entry : *config.transceivers()) {
    int id = *entry.id();
    modules[id] = createQsfpModule(entry);
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
    auto start = *port.hostLanes()->start();
    auto count = *port.hostLanes()->count();
    for (int i = 0; i < count; i++) {
      result[start + i] = mediaInterfaceCode;
    }
  }
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

// Extract APP and DSP firmware version strings from a module's transceiver
// info.
std::pair<std::string, std::string> readFirmwareVersions(QsfpModule* module) {
  auto info = module->getTransceiverInfo();
  const auto& tcvrState = *info.tcvrState();

  std::string appVer;
  std::string dspVer;
  if (tcvrState.status().has_value()) {
    const auto& status = *tcvrState.status();
    if (status.fwStatus().has_value()) {
      const auto& fwStatus = *status.fwStatus();
      if (fwStatus.version().has_value()) {
        appVer = *fwStatus.version();
      }
      if (fwStatus.dspFwVer().has_value()) {
        dspVer = *fwStatus.dspFwVer();
      }
    }
  }
  return {appVer, dspVer};
}

} // namespace

bool upgradeFirmware(QsfpModule* module, const cfg::Firmware& desiredFw) {
  auto tcvrId = module->getID();

  module->detectPresence();
  module->refresh();

  auto [currentAppVer, currentDspVer] = readFirmwareVersions(module);

  // Check if upgrade is needed by comparing current vs desired versions
  bool needsUpgrade = false;
  for (const auto& fwVersion : *desiredFw.versions()) {
    auto desiredVer = *fwVersion.version();
    if (*fwVersion.fwType() == cfg::FirmwareType::APPLICATION) {
      if (currentAppVer != desiredVer) {
        needsUpgrade = true;
        XLOG(INFO) << "Transceiver " << tcvrId << " APP firmware mismatch: "
                   << "current=" << currentAppVer << " desired=" << desiredVer;
      }
    } else if (*fwVersion.fwType() == cfg::FirmwareType::DSP) {
      if (currentDspVer != desiredVer) {
        needsUpgrade = true;
        XLOG(INFO) << "Transceiver " << tcvrId << " DSP firmware mismatch: "
                   << "current=" << currentDspVer << " desired=" << desiredVer;
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
  auto [postAppVer, postDspVer] = readFirmwareVersions(module);
  for (const auto& fwVersion : *desiredFw.versions()) {
    auto desiredVer = *fwVersion.version();
    if (*fwVersion.fwType() == cfg::FirmwareType::APPLICATION &&
        postAppVer != desiredVer) {
      throw FbossError(
          "Transceiver ",
          tcvrId,
          " APP firmware version mismatch after upgrade: expected=",
          desiredVer,
          " actual=",
          postAppVer);
    }
    if (*fwVersion.fwType() == cfg::FirmwareType::DSP &&
        postDspVer != desiredVer) {
      throw FbossError(
          "Transceiver ",
          tcvrId,
          " DSP firmware version mismatch after upgrade: expected=",
          desiredVer,
          " actual=",
          postDspVer);
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
