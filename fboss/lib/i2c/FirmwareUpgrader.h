// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <memory>
#include <utility>
#include "fboss/lib/firmware_storage/FbossFirmware.h"
#include "fboss/lib/i2c/CdbCommandBlock.h"
#include "fboss/lib/usb/TransceiverI2CApi.h"

namespace facebook::fboss {

/*
 * This class provides the data and function to perform firmware upgrade on a
 * optics module. This can be used from CLI utility or from the process like
 * qsfp_service. It will either get the firmware data from utility or interface
 * with firmware_storage library to get the data.
 */
class CmisFirmwareUpgrader {
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

  // Private function to finally download firmware image on module using cdb
  // process
  bool cmisModuleFirmwareDownload(const uint8_t* imageBuf, int imageLen);
};

} // namespace facebook::fboss
