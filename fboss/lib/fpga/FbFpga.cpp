/*
 *  Copyright (c) 2018-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/lib/fpga/FbFpga.h"
#include <folly/Format.h>
#include <folly/logging/xlog.h>

namespace facebook::fboss {
void FbFpga::initHW() {
  if (isHwInitialized_) {
    return;
  }
  fpgaDevice_->mmap();
  isHwInitialized_ = true;
}

void FbFpga::setFrontPanelLedColor(
    uint8_t pim,
    int qsfp,
    FbDomFpga::LedColor ledColor) {
  getDomFpga(pim)->setFrontPanelLedColor(qsfp, ledColor);
}

FbDomFpga* FbFpga::getDomFpga(uint8_t pim) {
  return pimFpgas_[pim - pimStartNum_].get();
}

FbDomFpga::PimType FbFpga::getPimType(uint8_t pim) {
  return pimFpgas_[pim - pimStartNum_]->getPimType();
}
} // namespace facebook::fboss
