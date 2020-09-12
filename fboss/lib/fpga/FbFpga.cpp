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
  // mmap the 32bit io physical memory
  phyMem32_->mmap();
  for (uint32_t pim = 0; pim < FbFpga::kNumberPim; ++pim) {
    pimFpgas_[pim]->initHW();
  }
  isHwInitialized_ = true;
}

uint32_t FbFpga::readSmb(uint32_t offset) {
  CHECK(offset >= 0 && offset <= fpgaPimBase_ + kNumberPim * fpgaPimSize_);
  uint32_t ret = phyMem32_->read(offset);
  XLOG(DBG5) << folly::format("FPGA read {:#x}={:#x}", offset, ret);
  return ret;
}

uint32_t FbFpga::readPim(uint8_t pim, uint32_t offset) {
  return pimFpgas_[pim - pimStartNum_]->read(offset);
}

void FbFpga::writePim(uint8_t pim, uint32_t offset, uint32_t value) {
  return pimFpgas_[pim - pimStartNum_]->write(offset, value);
}

void FbFpga::writeSmb(uint32_t offset, uint32_t value) {
  CHECK(offset >= 0 && offset <= fpgaPimBase_ + kNumberPim * fpgaPimSize_);
  XLOG(DBG5) << folly::format("FPGA write {:#x} to {:#x}", value, offset);
  phyMem32_->write(offset, value);
}

void FbFpga::setFrontPanelLedColor(
    uint8_t pim,
    int qsfp,
    FbDomFpga::LedColor ledColor) {
  pimFpgas_[pim - pimStartNum_]->setFrontPanelLedColor(qsfp, ledColor);
}

FbDomFpga* FbFpga::getDomFpga(uint8_t pim) {
  return pimFpgas_[pim - pimStartNum_].get();
}

FbDomFpga::PimType FbFpga::getPimType(uint8_t pim) {
  return pimFpgas_[pim - pimStartNum_]->getPimType();
}
} // namespace facebook::fboss
