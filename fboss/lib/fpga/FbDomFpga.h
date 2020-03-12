// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/lib/PhysicalMemory.h"

namespace facebook::fboss {
class FbDomFpga {
 public:
  enum class LedColor : uint32_t {
    OFF = 0x0,
    WHITE = 0x01,
    CYAN = 0x05,
    BLUE = 0x09,
    PINK = 0x0D,
    RED = 0x11,
    ORANGE = 0x15,
    YELLOW = 0x19,
    GREEN = 0x1D,
  };

  enum class PimType : uint32_t {
    MINIPACK_16Q = 0xA3000000,
    MINIPACK_16O = 0xA5000000,
  };

  FbDomFpga(uint32_t domBaseAddr, uint32_t domFpgaSize, uint8_t pim);

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
  virtual uint32_t read(uint32_t offset) const;
  virtual void write(uint32_t offset, uint32_t value);

  bool isQsfpPresent(int qsfp);
  uint32_t getQsfpsPresence();
  void ensureQsfpOutOfReset(int qsfp);

  /* Function to trigger the QSFP hard reset by toggling the bit from
   * FPGA register in some platforms
   */
  void triggerQsfpHardReset(int qsfp);

  void setFrontPanelLedColor(int qsfp, LedColor ledColor);

  virtual ~FbDomFpga() {}

  PimType getPimType();

 private:
  static constexpr uint32_t kFacebookFpgaVendorID = 0x1d9b;

  using FbFpgaPhysicalMemory32 = PhysicalMemory32<PhysicalMemory>;

  std::unique_ptr<FbFpgaPhysicalMemory32> phyMem32_;

  uint8_t pim_;
  uint32_t domBaseAddr_;
  uint32_t domFpgaSize_;
};

} // namespace facebook::fboss
