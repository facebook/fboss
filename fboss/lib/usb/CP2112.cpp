/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 *
 * Some notes about the CP2112 device:
 *
 * - The autoSendRead functionality is broken, so we can't use it.
 *   (When using autoSendRead, the first byte of the response is often
 *   returned as 0x00, regardless of the value actually returned on the I2C
 *   bus.)
 *
 * - There unfortunately isn't any transfer ID field.  This means it is very
 *   important that we stay in sync with the device.  Otherwise if we don't
 *   expect the device to send any further read responses, but it actually
 *   sends one more response, we can accidentally treat the remaining response
 *   as being for the next read request that we issue.
 *
 * - When we issue a READ_FORCE_SEND request, this may trigger multiple
 *   READ_RESPONSE transfers from the device.  However, it won't necessarily
 *   cause the device to return the full response, even if an
 *   XFER_STATUS_RESPONSE has previously indicated that the read is complete.
 */
#include "fboss/lib/usb/CP2112.h"
#include <glog/logging.h>
#include <sstream>
#include "fboss/lib/BmcRestClient.h"
#include "fboss/lib/usb/UsbError.h"

#include <folly/ScopeGuard.h>
#include <folly/lang/Bits.h>
#include <folly/logging/xlog.h>
#include <libusb-1.0/libusb.h>

using folly::ByteRange;
using folly::Endian;
using folly::MutableByteRange;
using folly::StringPiece;
using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::steady_clock;

namespace {

struct ReportType {
  enum : uint16_t {
    INPUT = 0x0100,
    OUTPUT = 0x0200,
    FEATURE = 0x0300,
  };
};

struct Hid {
  enum : uint8_t {
    GET_REPORT = 1,
    SET_REPORT = 9,
  };
};

template <typename INT>
void setBE(uint8_t* buf, INT value) {
  INT be = Endian::big<INT>(value);
  memcpy(buf, &be, sizeof(INT));
}

template <typename INT>
INT readBE(const uint8_t* buf) {
  INT be;
  memcpy(&be, buf, sizeof(INT));
  return Endian::big<INT>(be);
}

void vlogHex(
    int vlogLevel,
    StringPiece label,
    const uint8_t* buf,
    size_t length) {
  if (!VLOG_IS_ON(vlogLevel)) {
    return;
  }

  std::stringstream log;
  size_t idx = 0;
  log << label << '\n';
  while (idx < length) {
    log << fmt::format("{:04x}:", idx);

    for (unsigned int n = 0; n < 16 && idx < length; ++n, ++idx) {
      log << fmt::format("{:s}{:02x}", n == 8 ? "  " : " ", buf[idx]);
    }
    log << '\n';
  }
  VLOG(vlogLevel) << log.str();
}

} // namespace

namespace facebook::fboss {
CP2112::CP2112() : ownCtx_(true) {
  lastResetTime_ = std::chrono::steady_clock::now();
  int rc = libusb_init(&ctx_);
  if (rc != 0) {
    throw LibusbError(rc, "failed to initialize libusb");
  }
}

CP2112::CP2112(libusb_context* ctx) : ctx_(ctx), ownCtx_(false) {}

CP2112::~CP2112() {
  close();
  if (ctx_ && ownCtx_) {
    libusb_exit(ctx_);
  }
}

void CP2112::open(bool setSmbusConfig) {
  SCOPE_FAIL {
    close();
  };

  openDevice();
  if (setSmbusConfig) {
    initSettings();
  }
  // Just in case the device had a transfer in progress or anything
  // when we attached to it, call flushTransfers to cancel any outstanding
  // transfer and ignore any pending interrupt in packets.
  flushTransfers();
}

void CP2112::close() {
  handle_.close();
  dev_.reset();
}

void CP2112::resetFromUserver() {
  try {
    uint8_t buf[2]{ReportID::RESET_DEVICE, 1};
    featureReportOut(ReportID::RESET_DEVICE, buf, sizeof(buf));
  } catch (const LibusbError& ex) {
    // We expect to get a LIBUSB_ERROR_PIPE when resetting the device,
    // since it will need to be re-enumerated on the bus.  Ignore this error.
    if (ex.errorCode() != LIBUSB_ERROR_PIPE) {
      throw;
    }
  }
}

bool CP2112::resetFromRestEndpoint() {
  BmcRestClient bmcClient;
  bmcClient.setTimeout(std::chrono::milliseconds(2000));
  try {
    auto ret = bmcClient.resetCP2112();
    if (ret) {
      VLOG(1) << "Reset CP2112 via REST endpoint passed.";
      return true;
    } else {
      VLOG(1) << "Reset CP2112 via REST endpoint failed..returned error ";
      return false;
    }
  } catch (const std::exception& ex) {
    VLOG(2) << "Reset CP2112 via REST endpoint failed..fall back.";
    return false;
  }
}

/*
 * Don't reset the device if it was last reset less than minResetInterval_
 * milliseconds ago.  Throw UsbDeviceResetError if we don't attempt the
 * reset because insufficient time has elapsed.
 */
void CP2112::resetDevice() {
  auto nowTime = std::chrono::steady_clock::now();
  if ((nowTime - lastResetTime_) < minResetInterval_) {
    std::chrono::duration<double> elapsed = nowTime - lastResetTime_;
    throw UsbDeviceResetError(
        "CP2112: insufficient time since last reset " +
        folly::to<std::string>(elapsed.count()) + " seconds ago");
  }
  VLOG(1) << "resetting CP2112 device";
  lastResetTime_ = nowTime;
  resetFromUserver();
}

CP2112::VersionInfo CP2112::getVersion() {
  uint8_t buf[3]{0};
  fullFeatureReportIn(ReportID::GET_VERSION, buf, sizeof(buf));
  DCHECK_EQ(buf[0], ReportID::GET_VERSION);
  return VersionInfo(buf[1], buf[2]);
}

CP2112::GpioConfig CP2112::getGpioConfig() {
  uint8_t buf[5]{0};
  fullFeatureReportIn(ReportID::GPIO_CONFIG, buf, sizeof(buf));
  DCHECK_EQ(buf[0], ReportID::GPIO_CONFIG);
  return GpioConfig(buf[1], buf[2], buf[3], buf[4]);
}

void CP2112::setGpioConfig(GpioConfig config) {
  uint8_t buf[5]{
      ReportID::GPIO_CONFIG,
      config.direction,
      config.pushPull,
      config.special,
      config.clockDivider,
  };
  featureReportOut(ReportID::GPIO_CONFIG, buf, sizeof(buf));
}

uint8_t CP2112::getGpio() {
  uint8_t buf[2]{0};
  fullFeatureReportIn(ReportID::GET_GPIO, buf, sizeof(buf));
  DCHECK_EQ(buf[0], ReportID::GET_GPIO);
  return buf[1];
}

void CP2112::setGpio(uint8_t values, uint8_t mask) {
  uint8_t buf[3]{ReportID::SET_GPIO, values, mask};
  featureReportOut(ReportID::SET_GPIO, buf, sizeof(buf));
}

CP2112::SMBusConfig CP2112::getSMBusConfig() {
  uint8_t buf[14]{0};
  fullFeatureReportIn(ReportID::SMBUS_CONFIG, buf, sizeof(buf));
  DCHECK_EQ(buf[0], ReportID::SMBUS_CONFIG);

  SMBusConfig config;
  memcpy(&config.speed, buf + 1, 4);
  config.address = buf[5];
  config.autoSendRead = buf[6];
  memcpy(&config.writeTimeoutMS, buf + 7, 2);
  memcpy(&config.readTimeoutMS, buf + 9, 2);
  config.sclLowTimeout = buf[11];
  memcpy(&config.retryLimit, buf + 12, 2);

  config.speed = Endian::big<uint32_t>(config.speed);
  config.writeTimeoutMS = Endian::big<uint16_t>(config.writeTimeoutMS);
  config.readTimeoutMS = Endian::big<uint16_t>(config.readTimeoutMS);
  config.retryLimit = Endian::big<uint16_t>(config.retryLimit);

  return config;
}

void CP2112::setSMBusConfig(SMBusConfig config) {
  // The chip will ignore read or write timeouts greater than 1000ms
  DCHECK_LE(config.writeTimeoutMS, 1000);
  DCHECK_LE(config.readTimeoutMS, 1000);
  // The retry limit must also be less than 1000
  DCHECK_LE(config.retryLimit, 1000);

  config.speed = Endian::big<uint32_t>(config.speed);
  config.writeTimeoutMS = Endian::big<uint16_t>(config.writeTimeoutMS);
  config.readTimeoutMS = Endian::big<uint16_t>(config.readTimeoutMS);
  config.retryLimit = Endian::big<uint16_t>(config.retryLimit);

  uint8_t buf[14]{ReportID::SMBUS_CONFIG, 0};
  memcpy(buf + 1, &config.speed, 4);
  buf[5] = config.address;
  buf[6] = config.autoSendRead;
  memcpy(buf + 7, &config.writeTimeoutMS, 2);
  memcpy(buf + 9, &config.readTimeoutMS, 2);
  buf[11] = config.sclLowTimeout;
  memcpy(buf + 12, &config.retryLimit, 2);

  featureReportOut(ReportID::SMBUS_CONFIG, buf, sizeof(buf));
}

void CP2112::read(uint8_t address, MutableByteRange buf, milliseconds timeout) {
  // Increment the counter for I2c read transaction issued
  incrReadTotal();

  if (buf.size() > 512) {
    LOG(ERROR) << "I2c read parameter error";
    throw UsbError("cannot read more than 512 bytes at once");
  }
  if (buf.size() < 1) {
    // As far as I can tell, CP2112 doesn't support 0-length "quick" reads.
    // The docs indicate that 0-lengths reads will be ignored.  The transfer
    // status after issuing a 0-length read appears to confirm this.
    LOG(ERROR) << "I2c read parameter error";
    throw UsbError("0-length reads are not allowed");
  }
  ensureGoodState();

  // Send the read request
  uint8_t usbBuf[64];
  usbBuf[0] = ReportID::READ_REQUEST;
  usbBuf[1] = address;
  setBE<uint16_t>(usbBuf + 2, buf.size());
  intrOut("read", usbBuf, sizeof(usbBuf), timeout);

  // Wait for the response data
  try {
    processReadResponse(buf, timeout);
  } catch (UsbError& e) {
    XLOG(DBG5) << "CP2112 i2c read error";
    // Increment the counter for I2c read failure and throw error
    incrReadFailed();
    throw;
  }

  // Update i2c bytes read stats
  incrReadBytes(buf.size());
}

void CP2112::write(uint8_t address, ByteRange buf, milliseconds timeout) {
  // Increment the counter for I2c write transaction issued
  incrWriteTotal();

  if (buf.size() > 61) {
    LOG(ERROR) << "I2c write parameter error";
    throw UsbError("cannot write more than 61 bytes at once");
  }
  if (buf.size() < 1) {
    // As far as I can tell, CP2112 doesn't support 0-length "quick" writes.
    // The docs indicate that 0-lengths writes will be ignored.  The transfer
    // status after issuing a 0-length write appears to confirm this.
    LOG(ERROR) << "I2c write parameter error";
    throw UsbError("attempted 0-length write");
  }
  ensureGoodState();

  VLOG(5) << "writing to i2c address " << std::hex << (int)address;

  // Send the write request
  uint8_t usbBuf[64];
  usbBuf[0] = ReportID::WRITE;
  usbBuf[1] = address;
  usbBuf[2] = buf.size();
  memcpy(usbBuf + 3, buf.begin(), buf.size());

  // Wait for the write to complete
  auto end = steady_clock::now() + timeout;
  intrOut("write request", usbBuf, sizeof(usbBuf), timeout);

  try {
    waitForTransfer("write", end);
  } catch (UsbError& e) {
    XLOG(DBG5) << "cp2112 i2c write error";
    // Increment the counter for I2c write failure and throw error
    incrWriteFailed();

    throw;
  }

  // Update i2c write stats
  incrWriteBytes(buf.size());
}

void CP2112::writeReadUnsafe(
    uint8_t address,
    ByteRange writeBuf,
    MutableByteRange readBuf,
    milliseconds timeout) {
  // Increment the counter for I2c write and read transaction
  incrReadTotal();
  incrWriteTotal();

  if (writeBuf.size() > 16) {
    LOG(ERROR) << "I2c write parameter error";
    throw UsbError(
        "cannot write more than 16 bytes at once for "
        "read-after-write");
  }
  if (writeBuf.size() < 1) {
    LOG(ERROR) << "I2c write parameter error";
    throw UsbError("must write at least 1 byte for read-after-write");
  }
  if (readBuf.size() > 512) {
    LOG(ERROR) << "I2c read parameter error";
    throw UsbError("cannot read more than 512 bytes at once");
  }
  if (readBuf.size() < 1) {
    LOG(ERROR) << "I2c read parameter error";
    throw UsbError("0-length reads are not allowed");
  }
  ensureGoodState();

  // Send the write-read request
  uint8_t usbBuf[64];
  usbBuf[0] = ReportID::WRITE_READ_REQUEST;
  usbBuf[1] = address;
  setBE<uint16_t>(usbBuf + 2, readBuf.size());
  usbBuf[4] = writeBuf.size();
  memcpy(usbBuf + 5, writeBuf.begin(), writeBuf.size());
  intrOut("addressed read", usbBuf, sizeof(usbBuf), timeout);

  // Wait for the response data
  try {
    processReadResponse(readBuf, timeout);
  } catch (UsbError& e) {
    LOG(ERROR) << "cp2112 i2c write read error";
    // Increment the counter for I2c write and read failure, then throw error
    incrReadFailed();
    incrWriteFailed();

    throw;
  }

  // Update the i2c read and write byte stats
  incrReadBytes(readBuf.size());
  incrWriteBytes(writeBuf.size());
}

void CP2112::openDevice() {
  dev_ = UsbDevice::find(ctx_, VENDOR_ID, PRODUCT_ID);
  handle_ = dev_.open();
  handle_.claimInterface(0);
}

void CP2112::initSettings() {
  // Initialize the SMBus config the way we expect.
  SMBusConfig config = getSMBusConfig();
  SMBusConfig desiredConfig = config;

  // Note: It would be nicer to enable autoSendRead.  Unfortunately it is
  // broken, and appears to return incorrect data most of the time.
  //
  // As of 2014-10-21 Silicon Labs now has a Knowledge Base article confirming
  // that this mode is broken:
  // http://community.silabs.com/t5/Silicon-Labs-Knowledge-Base
  //   /CP2112-returns-incorrect-data-after-an-Address-Read-Request/ta-p/126064
  //
  desiredConfig.autoSendRead = 0;

  // The CP2112 has a bug where it sometimes sends two back-to-back start
  // conditions at the start of the transaction.  This doesn't really seem to
  // be allowed according to the I2C specification, and it causes some I2C
  // slave devices to hang.  (In particular the 3M 6B2A-0412A-0 optical QSFP+
  // transceiver hangs on a back-to-back start.)
  //
  // According to Silicon Labs this bug is less likely to occur at higher clock
  // speeds, so run at 400kHz to reduce the likelihood of this bug.
  desiredConfig.speed = 400000;

  // We implement our own timeout and cancel functionality,
  // so just disable the hardware auto-cancel timeouts.
  desiredConfig.writeTimeoutMS = 0;
  desiredConfig.readTimeoutMS = 0;

  // Don't let the CP2112 chip retry indefinitely (which is the default
  // behavior with retryLimit=0).  Setting retryLimit to 1 limits the chip
  // to one retry after a failed read/write.  There doesn't seem to be a way to
  // configure it to perform no retries at all.
  desiredConfig.retryLimit = 1;

  // It does seem preferred to have the device reset the bus on SCL low
  // timeout.
  desiredConfig.sclLowTimeout = 1;

  if (config != desiredConfig) {
    setSMBusConfig(desiredConfig);
  }
}

void CP2112::ensureGoodState() {
  if (busGood_) {
    return;
  }

  VLOG(1) << "flushing transfers to resync device state";
  flushTransfers();
  busGood_ = true;
}

CP2112::TransferStatus CP2112::cancelTransfer() {
  uint8_t buf[64];
  buf[0] = ReportID::CANCEL_XFER;
  buf[1] = 1;
  intrOut("cancel transfer", buf, sizeof(buf), milliseconds(5));

  // Read the status, to read and clear out any failure state.
  return getTransferStatus();
}

void CP2112::flushTransfers() {
  // In case there are any interrupt in packets waiting for receipt
  // in the kernel, pull them out and discard them now.
  //
  // This ensures that any future interrupt in packets we receive are in
  // response to requests that we sent.  Otherwise our code will get confused
  // if we send a request, but then pull out an old interrupt in packet for an
  // earlier request.
  uint8_t buf[64];
  while (true) {
    try {
      intrIn(buf, sizeof(buf), milliseconds(3));
      VLOG(1) << "discarding stale USB interrupt response packet "
              << (int)buf[0];
    } catch (const LibusbError& ex) {
      if (ex.errorCode() != LIBUSB_ERROR_TIMEOUT) {
        throw;
      }
      busGood_ = true;
      break;
    }
  }

  // Cancel any outstanding transfers.
  cancelTransfer();
}

std::string CP2112::getStatus0Msg(uint8_t status0) {
  switch (status0) {
    case 0:
      return "idle";
    case 1:
      return "busy";
    case 2:
      return "succeeded";
    case 3:
      return "failed";
  }
  return folly::to<std::string>("unexpected status0=", status0);
}

std::string CP2112::getStatus1Msg(uint8_t status0, uint8_t status1) {
  switch (status0) {
    case 0:
      // status1 is meaningless in this case.
      return folly::to<std::string>("status1=", status1);
    case 1:
      return getBusyStatusMsg(status1);
    case 2:
    case 3:
      return getCompleteStatusMsg(status1);
  }
  return folly::to<std::string>(
      "unexpected status0=", status0, ", status1=", status1);
}

std::string CP2112::getBusyStatusMsg(uint8_t status1) {
  switch (status1) {
    case 0:
      return "address acknowledged";
    case 1:
      return "address not acknowledged";
    case 2:
      return "read incomplete";
    case 3:
      return "write incomplete";
  }
  return folly::to<std::string>("unexpected timeout status=", status1);
}

std::string CP2112::getCompleteStatusMsg(uint8_t status1) {
  switch (status1) {
    case 0:
      return "address not acknowledged";
    case 1:
      return "bus not free";
    case 2:
      return "arbitration lost";
    case 3:
      return "read incomplete";
    case 4:
      return "write incomplete";
    case 5:
      return "succeeded";
  }
  return folly::to<std::string>("unexpected failure status=", status1);
}

void CP2112::processReadResponse(MutableByteRange buf, milliseconds timeout) {
  // Wait for the read response data from the device.
  //
  // This is unfortunately quite tricky and fragile.  The device's autoSendRead
  // functionality appears slightly broken, so we can't use it.  (With this
  // setting enabled if frequently returns the first byte of a read as 0,
  // instead of returning the correct first byte.)  Therefore we instead have
  // to send READ_FORCE_SEND requests to cause the device to return the data to
  // us.
  //
  // Therefore we have to send a READ_FORCE_SEND packet to trigger the device
  // to begin returning data via the interrupt in pipe.  However, this won't
  // necessarily trigger the device to send the full response.  For large reads
  // (512 bytes), a single READ_FORCE_SEND call causes it to return multple
  // READ_RESPONSE packets, but it usually stops after returning about 300
  // bytes of data.  We have to send another READ_FORCE_SEND to get it to
  // return the remaining data.
  //
  // However, we don't want to send more READ_FORCE_SEND requests than
  // necessary.  If we send a READ_FORCE_SEND while the device is already
  // sending the final READ_RESPONSE, this will trigger it to send another
  // READ_RESPONSE which we aren't expecting.  (This wouldn't be necessary if
  // we had strict control over interrupt in polling.  However, we this is
  // controlled by the OS and the hardware USB controller, so we can't tell if
  // the CP2112 chip is still responding to interrupt in packets or if it is
  // NAKing them.)

  // Poll first using XFER_STATUS_REQUEST.  (We could poll just using
  // READ_FORCE_SEND.  However, if the device sent an extra unexpected
  // READ_RESPONSE from a previous read attempt we might mistake that as being
  // for this request.  By using XFER_STATUS_REQUEST we know that any
  // READ_RESPONSE we receive right now is extraneous.)
  auto end = steady_clock::now() + timeout;
  milliseconds timeLeft = waitForTransfer("read", end);

  // The device has finished reading data from the I2C bus.
  // Now we just have to read it over USB.
  uint8_t usbBuf[64];
  uint16_t bytesRead{0};
  bool sendRead = true;
  while (true) {
    // Send READ_FORCE_SEND if we think the device won't send data to us
    // otherwise.
    if (sendRead) {
      usbBuf[0] = ReportID::READ_FORCE_SEND;
      usbBuf[1] = 1;
      intrOut("read force send", usbBuf, sizeof(usbBuf), milliseconds(5));
      sendRead = false;
    }

    // Wait for a READ_RESPONSE.
    // We use a fixed 10ms timeout here, regardless of timeLeft.
    // libusb doesn't deal very well with timeouts much smaller than this.
    // If we set the timeout too low, we may time out even though the device is
    // about to send a READ_RESPONSE.  However, we still want a relatively
    // small timeout in case the device is stuck waiting for another
    // READ_FORCE_SEND.  (Our logic to update sendRead below should generally
    // avoid this case, though.  I haven't seen the device get stuck waiting on
    // READ_FORCE_SEND yet with the current logic.)
    try {
      intrIn(usbBuf, sizeof(usbBuf), milliseconds(10));
    } catch (const LibusbError& ex) {
      VLOG(1) << "timed out waiting on READ_RESPONSE, sending READ_FORCE_SEND";
      // If we timed out, send a READ_FORCE_SEND and keep trying.
      // Rethrow all other errors
      if (ex.errorCode() != LIBUSB_ERROR_TIMEOUT) {
        throw;
      }
      timeLeft = updateTimeLeft(end, false);
      if (timeLeft <= milliseconds(0)) {
        throw UsbError("timed out waiting on read response data");
      }

      sendRead = true;
      busGood_ = true;
      continue;
    }

    if (usbBuf[0] != ReportID::READ_RESPONSE) {
      // Something has gone wrong if we get anything other than READ_RESPONSE,
      // and we are out of sync with the device state.
      LOG(DFATAL) << "received unexpected interrupt response while waiting on "
                     "read response: "
                  << (int)usbBuf[0];
      busGood_ = false;
      throw UsbError("unexpected device status waiting on read response");
    }

    uint8_t status = usbBuf[1];
    uint8_t length = usbBuf[2];
    VLOG(5) << "SMBus read response: status=" << (int)status
            << ", length=" << (int)length;

    memcpy(buf.begin() + bytesRead, usbBuf + 3, length);
    bytesRead += length;

    if (status == 0 || status == 2) {
      // 0 is idle.
      // 2 is read success.
      //
      // We should always see status 0 here since we already polled the status
      // using XFER_STATUS_REQUEST above.
      //
      // The device always seems to finish with a 0-length read response.
      // If we haven't seen a final 0-length response keep reading until we see
      // it.  (We keep going even if bytesRead == buf.size().)
      if (bytesRead == buf.size() && length == 0) {
        break;
      }
    } else if (status != 1) {
      // 1 is busy.
      // 3 is failure.
      // Anything else is unexpected.
      // None of these should occur since we already got a successful
      // XFER_STATUS_RESPONSE above.
      LOG(DFATAL) << "unexpected read failure after successful "
                  << "XFER_STATUS_RESPONSE";
      busGood_ = false;
      throw UsbError(
          "unexpected status ", status, " while waiting on read response");
    }

    sendRead = (bytesRead < buf.size()) && (length < 61);

    // If we are still here the transaction is still in progress.
    // Update timeLeft.  If no data was returned, also sleep briefly to avoid
    // spinning on the CPU.
    bool sleep = (length == 0);
    timeLeft = updateTimeLeft(end, sleep);
    if (timeLeft <= milliseconds(0)) {
      throw UsbError("timed out waiting on read response data");
    }
  }
}

CP2112::TransferStatus CP2112::getTransferStatus(milliseconds timeout) {
  uint8_t usbBuf[64];
  getTransferStatusImpl(usbBuf, timeout, "", 1);

  TransferStatus status;
  status.status0 = usbBuf[1];
  status.status1 = usbBuf[2];
  status.numRetries = readBE<uint16_t>(usbBuf + 3);
  status.bytesRead = readBE<uint16_t>(usbBuf + 5);
  return status;
}

void CP2112::getTransferStatusImpl(
    uint8_t* usbBuf,
    milliseconds timeout,
    StringPiece operation,
    uint32_t loopIter) {
  uint16_t bufSize = 64;

  // Send an XFER_STATUS_REQUEST
  usbBuf[0] = ReportID::XFER_STATUS_REQUEST;
  usbBuf[1] = 1;

  intrOut("get xfer status", usbBuf, bufSize, timeout);
  // Wait for the XFER_STATUS_RESPONSE.  Note that we ignore timeout here,
  // and always pass in a fixed timeout of 20ms.  The device should return
  // a XFER_STATUS_RESPONSE here.  If we time out in libusb and fail to
  // read it here, this may confuse state later on, as we will read the
  // XFER_STATUS_RESPONSE when we aren't expecting it.
  intrIn(usbBuf, bufSize, milliseconds(20));

  if (usbBuf[0] == ReportID::READ_RESPONSE) {
    // It is slightly tricky to anticipate how many READ_RESPONSE packets
    // the device will generate from a read.  It is possible for a previous
    // read attempt to not read a final empty READ_RESPONSE, in which case we
    // may receive it here.
    DCHECK_EQ(usbBuf[1], 0); // status should be idle
    DCHECK_EQ(loopIter, 1); // This should only happen on the first loop.
    if (usbBuf[2] != 0) {
      throw UsbError(
          "unexepected response length ", (int)usbBuf[2], "should be 0.");
    }

    // Ignore the read response, and do another ead to receive our
    // XFER_STATUS_RESPONSE.
    intrIn(usbBuf, bufSize, milliseconds(20));
  }

  if (usbBuf[0] != ReportID::XFER_STATUS_RESPONSE) {
    // This shouldn't happen unless communication has gotten out of sync
    // between us and the device.
    LOG(DFATAL) << "received unexpected interrupt response while waiting on "
                << operation << " transfer status: " << (int)usbBuf[0];
    busGood_ = false;
    throw UsbError(
        "unexpected response ",
        (int)usbBuf[0],
        "while waiting on ",
        operation,
        " transfer status");
  }
}

milliseconds CP2112::waitForTransfer(
    StringPiece operation,
    steady_clock::time_point end) {
  auto now = steady_clock::now();
  milliseconds timeLeft = duration_cast<milliseconds>(end - now);

  uint8_t usbBuf[64];
  uint32_t loopIter{0};
  while (true) {
    ++loopIter;
    getTransferStatusImpl(usbBuf, timeLeft, operation, loopIter);

    uint8_t status0 = usbBuf[1];
    uint8_t status1 = usbBuf[2];
    // Bytes 3-4 are status2, and bytes 5-6 are status3.  However we currently
    // don't use these values other than logging them here.
    VLOG(5) << operation << " xfer status:"
            << " status0=" << (int)status0 << " status1=" << (int)status1
            << " status2=" << readBE<uint16_t>(usbBuf + 3)
            << " status3=" << readBE<uint16_t>(usbBuf + 5);

    if (status0 == 2) {
      // successfully completed
      return timeLeft;
    } else if (status0 == 3) {
      // failed
      throw UsbError(operation, " failed: ", getCompleteStatusMsg(status1));
    } else if (status0 != 1) {
      // 1 is busy.  Any other status is unexpected.
      // 0 is idle, which shouldn't occur while our write is in progress.
      busGood_ = false;
      throw UsbError(
          "unexpected transaction status ",
          status0,
          " while waiting on ",
          operation,
          " completion");
    }

    timeLeft = updateTimeLeft(end, true);
    if (timeLeft < milliseconds(0)) {
      cancelTransfer();
      throw UsbError(
          "timed out waiting on ",
          operation,
          " response: ",
          getBusyStatusMsg(status1));
    }
  }
}

milliseconds CP2112::updateTimeLeft(steady_clock::time_point end, bool sleep) {
  auto now = steady_clock::now();
  auto timeLeft = duration_cast<milliseconds>(end - now);
  if (timeLeft >= milliseconds(0) && sleep) {
    auto sleepDuration = std::min(timeLeft, milliseconds(10));
    usleep(sleepDuration.count() * 1000);
    timeLeft -= sleepDuration;
  }

  return timeLeft;
}

uint16_t
CP2112::featureReportIn(ReportID report, uint8_t* buf, uint16_t length) {
  CHECK(isOpen());
  uint8_t bRequestType =
      (LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_CLASS |
       LIBUSB_RECIPIENT_INTERFACE);
  uint8_t bRequest = Hid::GET_REPORT;
  uint16_t wValue = ReportType::FEATURE | static_cast<uint16_t>(report);
  uint16_t wIndex = 0; // the interface index
  unsigned int timeoutMS = 1000;
  int rc = libusb_control_transfer(
      handle_.handle(),
      bRequestType,
      bRequest,
      wValue,
      wIndex,
      buf,
      length,
      timeoutMS);
  if (rc < 0) {
    throw LibusbError(rc, "failed to get feature report ", report);
  }

  return rc;
}

void CP2112::fullFeatureReportIn(
    ReportID report,
    uint8_t* buf,
    uint16_t length) {
  auto bytesRead = featureReportIn(report, buf, length);
  if (bytesRead != length) {
    throw UsbError(
        "unexpected length read from USB feature report ",
        report,
        ": read ",
        bytesRead,
        ", expected ",
        length);
  }
}

void CP2112::featureReportOut(
    ReportID report,
    const uint8_t* buf,
    uint16_t length) {
  uint8_t bRequestType =
      (LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_CLASS |
       LIBUSB_RECIPIENT_INTERFACE);
  uint8_t bRequest = Hid::SET_REPORT;
  uint16_t wValue = ReportType::FEATURE | static_cast<uint16_t>(report);
  uint16_t wIndex = 0; // the interface index
  unsigned int timeoutMS = 1000;
  int rc = libusb_control_transfer(
      handle_.handle(),
      bRequestType,
      bRequest,
      wValue,
      wIndex,
      const_cast<uint8_t*>(buf),
      length,
      timeoutMS);
  if (rc < 0) {
    throw LibusbError(rc, "failed to set feature report ", report);
  }
  if (rc != length) {
    throw UsbError(
        "failed to write full USB feature report ",
        report,
        ": written ",
        rc,
        ", expected ",
        length);
  }
}

void CP2112::intrOut(
    StringPiece name,
    const uint8_t* buf,
    uint16_t length,
    milliseconds timeout) {
  // The CP2112 always uses 64-byte interrupt transfers.
  DCHECK_EQ(length, 64);
  vlogHex(6, "intr out:", buf, length);

  // The CP2112 always uses endpoint 1 for interrupt transfers.
  uint8_t outEndpoint = LIBUSB_ENDPOINT_OUT | 1;
  int lenResult;

  // Always pass in a timeout of at least 5ms, even if the caller specifies
  // something smaller.  We generally don't want to timeout inside
  // libusb calls--if this occurs we can't easily tell if the tranfer was sent
  // to the device, and so we aren't sure what interrupt in packets to expect.
  //
  // This minimum timeout helps ensure that we timeout inside our own timeout
  // checks, and not inside libusb calls.
  int usbTimeout = std::max(timeout.count(), 5L);

  int rc = libusb_interrupt_transfer(
      handle_.handle(),
      outEndpoint,
      const_cast<uint8_t*>(buf),
      length,
      &lenResult,
      usbTimeout);
  if (rc != 0) {
    busGood_ = false;
    throw LibusbError(rc, "failed to send ", name, " request");
  }
}

void CP2112::intrIn(uint8_t* buf, uint16_t length, milliseconds timeout) {
  // The CP2112 always uses 64-byte interrupt transfers.
  DCHECK_EQ(length, 64);

  // The CP2112 always uses endpoint 1 for interrupt transfers.
  uint8_t outEndpoint = LIBUSB_ENDPOINT_IN | 1;
  int lenResult;

  // Pass in a timeout of at least 1ms for libusb.
  // With a timeout of 0 libusb won't even bother checking for available data,
  // it just returns a timeout error immediately.
  int usbTimeout = std::max(timeout.count(), 1L);
  int rc = libusb_interrupt_transfer(
      handle_.handle(), outEndpoint, buf, length, &lenResult, usbTimeout);
  if (rc != 0) {
    busGood_ = false;
    throw LibusbError(rc, "error waiting for interrupt response");
  }
  if (lenResult != 64) {
    busGood_ = false;
    throw UsbError(
        "unexpected interrupt response length received from "
        "CP2112:",
        lenResult);
  }
  vlogHex(6, "intr in:", buf, length);
}

bool CP2112::SMBusConfig::operator==(const SMBusConfig& other) const {
  return memcmp(this, &other, sizeof(SMBusConfig)) == 0;
}

bool CP2112::SMBusConfig::operator!=(const SMBusConfig& other) const {
  return !(*this == other);
}

} // namespace facebook::fboss
