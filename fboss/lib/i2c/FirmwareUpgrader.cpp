// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/lib/i2c/FirmwareUpgrader.h"

#include <folly/Conv.h>
#include <folly/Exception.h>
#include <folly/File.h>
#include <folly/FileUtil.h>
#include <folly/Format.h>
#include <folly/Memory.h>
#include <folly/init/Init.h>
#include <folly/io/IOBuf.h>
#include <folly/io/async/EventBase.h>
#include <folly/logging/xlog.h>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <chrono>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sysexits.h>

#include <thread>
#include <utility>
#include <vector>

using folly::MutableByteRange;
using folly::StringPiece;
using std::make_pair;
using std::pair;
using std::chrono::seconds;
using std::chrono::steady_clock;
using namespace facebook::fboss;

namespace facebook::fboss {

// CDB command definitions
static constexpr uint16_t kCdbCommandFirmwareDownloadStart = 0x0101;
static constexpr uint16_t kCdbCommandFirmwareDownloadImage = 0x0103;
static constexpr uint16_t kCdbCommandFirmwareDownloadComplete = 0x0107;
static constexpr uint16_t kCdbCommandFirmwareDownloadRun = 0x0109;
static constexpr uint16_t kCdbCommandFirmwareDownloadCommit = 0x010a;
static constexpr uint16_t kCdbCommandFirmwareDownloadFeature = 0x0041;
static constexpr uint16_t kCdbCommandModuleQuery = 0x0000;

// CDB command status values
static constexpr uint8_t kCdbCommandStatusSuccess = 0x01;
static constexpr uint8_t kCdbCommandStatusBusyCmdCaptured = 0x81;
static constexpr uint8_t kCdbCommandStatusBusyCmdCheck = 0x82;
static constexpr uint8_t kCdbCommandStatusBusyCmdExec = 0x83;

constexpr int cdbCommandTimeoutUsec = 2000000;
constexpr int cdbCommandIntervalUsec = 100000;
constexpr int moduleDatapathInitDurationUsec = 5000000;

// CMIS firmware related register offsets
constexpr uint8_t kCdbCommandStatusReg = 37;
constexpr uint8_t kfirmwareVersionReg = 39;
constexpr uint8_t kModulePasswordEntryReg = 122;
constexpr uint8_t kPageSelectReg = 127;
constexpr uint8_t kCdbCommandMsbReg = 128;
constexpr uint8_t kCdbCommandLsbReg = 129;
constexpr uint8_t kCdbRlplLengthReg = 132;

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
    TransceiverI2CApi* bus,
    unsigned int modId,
    std::unique_ptr<FbossFirmware> fbossFirmware)
    : bus_(bus), moduleId_(modId), fbossFirmware_(std::move(fbossFirmware)) {
  // Check the FbossFirmware object first
  if (fbossFirmware_.get() == nullptr) {
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
  uint32_t msaPwVal = folly::to<uint32_t>(msaPwStr);
  msaPassword_[0] = (msaPwVal & 0xFF000000) >> 24;
  msaPassword_[1] = (msaPwVal & 0x00FF0000) >> 16;
  msaPassword_[2] = (msaPwVal & 0x0000FF00) >> 8;
  msaPassword_[3] = (msaPwVal & 0x000000FF);
}

/*
 * cmisRunCdbCommand
 *
 * This function runs a CDB command on a CMIS module. The CDB command can be
 * accessed on page 0x9f. The command block except the op-code needs to be
 * written first. The command op-code gets written at offset 128, 128. The
 * write to 129 causes command to run. After this wait for command status to
 * become successful. For some command the result is returned in CDB LPL memory
 */
bool CmisFirmwareUpgrader::cmisRunCdbCommand(CdbCommandBlock* commandBlock) {
  // Command block length is 8 plus lpl memory length
  int len = commandBlock->cdbFields_.cdbLplLength + 8;

  uint8_t page = 0x9f;
  bus_->moduleWrite(
      moduleId_, TransceiverI2CApi::ADDR_QSFP, kPageSelectReg, 1, &page);

  // Since we are going to write byte 2 to len-1 in the module CDB memory
  // without interpreting it so let's take uint8_t* pointer here and do it
  const uint8_t* buf = (uint8_t*)commandBlock;
  uint8_t bufIndex = 2;
  uint8_t regOffset = 130;

  // As per the I2c bus files it looks like we can't write more than 63 bytes
  // in one go. So we will divide the write operation in the chunk of 32 bytes
  // and do the write. We will divide the buf[2] to buf[len-1] block in chunk
  // of 32 bytes do one write for each chunk. For last chunk we will do one
  // write
  int WRITE_BLOCK_SIZE = 32;
  int numBlock = (len - bufIndex) / WRITE_BLOCK_SIZE;

  auto i2cWriteAndContinue = [this](
                                 unsigned int module,
                                 uint8_t i2cAddress,
                                 int offset,
                                 int length,
                                 const uint8_t* buf) {
    try {
      this->bus_->moduleWrite(module, i2cAddress, offset, length, buf);
    } catch (const std::exception& e) {
      XLOG(INFO) << "write() raised exception: Sleep for 100ms and continue";
      usleep(cdbCommandIntervalUsec);
    }
  };

  for (int i = 0; i < numBlock; i++) {
    i2cWriteAndContinue(
        moduleId_,
        TransceiverI2CApi::ADDR_QSFP,
        regOffset,
        WRITE_BLOCK_SIZE,
        &buf[bufIndex]);

    regOffset += WRITE_BLOCK_SIZE;
    bufIndex += WRITE_BLOCK_SIZE;
  }

  // Write last chunk
  if (bufIndex < len) {
    i2cWriteAndContinue(
        moduleId_,
        TransceiverI2CApi::ADDR_QSFP,
        regOffset,
        len - bufIndex,
        &buf[bufIndex]);
  }

  // Now write the first two byte which is CDB command register. The write to
  // command register LSB will trigger the actual command

  i2cWriteAndContinue(
      moduleId_, TransceiverI2CApi::ADDR_QSFP, kCdbCommandMsbReg, 1, &buf[0]);
  i2cWriteAndContinue(
      moduleId_, TransceiverI2CApi::ADDR_QSFP, kCdbCommandLsbReg, 1, &buf[1]);

  // Special handling for RUN command
  if (commandBlock->cdbFields_.cdbCommandCode ==
      htons(kCdbCommandFirmwareDownloadRun)) {
    return true;
  }

  // Now read the CDB command status register till the status becomes success
  // or fail
  uint8_t status = 0;
  auto currTime = std::chrono::steady_clock::now();
  auto finishTime = currTime + std::chrono::microseconds(cdbCommandTimeoutUsec);
  usleep(cdbCommandIntervalUsec);
  while (true) {
    try {
      bus_->moduleRead(
          moduleId_,
          TransceiverI2CApi::ADDR_QSFP,
          kCdbCommandStatusReg,
          1,
          &status);
    } catch (const std::exception& e) {
      XLOG(INFO) << "read() raised exception: Sleep for 100ms and continue";
      usleep(cdbCommandIntervalUsec);
      status = kCdbCommandStatusBusyCmdCaptured;
    }
    if (status != kCdbCommandStatusBusyCmdCaptured &&
        status != kCdbCommandStatusBusyCmdCheck &&
        status != kCdbCommandStatusBusyCmdExec) {
      break;
    }

    currTime = std::chrono::steady_clock::now();
    if (currTime > finishTime) {
      break;
    }
    usleep(cdbCommandIntervalUsec);
  }

  if (status != kCdbCommandStatusSuccess) {
    XLOG(INFO) << folly::sformat(
        "cmisRunCdbCommand: Mod{:d}: CDB command {:#x}.{:#x} not successful, status {:#x}",
        moduleId_,
        buf[0],
        buf[1],
        status);
    return false;
  }

  // Check if the CDB block has returned some information in the LPL memory

  bus_->moduleRead(
      moduleId_,
      TransceiverI2CApi::ADDR_QSFP,
      kCdbRlplLengthReg,
      1,
      &commandBlock->cdbFields_.cdbRlplLength);

  auto i2cReadWithRetry = [this](
                              unsigned int module,
                              uint8_t i2cAddress,
                              int offset,
                              int length,
                              uint8_t* buf) {
    try {
      this->bus_->moduleRead(module, i2cAddress, offset, length, buf);
    } catch (const std::exception& e) {
      XLOG(INFO) << "read() raised exception: Sleep for 100ms and retry";
      usleep(cdbCommandIntervalUsec);
      this->bus_->moduleRead(module, i2cAddress, offset, length, buf);
    }
  };

  // If the CDB has returned some data (cdbRlplLength>0 indicates this) then
  // read the rlpl data from CDB's RLPL memory to the same given structure
  if (commandBlock->cdbFields_.cdbRlplLength > 0) {
    // While reading we can read upto 128 bytes so there no need to do
    // chunk of read here
    // Read the CDB's LPL memory content in our commandBlocks->cdbLplMemory
    i2cReadWithRetry(
        moduleId_,
        TransceiverI2CApi::ADDR_QSFP,
        136,
        commandBlock->cdbFields_.cdbRlplLength,
        commandBlock->cdbFields_.cdbLplMemory.cdbLplFlatMemory);
  }
  return true;
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

  XLOG(INFO) << folly::sformat(
      "cmisModuleFirmwareDownload: Mod{:d}: Starting to download the image with length {:d}",
      moduleId_,
      imageLen);

  // Set the password to let the privileged operation of firmware download
  bus_->moduleWrite(
      moduleId_,
      TransceiverI2CApi::ADDR_QSFP,
      kModulePasswordEntryReg,
      4,
      msaPassword_.data());

  CdbCommandBlock commandBlockBuf;
  CdbCommandBlock* commandBlock = &commandBlockBuf;

  // Basic validation first. Check if the firmware download is allowed by
  // issuing the Query command to CDB
  commandBlock->createCdbCmdModuleQuery();
  // Run the CDB command
  status = cmisRunCdbCommand(commandBlock);
  if (status) {
    // Query result will be in LPL memory at byte offset 2
    if (commandBlock->cdbFields_.cdbRlplLength >= 3) {
      if (commandBlock->cdbFields_.cdbLplMemory.cdbLplFlatMemory[2] == 0) {
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
  status = cmisRunCdbCommand(commandBlock);

  // If the CDB command is successfull then the Start Command Payload Size is
  // returned by CDB in LPL memory at offset 2

  if (status && commandBlock->cdbFields_.cdbRlplLength >= 3) {
    // Get the firmware header size from CDB
    startCommandPayloadSize =
        commandBlock->cdbFields_.cdbLplMemory.cdbLplFlatMemory[2];
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
  status = cmisRunCdbCommand(commandBlock);
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
    commandBlock->createCdbCmdFwDownloadImage(
        startCommandPayloadSize,
        imageLen,
        imageBuf,
        imageOffset,
        imageChunkLen);

    // Run the CDB command
    status = cmisRunCdbCommand(commandBlock);
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
  status = cmisRunCdbCommand(commandBlock);
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

  // Step 4: Issue CDB command: Run the downloaded firmware
  commandBlock->createCdbCmdFwImageRun();

  // Run the CDB command
  // No need to check status because RUN command issues soft reset to CDB
  // so we can't check status here
  status = cmisRunCdbCommand(commandBlock);

  XLOG(INFO) << folly::sformat(
      "cmisModuleFirmwareDownload: Mod{:d}: Step 4: Issued Firmware download Run command successfully",
      moduleId_);

  usleep(2 * moduleDatapathInitDurationUsec);

  // Set the password to let the privileged operation of firmware download
  bus_->moduleWrite(
      moduleId_,
      TransceiverI2CApi::ADDR_QSFP,
      kModulePasswordEntryReg,
      4,
      msaPassword_.data());

  // Step 5: Issue CDB command: Commit the downloaded firmware
  commandBlock->createCdbCmdFwCommit();

  // Run the CDB command
  status = cmisRunCdbCommand(commandBlock);

  if (!status) {
    XLOG(INFO) << folly::sformat(
        "cmisModuleFirmwareDownload: Mod{:d}: Step 5: Issued Firmware commit command failed",
        moduleId_);
  } else {
    XLOG(INFO) << folly::sformat(
        "cmisModuleFirmwareDownload: Mod{:d}: Step 5: Issued Firmware commit command successful",
        moduleId_);
  }

  usleep(10 * moduleDatapathInitDurationUsec);

  // Set the password to let the privileged operation of firmware download
  bus_->moduleWrite(
      moduleId_,
      TransceiverI2CApi::ADDR_QSFP,
      kModulePasswordEntryReg,
      4,
      msaPassword_.data());

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
  if (!result) {
    // If the download failed then print the message and return. No need
    // to do any recovery here
    XLOG(INFO) << folly::sformat(
        "cmisModuleFirmwareUpgrade: Mod{:d}: Firmware download function failed for the module",
        moduleId_);

    return false;
  }

  // Find out the current version running on module
  bus_->moduleRead(
      moduleId_,
      TransceiverI2CApi::ADDR_QSFP,
      kfirmwareVersionReg,
      2,
      versionNumber.data());
  XLOG(INFO) << folly::sformat(
      "cmisModuleFirmwareUpgrade: Mod{:d}: Module Active Firmware Revision now: {:d}.{:d}",
      moduleId_,
      versionNumber[0],
      versionNumber[1]);

  return true;
}

/*
 * createCdbCmdFwDownloadStart
 *
 * Creates CDB firmware download start command block. This command puts the
 * image header in command block lpl memory, overall image length. The function
 * returns the current image offset after reading the header
 */
void CmisFirmwareUpgrader::CdbCommandBlock::createCdbCmdFwDownloadStart(
    uint8_t startCommandPayloadSize,
    int imageLen,
    int& imageOffset,
    const uint8_t* imageBuf) {
  resetCdbBlock();
  cdbFields_.cdbCommandCode = htons(kCdbCommandFirmwareDownloadStart);
  cdbFields_.cdbEplLength = 0;

  int imageChunkLen = startCommandPayloadSize;
  cdbFields_.cdbLplLength = imageChunkLen + 8;

  cdbFields_.cdbLplMemory.cdbFwDnldStartData.cdbImageSize = htonl(imageLen);

  for (imageOffset = 0; imageOffset < imageChunkLen; imageOffset++) {
    cdbFields_.cdbLplMemory.cdbFwDnldStartData.cdbImageHeader[imageOffset] =
        imageBuf[imageOffset];
  }

  cdbFields_.cdbChecksum = onesComplementSum(cdbFields_.cdbLplLength + 8);
}

/*
 * createCdbCmdFwDownloadImage
 *
 * This function creates CDB command block for firmware image download. Upto
 * 116 bytes of firmware image is put in the lpl_memory region of this command
 * block. The image offset after reading the given number of bytes is returned
 * from this function.
 */
void CmisFirmwareUpgrader::CdbCommandBlock::createCdbCmdFwDownloadImage(
    uint8_t startCommandPayloadSize,
    int imageLen,
    const uint8_t* imageBuf,
    int& imageOffset,
    int& imageChunkLen) {
  resetCdbBlock();
  cdbFields_.cdbCommandCode = htons(kCdbCommandFirmwareDownloadImage);
  cdbFields_.cdbEplLength = 0;

  imageChunkLen = (imageLen - imageOffset > 116) ? 116 : imageLen - imageOffset;
  cdbFields_.cdbLplLength = imageChunkLen + 4;
  cdbFields_.cdbLplMemory.cdbFwDnldImageData.address =
      htonl(imageOffset - startCommandPayloadSize);

  for (int i = 0; i < imageChunkLen; i++, imageOffset++) {
    cdbFields_.cdbLplMemory.cdbFwDnldImageData.image[i] = imageBuf[imageOffset];
  }

  cdbFields_.cdbChecksum = onesComplementSum(cdbFields_.cdbLplLength + 8);
}

/*
 * createCdbCmdFwDownloadComplete
 *
 * This function creates the CDB command block for firmware download complete
 * command.
 */
void CmisFirmwareUpgrader::CdbCommandBlock::createCdbCmdFwDownloadComplete() {
  resetCdbBlock();
  cdbFields_.cdbCommandCode = htons(kCdbCommandFirmwareDownloadComplete);
  cdbFields_.cdbEplLength = 0;

  cdbFields_.cdbLplLength = 0;
  cdbFields_.cdbChecksum = onesComplementSum(8);
}

/*
 * createCdbCmdFwImageRun
 *
 * This function creates the CDB command block for the new firmware image run
 * command. This creates "run immediate" command.
 */
void CmisFirmwareUpgrader::CdbCommandBlock::createCdbCmdFwImageRun() {
  resetCdbBlock();
  cdbFields_.cdbCommandCode = htons(kCdbCommandFirmwareDownloadRun);
  cdbFields_.cdbEplLength = 0;

  cdbFields_.cdbLplLength = 4;
  // No delay needed before running this ccommand
  cdbFields_.cdbLplMemory.cdbLplFlatMemory[2] = 0;
  cdbFields_.cdbLplMemory.cdbLplFlatMemory[3] = 0;
  cdbFields_.cdbChecksum = onesComplementSum(cdbFields_.cdbLplLength + 8);
}

/*
 * createCdbCmdFwCommit
 *
 * This function creates the firmware commit command. This will commit the
 * previously downloaded image to module's memory. Thsi command works
 * differently in different module.
 */
void CmisFirmwareUpgrader::CdbCommandBlock::createCdbCmdFwCommit() {
  resetCdbBlock();
  cdbFields_.cdbCommandCode = htons(kCdbCommandFirmwareDownloadCommit);
  cdbFields_.cdbEplLength = 0;

  cdbFields_.cdbLplLength = 0;
  cdbFields_.cdbChecksum = onesComplementSum(cdbFields_.cdbLplLength + 8);
}

/*
 * createCdbCmdModuleQuery
 *
 * Creates CDB Module query command. This command returns the data with module
 * and its firmware related information.
 * Note: When the module firmware is corrupted or it is in bootloader mode
 * then this command may fail.
 */
void CmisFirmwareUpgrader::CdbCommandBlock::createCdbCmdModuleQuery() {
  resetCdbBlock();
  cdbFields_.cdbCommandCode = htons(kCdbCommandModuleQuery);
  cdbFields_.cdbEplLength = 0;

  cdbFields_.cdbLplLength = 2;
  cdbFields_.cdbChecksum = onesComplementSum(cdbFields_.cdbLplLength + 8);
}

/*
 * createCdbCmdGetFwFeatureInfo
 *
 * Creates CDB Firmware Feature Info get command block. This command returns
 * important information about firmware upgrade feature on the module.
 * Note: When the module firmware is corrupted or it is in bootloader mode
 * then this command may fail.
 */
void CmisFirmwareUpgrader::CdbCommandBlock::createCdbCmdGetFwFeatureInfo() {
  resetCdbBlock();
  cdbFields_.cdbCommandCode = htons(kCdbCommandFirmwareDownloadFeature);
  cdbFields_.cdbEplLength = 0;

  cdbFields_.cdbLplLength = 0;
  cdbFields_.cdbChecksum = onesComplementSum(cdbFields_.cdbLplLength + 5);
}

/*
 * onesComplementSum
 *
 * This function provides one's complemenet sum for a range of bytes. This is
 * required for the CDB command because the whole CDB block is protected by one
 * complement sum
 */
uint8_t CmisFirmwareUpgrader::CdbCommandBlock::onesComplementSum(int len) {
  uint8_t* buf = (uint8_t*)this;
  uint16_t sum = 0;
  uint8_t result;
  int i;

  for (i = 0; i < len; i++) {
    sum += buf[i];
  }
  sum = ~sum;
  result = sum & 0xff;
  return result;
}

} // namespace facebook::fboss
