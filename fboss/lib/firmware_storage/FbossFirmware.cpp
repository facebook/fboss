// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/lib/firmware_storage/FbossFirmware.h"
#include <fcntl.h>
#include <folly/FileUtil.h>
#include <folly/io/Cursor.h>
#include <folly/logging/xlog.h>
#include "folly/File.h"

namespace {
// Max supported firmware file size (10MB)
constexpr uint32_t kMaxFwFileFize = 10 * 1024 * 1024;
} // namespace

namespace facebook::fboss {

/*
 * load
 *
 * This function reads the image file for this firmware, store the content in
 * IOBuf in the FbossFirmware object. This IOBuf can be given to the caller
 * as "Cursor" through function getImage().
 */
void FbossFirmware::load() {
  // Get the file handle
  folly::File fwFileHandle(firmwareAttributes_.filename);

  // Find out the file size from file descriptor
  struct stat fileStat;
  int retval = fstat(fwFileHandle.fd(), &fileStat);
  if (retval == -1) {
    XLOG(ERR) << "FbossFirmware: Can't get the information " << "on the file "
              << firmwareAttributes_.filename;
    // Folly file destructor will close the fd
    throw FbossFirmwareError("Bad Firmware file");
  }

  uint32_t fileSize = static_cast<uint32_t>(fileStat.st_size);

  // Sanity check on file size, it can't be more than 10MB
  if (fileSize > kMaxFwFileFize) {
    XLOG(ERR) << "FbossFirmware: file seems to be too big, "
              << "something could be wrong, file size: " << fileSize;
    // Folly file destructor will close the fd
    throw FbossFirmwareError("Firmware file size problem");
  }

  // Create an IOBuf to hold the data read from Firmware file. The IOBuf has
  // 4 pointers - buffer(), data(), tail(), bufferEnd(). The actual data lies
  // between data() to tail(). Rest are headroom and tailroom
  fileIOBuffer_ = folly::IOBuf::createCombined(fileSize);

  // Read full file into the IOBuf
  auto nRead = folly::readFull(
      fwFileHandle.fd(),
      fileIOBuffer_->writableData(),
      fileIOBuffer_->tailroom());

  // Check if the read failed or done partially
  if (nRead < 0) {
    XLOG(ERR) << "FbossFirmware: Failed to read from file "
              << firmwareAttributes_.filename << " size "
              << folly::to<std::string>(fileSize);
    fwFileHandle.close();
    throw FbossFirmwareError("Firmware file can't be read");
  } else if (nRead < fileSize) {
    XLOG(ERR) << "FbossFirmware: Failed to read completely from " << "file "
              << firmwareAttributes_.filename << " size "
              << folly::to<std::string>(fileSize)
              << " number of bytes read only " << folly::to<std::string>(nRead);
    fwFileHandle.close();
    throw FbossFirmwareError("Firmware file incompletely read");
  }

  // Adjust the tail() pointer in IOBuf so that valid data is correctly
  // represented
  fileIOBuffer_->append(nRead);
}

/*
 * FbossFirmware::getImage
 *
 * This function returns the image payload pointer as Cursor. The caller can use
 * the Cursor functions like cursor.read<uint8_t>() to access the payload or
 * the caller can also treat the payoad as uint8_t pointer
 */
folly::io::Cursor FbossFirmware::getImage() const {
  // return image IOBuf as Cursor
  folly::io::Cursor imageCursor{fileIOBuffer_.get()};
  return imageCursor;
}

/*
 * FbossFirmware::getProperty
 *
 * Get the firmware property like "msa_password" and return it. If the
 * property is not found then it throws
 */
const std::string& FbossFirmware::getProperty(const std::string& name) const {
  auto it = firmwareAttributes_.properties.find(name);

  if (it == firmwareAttributes_.properties.end()) {
    XLOG(ERR) << "Firmware property " << name << "could not be read";
    throw FbossFirmwareError("Firmware property can't be obtained");
  }
  // return property
  return it->second;
}

/*
 * FbossFirmware::dumpFwInfo
 *
 * Dump the firmware related info to console. Also open the firmware image
 * file and dump first 16 bytes for verification
 */
void FbossFirmware::dumpFwInfo() {
  printf("Module: %s\n", firmwareAttributes_.prodName.c_str());
  printf("Version: %s\n", firmwareAttributes_.version.c_str());
  printf("File: %s\n", firmwareAttributes_.filename.c_str());
  printf("md5sum: %s\n", firmwareAttributes_.md5Checksum.c_str());

  for (auto& prop : firmwareAttributes_.properties) {
    printf("%s: %s\n", prop.first.c_str(), prop.second.c_str());
  }

  folly::io::Cursor c = getImage();

  printf("Data from file (16Bytes): \n0x");
  for (int i = 0; i < 16; i++) {
    printf("%02x.", c.read<uint8_t>());
  }
  printf("\n");
}

} // namespace facebook::fboss
