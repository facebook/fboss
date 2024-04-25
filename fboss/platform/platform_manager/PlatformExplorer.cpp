// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/platform_manager/PlatformExplorer.h"
#include "fboss/platform/weutil/IoctlSmbusEepromReader.h"

#include <chrono>
#include <exception>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <utility>

#include <folly/FileUtil.h>
#include <folly/logging/xlog.h>

#include "fboss/platform/platform_manager/Utils.h"
#include "fboss/platform/platform_manager/gen-cpp2/platform_manager_config_constants.h"

namespace {
constexpr auto kRootSlotPath = "/";
const re2::RE2 kGpioChipNameRe{"gpiochip\\d+"};
const std::string kGpioChip = "gpiochip";

std::string getSlotPath(
    const std::string& parentSlotPath,
    const std::string& slotName) {
  if (parentSlotPath == kRootSlotPath) {
    return fmt::format("{}{}", kRootSlotPath, slotName);
  } else {
    return fmt::format("{}/{}", parentSlotPath, slotName);
  }
}

std::optional<std::string> getPresenceFileContent(const std::string& path) {
  std::string value{};
  if (!folly::readFile(path.c_str(), value)) {
    return std::nullopt;
  }
  return folly::trimWhitespace(value).str();
}

bool hasEnding(std::string const& input, std::string const& ending) {
  if (input.length() >= ending.length()) {
    return input.compare(
               input.length() - ending.length(), ending.length(), ending) == 0;
  } else {
    return false;
  }
}
} // namespace

namespace facebook::fboss::platform::platform_manager {

using constants = platform_manager_config_constants;

PlatformExplorer::PlatformExplorer(const PlatformConfig& config)
    : platformConfig_(config),
      devicePathResolver_(platformConfig_, dataStore_, i2cExplorer_) {}

void PlatformExplorer::explore() {
  XLOG(INFO) << "Exploring the platform";
  for (const auto& [busName, busNum] :
       i2cExplorer_.getBusNums(*platformConfig_.i2cAdaptersFromCpu())) {
    dataStore_.updateI2cBusNum(std::nullopt, busName, busNum);
  }
  const PmUnitConfig& rootPmUnitConfig =
      platformConfig_.pmUnitConfigs()->at(*platformConfig_.rootPmUnitName());
  auto pmUnitName = getPmUnitNameFromSlot(
      *rootPmUnitConfig.pluggedInSlotType(), kRootSlotPath);
  CHECK(pmUnitName == *platformConfig_.rootPmUnitName());
  explorePmUnit(kRootSlotPath, *platformConfig_.rootPmUnitName());
  XLOG(INFO) << "Creating symbolic links ...";
  for (const auto& [linkPath, devicePath] :
       *platformConfig_.symbolicLinkToDevicePath()) {
    createDeviceSymLink(linkPath, devicePath);
  }
  reportExplorationSummary();
}

void PlatformExplorer::explorePmUnit(
    const std::string& slotPath,
    const std::string& pmUnitName) {
  auto pmUnitConfig = platformConfig_.pmUnitConfigs()->at(pmUnitName);
  XLOG(INFO) << fmt::format("Exploring PmUnit {} at {}", pmUnitName, slotPath);

  dataStore_.updatePmUnitName(slotPath, pmUnitName);

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
    if (const auto sysfsFileHandle = presenceDetection->sysfsFileHandle()) {
      auto presencePath = devicePathResolver_.resolvePresencePath(
          *sysfsFileHandle->devicePath(), *sysfsFileHandle->presenceFileName());
      if (!presencePath) {
        XLOG(ERR) << fmt::format(
            "No sysfs file could be found at DevicePath: {} and presenceFileName: {}",
            *sysfsFileHandle->devicePath(),
            *sysfsFileHandle->presenceFileName());
        return;
      }
      XLOG(INFO) << fmt::format(
          "The file {} at DevicePath {} resolves to {}",
          *sysfsFileHandle->presenceFileName(),
          *sysfsFileHandle->devicePath(),
          *presencePath);
      auto presenceFileContent = getPresenceFileContent(*presencePath);
      if (!presenceFileContent) {
        XLOG(ERR) << fmt::format("Could not read file {}", *presencePath);
        return;
      }
      int16_t presenceValue{0};
      try {
        presenceValue = std::stoi(*presenceFileContent, nullptr, 0);
      } catch (const std::exception& ex) {
        XLOG(ERR) << fmt::format(
            "Failed to process file content {}: {}",
            *presenceFileContent,
            folly::exceptionStr(ex));
        return;
      }
      bool isPresent = (presenceValue == *sysfsFileHandle->desiredValue());
      XLOG(INFO) << fmt::format(
          "Value at {} is {}. desiredValue is {}. "
          "Assuming {} of PmUnit at {}",
          *presencePath,
          *presenceFileContent,
          *sysfsFileHandle->desiredValue(),
          isPresent ? "presence" : "absence",
          childSlotPath);
      if (!isPresent) {
        return;
      }
    } else {
      XLOG(INFO) << fmt::format(
          "Invalid PresenceDetection for {}", childSlotPath);
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
        errorMessages_[slotPath].push_back(errMsg);
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
    } catch (const std::exception& e) {
      auto errMsg = fmt::format(
          "Could not fetch contents of IDPROM {} in {}. {}",
          eepromPath,
          slotPath,
          e.what());
      XLOG(ERR) << errMsg;
      errorMessages_[slotPath].push_back(errMsg);
    }
    if (pmUnitNameInEeprom) {
      XLOG(INFO) << fmt::format(
          "Found PmUnit name `{}` in IDPROM {} at {}",
          *pmUnitNameInEeprom,
          eepromPath,
          slotPath);
    }
  }

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
    return *slotTypeConfig.pmUnitName();
  }
  return pmUnitNameInEeprom;
}

void PlatformExplorer::exploreI2cDevices(
    const std::string& slotPath,
    const std::vector<I2cDeviceConfig>& i2cDeviceConfigs) {
  for (const auto& i2cDeviceConfig : i2cDeviceConfigs) {
    auto busNum = dataStore_.getI2cBusNum(slotPath, *i2cDeviceConfig.busName());
    auto devAddr = I2cAddr(*i2cDeviceConfig.address());
    if (i2cDeviceConfig.initRegSettings()) {
      setupI2cDevice(
          slotPath, busNum, devAddr, *i2cDeviceConfig.initRegSettings());
    }
    createI2cDevice(
        Utils().createDevicePath(slotPath, *i2cDeviceConfig.pmUnitScopedName()),
        *i2cDeviceConfig.kernelDeviceName(),
        busNum,
        devAddr);
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
      std::optional<uint16_t> gpioNum{std::nullopt};
      for (const auto& childDirEntry :
           std::filesystem::directory_iterator(i2cDevicePath)) {
        if (re2::RE2::FullMatch(
                childDirEntry.path().filename().string(), kGpioChipNameRe)) {
          gpioNum = folly::to<uint16_t>(
              childDirEntry.path().filename().string().substr(
                  kGpioChip.length()));
        }
      }
      if (!gpioNum) {
        throw std::runtime_error(fmt::format(
            "No GPIO chip found in {} for {}",
            i2cDevicePath,
            *i2cDeviceConfig.pmUnitScopedName()));
      }
      dataStore_.updateGpioChipNum(
          slotPath, *i2cDeviceConfig.pmUnitScopedName(), *gpioNum);
    }
  }
}

void PlatformExplorer::explorePciDevices(
    const std::string& slotPath,
    const std::vector<PciDeviceConfig>& pciDeviceConfigs) {
  for (const auto& pciDeviceConfig : pciDeviceConfigs) {
    auto pciDevice = PciDevice(
        *pciDeviceConfig.pmUnitScopedName(),
        *pciDeviceConfig.vendorId(),
        *pciDeviceConfig.deviceId(),
        *pciDeviceConfig.subSystemVendorId(),
        *pciDeviceConfig.subSystemDeviceId());
    auto charDevPath = pciDevice.charDevPath();
    auto instId =
        getFpgaInstanceId(slotPath, *pciDeviceConfig.pmUnitScopedName());
    for (const auto& i2cAdapterConfig : *pciDeviceConfig.i2cAdapterConfigs()) {
      auto busNums =
          pciExplorer_.createI2cAdapter(pciDevice, i2cAdapterConfig, instId++);
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
      auto gpioNum =
          pciExplorer_.createGpioChip(pciDevice, fpgaIpBlockConfig, instId++);
      dataStore_.updateGpioChipNum(
          slotPath, *fpgaIpBlockConfig.pmUnitScopedName(), gpioNum);
    }
    for (const auto& fpgaIpBlockConfig : *pciDeviceConfig.watchdogConfigs()) {
      auto watchdogCharDevPath =
          pciExplorer_.createWatchdog(pciDevice, fpgaIpBlockConfig, instId++);
      dataStore_.updateCharDevPath(
          Utils().createDevicePath(
              slotPath, *fpgaIpBlockConfig.pmUnitScopedName()),
          watchdogCharDevPath);
    }
    for (const auto& fanPwmCtrlConfig : *pciDeviceConfig.fanTachoPwmConfigs()) {
      auto fanCtrlSysfsPath =
          pciExplorer_.createFanPwmCtrl(pciDevice, fanPwmCtrlConfig, instId++);
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
      dataStore_.updateSysfsPath(devicePath, pciDevice.sysfsPath());
      dataStore_.updateInstanceId(devicePath, instId);
      pciExplorer_.createXcvrCtrl(pciDevice, xcvrCtrlConfig, instId++);
    }
    for (const auto& infoRomConfig : *pciDeviceConfig.infoRomConfigs()) {
      auto infoRomSysfsPath =
          pciExplorer_.createInfoRom(pciDevice, infoRomConfig, instId++);
      dataStore_.updateSysfsPath(
          Utils().createDevicePath(slotPath, *infoRomConfig.pmUnitScopedName()),
          infoRomSysfsPath);
    }
    for (const auto& fpgaIpBlockConfig : *pciDeviceConfig.miscCtrlConfigs()) {
      pciExplorer_.createFpgaIpBlock(pciDevice, fpgaIpBlockConfig, instId++);
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
  if (!Utils().createDirectories(linkParentPath.string())) {
    XLOG(ERR) << fmt::format(
        "Failed to create the parent path ({})", linkParentPath.string());
    return;
  }

  const auto [slotPath, deviceName] = Utils().parseDevicePath(devicePath);
  if (!dataStore_.hasPmUnit(slotPath)) {
    XLOG(ERR) << fmt::format("No PmUnit exists at {}", slotPath);
    return;
  }

  std::optional<std::filesystem::path> targetPath = std::nullopt;
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
    } else if (linkParentPath.string() == "/run/devmap/gpiochips") {
      targetPath = std::filesystem::path(fmt::format(
          "/dev/gpiochip{}", dataStore_.getGpioChipNum(slotPath, deviceName)));
    } else if (linkParentPath.string() == "/run/devmap/xcvrs") {
      auto pciDevPath = dataStore_.getSysfsPath(devicePath);
      auto expectedEnding =
          fmt::format(".xcvr_ctrl.{}", dataStore_.getInstanceId(devicePath));
      for (const auto& dirEntry :
           std::filesystem::directory_iterator(pciDevPath)) {
        if (hasEnding(dirEntry.path().string(), expectedEnding)) {
          targetPath = dirEntry.path().string();
        }
      }
      if (!targetPath) {
        XLOG(ERR) << fmt::format(
            "Couldn't find xcvr_ctrl directory under {}. DevicePath: {}",
            pciDevPath,
            devicePath);
        return;
      }
    } else if (linkParentPath.string() == "/run/devmap/flashes") {
      targetPath = dataStore_.getCharDevPath(devicePath);
    } else if (linkParentPath.string() == "/run/devmap/watchdogs") {
      targetPath = dataStore_.getCharDevPath(devicePath);
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
    errorMessages_[slotPath].push_back(errMsg);
    return;
  }

  XLOG(INFO) << fmt::format(
      "Creating symlink from {} to {}. DevicePath: {}",
      linkPath,
      targetPath->string(),
      devicePath);
  auto cmd = fmt::format("ln -sfnv {} {}", targetPath->string(), linkPath);
  auto [exitStatus, standardOut] = PlatformUtils().execCommand(cmd);
  if (exitStatus != 0) {
    XLOG(ERR) << fmt::format("Failed to run command ({})", cmd);
    return;
  }
}

void PlatformExplorer::reportExplorationSummary() {
  if (errorMessages_.empty()) {
    XLOG(INFO) << "SUCCESS. Completed setting up all the devices.";
    return;
  }
  XLOG(INFO) << "Completed setting up devices with errors";
  for (const auto& [slotPath, errMsgs] : errorMessages_) {
    XLOG(INFO) << fmt::format(
        "Failures in PmUnit {} at {}",
        dataStore_.getPmUnitName(slotPath),
        slotPath);
    int i = 1;
    for (const auto& errMsg : errMsgs) {
      XLOG(INFO) << fmt::format("{}. {}", i++, errMsg);
    }
  }
}

void PlatformExplorer::setupI2cDevice(
    const std::string& slotPath,
    uint16_t busNum,
    const I2cAddr& addr,
    const std::vector<I2cRegData>& initRegSettings) {
  try {
    i2cExplorer_.setupI2cDevice(busNum, addr, initRegSettings);
  } catch (const std::exception& ex) {
    XLOG(ERR) << ex.what();
    errorMessages_[slotPath].push_back(ex.what());
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
    errorMessages_[slotPath].push_back(ex.what());
  }
}

} // namespace facebook::fboss::platform::platform_manager
