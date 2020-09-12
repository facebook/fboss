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

namespace {
constexpr uint32_t kPimStartNum = 1;

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

MinipackFpga::MinipackFpga() {
  pimStartNum_ = kPimStartNum;
  fpgaPimBase_ = kFacebookFpgaPimBase;
  fpgaPimSize_ = kFacebookFpgaPimSize;

  XLOG(DBG2) << folly::format(
      "Creating Minipack FPGA at address={:#x} size={:d}",
      kFacebookFpgaBarAddr,
      kFacebookFpgaSmbSize);

  phyMem32_ = std::make_unique<FbFpgaPhysicalMemory32>(
      kFacebookFpgaBarAddr, kFacebookFpgaSmbSize, false);
  for (uint32_t pim = 0; pim < MinipackFpga::kNumberPim; ++pim) {
    auto domFpgaBaseAddr = kFacebookFpgaBarAddr + kFacebookFpgaPimBase +
        pim * kFacebookFpgaPimSize;
    pimFpgas_[pim] = std::make_unique<FbDomFpga>(
        domFpgaBaseAddr, kFacebookFpgaPimSize, pim + pimStartNum_);

    XLOG(DBG2) << folly::format(
        "Creating DOM FPGA at address={:#x} size={:d}",
        domFpgaBaseAddr,
        kFacebookFpgaPimSize);
  }
}

bool MinipackFpga::isQsfpPresent(uint8_t pim, int qsfp) {
  return pimFpgas_[pim - pimStartNum_]->isQsfpPresent(qsfp);
}

std::array<bool, MinipackFpga::kNumPortsPerPim> MinipackFpga::scanQsfpPresence(
    uint8_t pim) {
  uint32_t qsfpPresentReg = pimFpgas_[pim - pimStartNum_]->getQsfpsPresence();
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
  pimFpgas_[pim - pimStartNum_]->ensureQsfpOutOfReset(qsfp);
}

/* Trigger the QSFP hard reset of a given port on a given line card (PIM)
 * by toggling the FPGA bit. This function is for minipack line cards:
 * 16q and 4dd
 */
void MinipackFpga::triggerQsfpHardReset(uint8_t pim, int qsfp) {
  pimFpgas_[pim - pimStartNum_]->triggerQsfpHardReset(qsfp);
}

void MinipackFpga::clearAllTransceiverReset() {
  for (auto& pim : pimFpgas_) {
    pim->clearAllTransceiverReset();
  }
}

} // namespace facebook::fboss
