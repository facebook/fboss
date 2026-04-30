// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <chrono>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "fboss/qsfp_service/module/TransceiverImpl.h"

namespace facebook::fboss {

class TransceiverImpl;

// Definitions for CDB Get Firmware Info (CMD 0x0100) reply data
// FirmwareStatus byte offset and bitmasks
constexpr int kCdbFwInfoFwStatusOffset = 0;
constexpr uint8_t kCdbFwInfoBankARunningMask = 0x01;
constexpr uint8_t kCdbFwInfoBankBRunningMask = 0x10;
// Image A build number offsets (2 bytes, big-endian)
constexpr int kCdbFwInfoImageABuildHiOffset = 4;
constexpr int kCdbFwInfoImageABuildLoOffset = 5;
// Image B build number offsets (2 bytes, big-endian)
constexpr int kCdbFwInfoImageBBuildHiOffset = 40;
constexpr int kCdbFwInfoImageBBuildLoOffset = 41;
// Minimum reply length needed to read Image B build number
constexpr int kCdbFwInfoMinRlplLength = 42;

// CDB command 0x0041 (Firmware Management Features) LPL offsets
// MaxDurationCoding at byte 137, bit 3 (LPL offset 1)
constexpr int kCdbFwFeatureMaxDurationCodingOffset = 1;
constexpr uint8_t kCdbFwFeatureMaxDurationCodingMask = 0x08;
// MaxDurationWrite at bytes 148-149 (LPL offset 12-13, U16 big-endian)
constexpr int kCdbFwFeatureMaxDurationWriteHiOffset = 12;
constexpr int kCdbFwFeatureMaxDurationWriteLoOffset = 13;
// Minimum reply length needed to read MaxDurationWrite
constexpr int kCdbFwFeatureMinRlplLength = 14;

// Maximum CDB command timeout guardrail (120 seconds in microseconds).
// Prevents indefinite waits even if a module misreports MaxDurationWrite.
constexpr uint64_t kMaxCdbTimeoutUsec = 120000000;

// CMIS page numbers for I2C transaction logging
constexpr int kCdbPage = 0x9f;
constexpr int kLowerPage = -1;

/*
 * This class represents the CDB block which is written to the CMIS optics
 * CDB memory to trigger the CDB operation like firmware download.
 */
class CdbCommandBlock {
 public:
  static constexpr uint8_t kCdbLplMemoryLength = 120;

  // Constructor to initialize data block from 0
  CdbCommandBlock() {
    resetCdbBlock();
  }

  // Create Firmware download start command
  void createCdbCmdFwDownloadStart(
      uint8_t startCommandPayloadSize,
      int imageLen,
      int& imageOffset,
      const uint8_t* imageBuf);
  // Create Firmware download image command
  void createCdbCmdFwDownloadImageLpl(
      uint8_t startCommandPayloadSize,
      int imageLen,
      const uint8_t* imageBuf,
      int& imageOffset,
      int& imageChunkLen);
  void createCdbCmdFwDownloadImageEpl(
      uint8_t startCommandPayloadSize,
      int imageLen,
      int& imageOffset,
      int& imageChunkLen);
  void writeEplPayload(
      TransceiverImpl* bus,
      const uint8_t* imageBuf,
      int& imageOffset,
      int imageChunkLen);
  // Create Firmware download complete command
  void createCdbCmdFwDownloadComplete();
  // Create Firmware image Run command
  void createCdbCmdFwImageRun();
  // Create Firmwae commit command
  void createCdbCmdFwCommit();
  // Create module general query command
  void createCdbCmdModuleQuery();
  // Create firmware feature info query command
  void createCdbCmdGetFwFeatureInfo();
  // Create firmware info query command (CMD 0x0100)
  void createCdbCmdGetFirmwareInfo();
  // Create Symbol Error Histogram command
  void createCdbCmdSymbolErrorHistogram(uint8_t datapathId, bool mediaSide);
  // Create Rx Error Histogram command
  void createCdbCmdRxErrorHistogram(uint8_t laneId, bool mediaSide);
  // Create generic command structure
  void createCdbCmdGeneric(uint16_t commandCode, std::vector<uint8_t>& lplData);

  // Public function to run the CDB command on the module. An optional
  // timeout can be passed to override the default
  // FLAGS_cdb_command_timeout_usec.
  bool cmisRunCdbCommand(
      TransceiverImpl* bus,
      std::optional<uint64_t> overrideTimeoutUsec = std::nullopt);
  // Provide response data to caller
  uint8_t getResponseData(uint8_t** pResponse);

  // Utility functions for caller of this class
  void selectCdbPage(TransceiverImpl* bus);
  void setMsaPassword(TransceiverImpl* bus, uint32_t msaPw);

  // CDB block access functions for command code
  uint16_t getCdbCommandCode() const {
    return cdbFields_.cdbCommandCode;
  }
  // CDB block access functions for LPL length
  uint8_t getCdbLplLength() const {
    return cdbFields_.cdbLplLength;
  }
  // CDB block access functions for RLPL length
  uint8_t getCdbRlplLength() const {
    return cdbFields_.cdbRlplLength;
  }
  // CDB block access functions for LPL memory (120 Bytes)
  const uint8_t* getCdbLplFlatMemory() const {
    return cdbFields_.cdbLplMemory.cdbLplFlatMemory;
  }

  // Parse MaxDurationWrite from CDB command 0x0041 response
  // Returns the timeout in microseconds derived from MaxDurationWrite (bytes
  // 148-149) and MaxDurationCoding (byte 137, bit 3) per OIF-CMIS-05.3
  // specification. Returns 0 if the response doesn't contain sufficient data.
  uint64_t getMaxDurationWriteUsec() const {
    if (cdbFields_.cdbRlplLength < kCdbFwFeatureMinRlplLength) {
      return 0;
    }

    // MaxDurationCoding bit 3: 0 = 1ms multiplier, 1 = 10ms multiplier
    uint8_t maxDurationCoding =
        cdbFields_.cdbLplMemory
            .cdbLplFlatMemory[kCdbFwFeatureMaxDurationCodingOffset];
    uint32_t multiplierMs =
        (maxDurationCoding & kCdbFwFeatureMaxDurationCodingMask) ? 10 : 1;

    // MaxDurationWrite U16 big-endian
    uint16_t maxDurationWrite =
        (static_cast<uint16_t>(
             cdbFields_.cdbLplMemory
                 .cdbLplFlatMemory[kCdbFwFeatureMaxDurationWriteHiOffset])
         << 8) |
        static_cast<uint16_t>(
            cdbFields_.cdbLplMemory
                .cdbLplFlatMemory[kCdbFwFeatureMaxDurationWriteLoOffset]);

    // Convert to microseconds: maxDurationWrite * multiplierMs * 1000
    return static_cast<uint64_t>(maxDurationWrite) * multiplierMs * 1000;
  }

  uint64_t getCdbWaitTimeMsec() {
    return commandBlockCdbWaitTime_.count();
  }

  uint64_t getMemoryWriteTimeMsec() {
    return memoryWriteTime_.count();
  }

 private:
  // Data block
  struct __attribute__((__packed__)) {
    uint16_t cdbCommandCode; // Reg 128-129
    uint16_t cdbEplLength; // Reg 130-131
    uint8_t cdbLplLength; // Reg 132
    uint8_t cdbChecksum; // Reg 133
    uint8_t cdbRlplLength; // Reg 134
    uint8_t cdbRlplChecksum; // Reg 135
    union {
      uint8_t cdbLplFlatMemory[kCdbLplMemoryLength]; // Reg 136-255
      struct {
        uint32_t cdbImageSize; // Reg 136-139
        uint32_t reserved;
        uint8_t cdbImageHeader[112]; // Reg 144-255
      } cdbFwDnldStartData;

      struct {
        uint32_t address; // Reg 136-139
        uint8_t image[116]; // Reg 140-255
      } cdbFwDnldImageData;
    } cdbLplMemory;
  } cdbFields_;

  std::chrono::duration<uint32_t, std::milli> commandBlockCdbWaitTime_{0};
  std::chrono::duration<uint32_t, std::milli> memoryWriteTime_{0};

  // Utility function to compute the One's complement sum
  uint8_t onesComplementSum();

  // Function to reset this data block
  void resetCdbBlock() {
    memset(&cdbFields_, 0, sizeof(cdbFields_));
  }

  void i2cWriteAndContinue(
      TransceiverImpl* bus,
      uint8_t i2cAddress,
      int offset,
      int length,
      const uint8_t* buf,
      int page);
};

} // namespace facebook::fboss
