// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <pciaccess.h>
#include <memory>
#include "fboss/lib/PhysicalMemory.h"
#include "fboss/lib/fpga/FbDomFpga.h"
#include "fboss/lib/fpga/FpgaDevice.h"

namespace facebook::fboss {

constexpr uint32_t kFacebookFpgaVendorID = 0x1d9b;
constexpr uint32_t kFacebookFpgaSmbSize = 0x40000;
constexpr uint32_t kFacebookFpgaPimBase = 0x40000;
constexpr uint32_t kFacebookFpgaPimSize = 0x8000;

class FbFpga {
 public:
  static constexpr auto kNumberPim = 8;

  virtual ~FbFpga() {}

  /**
   * This function should be called before any read/write() to call any hardware
   * related functions to make FPGA ready.
   * Right now, it doesn't require any input parameter, but if in the future
   * we need to support different HW settings, like 4DD, we can leaverage more
   * input parameters to set up FPGA.
   */
  void initHW();

  void
  setFrontPanelLedColor(uint8_t pim, int qsfp, FbDomFpga::LedColor ledColor);

  FbDomFpga* getDomFpga(uint8_t pim);

  FbDomFpga::PimType getPimType(uint8_t pim);

 protected:
  uint32_t pimStartNum_ = 0;

  std::unique_ptr<FpgaDevice> fpgaDevice_;

  std::array<std::unique_ptr<FbDomFpga>, kNumberPim> pimFpgas_;

  bool isHwInitialized_{false};
};

} // namespace facebook::fboss
