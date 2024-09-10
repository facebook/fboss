// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/platform_manager/PlatformExplorer.h"

#include <algorithm>
#include <chrono>
#include <exception>
#include <filesystem>
#include <ranges>
#include <stdexcept>
#include <string>
#include <utility>

#include <fb303/ServiceData.h>
#include <folly/FileUtil.h>
#include <folly/logging/xlog.h>

#include "fboss/platform/helpers/PlatformFsUtils.h"
#include "fboss/platform/helpers/PlatformUtils.h"
#include "fboss/platform/platform_manager/Utils.h"
#include "fboss/platform/platform_manager/gen-cpp2/platform_manager_config_constants.h"
#include "fboss/platform/weutil/IoctlSmbusEepromReader.h"

namespace facebook::fboss::platform::platform_manager {
namespace {
constexpr auto kTotalFailures = "total_failures";
constexpr auto kRootSlotPath = "/";

std::string getSlotPath(
    const std::string& parentSlotPath,
    const std::string& slotName) {
  if (parentSlotPath == kRootSlotPath) {
    return fmt::format("{}{}", kRootSlotPath, slotName);
  } else {
    return fmt::format("{}/{}", parentSlotPath, slotName);
  }
}

// Read a singular version number from the file at given path. If there is any
// error reading the file, log and return a default of 0. The version number
// must be the first non-whitespace substring, but the file may contain
// additional non-numeric data after the version (e.g. human-readable comments).
// TODO: Handle hwmon/info_rom cases (by standardizing them away, if possible).
int readVersionNumber(
    const std::string& path,
    const PlatformFsUtils* platformFsUtils) {
  const auto versionFileContent = platformFsUtils->getStringFileContent(path);
  if (!versionFileContent) {
    // This log is necessary to distinguish read error vs reading "0".
    XLOGF(
        ERR,
        "Failed to open firmware version file {}: {}",
        path,
        folly::errnoStr(errno));
    return 0;
  }
  // Note: stoi infers base (e.g. 0xF) when 0 is passed as the base argument as
  // below. It also discards leading whitespace, and stops at non-digits.
  try {
    int version = stoi(versionFileContent.value(), nullptr, 0);
    return version;
  } catch (const std::exception& ex) {
    XLOGF(
        ERR,
        "Failed to parse firmware version from file {}: {}",
        path,
        folly::exceptionStr(ex));
    return 0;
  }
}

PlatformManagerStatus createPmStatus(
    const ExplorationStatus& explorationStatus,
    int64_t lastExplorationTime) {
  PlatformManagerStatus status;
  status.explorationStatus() = explorationStatus;
  status.lastExplorationTime() = lastExplorationTime;
  return status;
}

PlatformManagerStatus createPmStatus(
    const ExplorationStatus& explorationStatus) {
  return createPmStatus(explorationStatus, 0);
}
} // namespace

namespace constants = platform_manager_config_constants;

PlatformExplorer::PlatformExplorer(
    const PlatformConfig& config,
    const std::shared_ptr<PlatformFsUtils> platformFsUtils)
    : platformConfig_(config),
      dataStore_(platformConfig_),
      devicePathResolver_(platformConfig_, dataStore_, i2cExplorer_),
      presenceChecker_(devicePathResolver_),
      explorationErrMap_(platformConfig_, dataStore_),
      platformFsUtils_(platformFsUtils) {
  updatePmStatus(createPmStatus(ExplorationStatus::UNSTARTED));
}

void PlatformExplorer::explore() {
  XLOG(INFO) << "Exploring the platform";
  updatePmStatus(createPmStatus(ExplorationStatus::IN_PROGRESS));
  for (const auto& [busName, busNum] :
       i2cExplorer_.getBusNums(*platformConfig_.i2cAdaptersFromCpu())) {
    dataStore_.updateI2cBusNum(std::nullopt, busName, busNum);
  }
  auto pmUnitName =
      getPmUnitNameFromSlot(*platformConfig_.rootSlotType(), kRootSlotPath);
  CHECK(pmUnitName == *platformConfig_.rootPmUnitName());
  explorePmUnit(kRootSlotPath, *platformConfig_.rootPmUnitName());
  XLOG(INFO) << "Creating symbolic links ...";
  for (const auto& [linkPath, devicePath] :
       *platformConfig_.symbolicLinkToDevicePath()) {
    createDeviceSymLink(linkPath, devicePath);
  }
  publishFirmwareVersions();
  auto explorationStatus = concludeExploration();
  updatePmStatus(createPmStatus(
      explorationStatus,
      std::chrono::duration_cast<std::chrono::seconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count()));
}

void PlatformExplorer::explorePmUnit(
    const std::string& slotPath,
    const std::string& pmUnitName) {
  auto pmUnitConfig = dataStore_.resolvePmUnitConfig(slotPath);
  XLOG(INFO) << fmt::format("Exploring PmUnit {} at {}", pmUnitName, slotPath);

  XLOG(INFO) << fmt::format(
      "Exploring PCI Devices for PmUnit {} at SlotPath {}. Count {}",
      pmUnitName,
      slotPath,
      pmUnitConfig.pciDeviceConfigs()->size());
  explorePciDevices(slotPath, *pmUnitConfig.pciDeviceConfigs());

  XLOG(INFO) << fmt::format(
      "Exploring I2C Devices for PmUnit {} at SlotPath {}. Count {}",
      pmUnitName,
      slotPath,
      pmUnitConfig.i2cDeviceConfigs()->size());
  exploreI2cDevices(slotPath, *pmUnitConfig.i2cDeviceConfigs());

  if (!pmUnitConfig.embeddedSensorConfigs()->empty()) {
    XLOG(INFO) << fmt::format(
        "Processing Embedded Sensors for PmUnit {} at SlotPath {}. Count {}",
        pmUnitName,
        slotPath,
        pmUnitConfig.embeddedSensorConfigs()->size());
    for (const auto& embeddedSensorConfig :
         *pmUnitConfig.embeddedSensorConfigs()) {
      dataStore_.updateSysfsPath(
          Utils().createDevicePath(
              slotPath, *embeddedSensorConfig.pmUnitScopedName()),
          *embeddedSensorConfig.sysfsPath());
    }
  }

  XLOG(INFO) << fmt::format(
      "Exploring Slots for PmUnit {} at SlotPath {}. Count {}",
      pmUnitName,
      slotPath,
      pmUnitConfig.outgoingSlotConfigs_ref()->size());
  for (const auto& [slotName, slotConfig] :
       *pmUnitConfig.outgoingSlotConfigs()) {
    exploreSlot(slotPath, slotName, slotConfig);
  }
}

void PlatformExplorer::exploreSlot(
    const std::string& parentSlotPath,
    const std::string& slotName,
    const SlotConfig& slotConfig) {
  std::string childSlotPath = getSlotPath(parentSlotPath, slotName);
  XLOG(INFO) << fmt::format("Exploring SlotPath {}", childSlotPath);

  // If PresenceDetection is specified, proceed further only if the presence
  // condition is satisfied
  if (const auto presenceDetection = slotConfig.presenceDetection()) {
    try {
      if (!presenceChecker_.isPresent(
              presenceDetection.value(), childSlotPath)) {
        return;
      }
    } catch (const std::exception& ex) {
      XLOG(ERR) << fmt::format(
          "Error checking for presence in slotpath {}: {}",
          slotName,
          ex.what());
      return;
    }
  }

  // Either no PresenceDetection is specified, or the presence conditions in
  // PresenceDetection are satisfied. Setup the PmUnit in the slot.
  int i = 0;
  for (const auto& busName : *slotConfig.outgoingI2cBusNames()) {
    auto busNum = dataStore_.getI2cBusNum(parentSlotPath, busName);
    dataStore_.updateI2cBusNum(
        childSlotPath, fmt::format("INCOMING@{}", i++), busNum);
  }
  auto childPmUnitName =
      getPmUnitNameFromSlot(*slotConfig.slotType(), childSlotPath);

  if (!childPmUnitName) {
    XLOG(INFO) << fmt::format(
        "No device could be read in Slot {}", childSlotPath);
    return;
  }

  explorePmUnit(childSlotPath, *childPmUnitName);
}

std::optional<std::string> PlatformExplorer::getPmUnitNameFromSlot(
    const std::string& slotType,
    const std::string& slotPath) {
  auto slotTypeConfig = platformConfig_.slotTypeConfigs_ref()->at(slotType);
  CHECK(slotTypeConfig.idpromConfig() || slotTypeConfig.pmUnitName());
  std::optional<std::string> pmUnitNameInEeprom{std::nullopt};
  std::optional<int> productProductionStateInEeprom{std::nullopt};
  std::optional<int> productVersionInEeprom{std::nullopt};
  std::optional<int> productSubVersionInEeprom{std::nullopt};
  if (slotTypeConfig.idpromConfig_ref()) {
    auto idpromConfig = *slotTypeConfig.idpromConfig_ref();
    auto eepromI2cBusNum =
        dataStore_.getI2cBusNum(slotPath, *idpromConfig.busName());
    std::string eepromPath = "";

    /*
    Because of upstream kernel issues, we have to manually read the
    SCM EEPROM for the Meru800BFA/BIA platforms. It is read directly
    with ioctl and written to the /run/devmap file.
    See: https://github.com/facebookexternal/fboss.bsp.arista/pull/31/files
    */
    if ((platformConfig_.platformName().value() == "meru800bfa" ||
         platformConfig_.platformName().value() == "meru800bia") &&
        (!(idpromConfig.busName()->starts_with("INCOMING")) &&
         *idpromConfig.address() == "0x50")) {
      try {
        std::string eepromDir = "/run/devmap/eeproms/";
        std::string eepromName = "MERU_SCM_EEPROM";
        eepromPath = eepromDir + eepromName;
        IoctlSmbusEepromReader::readEeprom(
            eepromDir,
            eepromName,
            0,
            std::stoi(*idpromConfig.address(), nullptr, 16),
            dataStore_.getI2cBusNum(slotPath, *idpromConfig.busName()));
      } catch (const std::exception& e) {
        auto errMsg = fmt::format(
            "Could not read MERU_SCM_EEPROM for {}: {}",
            *idpromConfig.address(),
            e.what());
        XLOG(ERR) << errMsg;
        explorationErrMap_.add(slotPath, "IDPROM", errMsg);
      }
    } else {
      createI2cDevice(
          Utils().createDevicePath(slotPath, "IDPROM"),
          *idpromConfig.kernelDeviceName(),
          eepromI2cBusNum,
          I2cAddr(*idpromConfig.address()));
      eepromPath = i2cExplorer_.getDeviceI2cPath(
          eepromI2cBusNum, I2cAddr(*idpromConfig.address()));
      eepromPath = eepromPath + "/eeprom";
    }
    try {
      pmUnitNameInEeprom =
          eepromParser_.getProductName(eepromPath, *idpromConfig.offset());
      // TODO: Avoid this side effect in this function.
      // I think we can refactor this simpler once I2CDevicePaths are also
      // stored in DataStore. 1/ Create IDPROMs 2/ Read contents from eepromPath
      // stored in DataStore.
      productProductionStateInEeprom = eepromParser_.getProdutProductionState(
          eepromPath, *idpromConfig.offset());
      productVersionInEeprom =
          eepromParser_.getProductVersion(eepromPath, *idpromConfig.offset());
      productSubVersionInEeprom = eepromParser_.getProductSubVersion(
          eepromPath, *idpromConfig.offset());
      XLOG(INFO) << fmt::format(
          "Found ProductProductionState `{}` ProductVersion `{}` ProductSubVersion `{}` in IDPROM {} at {}",
          productProductionStateInEeprom
              ? std::to_string(*productProductionStateInEeprom)
              : "<ABSENT>",
          productVersionInEeprom ? std::to_string(*productVersionInEeprom)
                                 : "<ABSENT>",
          productSubVersionInEeprom ? std::to_string(*productSubVersionInEeprom)
                                    : "<ABSENT>",
          eepromPath,
          slotPath);
    } catch (const std::exception& e) {
      auto errMsg = fmt::format(
          "Could not fetch contents of IDPROM {} in {}. {}",
          eepromPath,
          slotPath,
          e.what());
      XLOG(ERR) << errMsg;
      explorationErrMap_.add(slotPath, "IDPROM", errMsg);
    }
    if (pmUnitNameInEeprom) {
      XLOG(INFO) << fmt::format(
          "Found PmUnit name `{}` in IDPROM {} at {}",
          *pmUnitNameInEeprom,
          eepromPath,
          slotPath);
    }
  }

  auto pmUnitName = pmUnitNameInEeprom;
  XLOG(INFO) << "SlotType PmUnitName: " << *slotTypeConfig.pmUnitName();
  if (slotTypeConfig.pmUnitName()) {
    if (pmUnitNameInEeprom &&
        *pmUnitNameInEeprom != *slotTypeConfig.pmUnitName()) {
      XLOG(WARNING) << fmt::format(
          "The PmUnit name in IDPROM -`{}` is different from the one "
          "in config - `{}`. NEEDS FIX.",
          *pmUnitNameInEeprom,
          *slotTypeConfig.pmUnitName());
    }
    XLOG(INFO) << fmt::format(
        "Going with PmUnit name `{}` defined in config for {}",
        *slotTypeConfig.pmUnitName(),
        slotPath);
    pmUnitName = *slotTypeConfig.pmUnitName();
  }
  if (!pmUnitName) {
    throw std::runtime_error(fmt::format(
        "PmUnitName must be configured in SlotTypeConfig::pmUnitName "
        "or SlotTypeConfig::idpromConfig at {}",
        slotPath));
  }
  dataStore_.updatePmUnitInfo(
      slotPath,
      *pmUnitName,
      productProductionStateInEeprom,
      productVersionInEeprom,
      productSubVersionInEeprom);
  return pmUnitName;
}

void PlatformExplorer::exploreI2cDevices(
    const std::string& slotPath,
    const std::vector<I2cDeviceConfig>& i2cDeviceConfigs) {
  for (const auto& i2cDeviceConfig : i2cDeviceConfigs) {
    try {
      auto busNum =
          dataStore_.getI2cBusNum(slotPath, *i2cDeviceConfig.busName());
      auto devAddr = I2cAddr(*i2cDeviceConfig.address());
      auto devicePath = Utils().createDevicePath(
          slotPath, *i2cDeviceConfig.pmUnitScopedName());
      if (i2cDeviceConfig.initRegSettings()) {
        setupI2cDevice(
            devicePath, busNum, devAddr, *i2cDeviceConfig.initRegSettings());
      }
      createI2cDevice(
          devicePath, *i2cDeviceConfig.kernelDeviceName(), busNum, devAddr);
      if (i2cDeviceConfig.numOutgoingChannels()) {
        auto channelToBusNums =
            i2cExplorer_.getMuxChannelI2CBuses(busNum, devAddr);
        if (channelToBusNums.size() != *i2cDeviceConfig.numOutgoingChannels()) {
          throw std::runtime_error(fmt::format(
              "Unexpected number mux channels for {}. Expected: {}. Actual: {}",
              *i2cDeviceConfig.pmUnitScopedName(),
              *i2cDeviceConfig.numOutgoingChannels(),
              channelToBusNums.size()));
        }
        for (const auto& [channelNum, busNum] : channelToBusNums) {
          dataStore_.updateI2cBusNum(
              slotPath,
              fmt::format(
                  "{}@{}", *i2cDeviceConfig.pmUnitScopedName(), channelNum),
              busNum);
        }
      }
      if (*i2cDeviceConfig.isGpioChip()) {
        auto i2cDevicePath = i2cExplorer_.getDeviceI2cPath(busNum, devAddr);
        dataStore_.updateCharDevPath(
            Utils().createDevicePath(
                slotPath, *i2cDeviceConfig.pmUnitScopedName()),
            Utils().resolveGpioChipCharDevPath(i2cDevicePath));
      }
      if (*i2cDeviceConfig.isWatchdog()) {
        auto i2cDevicePath = i2cExplorer_.getDeviceI2cPath(busNum, devAddr);
        dataStore_.updateCharDevPath(
            Utils().createDevicePath(
                slotPath, *i2cDeviceConfig.pmUnitScopedName()),
            Utils().resolveWatchdogCharDevPath(i2cDevicePath));
      }
    } catch (const std::exception& ex) {
      auto errMsg = fmt::format(
          "Failed to explore I2C device {} at {}. {}",
          *i2cDeviceConfig.pmUnitScopedName(),
          slotPath,
          ex.what());
      XLOG(ERR) << errMsg;
      explorationErrMap_.add(
          slotPath, *i2cDeviceConfig.pmUnitScopedName(), errMsg);
    }
  }
}

void PlatformExplorer::explorePciDevices(
    const std::string& slotPath,
    const std::vector<PciDeviceConfig>& pciDeviceConfigs) {
  for (const auto& pciDeviceConfig : pciDeviceConfigs) {
    try {
      auto pciDevice = PciDevice(
          *pciDeviceConfig.pmUnitScopedName(),
          *pciDeviceConfig.vendorId(),
          *pciDeviceConfig.deviceId(),
          *pciDeviceConfig.subSystemVendorId(),
          *pciDeviceConfig.subSystemDeviceId());
      auto charDevPath = pciDevice.charDevPath();
      auto instId =
          getFpgaInstanceId(slotPath, *pciDeviceConfig.pmUnitScopedName());
      for (const auto& i2cAdapterConfig :
           *pciDeviceConfig.i2cAdapterConfigs()) {
        auto busNums = pciExplorer_.createI2cAdapter(
            pciDevice, i2cAdapterConfig, instId++);
        if (*i2cAdapterConfig.numberOfAdapters() > 1) {
          CHECK_EQ(busNums.size(), *i2cAdapterConfig.numberOfAdapters());
          for (auto i = 0; i < busNums.size(); i++) {
            dataStore_.updateI2cBusNum(
                slotPath,
                fmt::format(
                    "{}@{}",
                    *i2cAdapterConfig.fpgaIpBlockConfig()->pmUnitScopedName(),
                    i),
                busNums[i]);
          }
        } else {
          CHECK_EQ(busNums.size(), 1);
          dataStore_.updateI2cBusNum(
              slotPath,
              *i2cAdapterConfig.fpgaIpBlockConfig()->pmUnitScopedName(),
              busNums[0]);
        }
      }
      for (const auto& spiMasterConfig : *pciDeviceConfig.spiMasterConfigs()) {
        auto spiCharDevPaths =
            pciExplorer_.createSpiMaster(pciDevice, spiMasterConfig, instId++);
        for (const auto& [pmUnitScopedName, spiCharDevPath] : spiCharDevPaths) {
          dataStore_.updateCharDevPath(
              Utils().createDevicePath(slotPath, pmUnitScopedName),
              spiCharDevPath);
        }
      }
      for (const auto& fpgaIpBlockConfig : *pciDeviceConfig.gpioChipConfigs()) {
        auto gpioCharDevPath =
            pciExplorer_.createGpioChip(pciDevice, fpgaIpBlockConfig, instId++);
        dataStore_.updateCharDevPath(
            Utils().createDevicePath(
                slotPath, *fpgaIpBlockConfig.pmUnitScopedName()),
            gpioCharDevPath);
      }
      for (const auto& fpgaIpBlockConfig : *pciDeviceConfig.watchdogConfigs()) {
        auto watchdogCharDevPath =
            pciExplorer_.createWatchdog(pciDevice, fpgaIpBlockConfig, instId++);
        dataStore_.updateCharDevPath(
            Utils().createDevicePath(
                slotPath, *fpgaIpBlockConfig.pmUnitScopedName()),
            watchdogCharDevPath);
      }
      for (const auto& fanPwmCtrlConfig :
           *pciDeviceConfig.fanTachoPwmConfigs()) {
        auto fanCtrlSysfsPath = pciExplorer_.createFanPwmCtrl(
            pciDevice, fanPwmCtrlConfig, instId++);
        dataStore_.updateSysfsPath(
            Utils().createDevicePath(
                slotPath,
                *fanPwmCtrlConfig.fpgaIpBlockConfig()->pmUnitScopedName()),
            fanCtrlSysfsPath);
      }
      for (const auto& fpgaIpBlockConfig : *pciDeviceConfig.ledCtrlConfigs()) {
        pciExplorer_.createLedCtrl(pciDevice, fpgaIpBlockConfig, instId++);
      }
      for (const auto& xcvrCtrlConfig : *pciDeviceConfig.xcvrCtrlConfigs()) {
        auto devicePath = Utils().createDevicePath(
            slotPath, *xcvrCtrlConfig.fpgaIpBlockConfig()->pmUnitScopedName());
        auto xcvrCtrlSysfsPath =
            pciExplorer_.createXcvrCtrl(pciDevice, xcvrCtrlConfig, instId++);
        dataStore_.updateSysfsPath(devicePath, xcvrCtrlSysfsPath);
      }
      for (const auto& infoRomConfig : *pciDeviceConfig.infoRomConfigs()) {
        auto infoRomSysfsPath =
            pciExplorer_.createInfoRom(pciDevice, infoRomConfig, instId++);
        dataStore_.updateSysfsPath(
            Utils().createDevicePath(
                slotPath, *infoRomConfig.pmUnitScopedName()),
            infoRomSysfsPath);
      }
      for (const auto& fpgaIpBlockConfig : *pciDeviceConfig.miscCtrlConfigs()) {
        pciExplorer_.createFpgaIpBlock(pciDevice, fpgaIpBlockConfig, instId++);
      }
    } catch (const std::exception& ex) {
      auto errMsg = fmt::format(
          "Failed to explore PCI device {} at {}. {}",
          *pciDeviceConfig.pmUnitScopedName(),
          slotPath,
          ex.what());
      XLOG(ERR) << errMsg;
      explorationErrMap_.add(
          slotPath, *pciDeviceConfig.pmUnitScopedName(), errMsg);
    }
  }
}

uint32_t PlatformExplorer::getFpgaInstanceId(
    const std::string& slotPath,
    const std::string& fpgaName) {
  auto key = std::make_pair(slotPath, fpgaName);
  auto it = fpgaInstanceIds_.find(key);
  if (it == fpgaInstanceIds_.end()) {
    fpgaInstanceIds_[key] = 1000 * (fpgaInstanceIds_.size() + 1);
  }
  return fpgaInstanceIds_[key];
}

void PlatformExplorer::createDeviceSymLink(
    const std::string& linkPath,
    const std::string& devicePath) {
  auto linkParentPath = std::filesystem::path(linkPath).parent_path();
  if (!platformFsUtils_->createDirectories(linkParentPath.string())) {
    XLOG(ERR) << fmt::format(
        "Failed to create the parent path ({})", linkParentPath.string());
    return;
  }

  std::string targetPath;
  try {
    if (linkParentPath.string() == "/run/devmap/eeproms") {
      targetPath = devicePathResolver_.resolveEepromPath(devicePath);
    } else if (linkParentPath.string() == "/run/devmap/sensors") {
      targetPath = devicePathResolver_.resolveSensorPath(devicePath);
    } else if (linkParentPath.string() == "/run/devmap/cplds") {
      if (auto i2cDevicePath =
              devicePathResolver_.tryResolveI2cDevicePath(devicePath)) {
        targetPath = *i2cDevicePath;
      } else {
        targetPath = devicePathResolver_.resolvePciDevicePath(devicePath);
      }
    } else if (linkParentPath.string() == "/run/devmap/fpgas") {
      targetPath = devicePathResolver_.resolvePciDevicePath(devicePath);
    } else if (linkParentPath.string() == "/run/devmap/i2c-busses") {
      targetPath = devicePathResolver_.resolveI2cBusPath(devicePath);
    } else if (
        linkParentPath.string() == "/run/devmap/gpiochips" ||
        linkParentPath.string() == "/run/devmap/flashes" ||
        linkParentPath.string() == "/run/devmap/watchdogs") {
      targetPath = devicePathResolver_.resolvePciSubDevCharDevPath(devicePath);
    } else if (linkParentPath.string() == "/run/devmap/xcvrs") {
      targetPath = devicePathResolver_.resolvePciSubDevSysfsPath(devicePath);
    } else {
      throw std::runtime_error(
          fmt::format("Symbolic link {} is not supported.", linkPath));
    }
  } catch (const std::exception& ex) {
    auto errMsg = fmt::format(
        "Failed to create a symlink {} for DevicePath {}. Reason: {}",
        linkPath,
        devicePath,
        ex.what());
    XLOG(ERR) << errMsg;
    explorationErrMap_.add(devicePath, errMsg);
    return;
  }
  XLOG(INFO) << fmt::format(
      "Creating symlink from {} to {}. DevicePath: {}",
      linkPath,
      targetPath,
      devicePath);
  auto cmd = fmt::format("ln -sfnv {} {}", targetPath, linkPath);
  auto [exitStatus, standardOut] = PlatformUtils().execCommand(cmd);
  if (exitStatus != 0) {
    XLOG(ERR) << fmt::format("Failed to run command ({})", cmd);
    return;
  }
}

ExplorationStatus PlatformExplorer::concludeExploration() {
  XLOG(INFO) << fmt::format(
      "Concluding {} exploration...", *platformConfig_.platformName());
  for (const auto& [devicePath, errorMessages] : explorationErrMap_) {
    auto [slotPath, deviceName] = Utils().parseDevicePath(devicePath);
    XLOG(INFO) << fmt::format(
        "{} Failures in Device {} for PmUnit {} at SlotPath {}",
        errorMessages.isExpected() ? "Expected" : "Unexpected",
        deviceName,
        dataStore_.hasPmUnit(slotPath)
            ? *dataStore_.getPmUnitInfo(slotPath).name()
            : "<ABSENT>",
        slotPath);
    int i = 1;
    for (const auto& errMsg : errorMessages.getMessages()) {
      XLOG(INFO) << fmt::format("{}. {}", i++, errMsg);
    }
  }
  fb303::fbData->setCounter(kTotalFailures, explorationErrMap_.size());
  if (explorationErrMap_.empty()) {
    return ExplorationStatus::SUCCEEDED;
  } else if (
      explorationErrMap_.size() == explorationErrMap_.numExpectedErrors()) {
    return ExplorationStatus::SUCCEEDED_WITH_EXPECTED_ERRORS;
  } else {
    return ExplorationStatus::FAILED;
  }
}

void PlatformExplorer::publishFirmwareVersions() {
  for (const auto& [linkPath, _] :
       *platformConfig_.symbolicLinkToDevicePath()) {
    auto deviceType = "";
    if (linkPath.starts_with("/run/devmap/cplds")) {
      deviceType = "cpld";
    } else if (linkPath.starts_with("/run/devmap/fpgas")) {
      deviceType = "fpga";
    } else {
      continue;
    }
    std::vector<folly::StringPiece> linkPathParts;
    folly::split('/', linkPath, linkPathParts, true);
    // Note: The vector is guaranteed to be non-empty due to the prefix check.
    CHECK(!linkPathParts.empty());
    const auto deviceName = linkPathParts.back();
    std::string verDirPath = linkPath;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(linkPath)) {  
        if (entry.is_regular_file() && entry.path().filename().string().find(fmt::format("{}_ver", deviceType)) != std::string::npos) {  
            verDirPath = entry.path().parent_path();  
        }  
    } 
    const auto version = readVersionNumber(
        fmt::format("{}/{}_ver", verDirPath, deviceType),
        platformFsUtils_.get());
    const auto subversion = readVersionNumber(
        fmt::format("{}/{}_sub_ver", verDirPath, deviceType),
        platformFsUtils_.get());

    std::string fullVersionString = fmt::format("{}.{}", version, subversion);
    int odsValue = version * 1000 + subversion;

    XLOGF(
        INFO,
        "Reporting firmware version for {} - version string:{} ODS value:{}",
        deviceName,
        fullVersionString,
        odsValue);
    fb303::fbData->setCounter(
        fmt::format(kFirmwareVersion, deviceName), odsValue);
    fb303::fbData->setCounter(
        fmt::format(kGroupedFirmwareVersion, deviceName, fullVersionString), 1);
  }
}

PlatformManagerStatus PlatformExplorer::getPMStatus() const {
  return platformManagerStatus_.copy();
}

PmUnitInfo PlatformExplorer::getPmUnitInfo(const std::string& slotPath) const {
  return dataStore_.getPmUnitInfo(slotPath);
}

void PlatformExplorer::updatePmStatus(const PlatformManagerStatus& newStatus) {
  platformManagerStatus_.withWLock(
      [&](PlatformManagerStatus& status) { status = newStatus; });
}

void PlatformExplorer::setupI2cDevice(
    const std::string& devicePath,
    uint16_t busNum,
    const I2cAddr& addr,
    const std::vector<I2cRegData>& initRegSettings) {
  try {
    i2cExplorer_.setupI2cDevice(busNum, addr, initRegSettings);
  } catch (const std::exception& ex) {
    XLOG(ERR) << ex.what();
    explorationErrMap_.add(devicePath, ex.what());
  }
}

void PlatformExplorer::createI2cDevice(
    const std::string& devicePath,
    const std::string& deviceName,
    uint16_t busNum,
    const I2cAddr& addr) {
  const auto& [slotPath, pmUnitScopedName] =
      Utils().parseDevicePath(devicePath);
  try {
    i2cExplorer_.createI2cDevice(pmUnitScopedName, deviceName, busNum, addr);
  } catch (const std::exception& ex) {
    XLOG(ERR) << ex.what();
    explorationErrMap_.add(devicePath, ex.what());
  }
}
} // namespace facebook::fboss::platform::platform_manager
