// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/lib/i2c/I2cSmbusIo.h"
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
#include <memory>

namespace facebook::fboss {

void I2cSmbusIo::write(
    uint8_t /* addr */,
    uint8_t /* offset */,
    const uint8_t* /* buf */,
    int /* len */) {}

void I2cSmbusIo::read(
    uint8_t /* addr */,
    uint8_t /* offset */,
    uint8_t* /* buf */,
    int /* len */) {}

} // namespace facebook::fboss
