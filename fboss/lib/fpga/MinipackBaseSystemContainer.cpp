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
  return dynamic_cast<MinipackBasePimContainer*>(
      MultiPimPlatformSystemContainer::getPimContainer(pim));
}
} // namespace facebook::fboss
