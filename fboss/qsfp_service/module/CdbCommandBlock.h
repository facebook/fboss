// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <chrono>
#include <memory>
#include <utility>
#include <vector>

#include "fboss/qsfp_service/module/TransceiverImpl.h"

namespace facebook::fboss {

class TransceiverImpl;

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
  // Create Symbol Error Histogram command
  void createCdbCmdSymbolErrorHistogram(uint8_t datapathId, bool mediaSide);
  // Create Rx Error Histogram command
  void createCdbCmdRxErrorHistogram(uint8_t laneId, bool mediaSide);
  // Create generic command structure
  void createCdbCmdGeneric(uint16_t commandCode, std::vector<uint8_t>& lplData);

  // Public function to run the CDB command on the module
  bool cmisRunCdbCommand(TransceiverImpl* bus);
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
      const uint8_t* buf);
};

} // namespace facebook::fboss
