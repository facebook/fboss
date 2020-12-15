// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/lib/fpga/MinipackBasePimContainer.h"

namespace {
constexpr uint32_t kQsfpManagementRegStart = 0x40;
constexpr uint32_t kQsfpManagementRegSize = 0x40;
} // namespace

namespace facebook::fboss {

// To avoid ambiguity, we explicitly decided the pim number starts from 2.
MinipackBasePimContainer::MinipackBasePimContainer(
    int pim,
    const std::string& name,
    FpgaDevice* device,
    uint32_t pimStart,
    uint32_t pimSize,
    unsigned int portsPerPim)
    : pim_(pim) {
  pimQsfpController_ = std::make_unique<FbFpgaPimQsfpController>(
      std::make_unique<FpgaMemoryRegion>(
          "PimQsfpController",
          device,
          pimStart + kQsfpManagementRegStart,
          kQsfpManagementRegSize),
      portsPerPim);
}

FbFpgaPimQsfpController* MinipackBasePimContainer::getPimQsfpController() {
  return pimQsfpController_.get();
}

} // namespace facebook::fboss
