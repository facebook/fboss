// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/lib/fpga/FbDomFpga.h"
#include "fboss/lib/i2c/I2cController.h"
#include "fboss/lib/usb/TransceiverI2CApi.h"

#include <folly/Range.h>
#include <folly/Synchronized.h>
#include <folly/io/async/EventBase.h>

#include <stdint.h>
#include <thread>

namespace facebook::fboss {

class FbFpgaSpiError : public I2cError {
 public:
  explicit FbFpgaSpiError(const std::string& what) : I2cError(what) {}
};

// TODO: Create a base class for TransceiverIOController and derive this off it.
// Deriving off I2cController is wrong technically, both are different
// protocols. However, doing this for now because our codebase currently assumes
// i2c is the only way to access a transceiver and uses 'i2c' in a lot of places
class FbFpgaSpi : public I2cController {
 public:
  FbFpgaSpi(FbDomFpga* fpga, uint32_t spiId, uint32_t pim);

  FbFpgaSpi(std::unique_ptr<FpgaMemoryRegion> io, uint32_t spiId, uint32_t pim);

  uint8_t readByte(uint8_t offset, int page);
  void read(uint8_t offset, int page, folly::MutableByteRange buf);

  void writeByte(uint8_t offset, uint8_t val, int page);
  void write(uint8_t offset, int page, folly::ByteRange buf);

  void initializeSPIController(bool forceInit);

 private:
  template <typename Register>
  void readReg(Register& value);
  template <typename Register>
  void writeReg(Register& value);
  uint32_t getRegAddr(uint32_t regBase, uint32_t regIncr);
  // Waits till the SPI transaction is done
  bool waitForSpiDone();
  // Checks the flow control byte and returns true if it's asserted
  bool flowControlAsserted();
  // Triggers a dummy read to confirm that the OBO is ready for read/write
  // transactions. This needs to be done before every read/write txn to the OBO
  bool oboReadyForIO();

  FbDomFpga* fpga_{nullptr};
  std::unique_ptr<FbDomFpga> io_;

  int spiId_{-1};
};

class FbFpgaSpiController {
 public:
  FbFpgaSpiController(FbDomFpga* fpga, uint32_t spiId, uint32_t pim);

  FbFpgaSpiController(
      std::unique_ptr<FpgaMemoryRegion> fpga,
      uint32_t spiId,
      uint32_t pim);
  ~FbFpgaSpiController();

  uint8_t readByte(uint8_t offset, int page);
  void read(uint8_t offset, int page, folly::MutableByteRange buf);

  void writeByte(uint8_t offset, uint8_t val, int page);
  void write(uint8_t offset, int page, folly::ByteRange buf);

  folly::EventBase* getEventBase();

  /* Get the I2c transaction stats from this controller with the lock
   */
  const I2cControllerStats getI2cControllerPlatformStats() const {
    return syncedFbSpi_.lock()->getI2cControllerPlatformStats();
  }

  void initializeSPIController(bool forceInit);

 private:
  folly::Synchronized<FbFpgaSpi, std::mutex> syncedFbSpi_;
  std::unique_ptr<folly::EventBase> eventBase_;
  std::unique_ptr<std::thread> thread_;
  uint32_t pim_;
  uint32_t spiId_;
};

} // namespace facebook::fboss
