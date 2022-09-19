// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/lib/bsp/BspTransceiverIO.h"

#include <fcntl.h>
#include <folly/Format.h>
#include <folly/Range.h>
#include <folly/lang/Bits.h>
#include <folly/logging/xlog.h>
#include <gflags/gflags.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include "fboss/lib/CommonFileUtils.h"
#include "fboss/lib/bsp/gen-cpp2/bsp_platform_mapping_types.h"

namespace facebook::fboss {

BspTransceiverIO::BspTransceiverIO(
    uint32_t tcvr,
    BspTransceiverMapping& tcvrMapping)
    : I2cController("BspTransceiverIOController" + std::to_string(tcvr)) {
  auto i2cDevName = *tcvrMapping.io()->devicePath();
  i2cDev_ = std::make_unique<I2cDevIo>(i2cDevName);
  tcvrMapping_ = tcvrMapping;
  tcvrID_ = *tcvrMapping_.tcvrId();
  XLOG(DBG5) << fmt::format(
      "BspTransceiverIOTrace: BspTransceiverIO() successfully opened tcvr {:d}, {}",
      tcvr,
      i2cDevName);
}

void BspTransceiverIO::write(
    const TransceiverAccessParameter& param,
    const uint8_t* buf) {
  uint8_t addr =
      param.i2cAddress ? *param.i2cAddress : TransceiverI2CApi::ADDR_QSFP;
  uint8_t offset = param.offset;
  uint8_t len = param.len;
  // Increment the counter for I2C write transaction issued
  incrWriteTotal();

  try {
    i2cDev_->write(addr, offset, buf, len);
    XLOG(DBG1) << fmt::format(
        "BspTransceiverIOTrace: write() successfully wrote to tcvr {:d}",
        tcvrID_);

    // Increment the number of I2c bytes written successfully
    incrWriteBytes(len);
  } catch (const I2cDevIoError& ex) {
    incrWriteFailed();
    throw BspTransceiverIOError(fmt::format(
        "BspTransceiverIO::write() failed to write to tcvr {:d}: {}",
        tcvrID_,
        ex.what()));
  }
}

void BspTransceiverIO::read(
    const TransceiverAccessParameter& param,
    uint8_t* buf) {
  uint8_t addr =
      param.i2cAddress ? *param.i2cAddress : TransceiverI2CApi::ADDR_QSFP;
  uint8_t offset = param.offset;
  uint8_t len = param.len;
  // Increment the counter for I2C read transaction issued
  incrReadTotal();

  try {
    i2cDev_->read(addr, offset, buf, len);
    XLOG(DBG1) << fmt::format(
        "BspTransceiverIOTrace: read() successfully read from tcvr {:d}",
        tcvrID_);
    // Increment the number of I2c bytes read successfully
    incrReadBytes(len);
  } catch (const I2cDevIoError& ex) {
    incrReadFailed();
    throw BspTransceiverIOError(fmt::format(
        "BspTransceiverIO::read() failed to read from tcvr {:d}: {}",
        tcvrID_,
        ex.what()));
  }
}

} // namespace facebook::fboss
