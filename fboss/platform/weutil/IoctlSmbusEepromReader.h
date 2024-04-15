// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.
#pragma once

#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <unistd.h>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace facebook::fboss::platform {

// Read full contents: 15K offset for Meta EEPROM V5 + 1024
#define DEFAULT_SIZE (15 * 1024) + 1024
#define MAX_EEPROM_READ_SIZE 2

// Reads EEPROM data directly from I2C device using SMBUS. This is needed
// specifically for Meru systems due to upstream kernel issue which
// prevents the proper driver from attaching.
// See: https://github.com/facebookexternal/fboss.bsp.arista/pull/31/files
class IoctlSmbusEepromReader {
 public:
  static void readEeprom(
      const std::string& outputDir,
      const std::string& outputPath,
      const uint16_t offset = 0,
      const uint16_t i2cAddr = 0x50,
      const uint16_t i2cBusNum = 1,
      const uint16_t length = DEFAULT_SIZE);

 private:
  class EepromReadMetadata {
   public:
    EepromReadMetadata(
        std::string inputPath,
        std::filesystem::path outputDir,
        std::string outputPath,
        int maxReadLen,
        int eepromOffset,
        int length);

    ~EepromReadMetadata() {
      close(inputFile);
      outputFile.close();
    }

    int inputFile;
    std::filesystem::path outputDir;
    std::ofstream outputFile;
    int maxReadLen;
    int eepromOffset;
    int length;
  };
  EepromReadMetadata metadata_;

  static int setSlaveAddr(int file, int address);

  static inline int i2cSmbusAccess(
      int file,
      char readWrite,
      uint8_t command,
      int size,
      union i2c_smbus_data& data);

  static uint8_t i2cSmbusReadByte(EepromReadMetadata& md);

  static int i2cSmbusReadData(
      EepromReadMetadata& md,
      uint32_t reg,
      int rcount,
      std::vector<uint8_t>& buf,
      int bufOffset);

  static int
  i2cSmbusWriteByteData(EepromReadMetadata& md, uint8_t command, uint8_t buf);

  static std::vector<uint8_t> readBytesFromEeprom(
      EepromReadMetadata& md,
      uint16_t offset);
};
} // namespace facebook::fboss::platform
