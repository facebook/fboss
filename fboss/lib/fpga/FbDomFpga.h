// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/lib/fpga/FpgaIoBase.h"

namespace facebook::fboss {
class FbDomFpga : public FpgaIoBase {
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
   * FPGA PCIe Register has been upgraded to 32bits data width on 32 bits
   * address.
   */
  virtual uint32_t read(uint32_t offset) const override;
  virtual void write(uint32_t offset, uint32_t value) override;

  bool isQsfpPresent(int qsfp);
  uint32_t getQsfpsPresence();
  void ensureQsfpOutOfReset(int qsfp);

  /* Function to trigger the QSFP hard reset by toggling the bit from
   * FPGA register in some platforms
   */
  void triggerQsfpHardReset(int qsfp);

  // This function will bring all the transceivers out of reset on this pim.
  void clearAllTransceiverReset();

  void setFrontPanelLedColor(int qsfp, LedColor ledColor);

  virtual ~FbDomFpga() {}

  PimType getPimType();

 private:
  static constexpr uint32_t kFacebookFpgaVendorID = 0x1d9b;

  uint8_t pim_;
  uint32_t domBaseAddr_;
  uint32_t domFpgaSize_;
};

} // namespace facebook::fboss
