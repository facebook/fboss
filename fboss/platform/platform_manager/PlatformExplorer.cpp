// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/platform_manager/PlatformExplorer.h"

#include <chrono>
#include <exception>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <utility>

#include <folly/logging/xlog.h>

#include "fboss/platform/platform_manager/gen-cpp2/platform_manager_config_constants.h"

namespace {
constexpr auto kRootSlotPath = "/";
constexpr auto kIdprom = "IDPROM";
const re2::RE2 kValidHwmonDirName{"hwmon[0-9]+"};

std::string getSlotPath(
    const std::string& parentSlotPath,
    const std::string& slotName) {
  if (parentSlotPath == kRootSlotPath) {
    return fmt::format("{}{}", kRootSlotPath, slotName);
  } else {
    return fmt::format("{}/{}", parentSlotPath, slotName);
  }
}
} // namespace

namespace facebook::fboss::platform::platform_manager {

using constants = platform_manager_config_constants;

PlatformExplorer::PlatformExplorer(
    std::chrono::seconds exploreInterval,
    const PlatformConfig& config)
    : platformConfig_(config) {
  scheduler_.addFunction(
      [this, exploreInterval]() {
        try {
          explore();
        } catch (const std::exception& ex) {
          XLOG(ERR) << fmt::format(
              "Exception while exploring platform: {}. Will retry after {} seconds.",
              folly::exceptionStr(ex),
              exploreInterval.count());
        }
      },
      exploreInterval);
  scheduler_.start();
}

void PlatformExplorer::explore() {
  XLOG(INFO) << "Exploring the platform";
  for (const auto& [busName, busNum] :
       i2cExplorer_.getBusNums(*platformConfig_.i2cAdaptersFromCpu())) {
    updateI2cBusNum(std::nullopt, busName, busNum);
  }
  const PmUnitConfig& rootPmUnitConfig =
      platformConfig_.pmUnitConfigs()->at(*platformConfig_.rootPmUnitName());
  auto pmUnitName = getPmUnitNameFromSlot(
      *rootPmUnitConfig.pluggedInSlotType(), kRootSlotPath);
  CHECK(pmUnitName == *platformConfig_.rootPmUnitName());
  explorePmUnit(kRootSlotPath, *platformConfig_.rootPmUnitName());
  for (const auto& [linkPath, pmDevicePath] :
       *platformConfig_.symbolicLinkToDevicePath()) {
    createDeviceSymLink(linkPath, pmDevicePath);
  }
}

void PlatformExplorer::explorePmUnit(
    const std::string& slotPath,
    const std::string& pmUnitName) {
  auto pmUnitConfig = platformConfig_.pmUnitConfigs()->at(pmUnitName);
  XLOG(INFO) << fmt::format("Exploring PmUnit {} at {}", pmUnitName, slotPath);

  slotPathToPmUnitName_[slotPath] = pmUnitName;

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

  if (slotConfig.presenceDetection() &&
      !presenceDetector_.isPresent(*slotConfig.presenceDetection())) {
    XLOG(INFO) << fmt::format(
        "No device could be detected in SlotPath {}", childSlotPath);
  }

  int i = 0;
  for (const auto& busName : *slotConfig.outgoingI2cBusNames()) {
    auto busNum = getI2cBusNum(parentSlotPath, busName);
    updateI2cBusNum(childSlotPath, fmt::format("INCOMING@{}", i++), busNum);
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
    auto eepromI2cBusNum = getI2cBusNum(slotPath, *idpromConfig.busName());
    i2cExplorer_.createI2cDevice(
        "IDPROM",
        *idpromConfig.kernelDeviceName(),
        eepromI2cBusNum,
        I2cAddr(*idpromConfig.address()));
    auto eepromPath = i2cExplorer_.getDeviceI2cPath(
        eepromI2cBusNum, I2cAddr(*idpromConfig.address()));
    try {
      // TODO: One eeprom parsing library is implemented, get the
      // Product Name from eeprom contents of eepromPath and use it here.
      pmUnitNameInEeprom = std::nullopt;
    } catch (const std::exception& e) {
      XLOG(ERR) << fmt::format(
          "Could not fetch contents of IDPROM {}. {}", eepromPath, e.what());
    }
  }
  if (slotTypeConfig.pmUnitName()) {
    if (pmUnitNameInEeprom &&
        *pmUnitNameInEeprom != *slotTypeConfig.pmUnitName()) {
      XLOG(WARNING) << fmt::format(
          "The PmUnit name in eeprom `{}` is different from the one in config `{}`",
          *pmUnitNameInEeprom,
          *slotTypeConfig.pmUnitName());
    }
    return *slotTypeConfig.pmUnitName();
  }
  return pmUnitNameInEeprom;
}

void PlatformExplorer::exploreI2cDevices(
    const std::string& slotPath,
    const std::vector<I2cDeviceConfig>& i2cDeviceConfigs) {
  for (const auto& i2cDeviceConfig : i2cDeviceConfigs) {
    i2cExplorer_.createI2cDevice(
        *i2cDeviceConfig.pmUnitScopedName(),
        *i2cDeviceConfig.kernelDeviceName(),
        getI2cBusNum(slotPath, *i2cDeviceConfig.busName()),
        I2cAddr(*i2cDeviceConfig.address()));
    if (i2cDeviceConfig.numOutgoingChannels()) {
      auto channelToBusNums = i2cExplorer_.getMuxChannelI2CBuses(
          getI2cBusNum(slotPath, *i2cDeviceConfig.busName()),
          I2cAddr(*i2cDeviceConfig.address()));
      assert(channelToBusNums.size() == i2cDeviceConfig.numOutgoingChannels());
      for (const auto& [channelNum, busNum] : channelToBusNums) {
        updateI2cBusNum(
            slotPath,
            fmt::format(
                "{}@{}", *i2cDeviceConfig.pmUnitScopedName(), channelNum),
            busNum);
      }
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
          updateI2cBusNum(
              slotPath,
              fmt::format(
                  "{}@{}",
                  *i2cAdapterConfig.fpgaIpBlockConfig()->pmUnitScopedName(),
                  i),
              busNums[i]);
        }
      } else {
        CHECK_EQ(busNums.size(), 1);
        updateI2cBusNum(
            slotPath,
            *i2cAdapterConfig.fpgaIpBlockConfig()->pmUnitScopedName(),
            busNums[0]);
      }
    }
    for (const auto& spiMasterConfig : *pciDeviceConfig.spiMasterConfigs()) {
      pciExplorer_.createSpiMaster(charDevPath, spiMasterConfig, instId++);
    }
    for (const auto& fpgaIpBlockConfig : *pciDeviceConfig.gpioChipConfigs()) {
      pciExplorer_.createFpgaIpBlock(charDevPath, fpgaIpBlockConfig, instId++);
    }
    for (const auto& fpgaIpBlockConfig : *pciDeviceConfig.watchdogConfigs()) {
      pciExplorer_.createFpgaIpBlock(charDevPath, fpgaIpBlockConfig, instId++);
    }
    for (const auto& fpgaIpBlockConfig :
         *pciDeviceConfig.fanTachoPwmConfigs()) {
      pciExplorer_.createFpgaIpBlock(charDevPath, fpgaIpBlockConfig, instId++);
    }
    for (const auto& fpgaIpBlockConfig : *pciDeviceConfig.ledCtrlConfigs()) {
      pciExplorer_.createLedCtrl(charDevPath, fpgaIpBlockConfig, instId++);
    }
    for (const auto& xcvrCtrlConfig : *pciDeviceConfig.xcvrCtrlConfigs()) {
      pciExplorer_.createXcvrCtrl(charDevPath, xcvrCtrlConfig, instId++);
    }
  }
}

uint16_t PlatformExplorer::getI2cBusNum(
    const std::optional<std::string>& slotPath,
    const std::string& pmUnitScopeBusName) const {
  auto it = i2cBusNums_.find(std::make_pair(std::nullopt, pmUnitScopeBusName));
  if (it != i2cBusNums_.end()) {
    return it->second;
  } else {
    return i2cBusNums_.at(std::make_pair(slotPath, pmUnitScopeBusName));
  }
}

void PlatformExplorer::updateI2cBusNum(
    const std::optional<std::string>& slotPath,
    const std::string& pmUnitScopeBusName,
    uint16_t busNum) {
  XLOG(INFO) << fmt::format(
      "Updating bus {} in {} to bus number {} (i2c-{})",
      pmUnitScopeBusName,
      slotPath ? fmt::format("SlotPath {}", *slotPath) : "Global Scope",
      busNum,
      busNum);
  i2cBusNums_[std::make_pair(slotPath, pmUnitScopeBusName)] = busNum;
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
    const std::string& pmDevicePath) {
  std::error_code errCode;
  auto linkParentPath = std::filesystem::path(linkPath).parent_path();
  std::filesystem::create_directories(linkParentPath, errCode);
  if (errCode.value() != 0) {
    XLOG(ERR) << fmt::format(
        "Failed to create the parent path ({}) with error code {}",
        linkParentPath.string(),
        errCode.value());
    return;
  }

  re2::RE2 re("(?P<SlotPath>.*)\\[(?P<DeviceName>.*)\\]", re2::RE2::Options{});
  auto nsubmatch = re.NumberOfCapturingGroups() + 1;
  std::vector<re2::StringPiece> submatches(nsubmatch);
  re.Match(
      pmDevicePath,
      0,
      pmDevicePath.length(),
      re2::RE2::UNANCHORED,
      submatches.data(),
      nsubmatch);
  std::string slotPath = submatches[1].as_string();
  std::string deviceName = submatches[2].as_string();
  // Remove trailling '/' (e.g /abc/dfg/)
  if (slotPath.length() > 1) {
    slotPath.pop_back();
  }

  if (slotPathToPmUnitName_.find(slotPath) == std::end(slotPathToPmUnitName_)) {
    XLOG(ERR) << fmt::format(
        "({}) doesn't have associated pm unit name", slotPath);
    return;
  }
  auto pmUnitName = slotPathToPmUnitName_[slotPath];
  auto pmUnitConfig = platformConfig_.pmUnitConfigs()->at(pmUnitName);

  auto idpromConfig = platformConfig_.slotTypeConfigs()
                          ->at(*pmUnitConfig.pluggedInSlotType())
                          .idpromConfig();
  auto i2cDeviceConfig = std::find_if(
      pmUnitConfig.i2cDeviceConfigs()->begin(),
      pmUnitConfig.i2cDeviceConfigs()->end(),
      [&](auto i2cDeviceConfig) {
        return *i2cDeviceConfig.pmUnitScopedName() == deviceName;
      });
  auto pciDeviceConfig = std::find_if(
      pmUnitConfig.pciDeviceConfigs()->begin(),
      pmUnitConfig.pciDeviceConfigs()->end(),
      [&](auto pciDeviceConfig) {
        return *pciDeviceConfig.pmUnitScopedName() == deviceName;
      });

  std::optional<std::filesystem::path> targetPath = std::nullopt;
  if (linkParentPath.string() == "/run/devmap/eeproms") {
    if (deviceName == kIdprom) {
      CHECK(idpromConfig);
      targetPath = std::filesystem::path(i2cExplorer_.getDeviceI2cPath(
          getI2cBusNum(slotPath, *idpromConfig->busName()),
          I2cAddr(*idpromConfig->address())));
    } else {
      if (i2cDeviceConfig == pmUnitConfig.i2cDeviceConfigs()->end()) {
        XLOG(ERR) << fmt::format(
            "Couldn't find i2c device config for ({})", deviceName);
      }
      auto busNum = getI2cBusNum(slotPath, *i2cDeviceConfig->busName());
      auto i2cAddr = I2cAddr(*i2cDeviceConfig->address());
      if (!i2cExplorer_.isI2cDevicePresent(busNum, i2cAddr)) {
        XLOG(ERR) << fmt::format(
            "{} is not plugged-in to the platform", deviceName);
        return;
      }
      targetPath = std::filesystem::path(
                       i2cExplorer_.getDeviceI2cPath(busNum, i2cAddr)) /
          "eeprom";
    }
  } else if (linkParentPath.string() == "/run/devmap/sensors") {
    if (i2cDeviceConfig == pmUnitConfig.i2cDeviceConfigs()->end()) {
      XLOG(ERR) << fmt::format(
          "Couldn't find i2c device config for ({})", deviceName);
    }
    auto busNum = getI2cBusNum(slotPath, *i2cDeviceConfig->busName());
    auto i2cAddr = I2cAddr(*i2cDeviceConfig->address());
    if (!i2cExplorer_.isI2cDevicePresent(busNum, i2cAddr)) {
      XLOG(ERR) << fmt::format(
          "{} is not plugged-in to the platform", deviceName);
      return;
    }
    targetPath =
        std::filesystem::path(i2cExplorer_.getDeviceI2cPath(busNum, i2cAddr)) /
        "hwmon";
    std::string hwmonSubDir = "";
    for (const auto& dirEntry :
         std::filesystem::directory_iterator(*targetPath)) {
      auto dirName = dirEntry.path().filename();
      if (re2::RE2::FullMatch(dirName.string(), kValidHwmonDirName)) {
        hwmonSubDir = dirName.string();
        break;
      }
    }
    if (hwmonSubDir.empty()) {
      XLOG(ERR) << fmt::format(
          "Couldn't find hwmon[num] folder within ({})", targetPath->string());
      return;
    }
    targetPath = *targetPath / hwmonSubDir;
  } else if (linkParentPath.string() == "/run/devmap/cplds") {
    if (i2cDeviceConfig != pmUnitConfig.i2cDeviceConfigs()->end()) {
      auto busNum = getI2cBusNum(slotPath, *i2cDeviceConfig->busName());
      auto i2cAddr = I2cAddr(*i2cDeviceConfig->address());
      if (!i2cExplorer_.isI2cDevicePresent(busNum, i2cAddr)) {
        XLOG(ERR) << fmt::format(
            "{} is not plugged-in to the platform", deviceName);
        return;
      }
      targetPath =
          std::filesystem::path(i2cExplorer_.getDeviceI2cPath(busNum, i2cAddr));
    } else if (pciDeviceConfig != pmUnitConfig.pciDeviceConfigs()->end()) {
      auto pciDevice = PciDevice(
          *pciDeviceConfig->pmUnitScopedName(),
          *pciDeviceConfig->vendorId(),
          *pciDeviceConfig->deviceId(),
          *pciDeviceConfig->subSystemVendorId(),
          *pciDeviceConfig->subSystemDeviceId());
      targetPath = std::filesystem::path(pciDevice.sysfsPath());
    } else {
      XLOG(ERR) << fmt::format(
          "Couldn't resolve target path for ({})", deviceName);
      return;
    }
  } else if (linkParentPath.string() == "/run/devmap/fpgas") {
    if (pciDeviceConfig == pmUnitConfig.pciDeviceConfigs()->end()) {
      XLOG(ERR) << fmt::format(
          "Couldn't find PCI device config for ({})", deviceName);
      return;
    }
    auto pciDevice = PciDevice(
        *pciDeviceConfig->pmUnitScopedName(),
        *pciDeviceConfig->vendorId(),
        *pciDeviceConfig->deviceId(),
        *pciDeviceConfig->subSystemVendorId(),
        *pciDeviceConfig->subSystemDeviceId());
    targetPath = std::filesystem::path(pciDevice.sysfsPath());
  } else if (linkParentPath.string() == "/run/devmap/i2c-busses") {
    targetPath = std::filesystem::path(
        fmt::format("/dev/i2c-{}", getI2cBusNum(slotPath, deviceName)));
  }

  XLOG(INFO) << fmt::format(
      "Creating symlink from {} to {}", linkPath, targetPath->string());
  auto cmd = fmt::format("ln -sfnv {} {}", targetPath->string(), linkPath);
  auto [exitStatus, standardOut] = PlatformUtils().execCommand(cmd);
  if (exitStatus != 0) {
    XLOG(ERR) << fmt::format("Failed to run command ({})", cmd);
    return;
  }
  XLOG(INFO) << fmt::format(
      "{} resolves to {}", linkPath, targetPath->string());
}

} // namespace facebook::fboss::platform::platform_manager
