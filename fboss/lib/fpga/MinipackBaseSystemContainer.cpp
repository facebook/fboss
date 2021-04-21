// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/lib/fpga/MinipackBaseSystemContainer.h"

namespace facebook::fboss {

void MinipackBaseSystemContainer::initHW(bool /* forceReset */) {
  if (isHwInitialized_) {
    return;
  }
  getFpgaDevice()->mmap();
  isHwInitialized_ = true;
}

uint32_t MinipackBaseSystemContainer::getPimOffset(int pim) {
  // To avoid ambiguity, we explicitly decided the pim number starts from 2.
  return kFacebookFpgaPimBase + kFacebookFpgaPimSize * (pim - 2);
}

MinipackBasePimContainer* MinipackBaseSystemContainer::getPimContainer(
    int pim) const {
  // To avoid ambiguity, we explicitly decided the pim number starts from 2.
  CHECK(pim >= 2 && pim <= kNumberPim + 1);
  return pims_[pim - 2].get();
}

std::vector<MinipackBasePimContainer*>
MinipackBaseSystemContainer::getAllPimContainers() {
  std::vector<MinipackBasePimContainer*> pims;
  for (const auto& pim : pims_) {
    pims.push_back(pim.get());
  }
  return pims;
}

} // namespace facebook::fboss
