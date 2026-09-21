// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.
#include "fboss/platform/weutil/IoctlSmbusEepromReader.h"

#include <assert.h>
#include <fmt/core.h>
#include <folly/String.h>
#include <linux/i2c.h>
#include <sys/ioctl.h>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

namespace facebook::fboss::platform {

IoctlSmbusEepromReader::EepromReadMetadata::EepromReadMetadata(
    std::string inputFilename,
    std::filesystem::path outputDir,
    std::string outputFilename,
    int maxReadLen,
    int eepromOffset,
    int length)
    : outputDir(outputDir),
      maxReadLen(maxReadLen),
      eepromOffset(eepromOffset),
      length(length) {
  inputFile = open(inputFilename.c_str(), O_RDONLY);
  if (inputFile < 0) {
    throw std::runtime_error(
        fmt::format(
            "Failed to open the i2c bus: {}, error: {}",
            inputFilename,
            folly::errnoStr(errno)));
  }

  if (!std::filesystem::exists(outputDir)) {
    std::filesystem::create_directories(outputDir);
  }

  outputFile = std::ofstream(
      outputDir / outputFilename, std::ios::out | std::ios::trunc);
  if (!outputFile.is_open()) {
    throw std::runtime_error(
        fmt::format("Failed to open the output file: {}", outputFilename));
  }
}

void IoctlSmbusEepromReader::readEeprom(
    const std::string& outputDir,
    const std::string& outputPath,
    const uint16_t offset,
    const uint16_t i2cAddr,
    const uint16_t i2cBusNum,
    const uint16_t length) {
  // Setup input/output fds
  std::string inputPath = fmt::format("/dev/i2c-{}", i2cBusNum);

  EepromReadMetadata readCtrl(
      inputPath,
      std::filesystem::path(outputDir),
      outputPath,
      MAX_EEPROM_READ_SIZE,
      offset,
      length);

  // Set i2c slave and allocate read buffer
  auto res = setSlaveAddr(readCtrl.inputFile, i2cAddr);
  if (res) {
    throw std::runtime_error(
        fmt::format("Failed to set i2c slave address: 0x{:02x}", i2cAddr));
  }

  // Read EEPROM content and write to file
  auto bytes = readBytesFromEeprom(readCtrl, offset);
  if (bytes.empty()) {
    throw std::runtime_error("Failed to read EEPROM");
  }
  readCtrl.outputFile.write(reinterpret_cast<char*>(bytes.data()), length);
  if (readCtrl.outputFile.fail()) {
    throw std::runtime_error("Failed to write to output file");
  }
  // std::cout << fmt::format("Wrote to {}", readCtrl.outputFile) << std::endl;
}

int IoctlSmbusEepromReader::setSlaveAddr(int file, int address) {
  if (ioctl(file, I2C_SLAVE_FORCE, address) < 0) {
    std::cout << fmt::format(
        "set_slave_addr: Could not set address to 0x{:02x}: {}",
        address,
        folly::errnoStr(errno));
    return -errno;
  }
  return 0;
}

int IoctlSmbusEepromReader::i2cSmbusAccess(
    int file,
    char read_write,
    uint8_t command,
    int size,
    union i2c_smbus_data& data) {
  struct i2c_smbus_ioctl_data args;

  assert(file >= 0);

  args.read_write = read_write;
  args.command = command;
  args.size = size;
  args.data = &data;
  return ioctl(file, I2C_SMBUS, &args);
}

int IoctlSmbusEepromReader::i2cSmbusWriteByteData(
    EepromReadMetadata& md,
    uint8_t command,
    uint8_t byte) {
  union i2c_smbus_data data;
  assert(md.inputFile >= 0);
  data.byte = byte;
  if (i2cSmbusAccess(
          md.inputFile, I2C_SMBUS_WRITE, command, I2C_SMBUS_BYTE_DATA, data) <
      0) {
    std::cout << fmt::format(
                     "{}: ioctl error {}\n",
                     __FUNCTION__,
                     folly::errnoStr(errno))
              << std::endl;
    return -1;
  }
  return 0;
}

uint8_t IoctlSmbusEepromReader::i2cSmbusReadByte(EepromReadMetadata& md) {
  union i2c_smbus_data data;
  assert(md.inputFile >= 0);
  if (i2cSmbusAccess(md.inputFile, I2C_SMBUS_READ, 0, I2C_SMBUS_BYTE, data) <
      0) {
    throw std::runtime_error(
        fmt::format(
            "{}: ioctl error {}\n", __FUNCTION__, folly::errnoStr(errno)));
  }
  return data.byte;
}

int IoctlSmbusEepromReader::i2cSmbusReadData(
    EepromReadMetadata& md,
    uint32_t reg,
    int rcount,
    std::vector<uint8_t>& buf,
    int bufOffset) {
  if (buf.empty()) {
    return -1;
  }
  uint8_t cmd[4];
  int i;

  cmd[0] = (uint8_t)((reg >> 8) & 0xFF);
  cmd[1] = (uint8_t)(reg & 0xFF);
  // currently only supports byte reads. Need to set address, then read
  // bytes. block read is not supported.
  if (i2cSmbusWriteByteData(md, cmd[0], cmd[1]) < 0) {
    return -1;
  }
  for (i = 0; i < rcount; i++) {
    buf[bufOffset + i] = i2cSmbusReadByte(md);
  }
  return rcount;
}

std::vector<uint8_t> IoctlSmbusEepromReader::readBytesFromEeprom(
    EepromReadMetadata& md,
    uint16_t offset) {
  int length = md.length;
  std::vector<uint8_t> bytes(length);
  int i = 0;
  while (i < length) {
    int res;
    if (length - i >= md.maxReadLen) {
      // read maxReadLen bytes
      res = i2cSmbusReadData(md, offset + i, md.maxReadLen, bytes, i);
      if (res < 0) {
        // ERROR HANDLING: i2c transaction failed
        std::cout << fmt::format(
                         "Failed to read from the i2c bus: {}\n",
                         folly::errnoStr(errno))
                  << std::endl;
        bytes.clear();
        break;
      }
      i += md.maxReadLen;
    } else {
      // read remaining bytes
      res = i2cSmbusReadData(md, offset + i, (length - i), bytes, i);
      if (res < 0) {
        // ERROR HANDLING: i2c transaction failed
        std::cout << fmt::format(
                         "Failed to read from the i2c bus: {}\n",
                         folly::errnoStr(errno))
                  << std::endl;
        bytes.clear();
        break;
      }
      i += (length - i);
    }
  }
  return bytes;
}

} // namespace facebook::fboss::platform
