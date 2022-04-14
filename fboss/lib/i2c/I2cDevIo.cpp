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
#include <folly/Range.h>
#include <folly/lang/Bits.h>
#include <folly/logging/xlog.h>
#include <gflags/gflags.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <stdint.h>
#include <sys/ioctl.h>

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
}

I2cDevIo::~I2cDevIo() {
  close(fd_);
}

void I2cDevIo::write(
    uint8_t addr,
    uint8_t offset,
    const uint8_t* buf,
    int len) {
  // Create (len + 1) byte buffer, put device register offset in first byte,
  // then put the rest of the data in buffer
  uint8_t outbuf[len + 1];
  outbuf[0] = offset;
  memcpy(&outbuf[1], buf, sizeof(uint8_t) * len);
  struct i2c_rdwr_ioctl_data packets;
  struct i2c_msg messages[1];

  messages[0].addr = addr;
  messages[0].flags = 0;
  messages[0].len = sizeof(outbuf);
  messages[0].buf = outbuf;

  packets.msgs = messages;
  packets.nmsgs = 1;

  if (ioctl(fd_, I2C_RDWR, &packets) >= 0) {
    // Replicate a 50 ms delay after each write from YampI2c class to avoid
    // invalidating previously qualified transceivers per review with HW optics
    const auto usDelay = 50000;
    /* sleep override */ usleep(usDelay);
  } else {
    throw I2cDevIoError(fmt::format(
        "write() failed to write to {}, errno = {}",
        devName_,
        folly::errnoStr(errno)));
  }
}

void I2cDevIo::read(uint8_t addr, uint8_t offset, uint8_t* buf, int len) {
  struct i2c_rdwr_ioctl_data packets;
  struct i2c_msg messages[2];

  messages[0].addr = addr;
  messages[0].flags = 0;
  messages[0].len = sizeof(offset);
  messages[0].buf = &offset;

  messages[1].addr = addr;
  messages[1].flags = I2C_M_RD;
  messages[1].len = len;
  messages[1].buf = buf;

  packets.msgs = messages;
  packets.nmsgs = 2;

  if (ioctl(fd_, I2C_RDWR, &packets) >= 0) {
    // Replicate a 50 ms delay after each read from YampI2c class to avoid
    // invalidating previously qualified transceivers per review with HW optics
    const auto usDelay = 50000;
    /* sleep override */ usleep(usDelay);
  } else {
    throw I2cDevIoError(fmt::format(
        "read() failed to read from port {}, errno = {}",
        devName_,
        folly::errnoStr(errno)));
  }
}

} // namespace facebook::fboss
