// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <memory>
#include <utility>
#include "fboss/lib/firmware_storage/FbossFirmware.h"
#include "fboss/lib/usb/TransceiverI2CApi.h"

namespace facebook::fboss {
/*
 * This class provides the data and function to perform firmware upgrade on a
 * optics module. This can be used from CLI utility or from the process like
 * qsfp_service. It will either get the firmware data from utility or interface
 * with firmware_storage library to get the data.
 */
class CmisFirmwareUpgrader {
  /*
   * This class represents the CDB block which is written to the CMIS optics
   * CDB memory to trigger the CDB operation like firmware download.
   */
  class CdbCommandBlock {
    // CmisFirmwareUpgrader is a friend class here as that access all data the
    // data fields of this class
    friend class CmisFirmwareUpgrader;

   public:
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
    void createCdbCmdFwDownloadImage(
        uint8_t startCommandPayloadSize,
        int imageLen,
        const uint8_t* imageBuf,
        int& imageOffset,
        int& imageChunkLen);
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
        uint8_t cdbLplFlatMemory[120]; // Reg 136-255
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

    // Utility function to compute the One's complement sum
    uint8_t onesComplementSum(int len);

    // Function to reset this data block
    void resetCdbBlock() {
      memset(&cdbFields_, 0, sizeof(cdbFields_));
    }
  };

 public:
  // Constructor. The caller is responsible for interfacing with Firmware
  // Store and provide the FbossFirmware object
  CmisFirmwareUpgrader(
      TransceiverI2CApi* bus,
      unsigned int modId,
      std::unique_ptr<FbossFirmware> fbossFirmware);

  // Function to trigger the firmware download to the QSFP module of CMIS type
  bool cmisModuleFirmwareUpgrade();

 private:
  // Bus class for moduleRead/Write() functions
  TransceiverI2CApi* bus_;
  // module Id for upgrade
  unsigned int moduleId_;
  // FbossFirmware object
  std::unique_ptr<FbossFirmware> fbossFirmware_;
  // Firmware image pointer
  folly::io::Cursor imageCursor_{nullptr};
  // Image IO buffer. This contains the entire firmware image file content
  // mapped in memory
  std::unique_ptr<folly::IOBuf> fileIOBuffer_;
  // MSA password for privilege operation
  std::array<uint8_t, 4> msaPassword_;
  // Default image header length
  uint32_t imageHeaderLen_;

  // Private function to run the CDB command on the module
  bool cmisRunCdbCommand(CdbCommandBlock* commandBlock);
  // Private function to finally download firmware image on module using cdb
  // process
  bool cmisModuleFirmwareDownload(const uint8_t* imageBuf, int imageLen);
};

} // namespace facebook::fboss
