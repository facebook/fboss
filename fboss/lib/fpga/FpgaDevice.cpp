// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/lib/fpga/FpgaDevice.h"

namespace facebook::fboss {

FpgaDevice::FpgaDevice(PciVendorId vendorId, PciDeviceId deviceId) {
  pciDevice_ = std::make_unique<PciDevice>(vendorId, deviceId);
  pciDevice_->open();

  uint32_t fpgaBar0 = pciDevice_->getMemoryRegionAddress(0);
  uint32_t fpgaBar0Size = pciDevice_->getMemoryRegionSize(0);

  phyMem_ = std::make_unique<PhyMem>(fpgaBar0, fpgaBar0Size, false);

  XLOG(DBG1) << folly::format(
      "Created FPGA Device at address={:#x} size={:d}", fpgaBar0, fpgaBar0Size);
}

} // namespace facebook::fboss
