// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.
#include "fboss/platform/platform_manager/PciExplorer.h"

#include <sys/ioctl.h>

#include <folly/FileUtil.h>
#include <folly/String.h>
#include <folly/logging/xlog.h>

#include "fboss/platform/platform_manager/uapi/fbiob-ioctl.h"

using namespace facebook::fboss::platform::platform_manager;

namespace facebook::fboss::platform::platform_manager {

std::optional<std::string> PciExplorer::getDevicePath(
    const std::string& vendorId,
    const std::string& deviceId,
    const std::string& subSystemVendorId,
    const std::string& subSystemDeviceId) {
  return fmt::format(
      "/dev/fbiob_{}_{}",
      std::string(vendorId, 2, 4),
      std::string(deviceId, 2, 4));
}

std::vector<uint16_t> PciExplorer::createI2cAdapter(
    const std::string& pciDevPath,
    const I2cAdapterConfig& i2cAdapterConfig,
    uint32_t instanceId) {
  create(pciDevPath, *i2cAdapterConfig.fpgaIpBlockConfig(), instanceId);
  return {};
}

void PciExplorer::createSpiMaster(
    const std::string& pciDevPath,
    const SpiMasterConfig& spiMasterConfig,
    uint32_t instanceId) {
  create(pciDevPath, *spiMasterConfig.fpgaIpBlockConfig(), instanceId);
}

void PciExplorer::createLedCtrl(
    const std::string& pciDevPath,
    const LedCtrlConfig& ledCtrlConfig,
    uint32_t instanceId) {
  create(pciDevPath, *ledCtrlConfig.fpgaIpBlockConfig(), instanceId);
}

void PciExplorer::createXcvrCtrl(
    const std::string& pciDevPath,
    const XcvrCtrlConfig& xcvrCtrlConfig,
    uint32_t instanceId) {
  create(pciDevPath, *xcvrCtrlConfig.fpgaIpBlockConfig(), instanceId);
}

void PciExplorer::create(
    const std::string& pciDevPath,
    const FpgaIpBlockConfig& fpgaIpBlockConfig,
    uint32_t instanceId) {
  XLOG(INFO) << fmt::format(
      "Creating a new device using: {}, deviceName: {} instanceId: {}, "
      "csrOffset: {}, iobufOffset: {}",
      pciDevPath,
      *fpgaIpBlockConfig.deviceName(),
      instanceId,
      *fpgaIpBlockConfig.csrOffset(),
      *fpgaIpBlockConfig.iobufOffset());
  struct fbiob_aux_data aux_data {};
  strcpy(aux_data.id.name, fpgaIpBlockConfig.deviceName()->c_str());
  aux_data.id.id = instanceId;
  if (!fpgaIpBlockConfig.csrOffset()->empty()) {
    aux_data.csr_offset =
        std::stoi(*fpgaIpBlockConfig.csrOffset(), nullptr, 16);
  }
  if (!fpgaIpBlockConfig.iobufOffset()->empty()) {
    aux_data.iobuf_offset =
        std::stoi(*fpgaIpBlockConfig.iobufOffset(), nullptr, 16);
  }

  auto fd = open(pciDevPath.c_str(), O_RDWR);
  if (fd < 0) {
    XLOG(ERR) << fmt::format(
        "Failed to open {}: {}", pciDevPath, folly::errnoStr(errno));
    return;
  }
  auto ret = ioctl(fd, FBIOB_IOC_NEW_DEVICE, &aux_data);
  if (ret < 0) {
    XLOG(ERR) << fmt::format(
        "Failed to create new device using: {}, instanceId: {}, "
        "csrOffset: {:#04x}, iobufOffset: {:#04x}, error: {} ",
        pciDevPath,
        aux_data.id.id,
        aux_data.csr_offset,
        aux_data.iobuf_offset,
        folly::errnoStr(errno));
  }
  close(fd);
}

} // namespace facebook::fboss::platform::platform_manager
