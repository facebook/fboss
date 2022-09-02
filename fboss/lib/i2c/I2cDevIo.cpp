/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/lib/i2c/I2cDevIo.h"
#include <errno.h>
#include <fcntl.h>
#include <folly/Format.h>
#include <folly/logging/xlog.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <sys/ioctl.h>
#include <memory>
#include "fboss/lib/i2c/I2cDevError.h"
#include "fboss/lib/i2c/I2cRdWrIo.h"

namespace facebook::fboss {

I2cDevIo::I2cDevIo(const std::string& devName) {
  devName_ = devName;
  fd_ = open(devName.c_str(), O_RDWR);

  if (fd_ < 0) {
    // Crash the program because the callers are not handling this exception
    throw I2cDevIoError(fmt::format(
        "I2cDevIo() failed to open {}, retVal = {:d}, errno = {}",
        devName,
        fd_,
        folly::errnoStr(errno)));
  }
  // TODO: Check if I2C_RDWR is supported
  i2cDevImpl_ = std::make_unique<I2cRdWrIo>(devName_, fd_);
}

I2cDevIo::~I2cDevIo() {
  close(fd_);
}

void I2cDevIo::write(
    uint8_t addr,
    uint8_t offset,
    const uint8_t* buf,
    int len) {
  i2cDevImpl_->write(addr, offset, buf, len);
}

void I2cDevIo::read(uint8_t addr, uint8_t offset, uint8_t* buf, int len) {
  i2cDevImpl_->read(addr, offset, buf, len);
}

} // namespace facebook::fboss
