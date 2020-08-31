// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/lib/PhysicalMemory.h"
#include "fboss/lib/fpga/FbDomFpga.h"

namespace facebook::fboss {

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

  /**
   * FPGA PCIe Register has been upgraded to 32bits data width on 32 bits
   * address.
   */
  uint32_t readSmb(uint32_t offset);
  uint32_t readPim(uint8_t pim, uint32_t offset);

  void writeSmb(uint32_t offset, uint32_t value);
  void writePim(uint8_t pim, uint32_t offset, uint32_t value);

  void
  setFrontPanelLedColor(uint8_t pim, int qsfp, FbDomFpga::LedColor ledColor);

  FbDomFpga* getDomFpga(uint8_t pim);

  FbDomFpga::PimType getPimType(uint8_t pim);

 protected:
  static constexpr uint32_t kFacebookFpgaVendorID = 0x1d9b;

  uint32_t pimStartNum_ = 0;
  uint32_t fpgaPimBase_ = 0;
  uint32_t fpgaPimSize_ = 0;

  using FbFpgaPhysicalMemory32 = PhysicalMemory32<PhysicalMemory>;

  std::unique_ptr<FbFpgaPhysicalMemory32> phyMem32_;

  std::array<std::unique_ptr<FbDomFpga>, kNumberPim> pimFpgas_;

  bool isHwInitialized_{false};
};

} // namespace facebook::fboss
