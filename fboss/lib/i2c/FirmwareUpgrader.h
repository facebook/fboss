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
  // Mapping the module type string to module type part number string
  // The module type string is provided by user CLI and the mapped module part
  // number string can be compared with modules's register page 1 byte 148-163
  static inline std::map<std::string, std::vector<std::string>> partNoMap{
      {"finisar-200g-fr4", {"FTCC1112E1PLL-FB"}},
      {"finisar-400g-fr4",
       {"FTCD4313E2PCL   ",
        "FTCD4313E2PCL-FB",
        "FTCD4313E2PCLFB1",
        "FTCD4313E2PCLFB2",
        "FTCD4313E2PCLFB3",
        "FTCD4313E2PCLFB4"}},
      {"finisar-400g-lr4", {"FTCD4323E2PCL   "}},
      {"innolight-200g-fr4",
       {"T-FX4FNT-HFB    ", "T-FX4FNT-HFP    ", "T-FX4FNT-HFS    "}},
      {"innolight-400g-fr4",
       {"T-DQ4CNT-NFB    ", "T-DQ4CNT-NF2    ", "T-DQ4CNT-NFM    "}},
      {"intel-200g-fr4", {"SPTSMP3CLCK8    ", "SPTSMP3CLCK9    "}},
      {"intel-400g-fr4", {"SPTSHP3CLCKS    "}},
  };

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
  // Image type (App/Dsp)
  bool appImage_{true};

  // Private function to finally download firmware image on module using cdb
  // process
  bool cmisModuleFirmwareDownload(const uint8_t* imageBuf, int imageLen);
};

} // namespace facebook::fboss
