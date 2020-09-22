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
#include "fboss/agent/FbossError.h"

namespace {
// normally, PIM index should start at 2
constexpr uint32_t kPimStartNum = 2;
} // namespace

namespace facebook::fboss {
folly::Singleton<FujiFpga> _fujiFpga;
std::shared_ptr<FujiFpga> FujiFpga::getInstance() {
  return _fujiFpga.try_get();
}

FujiFpga::FujiFpga() {
  pimStartNum_ = kPimStartNum;
  fpgaDevice_ = std::make_unique<FpgaDevice>(
      PciVendorId(kFacebookFpgaVendorID), PciDeviceId(PCI_MATCH_ANY));

  for (uint32_t pim = 0; pim < FujiFpga::kNumberPim; ++pim) {
    auto domFpgaBaseAddr = kFacebookFpgaPimBase + pim * kFacebookFpgaPimSize;
    pimFpgas_[pim] =
        std::make_unique<FbDomFpga>(std::make_unique<FpgaMemoryRegion>(
            folly::format("pim{:d}", pim + pimStartNum_).str(),
            fpgaDevice_.get(),
            domFpgaBaseAddr,
            kFacebookFpgaPimSize));
  }
}

} // namespace facebook::fboss
