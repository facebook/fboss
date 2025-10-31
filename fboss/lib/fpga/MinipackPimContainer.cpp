// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/lib/fpga/MinipackPimContainer.h"

namespace {
constexpr auto kPortsPerPim = 16;
constexpr auto kPortsPerI2cController = 4;
constexpr auto kI2cControllerVersion = 0;
} // namespace

namespace facebook::fboss {

// To avoid ambiguity, we explicitly decided the pim number starts from 2.
MinipackPimContainer::MinipackPimContainer(
    int pim,
    const std::string& name,
    FpgaDevice* device,
    uint32_t start,
    uint32_t size)
    : MinipackBasePimContainer(pim, name, device, start, size, kPortsPerPim) {
  // Initialize the real time I2C access controllers.
  // On Minipack1 we have one I2C controller talking with four modules.
  for (uint32_t rtc = 0; rtc < kPortsPerPim / kPortsPerI2cController; rtc++) {
    i2cControllers_.push_back(
        std::make_unique<FbFpgaI2cController>(
            std::make_unique<FpgaMemoryRegion>(
                folly::format("pim{:d}", pim).str(), device, start, size),
            rtc,
            pim,
            kI2cControllerVersion));
  }
}

FbFpgaI2cController* MinipackPimContainer::getI2cController(unsigned int port) {
  CHECK(port >= 0 && port < kPortsPerPim);
  // On Minipack1 we have one I2C controller talking with four modules.
  auto controllerId = port / kPortsPerI2cController;
  return i2cControllers_[controllerId].get();
}

} // namespace facebook::fboss
