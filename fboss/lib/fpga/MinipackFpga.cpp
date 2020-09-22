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
constexpr uint32_t kFacebookFpgaBarAddr = 0xfbd00000;
constexpr uint32_t kFacebookFpgaBarSize = 0x80000;
constexpr uint32_t kPimStartNum = 1;
} // namespace

namespace facebook::fboss {
folly::Singleton<MinipackFpga> _minipackFpga;
std::shared_ptr<MinipackFpga> MinipackFpga::getInstance() {
  return _minipackFpga.try_get();
}

MinipackFpga::MinipackFpga() {
  pimStartNum_ = kPimStartNum;

  try {
    fpgaDevice_ = std::make_unique<FpgaDevice>(
        PciVendorId(kFacebookFpgaVendorID), PciDeviceId(PCI_MATCH_ANY));
  } catch (const std::exception& ex) {
    XLOG(ERR)
        << folly::format(
               "Could not access FPGA PCI device with vendorId={}, deviceId={}: ",
               kFacebookFpgaVendorID,
               PCI_MATCH_ANY)
        << folly::exceptionStr(ex);
    // fallback to default address
    fpgaDevice_ = std::make_unique<FpgaDevice>(
        kFacebookFpgaBarAddr, kFacebookFpgaBarSize);
  }

  for (uint32_t pim = 0; pim < MinipackFpga::kNumberPim; ++pim) {
    auto domFpgaBaseAddr = kFacebookFpgaPimBase + pim * kFacebookFpgaPimSize;
    pimFpgas_[pim] =
        std::make_unique<FbDomFpga>(std::make_unique<FpgaMemoryRegion>(
            folly::format("pim{:d}", pim + pimStartNum_).str(),
            fpgaDevice_.get(),
            domFpgaBaseAddr,
            kFacebookFpgaPimSize));
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
