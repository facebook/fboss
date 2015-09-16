/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "I2c.h"

#include "fboss/agent/SysError.h"
#include <folly/File.h>
#include <folly/Format.h>

extern "C" {
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
}

namespace facebook { namespace fboss {

I2cDevice::I2cDevice(int i2cBus, uint32_t address, bool slaveForce) :
    i2cBus_(i2cBus),
    devName(folly::sformat("/dev/i2c-{0}", i2cBus)),
    file_(devName.c_str(), O_RDWR) {
  // Will throw a std::system_error if it can't find any valid device node

  /* Set the slave address */
  auto req = slaveForce ? I2C_SLAVE_FORCE : I2C_SLAVE;
  if (ioctl(fd(), req, address) < 0) {
    throw SysError(errno, "Error setting slave address: ", address);
  }
}

int I2cDevice::fd() {
  return file_.fd();
}

int I2cDevice::readBlock(int offset, uint8_t fieldValue[]) {
  auto rc = i2c_smbus_read_i2c_block_data(fd(), offset, I2C_BLOCK_SIZE,
                                          fieldValue);
  if (rc < 0) {
    throw SysError(-rc, "Error reading i2c bus block data i2c bus: ", i2cBus_);
  }
  return rc;
}

uint16_t I2cDevice::readWord(int offset) {
  auto data = i2c_smbus_read_word_data(fd(), offset);
  if (data < 0) {
    throw SysError(-data, "Error reading word data: ", i2cBus_);
  }
  return data;
}

uint8_t I2cDevice::readByte(int offset) {
  auto data = i2c_smbus_read_byte_data(fd(), offset);
  if (data < 0) {
    throw SysError(-data, "Error reading word data: ", i2cBus_);
  }
  return data;
}

void I2cDevice::read(int offset, int length, uint8_t fieldValue[]) {
  int temp_len = length;
  int i = 0;
  int rc = 0;
  while (temp_len > 0) {
    rc = i2c_smbus_read_i2c_block_data(fd(), offset + i,
                                       std::min(temp_len, (int) I2C_BLOCK_SIZE),
                                       (fieldValue + i));
    if (rc < 0) {
      throw SysError(-rc, "Error reading data: ", i2cBus_);
    }
    i = i + rc;
    temp_len = temp_len - rc;
  }
}

}}
