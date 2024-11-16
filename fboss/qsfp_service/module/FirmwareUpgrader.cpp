// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/qsfp_service/module/FirmwareUpgrader.h"

#include <chrono>
#include <utility>

#include <folly/Conv.h>
#include <folly/File.h>
#include <folly/FileUtil.h>
#include <folly/Format.h>
#include <folly/init/Init.h>
#include <folly/io/IOBuf.h>
#include <folly/logging/xlog.h>
#include <glog/logging.h>

#include "fboss/qsfp_service/module/TransceiverImpl.h"

#include "fboss/qsfp_service/module/cmis/gen-cpp2/cmis_types.h"

using folly::MutableByteRange;
using folly::StringPiece;
using std::make_pair;
using std::pair;
using std::chrono::seconds;
using std::chrono::steady_clock;
using namespace facebook::fboss;

namespace facebook::fboss {

// CMIS firmware related register offsets
constexpr uint8_t kfirmwareVersionReg = 39;
constexpr uint8_t kModulePasswordEntryReg = 122;

constexpr int moduleDatapathInitDurationUsec = 5000000;

// CMIS FW Upgrade
constexpr int kFwUpgrade = static_cast<int>(CmisField::FW_UPGRADE);

/*
 * CmisFirmwareUpgrader
 *
 * This is one of the two constructor and it will be invoked if the upgrader
 * is called from qsfp_service process. The caller will get the FbossFirmware
 * object and using that this CmisFirmwareUpgrader will be created. This
 * function will load the image file to IOBuf and get image pointer to use in
 * loading the firmware
 */
CmisFirmwareUpgrader::CmisFirmwareUpgrader(
    TransceiverImpl* bus,
    unsigned int modId,
    FbossFirmware* fbossFirmware)
    : bus_(bus), moduleId_(modId), fbossFirmware_(fbossFirmware) {
  // Check the FbossFirmware object first
  if (fbossFirmware_ == nullptr) {
    XLOG(ERR) << "FbossFirmware object is null, returning...";
    return;
  }
  // Load the image
  fbossFirmware_->load();
  // Get the image pointer
  imageCursor_ = fbossFirmware_->getImage();

  // Get the header length of image
  std::string hdrLen = fbossFirmware_->getProperty("header_length");
  imageHeaderLen_ = folly::to<uint32_t>(hdrLen);

  // Get the msa password
  std::string msaPwStr = fbossFirmware_->getProperty("msa_password");
  uint32_t msaPwVal;
  if (msaPwStr.compare(0, 2, "0x") == 0) {
    msaPwVal = std::stoull(msaPwStr, nullptr, 16);
  } else {
    msaPwVal = std::stoull(msaPwStr);
  }
  msaPassword_[0] = (msaPwVal & 0xFF000000) >> 24;
  msaPassword_[1] = (msaPwVal & 0x00FF0000) >> 16;
  msaPassword_[2] = (msaPwVal & 0x0000FF00) >> 8;
  msaPassword_[3] = (msaPwVal & 0x000000FF);

  // Get the image type
  std::string imageTypeStr = fbossFirmware_->getProperty("image_type");
  appImage_ = (imageTypeStr == "application") ? true : false;
}

/*
 * cmisModuleFirmwareDownload
 *
 * This function runs the firmware download operation for a module. This takes
 * the image buffer as input. This is basic function to do firmware download
 * and it can be run in any context - single thread, multiple thread etc
 */
bool CmisFirmwareUpgrader::cmisModuleFirmwareDownload(
    const uint8_t* imageBuf,
    int imageLen) {
  uint8_t startCommandPayloadSize = 0;
  bool status;
  int imageOffset, imageChunkLen;
  bool eplSupported = false;

  XLOG(INFO) << folly::sformat(
      "cmisModuleFirmwareDownload: Mod{:d}: Starting to download the image with length {:d}",
      moduleId_,
      imageLen);

  // Start the IO profiling
  bus_->i2cTimeProfilingStart();

  // Set the password to let the privileged operation of firmware download
  bus_->writeTransceiver(
      {TransceiverAccessParameter::ADDR_QSFP, kModulePasswordEntryReg, 4},
      msaPassword_.data(),
      POST_I2C_WRITE_NO_DELAY_US,
      kFwUpgrade);

  CdbCommandBlock commandBlockBuf;
  CdbCommandBlock* commandBlock = &commandBlockBuf;

  // Basic validation first. Check if the firmware download is allowed by
  // issuing the Query command to CDB
  commandBlock->createCdbCmdModuleQuery();
  // Run the CDB command
  status = commandBlock->cmisRunCdbCommand(bus_);
  if (status) {
    // Query result will be in LPL memory at byte offset 2
    if (commandBlock->getCdbRlplLength() >= 3) {
      if (commandBlock->getCdbLplFlatMemory()[2] == 0) {
        // This should not happen because before calling this function
        // we supply the password to module to allow priviledged
        // operation. But still download feature is not available here
        // so return false here
        XLOG(INFO) << folly::sformat(
            "cmisModuleFirmwareDownload: Mod{:d}: The firmware download feature is locked by vendor",
            moduleId_);
        return false;
      }
    }
  } else {
    // The QUERY command can fail if the module is in bootloader mode
    // Not able to determine CDB module status but don't return from here
    XLOG(INFO) << folly::sformat(
        "cmisModuleFirmwareDownload: Mod{:d}: Could not get result from CDB Query command",
        moduleId_);
  }

  // Step 0: Retrieve the Start Command Payload Size (the image header size)
  // Done by sending Firmware upgrade feature command to CDB
  commandBlock->createCdbCmdGetFwFeatureInfo();
  // Run the CDB command
  status = commandBlock->cmisRunCdbCommand(bus_);

  // If the CDB command is successfull then the Start Command Payload Size is
  // returned by CDB in LPL memory at offset 2

  if (status && commandBlock->getCdbRlplLength() >= 3) {
    // Get the firmware header size from CDB
    startCommandPayloadSize = commandBlock->getCdbLplFlatMemory()[2];

    // Check if EPL memory is supported
    if (commandBlock->getCdbLplFlatMemory()[5] == 0x10 ||
        commandBlock->getCdbLplFlatMemory()[5] == 0x11 ||
        commandBlock->getCdbLplFlatMemory()[5] == 0x3) {
      /* Per the spec, the valid values for this register (page 9F, offset 141)
       * are 00h: Both LPL and EPL are not supported 01h: LPL supported 10h: EPL
       * supported 11h: Both LPL and EPL are supported Certain vendors (vendor
       * for 400G-XDR4 GEN1 modules) have incorrectly advertised their
       * capability as 0x3 instead of 0x11. As a workaround, also check for 0x3
       * to see if EPL is supported or not. We don't expect any other vendor to
       * have it incorrectly advertised as 0x3, thus it should be safe to add
       * this additional check as a workaround for this vendor
       */
      eplSupported = true;
      XLOG(INFO) << folly::sformat(
          "cmisModuleFirmwareDownload: Mod{:d} will use EPL memory for firmware download",
          moduleId_);
    }
  } else {
    XLOG(INFO) << folly::sformat(
        "cmisModuleFirmwareDownload: Mod{:d}: Could not get result from CDB Firmware Update Feature command",
        moduleId_);

    // Sometime when the optics is  in boot loader mode, this CDB command
    // fails. So fill in the header size if it is a known optics otherwise
    // return false
    startCommandPayloadSize = imageHeaderLen_;
    XLOG(INFO) << folly::sformat(
        "cmisModuleFirmwareDownload: Mod{:d}: Setting the module startCommandPayloadSize as {:d}",
        moduleId_,
        startCommandPayloadSize);
  }

  XLOG(INFO) << folly::sformat(
      "cmisModuleFirmwareDownload: Mod{:d}: Step 0: Got Start Command Payload Size as {:d}",
      moduleId_,
      startCommandPayloadSize);

  // Validate if the image length is greater than this. If not then our new
  // image is bad
  if (imageLen < startCommandPayloadSize) {
    XLOG(INFO) << folly::sformat(
        "cmisModuleFirmwareDownload: Mod{:d}: The image length {:d} is smaller than startCommandPayloadSize {:d}",
        moduleId_,
        imageLen,
        startCommandPayloadSize);
    return false;
  }

  // Step 1: Issue CDB command: Firmware Download start
  imageChunkLen = startCommandPayloadSize;
  commandBlock->createCdbCmdFwDownloadStart(
      startCommandPayloadSize, imageLen, imageOffset, imageBuf);

  // Run the CDB command
  status = commandBlock->cmisRunCdbCommand(bus_);
  if (!status) {
    // DOWNLOAD_START command failed
    XLOG(INFO) << folly::sformat(
        "cmisModuleFirmwareDownload: Mod{:d}: Could not run the CDB Firmware Download Start command",
        moduleId_);
    return false;
  }

  XLOG(INFO) << folly::sformat(
      "cmisModuleFirmwareDownload: Mod{:d}: Step 1: Issued Firmware download start command successfully",
      moduleId_);

  // Step 2: Issue CDB command: Firmware Download image

  XLOG(INFO) << folly::sformat(
      "cmisModuleFirmwareDownload: Mod{:d}: Step 2: Issuing Firmware Download Image command. Starting offset: {:d}",
      moduleId_,
      imageOffset);

  while (imageOffset < imageLen) {
    if (!eplSupported) {
      // Create CDB command block using internal LPL memory
      commandBlock->createCdbCmdFwDownloadImageLpl(
          startCommandPayloadSize,
          imageLen,
          imageBuf,
          imageOffset,
          imageChunkLen);
    } else {
      // Create CDB command block assuming external EPL memory
      commandBlock->createCdbCmdFwDownloadImageEpl(
          startCommandPayloadSize, imageLen, imageOffset, imageChunkLen);

      // Write the image payload to external EPL before invoking the command
      commandBlock->writeEplPayload(bus_, imageBuf, imageOffset, imageChunkLen);
    }

    // Run the CDB command
    status = commandBlock->cmisRunCdbCommand(bus_);
    if (!status) {
      // DOWNLOAD_IMAGE command failed
      XLOG(INFO) << folly::sformat(
          "cmisModuleFirmwareDownload: Mod{:d}: Could not run the CDB Firmware Download Image command",
          moduleId_);
      return false;
    }
    XLOG(INFO) << folly::sformat(
        "cmisModuleFirmwareDownload: Mod{:d}: Image wrote, offset: {:d} .. {:d}",
        moduleId_,
        imageOffset - imageChunkLen,
        imageOffset);
  }
  XLOG(INFO) << folly::sformat(
      "cmisModuleFirmwareDownload: Mod{:d}: Step 2: Issued Firmware Download Image successfully. Downloaded file size {:d}",
      moduleId_,
      imageOffset);

  // Step 3: Issue CDB command: Firmware download complete
  commandBlock->createCdbCmdFwDownloadComplete();

  // Run the CDB command
  status = commandBlock->cmisRunCdbCommand(bus_);
  if (!status) {
    // DOWNLOAD_COMPLETE command failed
    XLOG(INFO) << folly::sformat(
        "cmisModuleFirmwareDownload: Mod{:d}: Could not run the CDB Firmware Download Complete command",
        moduleId_);
    // Send the DOWNLOAD_ABORT command to CDB and return.
    return false;
  }

  XLOG(INFO) << folly::sformat(
      "cmisModuleFirmwareDownload: Mod{:d}: Step 3: Issued Firmware download complete command successfully",
      moduleId_);

  // Non App images like DSP image don't need last 2 steps (Run, Commit) for the
  // firmware download
  if (!appImage_) {
    return true;
  }

  // Step 4: Issue CDB command: Run the downloaded firmware
  commandBlock->createCdbCmdFwImageRun();

  // Run the CDB command
  // No need to check status because RUN command issues soft reset to CDB
  // so we can't check status here
  status = commandBlock->cmisRunCdbCommand(bus_);

  XLOG(INFO) << folly::sformat(
      "cmisModuleFirmwareDownload: Mod{:d}: Step 4: Issued Firmware download Run command successfully",
      moduleId_);

  usleep(2 * moduleDatapathInitDurationUsec);

  // Set the password to let the privileged operation of firmware download
  bus_->writeTransceiver(
      {TransceiverAccessParameter::ADDR_QSFP, kModulePasswordEntryReg, 4},
      msaPassword_.data(),
      POST_I2C_WRITE_NO_DELAY_US,
      kFwUpgrade);

  // Step 5: Issue CDB command: Commit the downloaded firmware
  commandBlock->createCdbCmdFwCommit();

  // Run the CDB command
  status = commandBlock->cmisRunCdbCommand(bus_);

  if (!status) {
    XLOG(INFO) << folly::sformat(
        "cmisModuleFirmwareDownload: Mod{:d}: Step 5: Issued Firmware commit command failed",
        moduleId_);
  } else {
    XLOG(INFO) << folly::sformat(
        "cmisModuleFirmwareDownload: Mod{:d}: Step 5: Issued Firmware commit command successful",
        moduleId_);
  }

  XLOG(INFO) << folly::sformat(
      "cmisModuleFirmwareDownload: Mod{:d}: CDB wait time = {:d} ms, Memory write time = {:d} ms",
      moduleId_,
      commandBlock->getCdbWaitTimeMsec(),
      commandBlock->getMemoryWriteTimeMsec());

  usleep(10 * moduleDatapathInitDurationUsec);

  // Set the password to let the privileged operation of firmware download
  bus_->writeTransceiver(
      {TransceiverAccessParameter::ADDR_QSFP, kModulePasswordEntryReg, 4},
      msaPassword_.data(),
      POST_I2C_WRITE_NO_DELAY_US,
      kFwUpgrade);

  // Print IO profiling info
  auto ioTiming = bus_->getI2cTimeProfileMsec();
  XLOG(INFO) << folly::sformat(
      "cmisModuleFirmwareDownload: Mod{:d}: Total IO access - Read time = {:d} ms, Write time = {:d} ms",
      moduleId_,
      ioTiming.first,
      ioTiming.second);
  bus_->i2cTimeProfilingEnd();

  return true;
}

/*
 * cmisModuleFirmwareUpgrade
 *
 * This function triggers the firmware download to a module. This specific
 * function does the firmware download for a module in a single thread under
 * the context of calling thread. This function validates the image file and
 * calls the above cmisModuleFirmwareDownload function to do firmware upgrade
 */
bool CmisFirmwareUpgrader::cmisModuleFirmwareUpgrade() {
  std::array<uint8_t, 2> versionNumber;
  bool result;

  XLOG(INFO) << folly::sformat(
      "cmisModuleFirmwareUpgrade: Mod{:d}: Called for port {:d}",
      moduleId_,
      moduleId_);

  // Call the firmware download operation with this image content
  result = cmisModuleFirmwareDownload(
      imageCursor_.data(), imageCursor_.totalLength());
  // Always revert the MSA password at the end. Certain commands like releasing
  // low power don't work when certain modules (like xdr4) are still in the CDB
  // mode which is the mode that's activated when the msa password is written
  // during firmware upgrade
  std::array<uint8_t, 4> msaPassword_;
  msaPassword_[0] = 0;
  msaPassword_[1] = 0;
  msaPassword_[2] = 0;
  msaPassword_[3] = 0;
  bus_->writeTransceiver(
      {TransceiverAccessParameter::ADDR_QSFP, kModulePasswordEntryReg, 4},
      msaPassword_.data(),
      POST_I2C_WRITE_NO_DELAY_US,
      kFwUpgrade);
  if (!result) {
    // If the download failed then print the message and return. No need
    // to do any recovery here
    XLOG(INFO) << folly::sformat(
        "cmisModuleFirmwareUpgrade: Mod{:d}: Firmware download function failed for the module",
        moduleId_);

    return false;
  }

  // Find out the current version running on module
  bus_->readTransceiver(
      {TransceiverAccessParameter::ADDR_QSFP, kfirmwareVersionReg, 2},
      versionNumber.data(),
      kFwUpgrade);
  XLOG(INFO) << folly::sformat(
      "cmisModuleFirmwareUpgrade: Mod{:d}: Module Active Firmware Revision now: {:d}.{:d}",
      moduleId_,
      versionNumber[0],
      versionNumber[1]);

  return true;
}

} // namespace facebook::fboss
