/*
 *  Copyright (c) 2018-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/lib/fpga/FujiFpga.h"
#include <folly/Format.h>
#include <folly/Singleton.h>
#include <folly/logging/xlog.h>
#include <pciaccess.h>
#include "fboss/agent/FbossError.h"
#include "fboss/lib/PciDevice.h"

namespace {
constexpr uint32_t kPimStartNum = 2;

constexpr uint32_t kFacebookFpgaSmbSize = 0x40000;
constexpr uint32_t kFacebookFpgaPimBase = 0x40000;
constexpr uint32_t kFacebookFpgaPimSize = 0x8000;
} // namespace

namespace facebook::fboss {
folly::Singleton<FujiFpga> _fujiFpga;
std::shared_ptr<FujiFpga> FujiFpga::getInstance() {
  return _fujiFpga.try_get();
}

FujiFpga::FujiFpga() {
  pimStartNum_ = kPimStartNum;
  fpgaPimBase_ = kFacebookFpgaPimBase;
  fpgaPimSize_ = kFacebookFpgaPimSize;

  uint64_t fpgaBar0;
  // Get the BAR address for FPGA device
  PciDevice fpgaPciDevice(kFacebookFpgaVendorID, PCI_MATCH_ANY);
  fpgaPciDevice.open();
  if (fpgaPciDevice.isGood()) {
    fpgaBar0 = fpgaPciDevice.getMemoryRegionAddress(0);
  } else {
    throw FbossError("cannot get BAR address for Fuji FPGA device");
  }

  phyMem32_ = std::make_unique<FbFpgaPhysicalMemory32>(
      fpgaBar0, kFacebookFpgaSmbSize, false);
  for (uint32_t pim = 0; pim < FujiFpga::kNumberPim; ++pim) {
    auto domFpgaBaseAddr =
        fpgaBar0 + kFacebookFpgaPimBase + pim * kFacebookFpgaPimSize;
    // pim id start from 2
    pimFpgas_[pim] = std::make_unique<FbDomFpga>(
        domFpgaBaseAddr, kFacebookFpgaPimSize, pim + pimStartNum_);

    XLOG(DBG2) << folly::format(
        "Creating DOM FPGA at address={:#x} size={:d}",
        domFpgaBaseAddr,
        kFacebookFpgaPimSize);
  }
}

} // namespace facebook::fboss
