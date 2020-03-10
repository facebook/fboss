/*
 *  Copyright (c) 2018-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "MinipackFpga.h"

#include <folly/Format.h>
#include <folly/Singleton.h>
#include <folly/logging/xlog.h>
#include <gflags/gflags.h>

namespace {
constexpr uint32_t kFacebookFpgaBarAddr = 0xfbd00000;
constexpr uint32_t kFacebookFpgaSmbSize = 0x40000;

constexpr uint32_t kFacebookFpgaPimBase = 0x40000;
constexpr uint32_t kFacebookFpgaPimSize = 0x8000;
} // namespace

namespace facebook::fboss {
folly::Singleton<MinipackFpga> _minipackFpga;
std::shared_ptr<MinipackFpga> MinipackFpga::getInstance() {
  return _minipackFpga.try_get();
}

MinipackFpga::MinipackFpga()
    : phyMem32_(std::make_unique<MinipackPhysicalMemory32>(
          kFacebookFpgaBarAddr,
          kFacebookFpgaSmbSize,
          false)) {
  XLOG(DBG2) << folly::format(
      "Creating Minipack FPGA at address={:#x} size={:d}",
      kFacebookFpgaBarAddr,
      kFacebookFpgaSmbSize);

  for (uint32_t pim = 0; pim < MinipackFpga::kNumberPim; ++pim) {
    auto domFpgaBaseAddr = kFacebookFpgaBarAddr + kFacebookFpgaPimBase +
        pim * kFacebookFpgaPimSize;
    pimFpgas_[pim] = std::make_unique<FbDomFpga>(
        domFpgaBaseAddr, kFacebookFpgaPimSize, pim + 1);

    XLOG(DBG2) << folly::format(
        "Creating DOM FPGA at address={:#x} size={:d}",
        domFpgaBaseAddr,
        kFacebookFpgaPimSize);
  }
}

void MinipackFpga::initHW() {
  if (isHwInitialized_) {
    return;
  }
  // mmap the 32bit io physical memory
  phyMem32_->mmap();
  for (uint32_t pim = 0; pim < MinipackFpga::kNumberPim; ++pim) {
    pimFpgas_[pim]->initHW();
  }
  isHwInitialized_ = true;
}

uint32_t MinipackFpga::readSmb(uint32_t offset) {
  CHECK(
      offset >= 0 &&
      offset <= kFacebookFpgaPimBase + kNumberPim * kFacebookFpgaPimSize);
  uint32_t ret = phyMem32_->read(offset);
  XLOG(DBG5) << folly::format("FPGA read {:#x}={:#x}", offset, ret);
  return ret;
}

uint32_t MinipackFpga::readPim(uint8_t pim, uint32_t offset) {
  return pimFpgas_[pim - 1]->read(offset);
}

void MinipackFpga::writeSmb(uint32_t offset, uint32_t value) {
  CHECK(
      offset >= 0 &&
      offset <= kFacebookFpgaPimBase + kNumberPim * kFacebookFpgaPimSize);
  XLOG(DBG5) << folly::format("FPGA write {:#x} to {:#x}", value, offset);
  phyMem32_->write(offset, value);
}

void MinipackFpga::writePim(uint8_t pim, uint32_t offset, uint32_t value) {
  return pimFpgas_[pim - 1]->write(offset, value);
}

bool MinipackFpga::isQsfpPresent(uint8_t pim, int qsfp) {
  return pimFpgas_[pim - 1]->isQsfpPresent(qsfp);
}

std::array<bool, MinipackFpga::kNumPortsPerPim> MinipackFpga::scanQsfpPresence(
    uint8_t pim) {
  uint32_t qsfpPresentReg = pimFpgas_[pim - 1]->getQsfpsPresence();
  // From the lower end, each bit of this register represent the presence of a
  // QSFP.
  XLOG(DBG5) << folly::format("qsfpPresentReg value:{:#x}", qsfpPresentReg);
  std::array<bool, kNumPortsPerPim> qsfpPresence;
  for (int i = 0; i < kNumPortsPerPim; i++) {
    qsfpPresence[i] = (qsfpPresentReg >> i) & 1;
  }
  return qsfpPresence;
}

void MinipackFpga::ensureQsfpOutOfReset(uint8_t pim, int qsfp) {
  pimFpgas_[pim - 1]->ensureQsfpOutOfReset(qsfp);
}

/* Trigger the QSFP hard reset of a given port on a given line card (PIM)
 * by toggling the FPGA bit. This function is for minipack line cards:
 * 16q and 4dd
 */
void MinipackFpga::triggerQsfpHardReset(uint8_t pim, int qsfp) {
  pimFpgas_[pim - 1]->triggerQsfpHardReset(qsfp);
}

void MinipackFpga::setFrontPanelLedColor(
    uint8_t pim,
    int qsfp,
    FbDomFpga::LedColor ledColor) {
  pimFpgas_[pim - 1]->setFrontPanelLedColor(qsfp, ledColor);
}

FbDomFpga* MinipackFpga::getDomFpga(uint8_t pim) {
  return pimFpgas_[pim - 1].get();
}

FbDomFpga::PimType MinipackFpga::getPimType(uint8_t pim) {
  return pimFpgas_[pim - 1]->getPimType();
}
} // namespace facebook::fboss
