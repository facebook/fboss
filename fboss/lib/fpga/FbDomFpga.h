// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/lib/fpga/FpgaDevice.h"
#include "fboss/lib/fpga/HwMemoryRegion.h"

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

  explicit FbDomFpga(std::unique_ptr<FpgaMemoryRegion> io)
      : io_(std::move(io)) {}

  uint32_t read(uint32_t offset) const {
    return io_->read(offset);
  }

  void write(uint32_t offset, uint32_t value) {
    io_->write(offset, value);
  }

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

  PimType getPimType();

  // TODO(pgardideh): this is temporary. eventually memory region ownership
  // shall be moved to separate controllers and should not be accessed from
  // outside
  FpgaMemoryRegion* getMemoryRegion() {
    return io_.get();
  }

 private:
  static constexpr uint32_t kFacebookFpgaVendorID = 0x1d9b;
  std::unique_ptr<FpgaMemoryRegion> io_;
  // Due to one dom fpga might control multiple transceivers and all these
  // transceivers use the same register to control the reset mode, we need to
  // make sure there's a lock everytime when we access
  // `kFacebookFpgaQsfpResetReg` register
  std::mutex qsfpResetRegLock_;
};

} // namespace facebook::fboss
