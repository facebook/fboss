// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.
#include <fmt/format.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include "fboss/platform/weutil/Crc16CcittAug.h"
#include "fboss/platform/weutil/ParserUtils.h"

namespace facebook::fboss::platform {

namespace {

// Field Type and Length are 1 byte each.
constexpr int kEepromTypeLengthSize = 2;

// CRC size (16 bits)
constexpr int kCrcSize = 2;

std::string parseMacHelper(int len, unsigned char* ptr, bool useBigEndian) {
  std::string retVal;
  int juice = 0;
  while (juice < len) {
    unsigned int val = useBigEndian ? ptr[juice] : ptr[len - juice - 1];
    std::ostringstream ss;
    ss << std::hex << val;
    std::string strElement = ss.str();
    // Pad 0 if the hex value is only 1 digit. Also,
    // add ':' between 2 hex digits except for the last element
    strElement =
        (val < 16 ? "0" : "") + strElement + (juice != len - 1 ? ":" : "");
    retVal += strElement;
    juice = juice + 1;
  }
  return retVal;
}

} // namespace

// Calculate the CRC16 of the EEPROM. The last 4 bytes of EEPROM
// contents are the TLV (Type, Length, Value) of CRC, and should not
// be included in the CRC calculation.
uint16_t ParserUtils::calculateCrc16(const uint8_t* buffer, size_t len) {
  if (len <= (kEepromTypeLengthSize + kCrcSize)) {
    throw std::runtime_error("EEPROM blob size is too small.");
  }
  const size_t eepromSizeWithoutCrc = len - kEepromTypeLengthSize - kCrcSize;
  return helpers::crc_ccitt_aug(buffer, eepromSizeWithoutCrc);
}

std::vector<uint8_t> ParserUtils::loadEeprom(
    const std::string& eeprom,
    int offset) {
  // Declare buffer, and fill it up with 0s
  long fileSize = 0;
  long bytesToRead = 0;
  std::ifstream file(eeprom, std::ios::binary);
  std::vector<uint8_t> result;
  // First, detect EEPROM size
  try {
    file.seekg(0, std::ios::end);
    fileSize = file.tellg();
    if (fileSize < 0) {
      throw std::runtime_error(
          fmt::format("EEPROM {} does not exist, or is empty!", eeprom));
    }

    // bytesToRead cannot be bigger than the remaining bytes of the file from
    // the offset. That is, we cannot read beyond the end of the file.
    // If the remaining bytes are smaller than max, then we only read up to
    // the end of the file.
    bytesToRead = fileSize - offset;
    if (bytesToRead < 0) {
      throw std::runtime_error("Offset greater than file size");
    }
    result.resize(bytesToRead);
  } catch (std::exception& ex) {
    std::cout << "Failed to detect EEPROM size (" << eeprom
              << "): " << ex.what() << std::endl;
    throw std::runtime_error("Unable to detect EEPROM size.");
  }

  // Now, read the eeprom
  try {
    file.seekg(offset, std::ios::beg);
    file.read((char*)result.data(), bytesToRead);
    file.close();
  } catch (std::exception& ex) {
    std::cout << "Failed to read EEPROM contents " << ex.what() << std::endl;
  }
  return result;
}

std::string ParserUtils::parseBeUint(int len, unsigned char* ptr) {
  if (len > 4) {
    throw std::runtime_error("Unsigned int can only be up to 4 bytes.");
  }
  unsigned int readVal = 0;
  for (int i = 0; i < len; i++) {
    readVal <<= 8;
    readVal |= (unsigned int)ptr[i];
  }
  return std::to_string(readVal);
}

std::string ParserUtils::parseBeHex(int len, unsigned char* ptr) {
  std::string retVal;
  for (int i = 0; i < len; i++) {
    int val = ptr[i];
    std::string converter = "0123456789abcdef";
    retVal =
        retVal + converter[static_cast<int>(val / 16)] + converter[val % 16];
  }
  return "0x" + retVal;
}

std::string ParserUtils::parseString(int len, unsigned char* ptr) {
  std::string retVal;
  int juice = 0;
  while ((juice < len) && (ptr[juice] != 0)) {
    retVal += (ptr[juice]);
    juice = juice + 1;
  }
  return retVal;
}

// For EEPROM V5, Parse MAC with the format XX:XX:XX:XX:XX:XX, along with two
// bytes MAC size
std::string ParserUtils::parseMac(int len, unsigned char* ptr) {
  std::string retVal;
  // Pack two string with "," in between. This will be unpacked in the
  // dump functions.
  retVal =
      parseMacHelper(len - 2, ptr, true) + "," + parseBeUint(2, &ptr[len - 2]);
  return retVal;
}

} // namespace facebook::fboss::platform
