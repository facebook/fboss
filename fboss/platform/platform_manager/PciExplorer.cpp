// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.
#include "fboss/platform/platform_manager/PciExplorer.h"

#include <sys/ioctl.h>
#include <filesystem>

#include <folly/FileUtil.h>
#include <folly/String.h>
#include <folly/logging/xlog.h>

#include "fboss/platform/platform_manager/I2cExplorer.h"

namespace fs = std::filesystem;
using namespace facebook::fboss::platform::platform_manager;

namespace {
bool isSamePciId(const std::string& id1, const std::string& id2) {
  return RE2::FullMatch(id1, PciExplorer().kPciIdRegex) &&
      RE2::FullMatch(id2, PciExplorer().kPciIdRegex) && id1 == id2;
}

bool hasEnding(std::string const& input, std::string const& ending) {
  if (input.length() >= ending.length()) {
    return input.compare(
               input.length() - ending.length(), ending.length(), ending) == 0;
  } else {
    return false;
  }
}

fbiob_aux_data getAuxData(
    const FpgaIpBlockConfig& fpgaIpBlockConfig,
    uint32_t instanceId) {
  struct fbiob_aux_data auxData {};
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
    const std::string& name,
    const std::string& vendorId,
    const std::string& deviceId,
    const std::string& subSystemVendorId,
    const std::string& subSystemDeviceId) {
  charDevPath_ = fmt::format(
      "/dev/fbiob_{}_{}",
      std::string(vendorId, 2, 4),
      std::string(deviceId, 2, 4));
  if (!fs::exists(charDevPath_)) {
    throw std::runtime_error(fmt::format(
        "No character device found at {} for {}", charDevPath_, name));
  }
  XLOG(INFO) << fmt::format(
      "Found character device {} for {}", charDevPath_, name);

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
    if (isSamePciId(folly::trimWhitespace(vendor).str(), vendorId) &&
        isSamePciId(folly::trimWhitespace(device).str(), deviceId) &&
        isSamePciId(
            folly::trimWhitespace(subSystemVendor).str(), subSystemVendorId) &&
        isSamePciId(
            folly::trimWhitespace(subSystemDevice).str(), subSystemDeviceId)) {
      sysfsPath_ = dirEntry.path().string();
      XLOG(INFO) << fmt::format(
          "Found sysfs path {} for device {}", sysfsPath_, name);
    }
  }
  if (sysfsPath_.empty()) {
    throw std::runtime_error(fmt::format(
        "No sysfs path found for {} with vendorId: {}, deviceId: {}, "
        "subSystemVendorId: {}, subSystemDeviceId: {}",
        name,
        vendorId,
        deviceId,
        subSystemVendorId,
        subSystemDeviceId));
  }
}

std::string PciDevice::sysfsPath() const {
  return sysfsPath_;
}

std::string PciDevice::charDevPath() const {
  return charDevPath_;
}

std::vector<uint16_t> PciExplorer::createI2cAdapter(
    const PciDevice& pciDevice,
    const I2cAdapterConfig& i2cAdapterConfig,
    uint32_t instanceId) {
  auto auxData = getAuxData(*i2cAdapterConfig.fpgaIpBlockConfig(), instanceId);
  auxData.i2c_data.num_channels = *i2cAdapterConfig.numberOfAdapters();
  create(
      *i2cAdapterConfig.fpgaIpBlockConfig()->deviceName(),
      pciDevice.charDevPath(),
      auxData);
  std::string expectedEnding = fmt::format(
      ".{}.{}",
      *i2cAdapterConfig.fpgaIpBlockConfig()->deviceName(),
      instanceId);
  for (const auto& dirEntry : fs::directory_iterator(pciDevice.sysfsPath())) {
    if (hasEnding(dirEntry.path().string(), expectedEnding)) {
      for (const auto& childDirEntry :
           fs::directory_iterator(dirEntry.path())) {
        if (re2::RE2::FullMatch(
                childDirEntry.path().filename().string(),
                I2cExplorer().kI2cBusNameRegex)) {
          return {I2cExplorer().extractBusNumFromPath(childDirEntry.path())};
        }
      }
    }
  }
  XLOG(ERR) << fmt::format(
      "Could not find any I2C buses under {} for {}",
      pciDevice.sysfsPath(),
      *i2cAdapterConfig.fpgaIpBlockConfig()->deviceName());
  return {};
}

void PciExplorer::createSpiMaster(
    const std::string& pciDevPath,
    const SpiMasterConfig& spiMasterConfig,
    uint32_t instanceId) {
  auto auxData = getAuxData(*spiMasterConfig.fpgaIpBlockConfig(), instanceId);
  auxData.spi_data.num_spidevs = *spiMasterConfig.numberOfCsPins();
  create(
      *spiMasterConfig.fpgaIpBlockConfig()->deviceName(), pciDevPath, auxData);
}

void PciExplorer::createLedCtrl(
    const std::string& pciDevPath,
    const LedCtrlConfig& ledCtrlConfig,
    uint32_t instanceId) {
  auto auxData = getAuxData(*ledCtrlConfig.fpgaIpBlockConfig(), instanceId);
  auxData.led_data.led_idx = *ledCtrlConfig.ledId();
  auxData.led_data.port_num = *ledCtrlConfig.portNumber();
  create(*ledCtrlConfig.fpgaIpBlockConfig()->deviceName(), pciDevPath, auxData);
}

void PciExplorer::createXcvrCtrl(
    const std::string& pciDevPath,
    const XcvrCtrlConfig& xcvrCtrlConfig,
    uint32_t instanceId) {
  auto auxData = getAuxData(*xcvrCtrlConfig.fpgaIpBlockConfig(), instanceId);
  auxData.xcvr_data.port_num = *xcvrCtrlConfig.portNumber();
  create(
      *xcvrCtrlConfig.fpgaIpBlockConfig()->deviceName(), pciDevPath, auxData);
}

void PciExplorer::createFpgaIpBlock(
    const std::string& pciDevPath,
    const FpgaIpBlockConfig& fpgaIpBlockConfig,
    uint32_t instanceId) {
  auto auxData = getAuxData(fpgaIpBlockConfig, instanceId);
  create(*fpgaIpBlockConfig.deviceName(), pciDevPath, auxData);
}

void PciExplorer::create(
    const std::string& devName,
    const std::string& pciDevPath,
    const struct fbiob_aux_data& auxData) {
  XLOG(INFO) << fmt::format(
      "Creating a new device using: {}, deviceName: {} instanceId: {}, "
      "csrOffset: {}, iobufOffset: {}",
      pciDevPath,
      devName,
      auxData.id.id,
      auxData.csr_offset,
      auxData.iobuf_offset);

  auto fd = open(pciDevPath.c_str(), O_RDWR);
  if (fd < 0) {
    XLOG(ERR) << fmt::format(
        "Failed to open {}: {}", pciDevPath, folly::errnoStr(errno));
    return;
  }
  auto ret = ioctl(fd, FBIOB_IOC_NEW_DEVICE, &auxData);
  if (ret < 0) {
    XLOG(ERR) << fmt::format(
        "Failed to create new device using: {}, instanceId: {}, "
        "csrOffset: {:#04x}, iobufOffset: {:#04x}, error: {} ",
        pciDevPath,
        auxData.id.id,
        auxData.csr_offset,
        auxData.iobuf_offset,
        folly::errnoStr(errno));
  }
  close(fd);
}

} // namespace facebook::fboss::platform::platform_manager
