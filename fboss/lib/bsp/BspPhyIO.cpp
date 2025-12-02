// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/lib/bsp/BspPhyIO.h"

#include <folly/Format.h>
#include <folly/Range.h>
#include <folly/logging/xlog.h>
#include <stdint.h>
#include "fboss/lib/CommonFileUtils.h"
#include "fboss/lib/bsp/gen-cpp2/bsp_platform_mapping_types.h"
#include "fboss/mdio/BspDeviceMdio.h"

namespace facebook::fboss {

BspPhyIO::BspPhyIO(int pimID, BspPhyIOControllerInfo& controllerInfo)
    : pimID_(pimID) {
  auto mdioDevName = *controllerInfo.devicePath();
  auto controllerId = *controllerInfo.controllerId();
  XLOG(DBG5) << __func__ << ": mdioDevName=" << mdioDevName
             << " controllerId=" << controllerId;
  mdioController_ = std::make_unique<BspDeviceMdioController>(
      controllerId, pimID, controllerId, mdioDevName);
  mdioController_->init();
  controllerInfo_ = controllerInfo;
  XLOG(DBG5) << fmt::format(
      "BspPhyIOTrace: {} successfully opened mdio {:d}, {}",
      __func__,
      controllerId,
      mdioDevName);
}

phy::Cl45Data BspPhyIO::readRegister(
    phy::PhyAddress phyAddr,
    phy::Cl45DeviceAddress deviceAddr,
    phy::Cl45RegisterAddress reg) {
  XLOG(DBG5) << fmt::format(
      "{}: Starting read - phyAddr={:#x}, deviceAddr={:#x}, reg={:#x}",
      __func__,
      phyAddr,
      deviceAddr,
      reg);
  try {
    return mdioController_->readCl45(phyAddr, deviceAddr, reg);
  } catch (const std::exception& ex) {
    throw BspPhyIOError(
        fmt::format(
            "{}() failed for phyAddr={:#x}, deviceAddr={:#x}, reg={:#x}: {}",
            __func__,
            phyAddr,
            deviceAddr,
            reg,
            ex.what()));
  }
}

void BspPhyIO::writeRegister(
    phy::PhyAddress phyAddr,
    phy::Cl45DeviceAddress deviceAddr,
    phy::Cl45RegisterAddress reg,
    phy::Cl45Data data) {
  XLOG(DBG5) << fmt::format(
      "{}: Starting write - phyAddr={:#x}, deviceAddr={:#x}, reg={:#x}, data={:#x}",
      __func__,
      phyAddr,
      deviceAddr,
      reg,
      data);

  try {
    mdioController_->writeCl45(phyAddr, deviceAddr, reg, data);
  } catch (const std::exception& ex) {
    throw BspPhyIOError(
        fmt::format(
            "{}() failed for phyAddr={:#x}, deviceAddr={:#x}, reg={:#x}, data={:#x}: {}",
            __func__,
            phyAddr,
            deviceAddr,
            reg,
            data,
            ex.what()));
  }
}

} // namespace facebook::fboss
