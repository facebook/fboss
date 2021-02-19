// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/lib/fpga/MinipackSystemContainer.h"

#include "fboss/lib/fpga/MinipackPimContainer.h"

#include <folly/Singleton.h>
#include <pciaccess.h>

namespace facebook::fboss {

folly::Singleton<MinipackSystemContainer> _minipackSystemContainer;
std::shared_ptr<MinipackSystemContainer>
MinipackSystemContainer::getInstance() {
  return _minipackSystemContainer.try_get();
}

MinipackSystemContainer::MinipackSystemContainer()
    : MinipackSystemContainer(std::make_unique<FpgaDevice>(
          PciVendorId(kFacebookFpgaVendorID),
          PciDeviceId(PCI_MATCH_ANY))) {}

MinipackSystemContainer::MinipackSystemContainer(
    std::unique_ptr<FpgaDevice> fpgaDevice)
    : MinipackBaseSystemContainer(std::move(fpgaDevice)) {
  // create all PIM FPGA controllers
  for (auto pim = kPimStartNum; pim < kPimStartNum + kNumberPim; pim++) {
    // Should we align the PIM number to start from 2 or not?
    pims_[pim - kPimStartNum] = std::make_unique<MinipackPimContainer>(
        pim,
        folly::format("pim{:d}", pim).str(),
        getFpgaDevice(),
        MinipackBaseSystemContainer::getPimOffset(pim),
        kFacebookFpgaPimSize);
  }
}

} // namespace facebook::fboss
