/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include "fboss/lib/i2c/I2cController.h"
#include "fboss/lib/usb/UsbDevice.h"
#include "fboss/lib/usb/UsbHandle.h"

#include <folly/Range.h>

#include <chrono>
#include <cstdint>

struct libusb_transfer;

namespace facebook::fboss {
class CP2112Intf : public I2cController {
  // Bare-bones virtual interface to the CP2112 class. Used for gmock
  // testing.
 public:
  CP2112Intf()
      : I2cController(folly::to<std::string>("i2cController.cp2112")) {}
  virtual ~CP2112Intf() {}

  virtual void open(bool setSmbusConfig = true) = 0;
  virtual void close() = 0;
  virtual void resetDevice() = 0;
  virtual bool isOpen() const = 0;

  virtual void read(
      uint8_t address,
      folly::MutableByteRange buf,
      std::chrono::milliseconds timeout) = 0;
  virtual void write(
      uint8_t address,
      folly::ByteRange buf,
      std::chrono::milliseconds timeout) = 0;
  virtual void writeReadUnsafe(
      uint8_t address,
      folly::ByteRange writeBuf,
      folly::MutableByteRange readBuf,
      std::chrono::milliseconds timeout) = 0;

  virtual std::chrono::milliseconds getDefaultTimeout() const = 0;

  // Move shortened read/write forms to base interface so they
  // can be used in tests
  void read(uint8_t address, folly::MutableByteRange buf) {
    read(address, buf, getDefaultTimeout());
  }

  void write(uint8_t address, folly::ByteRange buf) {
    write(address, buf, getDefaultTimeout());
  }

  void writeReadUnsafe(
      uint8_t address,
      folly::ByteRange writeBuf,
      folly::MutableByteRange readBuf) {
    writeReadUnsafe(address, writeBuf, readBuf, getDefaultTimeout());
  }

  void
  writeByte(uint8_t address, uint8_t value, std::chrono::milliseconds timeout) {
    write(address, folly::ByteRange(&value, sizeof(value)), timeout);
  }
  void writeByte(uint8_t address, uint8_t value) {
    write(
        address, folly::ByteRange(&value, sizeof(value)), getDefaultTimeout());
  }
};

/*
 * An interface to the Silicon Labs CP2112 USB to SMBus bridge.
 *
 * This currently provides only a blocking API.  It would be straightforward to
 * implement a non-blocking API, but Linux's standard I2C APIs only provide
 * blocking APIs.  Code that wants to deal with other I2C interfaces therefore
 * already has to support blocking operation.
 */
class CP2112 : public CP2112Intf {
 public:
  /*
   * The standard vendor and product IDs reported by cp2112 devices
   * (unless overridden in the customizable PROM).
   */
  enum : uint16_t {
    VENDOR_ID = 0x10c4,
    PRODUCT_ID = 0xea90,
  };
  /*
   * The part number reported by the getVersion() call.
   */
  enum : uint8_t {
    PART_NUMBER = 0x0c,
  };

  struct VersionInfo {
    VersionInfo() {}
    VersionInfo(uint8_t pn, uint8_t vers) : partNumber(pn), version(vers) {}

    /*
     * The part number.  This should always be 0x0c.
     */
    uint8_t partNumber{0};
    /*
     * The part version.  Currently the following versions exist:
     *   1: CP2112-F01-GM
     *   2: CP2112-F02-GM
     *
     * The CP2112 datasheet documents some minor behavior differences between
     * the two versions in section 12.  (Version 1 sends a stop then start for
     * addressed read, whereas version 2 uses a repeated start.  This means
     * that addressed reads aren't necessarily atomic on version 1--another
     * master could potentially grab the bus between the address write and the
     * read.)
     */
    uint8_t version{0};
  };

  /*
   * Configuration for the 8 GPIO pins.
   *
   * Each field contains one bit per pin, with the LSB referring to GPIO 0,
   * and the MSB referring to GPIO 7.
   */
  struct GpioConfig {
    GpioConfig() {}
    GpioConfig(uint8_t d, uint8_t pp, uint8_t s, uint8_t cd)
        : direction(d), pushPull(pp), special(s), clockDivider(cd) {}

    /*
     * Pin direction: 0 for input, 1 for output
     */
    uint8_t direction{0};
    /*
     * 0: Open drain (pull-up only)
     * 1: Push pull (pull-up when high, pull-down when low)
     */
    uint8_t pushPull{0};
    /*
     * Special pin behavior.  Only valid for GPIOs 0, 1, and 7.
     * 0: normal
     * 1: special
     *
     * GPIO 0: TX Toggle (To support a TX LED.  Outputs low when transmitting.)
     * GPIO 1: RX Toggle (To support a RX LED  Outputs low when receiving.)
     * GPIO 7: Clock output
     */
    uint8_t special{0};
    /*
     * Clock divider for the output frequency on GPIO 7 when clock output is
     * enabled.
     */
    uint8_t clockDivider{0};
  };

  /*
   * SMBus configuration parameters.
   */
  struct SMBusConfig {
    bool operator==(const SMBusConfig& other) const;
    bool operator!=(const SMBusConfig& other) const;

    /*
     * Speed to drive the SMBus, in hertz.
     * The hardware defaults to 100,000 (100KHz)
     */
    uint32_t speed{100000};
    /*
     * Slave address to respond on.  The CP2112 will ACK this address,
     * but won't respond to anything else.
     * The hardware default for this value is 2.
     */
    uint8_t address{2};
    /*
     * Auto send read.
     * 0: Only send a read response after a READ_FORCE_SEND request
     * 1: Send read response data (including partial data) whenever it is
     *    available when an interrupt poll is received.
     * Defaults to 0.
     *
     * Note: The auto-send read behavior appears to be slightly broken based on
     * my testing.  See the comments in initSettings().
     */
    uint8_t autoSendRead{0};
    /*
     * Write timeout, in milliseconds.
     * Defaults to 0 (no timeout).
     * Maximum allowed is 1000ms.
     */
    uint16_t writeTimeoutMS{0};
    /*
     * Read timeout, in milliseconds.
     * Defaults to 0 (no timeout).
     * Maximum allowed is 1000ms.
     */
    uint16_t readTimeoutMS{0};
    /*
     * If sclLowTimeout is 1, the CP2112 will reset the bus if a slave
     * device holds the SCL line low for more than 25ms.
     *
     * Defaults to 0 (disabled).
     */
    uint8_t sclLowTimeout{0};
    /*
     * The number of times to retry a transfer before giving up.
     * Defaults to 0.  Values over 1000 are ignored.
     */
    uint16_t retryLimit{0};
  };

  struct TransferStatus {
    /*
     * Values for status0:
     *   0: Idle
     *   1: Busy
     *   2: Completed
     *   3: Completed with error
     *
     * If status0 is 0, none of the remaining fields contain valid data.
     */
    uint8_t status0;
    /*
     * The meaning of status1 depends on the value of status0.
     *
     * If status0 == 1 (busy):
     *   0: Address ACKed
     *   1: Address NACKed
     *   2: Data read in progress
     *   3: Data write in progress
     *
     * If status0 == 2 or 3 (completed):
     *   0: Timeout address NACKed
     *   1: Timeout bus not free (SCL low timeout)
     *   2: Arbitration lost
     *   3: Read incomplete
     *   4: Write incomplete
     *   5: Succeeded after numRetries
     */
    uint8_t status1;
    /*
     * The number of retries before being completed, cancelled, or timing out.
     */
    uint16_t numRetries;
    /*
     * The number of bytes successfully read.
     */
    uint16_t bytesRead;
  };

  CP2112();
  explicit CP2112(libusb_context* ctx);
  ~CP2112() override;

  void open(bool setSmbusConfig = true) override;
  void close() override;
  bool isOpen() const override {
    return handle_.isOpen();
  }

  std::chrono::milliseconds getDefaultTimeout() const override {
    return defaultTimeout_;
  }
  void setDefaultTimeout(std::chrono::milliseconds timeout) {
    defaultTimeout_ = timeout;
  }

  /*
   * Reset the CP2112 chip.
   *
   * This will cause the chip to disconnect and re-enumerate on the USB bus.
   * This function will wait for the device to reinitialize and then will
   * re-connect to it before returning.
   */
  void resetFromUserver();
  bool resetFromRestEndpoint();
  void resetDevice() override;

  VersionInfo getVersion();

  GpioConfig getGpioConfig();
  void setGpioConfig(GpioConfig config);
  uint8_t getGpio();
  void setGpio(uint8_t values, uint8_t mask);

  SMBusConfig getSMBusConfig();

  /*
   * Set the SMBus settings.
   *
   * Note that this class disables the autoSendRead flag when opening the
   * device, and assumes this flag is always 0.  If you explicitly enable this
   * flag using this call this will cause problems for the read logic in this
   * class.
   */
  void setSMBusConfig(SMBusConfig config);

  /*
   * Read from the SMBus.
   *
   * The least significant bit of the address must be 0, since SMBus addresses
   * are only 7 bits.
   *
   * The length must be between 1 and 512 bytes.
   */
  void read(
      uint8_t address,
      folly::MutableByteRange buf,
      std::chrono::milliseconds timeout) override;
  using CP2112Intf::read;

  /*
   * Write to the SMBus.
   *
   * The length must be between 1 and 61 bytes.
   */
  void write(
      uint8_t address,
      folly::ByteRange buf,
      std::chrono::milliseconds timeout) override;
  using CP2112Intf::write;

  /*
   * An atomic write followed by read, without releasing the I2C bus.
   * This uses a repeated start, without a stop condition between the
   * write and read operations. This is the sequence from device
   * (CPAS for cp2112 to slave direction and small for slave to cp2112
   * direction):
   * [START, SLAVE_ADDRESS, WRITE, ack, DATA_ADDRESS, ack, REPEATED_START,
   *  SLAVE_ADDRESS, READ, ack, data, STOP]
   *
   * The writeLength must be between 1 and 16 bytes.
   * The readLength must be between 1 and 512 bytes.
   *
   * WARNING: This command appears to cause the CP2112 chip to lock
   * up if it times out.  The only way to recover is to reset the chip using
   * the reset pin.  (A USB reset command is not sufficient.)
   */
  void writeReadUnsafe(
      uint8_t address,
      folly::ByteRange writeBuf,
      folly::MutableByteRange readBuf,
      std::chrono::milliseconds timeout) override;
  using CP2112Intf::writeReadUnsafe;

  /*
   * Cancel any pending transfers.
   *
   * This command should not normally need to be used directly.  It is exposed
   * publicly primarily for debugging and diagnostic purposes.
   */
  TransferStatus cancelTransfer();

  /*
   * Get the current transfer status.
   *
   * This command should not normally need to be used directly.  It is exposed
   * publicly primarily for debugging and diagnostic purposes.
   */
  TransferStatus getTransferStatus(std::chrono::milliseconds timeout);
  TransferStatus getTransferStatus() {
    return getTransferStatus(defaultTimeout_);
  }

  static std::string getStatus0Msg(uint8_t status0);
  static std::string getStatus1Msg(uint8_t status0, uint8_t status1);

 private:
  enum ReportID : uint8_t {
    // Feature reports
    RESET_DEVICE = 0x01,
    GPIO_CONFIG = 0x02,
    GET_GPIO = 0x03,
    SET_GPIO = 0x04,
    GET_VERSION = 0x05,
    SMBUS_CONFIG = 0x06,
    // Interrupt reports
    READ_REQUEST = 0x10,
    WRITE_READ_REQUEST = 0x11,
    READ_FORCE_SEND = 0x12,
    READ_RESPONSE = 0x13,
    WRITE = 0x14,
    XFER_STATUS_REQUEST = 0x15,
    XFER_STATUS_RESPONSE = 0x16,
    CANCEL_XFER = 0x17,
    // Feature reports for one-time USB PROM customization
    LOCK_BYTE = 0x20,
    USB_CONFIG = 0x21,
    MFGR_STRING = 0x22,
    PRODUCT_STRING = 0x23,
    SERIAL_STRING = 0x24,
  };

  // Forbidden copy constructor and assignment operator
  CP2112(CP2112 const&) = delete;
  CP2112& operator=(CP2112 const&) = delete;

  void openDevice();
  void initSettings();

  void ensureGoodState();
  void flushTransfers();

  static std::string getBusyStatusMsg(uint8_t status1);
  static std::string getCompleteStatusMsg(uint8_t status1);
  void processReadResponse(
      folly::MutableByteRange buf,
      std::chrono::milliseconds timeout);
  void getTransferStatusImpl(
      uint8_t* usbBuf,
      std::chrono::milliseconds timeout,
      folly::StringPiece operation,
      uint32_t loopIter);
  std::chrono::milliseconds waitForTransfer(
      folly::StringPiece operation,
      std::chrono::steady_clock::time_point end);
  std::chrono::milliseconds updateTimeLeft(
      std::chrono::steady_clock::time_point end,
      bool sleep);

  uint16_t featureReportIn(ReportID report, uint8_t* buf, uint16_t length);
  void fullFeatureReportIn(ReportID report, uint8_t* buf, uint16_t length);
  void featureReportOut(ReportID report, const uint8_t* buf, uint16_t length);
  void intrOut(
      folly::StringPiece name,
      const uint8_t* buf,
      uint16_t length,
      std::chrono::milliseconds timeout);
  void intrIn(uint8_t* buf, uint16_t length, std::chrono::milliseconds timeout);

  libusb_context* ctx_{nullptr};
  UsbDevice dev_;
  UsbHandle handle_;
  bool ownCtx_{false};
  bool busGood_{true};
  std::chrono::milliseconds defaultTimeout_{500};
  std::chrono::time_point<std::chrono::steady_clock> lastResetTime_;
  std::chrono::milliseconds minResetInterval_{10000}; /* 10 seconds */
};

} // namespace facebook::fboss
