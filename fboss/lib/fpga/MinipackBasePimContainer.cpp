// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/lib/fpga/MinipackBasePimContainer.h"

namespace {
constexpr uint32_t kQsfpManagementRegStart = 0x40;
constexpr uint32_t kQsfpManagementRegSize = 0x40;
constexpr uint32_t kPortLedStart = 0x0310;
constexpr uint32_t kPortLedSize = 0x4;
constexpr uint32_t kMdioBaseAddr = 0x200;

constexpr uint32_t kIsPimInitializedAddr = 0x4;
constexpr uint32_t kPimInitializedMarkerSize = 0x1;
constexpr uint32_t kBasePimControlInitializedMarkerOffset = 0x0;
constexpr uint32_t kIsPimInitializedMarker = 0x55;
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
    : MultiPimPlatformPimContainer(), pim_(pim) {
  pimQsfpController_ = std::make_unique<FbFpgaPimQsfpController>(
      std::make_unique<FpgaMemoryRegion>(
          "PimQsfpController",
          device,
          pimStart + kQsfpManagementRegStart,
          kQsfpManagementRegSize),
      portsPerPim);

  for (auto led = 0; led < kNumLedPerPim; led++) {
    ledControllers_[led] =
        std::make_unique<MinipackLed>(std::make_unique<FpgaMemoryRegister>(
            folly::format("{}-led{}", name, led).str(),
            device,
            pimStart + kPortLedStart + led * kPortLedSize));
  }

  pimController_ = std::make_unique<MinipackPimController>(
      std::make_unique<FpgaMemoryRegion>(
          folly::format("pim{:d}-controller", pim).str(),
          device,
          pimStart + kMdioBaseAddr,
          pimSize));
  basePimControl_ = std::make_unique<FpgaMemoryRegion>(
      fmt::format("pim{:d}-controller-without-offset", pim),
      device,
      pimStart + kIsPimInitializedAddr,
      kPimInitializedMarkerSize);
}

FbFpgaPimQsfpController* MinipackBasePimContainer::getPimQsfpController() {
  return pimQsfpController_.get();
}

MinipackLed* MinipackBasePimContainer::getLedController(int qsfp) const {
  CHECK(qsfp >= 0 && qsfp < kNumLedPerPim);
  return ledControllers_[qsfp].get();
}

bool MinipackBasePimContainer::isPimPresent() const {
  // TODO(joseph5wu)
  return true;
}

void MinipackBasePimContainer::markPimInitialized() {
  basePimControl_->write(
      kBasePimControlInitializedMarkerOffset, kIsPimInitializedMarker);
  PIM_LOG(DBG2, pim_) << "PIM Initialized Marker: "
                      << basePimControl_->read(
                             kBasePimControlInitializedMarkerOffset);
}

bool MinipackBasePimContainer::isPimInitialized() const {
  PIM_LOG(DBG2, pim_) << "PIM Initialized Marker: "
                      << basePimControl_->read(
                             kBasePimControlInitializedMarkerOffset);
  auto scratchpadVal =
      basePimControl_->read(kBasePimControlInitializedMarkerOffset);
  if (scratchpadVal == kIsPimInitializedMarker) {
    return true;
  }
  if (scratchpadVal != 0) {
    PIM_LOG(WARNING, pim_)
        << "Initialization Scratchpad had an unexpected value 0x" << std::hex
        << scratchpadVal << std::dec
        << ". This may mean that some other code is using this field. Consider as initialized to be safe.";
    return true;
  }
  PIM_LOG(WARNING, pim_) << "PIM is not initialized based on scratchpad value.";
  return false;
}

PimState MinipackBasePimContainer::getPimState() const {
  PimState state;
  if (!isPimInitialized()) {
    state.errors()->push_back(
        PimError::PIM_MINIPACK_SCRATCHPAD_COUNTER_NOT_CLEARED);
  }
  return state;
}
} // namespace facebook::fboss
