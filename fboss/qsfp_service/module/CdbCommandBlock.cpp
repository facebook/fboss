// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/qsfp_service/module/CdbCommandBlock.h"

#include <chrono>

#include <folly/Format.h>
#include <folly/logging/xlog.h>

#include "fboss/qsfp_service/module/TransceiverImpl.h"

#include "fboss/qsfp_service/module/cmis/gen-cpp2/cmis_types.h"

namespace facebook::fboss {

// CDB command definitions
static constexpr uint16_t kCdbCommandFirmwareDownloadStart = 0x0101;
static constexpr uint16_t kCdbCommandFirmwareDownloadImageLpl = 0x0103;
static constexpr uint16_t kCdbCommandFirmwareDownloadImageEpl = 0x0104;
static constexpr uint16_t kCdbCommandFirmwareDownloadComplete = 0x0107;
static constexpr uint16_t kCdbCommandFirmwareDownloadRun = 0x0109;
static constexpr uint16_t kCdbCommandFirmwareDownloadCommit = 0x010a;
static constexpr uint16_t kCdbCommandFirmwareDownloadFeature = 0x0041;
static constexpr uint16_t kCdbCommandModuleQuery = 0x0000;
static constexpr uint16_t kCdbCommandSymbolErrorHistogram = 0x9000;
static constexpr uint16_t kCdbCommandRxErrorHistogram = 0x9001;

// CDB command status values
static constexpr uint8_t kCdbCommandStatusSuccess = 0x01;
static constexpr uint8_t kCdbCommandStatusBusyUnknown = 0x80;
static constexpr uint8_t kCdbCommandStatusBusyCmdCaptured = 0x81;
static constexpr uint8_t kCdbCommandStatusBusyCmdCheck = 0x82;
static constexpr uint8_t kCdbCommandStatusBusyCmdExec = 0x83;

// All the CDB commands finishes well within 2 seconds but one particular DSP
// firmware download takes too much time. During this each CDB command takes
// average 5 seconds to increasing this CDB timeout value to 10 seconds
constexpr int cdbCommandTimeoutUsec = 10000000;
constexpr int cdbCommandErrorIntervalUsec = 100000;
constexpr int cdbCommandStatusPollIntervalUsec = 10000;

// CMIS firmware related register offsets
constexpr uint8_t kCdbCommandStatusReg = 37;
constexpr uint8_t kModulePasswordEntryReg = 122;
constexpr uint8_t kPageSelectReg = 127;
constexpr uint8_t kCdbCommandMsbReg = 128;
constexpr uint8_t kCdbCommandLsbReg = 129;
constexpr uint8_t kCdbRlplLengthReg = 134;

// CMIS command
constexpr int kCmisCommand = static_cast<int>(CmisField::CDB_COMMAND);

/*
 * i2cWriteAndContinue
 * Local function to perform the i2c write to module and continue in case of
 * error. This is used during firmware upgrade where huge number of i2c
 * transactions are going on
 */
void CdbCommandBlock::i2cWriteAndContinue(
    TransceiverImpl* bus,
    uint8_t i2cAddress,
    int offset,
    int length,
    const uint8_t* buf) {
  auto startTime = std::chrono::steady_clock::now();

  try {
    // Sleep for 5 msec after every CDB memory block write to let the next
    // command run successfully. Some of the optics need this delay
    /* sleep override */
    bus->writeTransceiver(
        {i2cAddress, offset, length},
        buf,
        POST_I2C_WRITE_DELAY_CDB_US,
        kCmisCommand);
  } catch (const std::exception& e) {
    XLOG(INFO) << "write() raised exception: Sleep for 100ms and continue: "
               << e.what();
    /* sleep override */
    usleep(cdbCommandErrorIntervalUsec);
  }

  auto writeTime = std::chrono::steady_clock::now() - startTime;
  memoryWriteTime_ +=
      std::chrono::duration_cast<std::chrono::milliseconds>(writeTime) -
      std::chrono::milliseconds(POST_I2C_WRITE_DELAY_CDB_US / 1000);
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
bool CdbCommandBlock::cmisRunCdbCommand(TransceiverImpl* bus) {
  // Command block length is 8 plus lpl memory length
  int len = this->cdbFields_.cdbLplLength + 8;

  uint8_t page = 0x9f;
  bus->writeTransceiver(
      {TransceiverAccessParameter::ADDR_QSFP, kPageSelectReg, 1},
      &page,
      POST_I2C_WRITE_NO_DELAY_US,
      kCmisCommand);

  // Since we are going to write byte 2 to len-1 in the module CDB memory
  // without interpreting it so let's take uint8_t* pointer here and do it
  const uint8_t* buf = (uint8_t*)this;
  uint8_t bufIndex = 2;
  uint8_t regOffset = 130;

  // As per the I2c bus files it looks like we can't write more than 63 bytes
  // in one go. So we will divide the write operation in the chunk of 32 bytes
  // and do the write. We will divide the buf[2] to buf[len-1] block in chunk
  // of 32 bytes do one write for each chunk. For last chunk we will do one
  // write
  int WRITE_BLOCK_SIZE = 32;
  int numBlock = (len - bufIndex) / WRITE_BLOCK_SIZE;

  for (int i = 0; i < numBlock; i++) {
    i2cWriteAndContinue(
        bus,
        TransceiverAccessParameter::ADDR_QSFP,
        regOffset,
        WRITE_BLOCK_SIZE,
        &buf[bufIndex]);

    regOffset += WRITE_BLOCK_SIZE;
    bufIndex += WRITE_BLOCK_SIZE;
  }

  // Write last chunk
  if (bufIndex < len) {
    i2cWriteAndContinue(
        bus,
        TransceiverAccessParameter::ADDR_QSFP,
        regOffset,
        len - bufIndex,
        &buf[bufIndex]);
  }

  // Now write the first two byte which is CDB command register. The write to
  // command register LSB will trigger the actual command

  i2cWriteAndContinue(
      bus,
      TransceiverAccessParameter::ADDR_QSFP,
      kCdbCommandMsbReg,
      1,
      &buf[0]);
  i2cWriteAndContinue(
      bus,
      TransceiverAccessParameter::ADDR_QSFP,
      kCdbCommandLsbReg,
      1,
      &buf[1]);

  // Special handling for RUN command
  if (this->cdbFields_.cdbCommandCode ==
      htons(kCdbCommandFirmwareDownloadRun)) {
    return true;
  }

  // Now read the CDB command status register till the status becomes success
  // or fail
  uint8_t status = 0;
  auto startTime = std::chrono::steady_clock::now();
  auto finishTime =
      startTime + std::chrono::microseconds(cdbCommandTimeoutUsec);
  /* sleep override */
  usleep(cdbCommandStatusPollIntervalUsec);
  while (true) {
    try {
      bus->readTransceiver(
          {TransceiverAccessParameter::ADDR_QSFP, kCdbCommandStatusReg, 1},
          &status,
          kCmisCommand);
    } catch (const std::exception&) {
      XLOG(INFO) << folly::sformat(
          "cmisRunCdbCommand Mod{:d}: read status raised exception: Sleep for 100ms and continue",
          bus->getNum());
      /* sleep override */
      usleep(cdbCommandErrorIntervalUsec);
      status = kCdbCommandStatusBusyCmdCaptured;
    }
    if (status != kCdbCommandStatusBusyCmdCaptured &&
        status != kCdbCommandStatusBusyCmdCheck &&
        status != kCdbCommandStatusBusyCmdExec &&
        status != kCdbCommandStatusBusyUnknown) {
      break;
    }

    auto currTime = std::chrono::steady_clock::now();
    if (currTime > finishTime) {
      XLOG(INFO) << folly::sformat(
          "cmisRunCdbCommand Mod{:d}: Command status still busy, breaking out",
          bus->getNum());
      break;
    }
    /* sleep override */
    usleep(cdbCommandStatusPollIntervalUsec);
  }

  auto cdbWaitTime = std::chrono::steady_clock::now() - startTime;
  commandBlockCdbWaitTime_ +=
      std::chrono::duration_cast<std::chrono::milliseconds>(cdbWaitTime);

  if (status != kCdbCommandStatusSuccess) {
    auto modId = bus->getNum();
    XLOG(INFO) << folly::sformat(
        "cmisRunCdbCommand: Mod{:d}: CDB command {:#x}.{:#x} not successful, status {:#x}",
        modId,
        buf[0],
        buf[1],
        status);
    return false;
  }

  // Check if the CDB block has returned some information in the LPL memory

  bus->readTransceiver(
      {TransceiverAccessParameter::ADDR_QSFP, kCdbRlplLengthReg, 1},
      &this->cdbFields_.cdbRlplLength,
      kCmisCommand);

  auto i2cReadWithRetry = [&](uint8_t i2cAddress,
                              int offset,
                              int length,
                              uint8_t* buf) {
    try {
      bus->readTransceiver({i2cAddress, offset, length}, buf, kCmisCommand);
    } catch (const std::exception&) {
      XLOG(INFO) << folly::sformat(
          "cmisRunCdbCommand Mod{:d}: read generic raised exception: Sleep for 100ms and retry",
          bus->getNum());
      /* sleep override */
      usleep(cdbCommandErrorIntervalUsec);
      bus->readTransceiver({i2cAddress, offset, length}, buf, kCmisCommand);
    }
  };

  // If the CDB has returned some data (cdbRlplLength>0 indicates this) then
  // read the rlpl data from CDB's RLPL memory to the same given structure
  if (this->cdbFields_.cdbRlplLength > 0) {
    // While reading we can read upto 128 bytes so there no need to do
    // chunk of read here
    // Read the CDB's LPL memory content in our commandBlocks->cdbLplMemory
    uint8_t replyLength =
        std::min(this->cdbFields_.cdbRlplLength, kCdbLplMemoryLength);

    i2cReadWithRetry(
        TransceiverAccessParameter::ADDR_QSFP,
        136,
        replyLength,
        this->cdbFields_.cdbLplMemory.cdbLplFlatMemory);
  }
  return true;
}

/*
 * createCdbCmdFwDownloadStart
 *
 * Creates CDB firmware download start command block. This command puts the
 * image header in command block lpl memory, overall image length. The function
 * returns the current image offset after reading the header
 */
void CdbCommandBlock::createCdbCmdFwDownloadStart(
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

  cdbFields_.cdbChecksum = onesComplementSum();
}

/*
 * createCdbCmdFwDownloadImageLpl
 *
 * This function creates CDB command block for firmware image download. Upto
 * 116 bytes of firmware image is put in the lpl_memory region of this command
 * block. The image offset after reading the given number of bytes is returned
 * from this function.
 */
void CdbCommandBlock::createCdbCmdFwDownloadImageLpl(
    uint8_t startCommandPayloadSize,
    int imageLen,
    const uint8_t* imageBuf,
    int& imageOffset,
    int& imageChunkLen) {
  resetCdbBlock();
  cdbFields_.cdbCommandCode = htons(kCdbCommandFirmwareDownloadImageLpl);
  cdbFields_.cdbEplLength = 0;

  imageChunkLen = (imageLen - imageOffset > 116) ? 116 : imageLen - imageOffset;
  cdbFields_.cdbLplLength = imageChunkLen + 4;
  cdbFields_.cdbLplMemory.cdbFwDnldImageData.address =
      htonl(imageOffset - startCommandPayloadSize);

  for (int i = 0; i < imageChunkLen; i++, imageOffset++) {
    cdbFields_.cdbLplMemory.cdbFwDnldImageData.image[i] = imageBuf[imageOffset];
  }

  cdbFields_.cdbChecksum = onesComplementSum();
}

/*
 * createCdbCmdFwDownloadImageEpl
 *
 * This function creates CDB command block for firmware image download. This
 * function assumes that the firmware download will happen through module
 * EPL memory which is 2048 bytes external SRAM (faster) block.
 */
void CdbCommandBlock::createCdbCmdFwDownloadImageEpl(
    uint8_t startCommandPayloadSize,
    int imageLen,
    int& imageOffset,
    int& imageChunkLen) {
  resetCdbBlock();
  cdbFields_.cdbCommandCode = htons(kCdbCommandFirmwareDownloadImageEpl);

  imageChunkLen =
      (imageLen - imageOffset > 2048) ? 2048 : imageLen - imageOffset;
  cdbFields_.cdbEplLength = htons(imageChunkLen);

  cdbFields_.cdbLplLength = 4;
  cdbFields_.cdbLplMemory.cdbFwDnldImageData.address =
      htonl(imageOffset - startCommandPayloadSize);

  cdbFields_.cdbChecksum = onesComplementSum();
}

/*
 * writeEplPayload
 *
 * This function writes the image to EPL payload memory which is extrenal to
 * CDB but used by CDB. This memory needs to be written prior to issuing the
 * CDB command
 */
void CdbCommandBlock::writeEplPayload(
    TransceiverImpl* bus,
    const uint8_t* imageBuf,
    int& imageOffset,
    int imageChunkLen) {
  int WRITE_BLOCK_SIZE = 32;
  int finalImageOffset = imageOffset + imageChunkLen;
  uint8_t currPage = 0xa0;
  int currPageOffset = 128;

  // Set the page as 0xa0
  i2cWriteAndContinue(
      bus, TransceiverAccessParameter::ADDR_QSFP, kPageSelectReg, 1, &currPage);

  while (imageOffset < finalImageOffset) {
    // If the cuurent page offset has gone above 256 then move over to the
    // next page [a0..af]
    if (currPageOffset >= 256) {
      currPageOffset = 128;
      currPage++;
      i2cWriteAndContinue(
          bus,
          TransceiverAccessParameter::ADDR_QSFP,
          kPageSelectReg,
          1,
          &currPage);
    }
    int i2cChunk = ((finalImageOffset - imageOffset) > WRITE_BLOCK_SIZE)
        ? WRITE_BLOCK_SIZE
        : (finalImageOffset - imageOffset);
    i2cWriteAndContinue(
        bus,
        TransceiverAccessParameter::ADDR_QSFP,
        currPageOffset,
        i2cChunk,
        &imageBuf[imageOffset]);

    imageOffset += i2cChunk;
    currPageOffset += i2cChunk;
  }
}

/*
 * createCdbCmdFwDownloadComplete
 *
 * This function creates the CDB command block for firmware download complete
 * command.
 */
void CdbCommandBlock::createCdbCmdFwDownloadComplete() {
  resetCdbBlock();
  cdbFields_.cdbCommandCode = htons(kCdbCommandFirmwareDownloadComplete);
  cdbFields_.cdbEplLength = 0;

  cdbFields_.cdbLplLength = 0;
  cdbFields_.cdbChecksum = onesComplementSum();
}

/*
 * createCdbCmdFwImageRun
 *
 * This function creates the CDB command block for the new firmware image run
 * command. This creates "run immediate" command.
 */
void CdbCommandBlock::createCdbCmdFwImageRun() {
  resetCdbBlock();
  cdbFields_.cdbCommandCode = htons(kCdbCommandFirmwareDownloadRun);
  cdbFields_.cdbEplLength = 0;

  cdbFields_.cdbLplLength = 4;
  // No delay needed before running this ccommand
  cdbFields_.cdbLplMemory.cdbLplFlatMemory[2] = 0;
  cdbFields_.cdbLplMemory.cdbLplFlatMemory[3] = 0;
  cdbFields_.cdbChecksum = onesComplementSum();
}

/*
 * createCdbCmdFwCommit
 *
 * This function creates the firmware commit command. This will commit the
 * previously downloaded image to module's memory. Thsi command works
 * differently in different module.
 */
void CdbCommandBlock::createCdbCmdFwCommit() {
  resetCdbBlock();
  cdbFields_.cdbCommandCode = htons(kCdbCommandFirmwareDownloadCommit);
  cdbFields_.cdbEplLength = 0;

  cdbFields_.cdbLplLength = 0;
  cdbFields_.cdbChecksum = onesComplementSum();
}

/*
 * createCdbCmdModuleQuery
 *
 * Creates CDB Module query command. This command returns the data with module
 * and its firmware related information.
 * Note: When the module firmware is corrupted or it is in bootloader mode
 * then this command may fail.
 */
void CdbCommandBlock::createCdbCmdModuleQuery() {
  resetCdbBlock();
  cdbFields_.cdbCommandCode = htons(kCdbCommandModuleQuery);
  cdbFields_.cdbEplLength = 0;

  cdbFields_.cdbLplLength = 2;
  cdbFields_.cdbChecksum = onesComplementSum();
}

/*
 * createCdbCmdGetFwFeatureInfo
 *
 * Creates CDB Firmware Feature Info get command block. This command returns
 * important information about firmware upgrade feature on the module.
 * Note: When the module firmware is corrupted or it is in bootloader mode
 * then this command may fail.
 */
void CdbCommandBlock::createCdbCmdGetFwFeatureInfo() {
  resetCdbBlock();
  cdbFields_.cdbCommandCode = htons(kCdbCommandFirmwareDownloadFeature);
  cdbFields_.cdbEplLength = 0;

  cdbFields_.cdbLplLength = 0;
  cdbFields_.cdbChecksum = onesComplementSum();
}

/*
 * createCdbCmdSymbolErrorHistogram
 *
 * This function creates the CDB command block to get the Symbol Error Histogram
 * data for a data path on Media/Host side. The datapath id is 0 based and
 * corresponds to the software port
 */
void CdbCommandBlock::createCdbCmdSymbolErrorHistogram(
    uint8_t datapathId,
    bool mediaSide) {
  resetCdbBlock();
  cdbFields_.cdbCommandCode = htons(kCdbCommandSymbolErrorHistogram);
  cdbFields_.cdbEplLength = 0;
  cdbFields_.cdbLplLength = 3;

  cdbFields_.cdbLplMemory.cdbLplFlatMemory[1] = datapathId;
  cdbFields_.cdbLplMemory.cdbLplFlatMemory[2] = mediaSide ? 0x0 : 0x1;
  cdbFields_.cdbChecksum = onesComplementSum();
}

/*
 * createCdbCmdRxErrorHistogram
 *
 * This function creates the CDB command block to get the Rx Error Histogram
 * data for a lane on Media/Host side
 */
void CdbCommandBlock::createCdbCmdRxErrorHistogram(
    uint8_t laneId,
    bool mediaSide) {
  resetCdbBlock();
  cdbFields_.cdbCommandCode = htons(kCdbCommandRxErrorHistogram);
  cdbFields_.cdbEplLength = 0;
  cdbFields_.cdbLplLength = 4;

  cdbFields_.cdbLplMemory.cdbLplFlatMemory[1] = laneId;
  cdbFields_.cdbLplMemory.cdbLplFlatMemory[2] = mediaSide ? 0x0 : 0x1;
  cdbFields_.cdbChecksum = onesComplementSum();
}

/*
 * createCdbCmdGeneric
 *
 * Creates CDB firmware download start command block. This command puts the
 * image header in command block lpl memory, overall image length. The function
 * returns the current image offset after reading the header
 */
void CdbCommandBlock::createCdbCmdGeneric(
    uint16_t commandCode,
    std::vector<uint8_t>& lplData) {
  resetCdbBlock();
  cdbFields_.cdbCommandCode = htons(commandCode);
  cdbFields_.cdbEplLength = 0;

  cdbFields_.cdbLplLength = lplData.size();

  int memIndex = 0;
  for (auto& lplByte : lplData) {
    cdbFields_.cdbLplMemory.cdbLplFlatMemory[memIndex++] = lplByte;
  }

  cdbFields_.cdbChecksum = onesComplementSum();
}

/*
 * getResponseData
 *
 * Get the CDB response data from module and return to caller.
 */
uint8_t CdbCommandBlock::getResponseData(uint8_t** pResponse) {
  *pResponse = cdbFields_.cdbLplMemory.cdbLplFlatMemory;
  return cdbFields_.cdbRlplLength;
}

/*
 * onesComplementSum
 *
 * This function provides one's complemenet sum for a range of bytes. This is
 * required for the CDB command because the whole CDB block is protected by one
 * complement sum
 */
uint8_t CdbCommandBlock::onesComplementSum() {
  uint8_t* buf = (uint8_t*)&cdbFields_;
  uint16_t sum = 0;
  uint8_t result;
  int i;
  int len = cdbFields_.cdbLplLength + 8;

  for (i = 0; i < len; i++) {
    sum += buf[i];
  }
  sum = ~sum;
  result = sum & 0xff;
  return result;
}

/*
 * selectCdbPage
 *
 * Selects the CDB page (0x9f) for the module
 */
void CdbCommandBlock::selectCdbPage(TransceiverImpl* bus) {
  uint8_t page = 0x9f;
  bus->writeTransceiver(
      {TransceiverAccessParameter::ADDR_QSFP, kPageSelectReg, 1},
      &page,
      POST_I2C_WRITE_NO_DELAY_US,
      kCmisCommand);
}

/*
 * setMsaPassword
 *
 * Sets the MSA password for the module. The caller needs to send 4 byte
 * password in big-endian format
 */
void CdbCommandBlock::setMsaPassword(TransceiverImpl* bus, uint32_t msaPw) {
  std::array<uint8_t, 4> msaPwArray;
  for (int i = 0; i < 4; i++) {
    msaPwArray[i] = (msaPw >> (3 - i) * 8) & 0xFF;
  }
  bus->writeTransceiver(
      {TransceiverAccessParameter::ADDR_QSFP, kModulePasswordEntryReg, 4},
      msaPwArray.data(),
      POST_I2C_WRITE_NO_DELAY_US,
      kCmisCommand);
}

} // namespace facebook::fboss
