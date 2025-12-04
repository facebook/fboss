// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.
#include "fboss/platform/platform_manager/PciExplorer.h"

#include <sys/ioctl.h>
#include <cstdint>
#include <filesystem>
#include <stdexcept>

#include <fb303/ServiceData.h>
#include <folly/FileUtil.h>
#include <folly/String.h>
#include <folly/logging/xlog.h>

#include "fboss/platform/helpers/PlatformFsUtils.h"
#include "fboss/platform/platform_manager/I2cExplorer.h"
#include "fboss/platform/platform_manager/Utils.h"

namespace fs = std::filesystem;
using namespace facebook::fboss::platform::platform_manager;

namespace {
const re2::RE2 kSpiBusRe{"spi\\d+"};
const re2::RE2 kSpiDevIdRe{"spi(?P<BusNum>\\d+).(?P<ChipSelect>\\d+)"};
constexpr auto kPciWaitSecs =
    std::chrono::seconds(10); // T235561085 - Change back to 5 when root cause
                              // of iob creation delay is fixed
constexpr auto kPciDeviceCreationTimeoutThreshold = std::chrono::seconds(1);
const std::string kPciDeviceCreationTimeoutCounter =
    "pci_explorer.pci_device_creation_timeout";

// Wrapper function that tracks device readiness timing and increments fb303
// counter when device creation takes longer than the threshold
bool checkDeviceReadinessWithTimeout(
    const std::string& pciSubDeviceName,
    std::function<bool()>&& isDeviceReadyFunc,
    const std::string& onWaitMsg,
    std::chrono::seconds maxWaitSecs) {
  auto start = std::chrono::steady_clock::now();

  bool result = Utils().checkDeviceReadiness(
      std::move(isDeviceReadyFunc), onWaitMsg, maxWaitSecs);

  auto elapsed = std::chrono::steady_clock::now() - start;
  if (elapsed > kPciDeviceCreationTimeoutThreshold) {
    XLOG(WARNING) << fmt::format(
        "Device {} creation took {}s, which exceeds threshold of {}s. Incrementing timeout counter.",
        pciSubDeviceName,
        std::chrono::duration_cast<std::chrono::seconds>(elapsed).count(),
        kPciDeviceCreationTimeoutThreshold.count());
    facebook::fb303::fbData->incrementCounter(
        kPciDeviceCreationTimeoutCounter, 1);
  }

  return result;
}

fbiob_aux_data getAuxData(
    const FpgaIpBlockConfig& fpgaIpBlockConfig,
    uint32_t instanceId) {
  struct fbiob_aux_data auxData{};
  strcpy(auxData.id.name, fpgaIpBlockConfig.deviceName()->c_str());
  auxData.id.id = instanceId;
  if (!fpgaIpBlockConfig.csrOffset()->empty()) {
    auxData.csr_offset = std::stoi(*fpgaIpBlockConfig.csrOffset(), nullptr, 16);
  }
  if (!fpgaIpBlockConfig.iobufOffset()->empty()) {
    auxData.iobuf_offset =
        std::stoi(*fpgaIpBlockConfig.iobufOffset(), nullptr, 16);
  }
  return auxData;
}

} // namespace

namespace facebook::fboss::platform::platform_manager {

PciDevice::PciDevice(
    const PciDeviceConfig& pciDeviceConfig,
    std::shared_ptr<PlatformFsUtils> platformFsUtils)
    : name_(*pciDeviceConfig.pmUnitScopedName()),
      vendorId_(*pciDeviceConfig.vendorId()),
      deviceId_(*pciDeviceConfig.deviceId()),
      subSystemVendorId_(*pciDeviceConfig.subSystemVendorId()),
      subSystemDeviceId_(*pciDeviceConfig.subSystemDeviceId()),
      platformFsUtils_(std::move(platformFsUtils)) {
  checkSysfsReadiness();

  // Note: bindDriver() needs to be called after checkSysfsReadiness() but
  // before checkCharDevReadiness().
  if (pciDeviceConfig.desiredDriver()) {
    bindDriver(*pciDeviceConfig.desiredDriver());
  }

  checkCharDevReadiness();
}

void PciDevice::checkSysfsReadiness() {
  for (const auto& dirEntry : fs::directory_iterator("/sys/bus/pci/devices")) {
    std::string vendor, device, subSystemVendor, subSystemDevice;
    auto deviceFilePath = dirEntry.path() / "device";
    auto vendorFilePath = dirEntry.path() / "vendor";
    auto subSystemVendorFilePath = dirEntry.path() / "subsystem_vendor";
    auto subSystemDeviceFilePath = dirEntry.path() / "subsystem_device";
    if (!folly::readFile(vendorFilePath.c_str(), vendor)) {
      XLOG(ERR) << "Failed to read vendor file from " << dirEntry.path();
    }
    if (!folly::readFile(deviceFilePath.c_str(), device)) {
      XLOG(ERR) << "Failed to read device file from " << dirEntry.path();
    }
    if (!folly::readFile(subSystemVendorFilePath.c_str(), subSystemVendor)) {
      XLOG(ERR) << "Failed to read subsystem_vendor file from "
                << dirEntry.path();
    }
    if (!folly::readFile(subSystemDeviceFilePath.c_str(), subSystemDevice)) {
      XLOG(ERR) << "Failed to read subsystem_device file from "
                << dirEntry.path();
    }
    if (folly::trimWhitespace(vendor).str() == vendorId_ &&
        folly::trimWhitespace(device).str() == deviceId_ &&
        folly::trimWhitespace(subSystemVendor).str() == subSystemVendorId_ &&
        folly::trimWhitespace(subSystemDevice).str() == subSystemDeviceId_) {
      sysfsPath_ = dirEntry.path().string();
      XLOG(INFO) << fmt::format(
          "Found sysfs path {} for device {}", sysfsPath_, name_);
      break;
    }
  }
  if (sysfsPath_.empty()) {
    throw std::runtime_error(
        fmt::format(
            "No sysfs path found for {} with vendorId: {}, deviceId: {}, "
            "subSystemVendorId: {}, subSystemDeviceId: {}",
            name_,
            vendorId_,
            deviceId_,
            subSystemVendorId_,
            subSystemDeviceId_));
  }
}

void PciDevice::bindDriver(const std::string& desiredDriver) {
  // Don't do anything if a driver is already attached to the device: it
  // usually happens when the device ID was already passed to "new_id"
  // in the previous platform_manager runs.
  // TODO:
  //   - what if the driver is different from "desiredDriver"? Shall we
  //     fail gracefully in this case? Or force detaching the driver and
  //     attach it to the desired one? Let's make decision when we hit
  //     the issue in the future.
  auto curDriver = fmt::format("{}/driver", sysfsPath_);
  if (fs::exists(curDriver)) {
    XLOG(INFO) << fmt::format(
        "Device {} already has a driver. Skipped manual driver binding", name_);
    return;
  }

  fs::path desiredDriverPath = fs::path("/sys/bus/pci/drivers") / desiredDriver;
  if (!fs::exists(desiredDriverPath)) {
    throw std::runtime_error(
        fmt::format(
            "Failed to bind driver {} to device {}: {} does not exist",
            desiredDriver,
            name_,
            desiredDriverPath.string()));
  }

  // Add PCI device ID to the driver's "new_id" file. Check below doc for
  // details:
  // https://www.kernel.org/doc/Documentation/ABI/testing/sysfs-bus-pci
  auto pciDevId = fmt::format(
      "{} {} {} {}",
      std::string(vendorId_, 2, 4),
      std::string(deviceId_, 2, 4),
      std::string(subSystemVendorId_, 2, 4),
      std::string(subSystemDeviceId_, 2, 4));
  XLOG(INFO) << fmt::format(
      "Pass {} device ID <{}> to {} driver's ID table",
      name_,
      pciDevId,
      desiredDriver);
  platformFsUtils_->writeStringToSysfs(pciDevId, desiredDriverPath / "new_id");
}

void PciDevice::checkCharDevReadiness() {
  charDevPath_ = fmt::format(
      "/dev/fbiob_{}.{}.{}.{}",
      std::string(vendorId_, 2, 4),
      std::string(deviceId_, 2, 4),
      std::string(subSystemVendorId_, 2, 4),
      std::string(subSystemDeviceId_, 2, 4));

  if (!checkDeviceReadinessWithTimeout(
          name_,
          [&charDevPath_ = charDevPath_]() -> bool {
            return fs::exists(charDevPath_);
          },
          fmt::format(
              "No character device found at {} for {}. Waiting for at most {}s",
              charDevPath_,
              name_,
              kPciWaitSecs.count()),
          kPciWaitSecs)) {
    throw std::runtime_error(
        fmt::format(
            "No character device found at {} for {}. This could either mean the "
            "FPGA does not show up as PCI device (see lspci output), or the kmods "
            "are not setting up the character device for the PCI device at {}.",
            charDevPath_,
            name_,
            charDevPath_));
  }
  XLOG(INFO) << fmt::format(
      "Found character device {} for {}", charDevPath_, name_);
}

std::string PciDevice::sysfsPath() const {
  return sysfsPath_;
}

std::string PciDevice::charDevPath() const {
  return charDevPath_;
}

std::string PciDevice::name() const {
  return name_;
}

PciExplorer::PciExplorer(std::shared_ptr<PlatformFsUtils> platformFsUtils)
    : platformFsUtils_(std::move(platformFsUtils)) {}

std::vector<uint16_t> PciExplorer::createI2cAdapter(
    const PciDevice& pciDevice,
    const I2cAdapterConfig& i2cAdapterConfig,
    uint32_t instanceId) {
  auto auxData = getAuxData(*i2cAdapterConfig.fpgaIpBlockConfig(), instanceId);
  auxData.i2c_data.num_channels = *i2cAdapterConfig.numberOfAdapters();
  create(pciDevice, *i2cAdapterConfig.fpgaIpBlockConfig(), auxData);
  return getI2cAdapterBusNums(pciDevice, i2cAdapterConfig, instanceId);
}

std::map<std::string, std::string> PciExplorer::createSpiMaster(
    const PciDevice& pciDevice,
    const SpiMasterConfig& spiMasterConfig,
    uint32_t instanceId) {
  auto auxData = getAuxData(*spiMasterConfig.fpgaIpBlockConfig(), instanceId);
  auxData.spi_data.num_spidevs = spiMasterConfig.spiDeviceConfigs()->size();
  int i = 0;
  for (const auto& spiDeviceConfig : *spiMasterConfig.spiDeviceConfigs()) {
    XLOG(INFO) << fmt::format(
        "Defining SpiDevice {} under SpiController {}. Args - modalias: {}, chip_select: {}, max_speed_hz: {}",
        *spiDeviceConfig.pmUnitScopedName(),
        *spiMasterConfig.fpgaIpBlockConfig()->pmUnitScopedName(),
        *spiDeviceConfig.modalias(),
        *spiDeviceConfig.chipSelect(),
        *spiDeviceConfig.maxSpeedHz());
    auto& spiDevInfo = auxData.spi_data.spidevs[i++];
    // To suppress buffer overflow security lint.
    CHECK_LE(spiDeviceConfig.modalias()->size(), NAME_MAX);
    strcpy(spiDevInfo.modalias, spiDeviceConfig.modalias()->c_str());
    spiDevInfo.chip_select = *spiDeviceConfig.chipSelect();
    spiDevInfo.max_speed_hz = *spiDeviceConfig.maxSpeedHz();
  }
  create(pciDevice, *spiMasterConfig.fpgaIpBlockConfig(), auxData);
  return getSpiDeviceCharDevPaths(pciDevice, spiMasterConfig, instanceId);
}

std::string PciExplorer::createGpioChip(
    const PciDevice& pciDevice,
    const FpgaIpBlockConfig& fpgaIpBlockConfig,
    uint32_t instanceId) {
  auto auxData = getAuxData(fpgaIpBlockConfig, instanceId);
  create(pciDevice, fpgaIpBlockConfig, auxData);
  return getGpioChipCharDevPath(pciDevice, fpgaIpBlockConfig, instanceId);
}

void PciExplorer::createLedCtrl(
    const PciDevice& pciDevice,
    const LedCtrlConfig& ledCtrlConfig,
    uint32_t instanceId) {
  auto auxData = getAuxData(*ledCtrlConfig.fpgaIpBlockConfig(), instanceId);
  auxData.led_data.led_idx = *ledCtrlConfig.ledId();
  auxData.led_data.port_num = *ledCtrlConfig.portNumber();
  create(pciDevice, *ledCtrlConfig.fpgaIpBlockConfig(), auxData);
}

std::string PciExplorer::createXcvrCtrl(
    const PciDevice& pciDevice,
    const XcvrCtrlConfig& xcvrCtrlConfig,
    uint32_t instanceId) {
  auto auxData = getAuxData(*xcvrCtrlConfig.fpgaIpBlockConfig(), instanceId);
  auxData.xcvr_data.port_num = *xcvrCtrlConfig.portNumber();
  create(pciDevice, *xcvrCtrlConfig.fpgaIpBlockConfig(), auxData);
  return getXcvrCtrlSysfsPath(
      pciDevice, *xcvrCtrlConfig.fpgaIpBlockConfig(), instanceId);
}

std::string PciExplorer::createInfoRom(
    const PciDevice& pciDevice,
    const FpgaIpBlockConfig& infoRomConfig,
    uint32_t instanceId) {
  auto auxData = getAuxData(infoRomConfig, instanceId);
  create(pciDevice, infoRomConfig, auxData);
  return getInfoRomSysfsPath(infoRomConfig, instanceId);
}

std::string PciExplorer::createWatchdog(
    const PciDevice& pciDevice,
    const FpgaIpBlockConfig& fpgaIpBlockConfig,
    uint32_t instanceId) {
  auto auxData = getAuxData(fpgaIpBlockConfig, instanceId);
  create(pciDevice, fpgaIpBlockConfig, auxData);
  return getWatchDogCharDevPath(pciDevice, fpgaIpBlockConfig, instanceId);
}

std::string PciExplorer::createFanPwmCtrl(
    const PciDevice& pciDevice,
    const FanPwmCtrlConfig& fanPwmCtrlConfig,
    uint32_t instanceId) {
  auto auxData = getAuxData(*fanPwmCtrlConfig.fpgaIpBlockConfig(), instanceId);
  auxData.fan_data.num_fans = *fanPwmCtrlConfig.numFans();
  create(pciDevice, *fanPwmCtrlConfig.fpgaIpBlockConfig(), auxData);
  return getFanPwmCtrlSysfsPath(
      pciDevice, *fanPwmCtrlConfig.fpgaIpBlockConfig(), instanceId);
}

std::string PciExplorer::createMdioBus(
    const PciDevice& pciDevice,
    const FpgaIpBlockConfig& mdioBusConfig,
    uint32_t instanceId) {
  auto auxData = getAuxData(mdioBusConfig, instanceId);
  create(pciDevice, mdioBusConfig, auxData);
  return getMdioBusSysfsPath(pciDevice, mdioBusConfig, instanceId);
}

void PciExplorer::createFpgaIpBlock(
    const PciDevice& pciDevice,
    const FpgaIpBlockConfig& fpgaIpBlockConfig,
    uint32_t instanceId) {
  auto auxData = getAuxData(fpgaIpBlockConfig, instanceId);
  create(pciDevice, fpgaIpBlockConfig, auxData);
}

void PciExplorer::create(
    const PciDevice& pciDevice,
    const FpgaIpBlockConfig& fpgaIpBlockConfig,
    const struct fbiob_aux_data& auxData) {
  XLOG(INFO) << fmt::format(
      "Creating device {} in {} using {}. Args - deviceName: {} instanceId: {}, "
      "csrOffset: {:#x}, iobufOffset: {:#x}",
      *fpgaIpBlockConfig.pmUnitScopedName(),
      pciDevice.sysfsPath(),
      pciDevice.charDevPath(),
      *fpgaIpBlockConfig.deviceName(),
      auxData.id.id,
      auxData.csr_offset,
      auxData.iobuf_offset);
  if (isPciSubDeviceReady(pciDevice, fpgaIpBlockConfig, auxData.id.id)) {
    XLOG(INFO) << fmt::format(
        "Skipped creating device {} already exists at {}. "
        "Args - deviceName: {} instanceId: {}, "
        "csrOffset: {:#x}, iobufOffset: {:#x}",
        *fpgaIpBlockConfig.pmUnitScopedName(),
        *getPciSubDeviceIOBlockPath(
            pciDevice, fpgaIpBlockConfig, auxData.id.id),
        *fpgaIpBlockConfig.deviceName(),
        auxData.id.id,
        auxData.csr_offset,
        auxData.iobuf_offset);
    return;
  }
  auto fd = open(pciDevice.charDevPath().c_str(), O_RDWR);
  if (fd < 0) {
    XLOG(ERR) << fmt::format(
        "Failed to open {}: {}",
        pciDevice.charDevPath(),
        folly::errnoStr(errno));
    return;
  }
  auto ret = ioctl(fd, FBIOB_IOC_NEW_DEVICE, &auxData);
  auto savedErrno = errno;
  close(fd);
  // PciSubDevice creation failure cases.
  // 1. ioctl() failures (ret < 0).
  // More details: https://man7.org/linux/man-pages/man2/ioctl.2.html#ERRORS
  // 2. PciSubDevice driver binding failure (checkDeviceReadiness =
  // false).
  if (ret < 0) {
    throw PciSubDeviceRuntimeError(
        fmt::format(
            "Failed to create new device {} in {} using {}. "
            "Args - deviceName: {} instanceId: {}, "
            "csrOffset: {:#04x}, iobufOffset: {:#04x}, error: {}",
            *fpgaIpBlockConfig.pmUnitScopedName(),
            pciDevice.sysfsPath(),
            pciDevice.charDevPath(),
            *fpgaIpBlockConfig.deviceName(),
            auxData.id.id,
            auxData.csr_offset,
            auxData.iobuf_offset,
            folly::errnoStr(savedErrno)),
        *fpgaIpBlockConfig.pmUnitScopedName());
  }
  if (!checkDeviceReadinessWithTimeout(
          *fpgaIpBlockConfig.pmUnitScopedName(),
          [&]() -> bool {
            return isPciSubDeviceReady(
                pciDevice, fpgaIpBlockConfig, auxData.id.id);
          },
          fmt::format(
              "PciSubDevice {} with deviceName {} and instId {} is not yet initialized "
              "at {}. Waiting for at most {}s",
              *fpgaIpBlockConfig.pmUnitScopedName(),
              *fpgaIpBlockConfig.deviceName(),
              auxData.id.id,
              pciDevice.sysfsPath(),
              kPciWaitSecs.count()),
          kPciWaitSecs)) {
    throw PciSubDeviceRuntimeError(
        fmt::format(
            "Failed to initialize device {} in {} using {}. "
            "Args - deviceName: {} instanceId: {}, "
            "csrOffset: {:#04x}, iobufOffset: {:#04x}",
            *fpgaIpBlockConfig.pmUnitScopedName(),
            pciDevice.sysfsPath(),
            pciDevice.charDevPath(),
            *fpgaIpBlockConfig.deviceName(),
            auxData.id.id,
            auxData.csr_offset,
            auxData.iobuf_offset),
        *fpgaIpBlockConfig.pmUnitScopedName());
  }

  XLOG(INFO) << fmt::format(
      "Successfully created device {} at {} using {}. Args - deviceName: {} instanceId: {}, "
      "csrOffset: {:#x}, iobufOffset: {:#x}",
      *fpgaIpBlockConfig.pmUnitScopedName(),
      *getPciSubDeviceIOBlockPath(pciDevice, fpgaIpBlockConfig, auxData.id.id),
      pciDevice.charDevPath(),
      *fpgaIpBlockConfig.deviceName(),
      auxData.id.id,
      auxData.csr_offset,
      auxData.iobuf_offset);
}

std::vector<uint16_t> PciExplorer::getI2cAdapterBusNums(
    const PciDevice& pciDevice,
    const I2cAdapterConfig& i2cAdapterConfig,
    uint32_t instanceId) {
  std::string expectedEnding = fmt::format(
      ".{}.{}",
      *i2cAdapterConfig.fpgaIpBlockConfig()->deviceName(),
      instanceId);
  fs::directory_entry fpgaI2cDir{};
  for (const auto& dirEntry : fs::directory_iterator(pciDevice.sysfsPath())) {
    if (dirEntry.path().string().ends_with(expectedEnding)) {
      fpgaI2cDir = dirEntry;
    }
  }
  if (fpgaI2cDir.path().empty()) {
    throw PciSubDeviceRuntimeError(
        fmt::format(
            "Could not find FPGA I2C directory ending with {}", expectedEnding),
        *i2cAdapterConfig.fpgaIpBlockConfig()->pmUnitScopedName());
  }
  if (*i2cAdapterConfig.numberOfAdapters() > 1) {
    // If more than 1 bus exists for this i2c master, then we have to use the
    // channel symlinks to find the appropriate kernel assigned bus numbers.
    std::vector<uint16_t> busNumbers;
    for (auto channelNum = 0; channelNum < *i2cAdapterConfig.numberOfAdapters();
         ++channelNum) {
      auto channelFile =
          fpgaI2cDir.path() / fmt::format("channel-{}", channelNum);
      if (!fs::exists(channelFile) || !fs::is_symlink(channelFile)) {
        throw PciSubDeviceRuntimeError(
            fmt::format(
                "{} does not exist or not a symlink.", channelFile.string()),
            *i2cAdapterConfig.fpgaIpBlockConfig()->pmUnitScopedName());
      }
      busNumbers.push_back(
          I2cExplorer().extractBusNumFromPath(
              fs::read_symlink(channelFile).filename()));
    }
    return busNumbers;
  } else {
    // If the config does not specify bus count for the i2cAdapterConfig, or
    // if it is specified as 1, we just look for the file named 'i2c-N'.
    for (const auto& childDirEntry :
         fs::directory_iterator(fpgaI2cDir.path())) {
      if (re2::RE2::FullMatch(
              childDirEntry.path().filename().string(),
              I2cExplorer().kI2cBusNameRegex)) {
        return {I2cExplorer().extractBusNumFromPath(childDirEntry.path())};
      }
    }
    throw PciSubDeviceRuntimeError(
        fmt::format(
            "Could not find any I2C buses in {}", fpgaI2cDir.path().string()),
        *i2cAdapterConfig.fpgaIpBlockConfig()->pmUnitScopedName());
  }
}

std::map<std::string, std::string> PciExplorer::getSpiDeviceCharDevPaths(
    const PciDevice& pciDevice,
    const SpiMasterConfig& spiMasterConfig,
    uint32_t instanceId) {
  // PciDevice.SysfsPath
  // |── fbiob_pci.expectedEnding    (*.{deviceName}.{instanceId})
  // |   └── fpgaIpBlockConfig.deviceName    (`spiMasterPath`)
  // |       ├── spi0    (`kSpiBusRe`)
  // │       |   ├── spi0.0   (`kSpiDevIdRe`)
  // │       |   ├── spi0.1   (`kSpiDevIdRe`)
  // │       |   ├── spi0.2   (`kSpiDevIdRe`)
  std::string expectedEnding = fmt::format(
      ".{}.{}", *spiMasterConfig.fpgaIpBlockConfig()->deviceName(), instanceId);
  std::string spiMasterPath;
  for (const auto& dirEntry : fs::directory_iterator(pciDevice.sysfsPath())) {
    if (dirEntry.path().string().ends_with(expectedEnding)) {
      spiMasterPath = fmt::format(
          "{}/{}",
          dirEntry.path().string(),
          *spiMasterConfig.fpgaIpBlockConfig()->deviceName());
    }
  }
  if (spiMasterPath.empty()) {
    throw PciSubDeviceRuntimeError(
        fmt::format(
            "Could not find any directory ending with {} in {}",
            expectedEnding,
            pciDevice.sysfsPath()),
        *spiMasterConfig.fpgaIpBlockConfig()->pmUnitScopedName());
  }
  if (!fs::exists(spiMasterPath)) {
    throw PciSubDeviceRuntimeError(
        fmt::format("SPI Master path not found at: {}", spiMasterPath),
        *spiMasterConfig.fpgaIpBlockConfig()->pmUnitScopedName());
  }
  std::map<std::string, std::string> spiCharDevPaths;
  for (const auto& dirEntry : fs::directory_iterator(spiMasterPath)) {
    if (!re2::RE2::FullMatch(dirEntry.path().filename().string(), kSpiBusRe)) {
      continue;
    }
    for (const auto& childDirEntry : fs::directory_iterator(dirEntry)) {
      auto spiDevId = childDirEntry.path().filename().string();
      int busNum, chipSelect;
      if (!re2::RE2::FullMatch(spiDevId, kSpiDevIdRe, &busNum, &chipSelect)) {
        continue;
      }
      // Find corresponding SpiDeviceConfig
      auto spiDeviceConfigItr = std::find_if(
          spiMasterConfig.spiDeviceConfigs()->begin(),
          spiMasterConfig.spiDeviceConfigs()->end(),
          [chipSelect](auto spiDeviceConfig) {
            return *spiDeviceConfig.chipSelect() == chipSelect;
          });
      if (spiDeviceConfigItr == spiMasterConfig.spiDeviceConfigs()->end()) {
        throw PciSubDeviceRuntimeError(
            fmt::format(
                "Unexpected SpiDevice created at {}. \
                No matching SpiDeviceConfig defined with ChipSelect {} for SpiDevice {}",
                childDirEntry.path().string(),
                chipSelect,
                *spiDeviceConfigItr->pmUnitScopedName()),
            *spiDeviceConfigItr->pmUnitScopedName());
      }
      auto spiCharDevPath = fmt::format("/dev/spidev{}.{}", busNum, chipSelect);
      if (!fs::exists(spiCharDevPath)) {
        // For more details on the two commands:
        // https://github.com/torvalds/linux/blob/master/Documentation/spi/spidev.rst#device-creation-driver-binding
        // Overriding driver of the SpiDevice so spidev doesn't fail to probe.
        auto overrideDriver = platformFsUtils_->writeStringToSysfs(
            "spidev",
            fs::path("/sys/bus/spi/devices") / spiDevId / "driver_override");
        if (!overrideDriver) {
          throw PciSubDeviceRuntimeError(
              fmt::format(
                  "Failed overridng SpiDriver spidev to /sys/bus/spi/devices/{}/driver_override "
                  "for SpiDevice {}",
                  spiDevId,
                  *spiDeviceConfigItr->pmUnitScopedName()),
              *spiDeviceConfigItr->pmUnitScopedName());
        }
        // Bind SpiDevice to spidev driver in order to create its char device.
        auto bindSpiDev = platformFsUtils_->writeStringToSysfs(
            spiDevId, "/sys/bus/spi/drivers/spidev/bind");
        if (!bindSpiDev) {
          throw PciSubDeviceRuntimeError(
              fmt::format(
                  "Failed binding SpiDevice {} to /sys/bus/spi/drivers/spidev/bind for SpiDevice {}",
                  spiDevId,
                  *spiDeviceConfigItr->pmUnitScopedName()),
              *spiDeviceConfigItr->pmUnitScopedName());
        }
        XLOG(INFO) << fmt::format(
            "Completed initializing SpiDevice {} as {} for SpiDevice {}",
            spiDevId,
            spiCharDevPath,
            *spiDeviceConfigItr->pmUnitScopedName());
      } else {
        XLOG(INFO) << fmt::format(
            "{} already exists. Skipping binding SpiDevice {} for SpiDevice {}",
            spiCharDevPath,
            spiDevId,
            *spiDeviceConfigItr->pmUnitScopedName());
      }
      spiCharDevPaths[*spiDeviceConfigItr->pmUnitScopedName()] = spiCharDevPath;
    }
  }
  auto expectedSpiDeviceCount = spiMasterConfig.spiDeviceConfigs()->size();
  if (spiCharDevPaths.size() != expectedSpiDeviceCount) {
    throw PciSubDeviceRuntimeError(
        fmt::format(
            "SPI device count mismatch for SPI master {} at {}. "
            "Expected {} SPI devices but found {}. "
            "Directories matching pattern '{}' may be missing or "
            "SpiDeviceConfigs may be misconfigured",
            *spiMasterConfig.fpgaIpBlockConfig()->pmUnitScopedName(),
            spiMasterPath,
            expectedSpiDeviceCount,
            spiCharDevPaths.size(),
            kSpiBusRe.pattern()),
        *spiMasterConfig.fpgaIpBlockConfig()->pmUnitScopedName());
  }
  return spiCharDevPaths;
}

std::string PciExplorer::getGpioChipCharDevPath(
    const PciDevice& pciDevice,
    const FpgaIpBlockConfig& fpgaIpBlockConfig,
    uint32_t instanceId) {
  std::string expectedEnding =
      fmt::format(".{}.{}", *fpgaIpBlockConfig.deviceName(), instanceId);
  std::string gpio;
  for (const auto& dirEntry : fs::directory_iterator(pciDevice.sysfsPath())) {
    if (dirEntry.path().string().ends_with(expectedEnding)) {
      return Utils().resolveGpioChipCharDevPath(dirEntry.path().string());
    }
  }
  throw PciSubDeviceRuntimeError(
      fmt::format(
          "Couldn't derive GpioChip {} CharDevPath in {}",
          *fpgaIpBlockConfig.pmUnitScopedName(),
          pciDevice.sysfsPath()),
      *fpgaIpBlockConfig.pmUnitScopedName());
}

std::string PciExplorer::getInfoRomSysfsPath(
    const FpgaIpBlockConfig& infoRomConfig,
    uint32_t instanceId) {
  const auto auxDevSysfsPath = "/sys/bus/auxiliary/devices";
  if (!fs::exists(auxDevSysfsPath)) {
    throw PciSubDeviceRuntimeError(
        fmt::format(
            "Unable to find InfoRom sysfs path for {}. Reason: '{}' path doesn't exist.",
            *infoRomConfig.pmUnitScopedName(),
            auxDevSysfsPath),
        *infoRomConfig.pmUnitScopedName());
  }
  std::string expectedEnding =
      fmt::format(".{}.{}", *infoRomConfig.deviceName(), instanceId);
  for (const auto& dirEntry : fs::directory_iterator(auxDevSysfsPath)) {
    if (dirEntry.path().filename().string().ends_with(expectedEnding)) {
      return dirEntry.path().string();
    }
  }
  throw PciSubDeviceRuntimeError(
      fmt::format(
          "Couldn't find InfoRom {} sysfs path under {}",
          *infoRomConfig.pmUnitScopedName(),
          auxDevSysfsPath),
      *infoRomConfig.pmUnitScopedName());
}

std::string PciExplorer::getWatchDogCharDevPath(
    const PciDevice& pciDevice,
    const FpgaIpBlockConfig& fpgaIpBlockConfig,
    uint32_t instanceId) {
  // PciDevice.SysfsPath
  // |── {}.expectedEnding
  // |   └── watchdog
  // |       ├── watchdog[n]
  // │       |   ├── device
  // │       |   ├── subsystem
  // │       |   ├── uevent
  // |       |   ├── ...
  std::string expectedEnding =
      fmt::format(".{}.{}", *fpgaIpBlockConfig.deviceName(), instanceId);
  for (const auto& dirEntry : fs::directory_iterator(pciDevice.sysfsPath())) {
    if (dirEntry.path().filename().string().ends_with(expectedEnding)) {
      return Utils().resolveWatchdogCharDevPath(dirEntry.path().string());
    }
  }
  throw PciSubDeviceRuntimeError(
      fmt::format(
          "Could not find any directory ending with {} in {}",
          expectedEnding,
          pciDevice.sysfsPath()),
      *fpgaIpBlockConfig.pmUnitScopedName());
}

std::string PciExplorer::getFanPwmCtrlSysfsPath(
    const PciDevice& pciDevice,
    const FpgaIpBlockConfig& fpgaIpBlockConfig,
    uint32_t instanceId) {
  std::string expectedEnding =
      fmt::format(".{}.{}", *fpgaIpBlockConfig.deviceName(), instanceId);
  for (const auto& dirEntry : fs::directory_iterator(pciDevice.sysfsPath())) {
    if (dirEntry.path().filename().string().ends_with(expectedEnding)) {
      return dirEntry.path();
    }
  }
  throw PciSubDeviceRuntimeError(
      fmt::format(
          "Could not find any directory ending with {} in {}",
          expectedEnding,
          pciDevice.sysfsPath()),
      *fpgaIpBlockConfig.pmUnitScopedName());
}

std::string PciExplorer::getXcvrCtrlSysfsPath(
    const PciDevice& pciDevice,
    const FpgaIpBlockConfig& fpgaIpBlockConfig,
    uint32_t instanceId) {
  auto expectedEnding =
      fmt::format(".{}.{}", *fpgaIpBlockConfig.deviceName(), instanceId);
  for (const auto& dirEntry : fs::directory_iterator(pciDevice.sysfsPath())) {
    if (dirEntry.path().string().ends_with(expectedEnding)) {
      return dirEntry.path().string();
    }
  }
  throw PciSubDeviceRuntimeError(
      fmt::format(
          "Couldn't find XcvrCtrl {} under {}",
          *fpgaIpBlockConfig.deviceName(),
          pciDevice.sysfsPath()),
      *fpgaIpBlockConfig.pmUnitScopedName());
}

std::string PciExplorer::getMdioBusSysfsPath(
    const PciDevice& pciDevice,
    const FpgaIpBlockConfig& fpgaIpBlockConfig,
    uint32_t instanceId) {
  std::string expectedEnding =
      fmt::format(".{}.{}", *fpgaIpBlockConfig.deviceName(), instanceId);
  for (const auto& dirEntry : fs::directory_iterator(pciDevice.sysfsPath())) {
    if (dirEntry.path().string().ends_with(expectedEnding)) {
      return Utils().resolveMdioBusCharDevPath(instanceId);
    }
  }
  throw PciSubDeviceRuntimeError(
      fmt::format(
          "Couldn't find MdioBus {} under {}",
          *fpgaIpBlockConfig.deviceName(),
          pciDevice.sysfsPath()),
      *fpgaIpBlockConfig.pmUnitScopedName());
}

bool PciExplorer::isPciSubDeviceReady(
    const PciDevice& pciDevice,
    const FpgaIpBlockConfig& fpgaIpBlockConfig,
    uint32_t instanceId) {
  auto devPath =
      getPciSubDeviceIOBlockPath(pciDevice, fpgaIpBlockConfig, instanceId);

  // Device being ready means: 1) device is created 2) driver is attached
  return devPath && isPciSubDeviceDriverReady(devPath.value());
}

bool PciExplorer::isPciSubDeviceDriverReady(const std::string& devPath) {
  auto driverPath = fs::path(devPath) / "driver";

  return fs::exists(driverPath) && fs::is_symlink(driverPath);
}

std::optional<std::string> PciExplorer::getPciSubDeviceIOBlockPath(
    const PciDevice& pciDevice,
    const FpgaIpBlockConfig& fpgaIpBlockConfig,
    uint32_t instanceId) {
  std::string expectedEnding =
      fmt::format(".{}.{}", *fpgaIpBlockConfig.deviceName(), instanceId);
  for (const auto& dirEntry : fs::directory_iterator(pciDevice.sysfsPath())) {
    if (dirEntry.path().filename().string().ends_with(expectedEnding)) {
      return dirEntry.path();
    }
  }
  return std::nullopt;
}
} // namespace facebook::fboss::platform::platform_manager
