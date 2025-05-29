// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <fboss/platform/weutil/FbossEepromParser.h>

#include <fstream>
#include <ios>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <folly/logging/xlog.h>
#include "fboss/platform/weutil/Crc16CcittAug.h"
#include "fboss/platform/weutil/FbossEepromV5.h"
#include "fboss/platform/weutil/FbossEepromV6.h"

namespace {

auto constexpr kMaxEepromSize = 2048;
const std::optional<int> VARIABLE = std::nullopt;

enum entryType {
  FIELD_INVALID,
  FIELD_LE_UINT,
  FIELD_BE_UINT,
  FIELD_LE_HEX,
  FIELD_BE_HEX,
  FIELD_STRING,
  FIELD_MAC,
  FIELD_DATE
};

typedef struct {
  int typeCode;
  std::string fieldName;
  entryType fieldType;
  std::optional<int> length;
  std::optional<int> offset;
} EepromFieldEntry;

const std::vector<EepromFieldEntry> kFieldDictionaryV5 = {
    {0, "NA", FIELD_LE_UINT, -1, -1}, // TypeCode 0 is reserved
    {1, "Product Name", FIELD_STRING, VARIABLE, VARIABLE},
    {2, "Product Part Number", FIELD_STRING, VARIABLE, VARIABLE},
    {3, "System Assembly Part Number", FIELD_STRING, 8, VARIABLE},
    {4, "Meta PCBA Part Number", FIELD_STRING, 12, VARIABLE},
    {5, "Meta PCB Part Number", FIELD_STRING, 12, VARIABLE},
    {6, "ODM/JDM PCBA Part Number", FIELD_STRING, VARIABLE, VARIABLE},
    {7, "ODM/JDM PCBA Serial Number", FIELD_STRING, VARIABLE, VARIABLE},
    {8, "Product Production State", FIELD_BE_UINT, 1, VARIABLE},
    {9, "Product Version", FIELD_BE_UINT, 1, VARIABLE},
    {10, "Product Sub-Version", FIELD_BE_UINT, 1, VARIABLE},
    {11, "Product Serial Number", FIELD_STRING, VARIABLE, VARIABLE},
    {12, "System Manufacturer", FIELD_STRING, VARIABLE, VARIABLE},
    {13, "System Manufacturing Date", FIELD_STRING, 8, VARIABLE},
    {14, "PCB Manufacturer", FIELD_STRING, VARIABLE, VARIABLE},
    {15, "Assembled At", FIELD_STRING, VARIABLE, VARIABLE},
    {16, "EEPROM location on Fabric", FIELD_STRING, VARIABLE, VARIABLE},
    {17, "X86 CPU MAC", FIELD_MAC, 8, VARIABLE},
    {18, "BMC MAC", FIELD_MAC, 8, VARIABLE},
    {19, "Switch ASIC MAC", FIELD_MAC, 8, VARIABLE},
    {20, "META Reserved MAC", FIELD_MAC, 8, VARIABLE},
    {250, "CRC16", FIELD_BE_HEX, 2, VARIABLE},
};

const std::vector<EepromFieldEntry> kFieldDictionaryV6 = {
    {0, "NA", FIELD_LE_UINT, -1, -1}, // TypeCode 0 is reserved
    {1, "Product Name", FIELD_STRING, VARIABLE, VARIABLE},
    {2, "Product Part Number", FIELD_STRING, VARIABLE, VARIABLE},
    {3, "System Assembly Part Number", FIELD_STRING, 8, VARIABLE},
    {4, "Meta PCBA Part Number", FIELD_STRING, 12, VARIABLE},
    {5, "Meta PCB Part Number", FIELD_STRING, 12, VARIABLE},
    {6, "ODM/JDM PCBA Part Number", FIELD_STRING, VARIABLE, VARIABLE},
    {7, "ODM/JDM PCBA Serial Number", FIELD_STRING, VARIABLE, VARIABLE},
    {8, "Production State", FIELD_BE_UINT, 1, VARIABLE},
    {9, "Production Sub-State", FIELD_BE_UINT, 1, VARIABLE},
    {10, "Re-Spin/Variant Indicator", FIELD_BE_UINT, 1, VARIABLE},
    {11, "Product Serial Number", FIELD_STRING, VARIABLE, VARIABLE},
    {12, "System Manufacturer", FIELD_STRING, VARIABLE, VARIABLE},
    {13, "System Manufacturing Date", FIELD_STRING, 8, VARIABLE},
    {14, "PCB Manufacturer", FIELD_STRING, VARIABLE, VARIABLE},
    {15, "Assembled At", FIELD_STRING, VARIABLE, VARIABLE},
    {16, "EEPROM location on Fabric", FIELD_STRING, VARIABLE, VARIABLE},
    {17, "X86 CPU MAC", FIELD_MAC, 8, VARIABLE},
    {18, "BMC MAC", FIELD_MAC, 8, VARIABLE},
    {19, "Switch ASIC MAC", FIELD_MAC, 8, VARIABLE},
    {20, "META Reserved MAC", FIELD_MAC, 8, VARIABLE},
    {21, "RMA", FIELD_BE_UINT, 1, VARIABLE},
    {101, "Vendor Defined Field 1", FIELD_BE_HEX, VARIABLE, VARIABLE},
    {102, "Vendor Defined Field 2", FIELD_BE_HEX, VARIABLE, VARIABLE},
    {103, "Vendor Defined Field 3", FIELD_BE_HEX, VARIABLE, VARIABLE},
    {250, "CRC16", FIELD_BE_HEX, 2, VARIABLE},
};

// Header size in EEPROM. First two bytes are 0xFBFB followed
// by a byte specifying the EEPROM version and one byte of 0xFF
constexpr int kHeaderSize = 4;
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

namespace facebook::fboss::platform {

std::vector<std::pair<std::string, std::string>>
FbossEepromParser::getContents() {
  unsigned char buffer[kMaxEepromSize + 1] = {};

  int readCount = loadEeprom(eepromPath_, buffer, offset_, kMaxEepromSize);

  int eepromVer = buffer[2];
  auto parsedValue = parseEepromBlobTLV(
      eepromVer, buffer, std::min(readCount, kMaxEepromSize));

  return parsedValue->getContents();
}

// Calculate the CRC16 of the EEPROM. The last 4 bytes of EEPROM
// contents are the TLV (Type, Length, Value) of CRC, and should not
// be included in the CRC calculation.
uint16_t FbossEepromParser::calculateCrc16(const uint8_t* buffer, size_t len) {
  if (len <= (kEepromTypeLengthSize + kCrcSize)) {
    throw std::runtime_error("EEPROM blob size is too small.");
  }
  const size_t eepromSizeWithoutCrc = len - kEepromTypeLengthSize - kCrcSize;
  return helpers::crc_ccitt_aug(buffer, eepromSizeWithoutCrc);
}

/*
 * Helper function, given the eeprom path, read it and store the blob
 * to the char array output
 */
int FbossEepromParser::loadEeprom(
    const std::string& eeprom,
    unsigned char* output,
    int offset,
    int max) {
  // Declare buffer, and fill it up with 0s
  int fileSize = 0;
  int bytesToRead = max;
  std::ifstream file(eeprom, std::ios::binary);
  int readCount = 0;
  // First, detect EEPROM size, upto 2048B only
  try {
    file.seekg(0, std::ios::end);
    fileSize = file.tellg();
    // bytesToRead cannot be bigger than the remaining bytes of the file from
    // the offset. That is, we cannot read beyond the end of the file.
    // If the remaining bytes are smaller than max, then we only read up to
    // the end of the file.
    int remainingBytes = fileSize - offset;
    if (bytesToRead > remainingBytes) {
      bytesToRead = remainingBytes;
    }
  } catch (std::exception& ex) {
    std::cout << "Failed to detect EEPROM size (" << eeprom
              << "): " << ex.what() << std::endl;
    throw std::runtime_error("Unabled to detect EEPROM size.");
  }
  if (fileSize < 0) {
    std::cout << "EEPROM (" << eeprom << ") does not exist, or is empty!"
              << std::endl;
    throw std::runtime_error("Unable to read EEPROM.");
  }
  // Now, read the eeprom
  try {
    file.seekg(offset, std::ios::beg);
    file.read((char*)&output[0], bytesToRead);
    readCount = static_cast<int>(file.gcount());
    file.close();
  } catch (std::exception& ex) {
    std::cout << "Failed to read EEPROM contents " << ex.what() << std::endl;
    readCount = 0;
  }
  return readCount;
}

std::unique_ptr<FbossEepromInterface> FbossEepromParser::parseEepromBlobTLV(
    int eepromVer,
    const unsigned char* buffer,
    const int readCount) {
  // A variable to count the number of items parsed so far
  int juice = 0;
  // According to the Meta EEPROM V5 spec and later,
  // the actual data starts from 4th byte of eeprom.
  int cursor = kHeaderSize;

  std::unordered_map<int, std::string> parsedValue;
  std::string value;

  std::unique_ptr<FbossEepromInterface> result;
  if (eepromVer == 5) {
    result = std::make_unique<FbossEepromV5>();
  } else if (eepromVer == 6) {
    result = std::make_unique<FbossEepromV6>();
  } else {
    throw std::runtime_error(
        "Invalid EEPROM version : " + std::to_string(eepromVer));
  }
  auto fieldDictionary = result->getFieldDictionary();

  while (cursor < readCount) {
    // Increment the item counter (mainly for debugging purposes)
    // Very important to do this.
    juice = juice + 1;
    // First, get the itemCode of the TLV (T)
    int itemCode = static_cast<int>(buffer[cursor]);
    FbossEepromInterface::entryType itemType =
        FbossEepromInterface::FIELD_INVALID;
    std::string key;
    std::string* fieldPtr = nullptr;

    // Vendors pad EEPROM with 0xff. Therefore, if item code is
    // 0xff, then we reached to the end of the actual content.
    if (itemCode == 0xFF) {
      break;
    }
    // Look up our table to find the itemType and field name of this itemCode
    for (size_t i = 0; i < fieldDictionary.size(); i++) {
      if (fieldDictionary[i].typeCode == itemCode) {
        itemType = fieldDictionary[i].fieldType;
        key = fieldDictionary[i].fieldName;
        fieldPtr = fieldDictionary[i].fieldPtr;
      }
    }
    // If no entry found, throw an exception
    if (itemType == FbossEepromInterface::FIELD_INVALID) {
      std::cout << " Unknown field code " << itemCode << " at position "
                << cursor << " item number " << juice << std::endl;
      throw std::runtime_error(
          "Invalid field code in EEPROM at :" + std::to_string(cursor));
    }

    // Find Length and Variable (L and V)
    int itemLength = buffer[cursor + 1];
    unsigned char* itemDataPtr =
        (unsigned char*)&buffer[cursor + kEepromTypeLengthSize];
    // Parse the value according to the itemType
    switch (itemType) {
      case FbossEepromInterface::FIELD_BE_UINT:
        value = parseBeUint(itemLength, itemDataPtr);
        break;
      case FbossEepromInterface::FIELD_BE_HEX:
        value = parseBeHex(itemLength, itemDataPtr);
        break;
      case FbossEepromInterface::FIELD_STRING:
        value = parseString(itemLength, itemDataPtr);
        break;
      case FbossEepromInterface::FIELD_MAC:
        value = parseMac(itemLength, itemDataPtr);
        break;
      default:
        std::cout << " Unknown field type " << itemType << " at position "
                  << cursor << " item number " << juice << std::endl;
        throw std::runtime_error("Invalid field type in EEPROM.");
        break;
    }
    // Add the key-value pair to the result
    if (fieldPtr) {
      *fieldPtr = value;
    }
    // Increment the cursor
    cursor += itemLength + kEepromTypeLengthSize;
    // the CRC16 is the last content, parsing must stop.
    if (key == "CRC16" && fieldPtr) {
      uint16_t crcProgrammed = std::stoi(value, nullptr, 16);
      uint16_t crcCalculated = calculateCrc16(buffer, cursor);
      if (crcProgrammed == crcCalculated) {
        *fieldPtr = value + " (CRC Matched)";
      } else {
        std::stringstream ss;
        ss << std::hex << crcCalculated;
        *fieldPtr = value + " (CRC Mismatch. Expected 0x" + ss.str() + ")";
      }
      break;
    }
  }
  return result;
}

std::string FbossEepromParser::parseLeUint(int len, unsigned char* ptr) {
  if (len > 4) {
    throw std::runtime_error("Unsigned int can only be up to 4 bytes.");
  }
  unsigned int readVal = 0;
  int cursor = len - 1;
  for (int i = 0; i < len; i++) {
    readVal <<= 8;
    readVal |= (unsigned int)ptr[cursor];
    cursor -= 1;
  }
  return std::to_string(readVal);
}

std::string FbossEepromParser::parseBeUint(int len, unsigned char* ptr) {
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

std::string FbossEepromParser::parseLeHex(int len, unsigned char* ptr) {
  std::string retVal;
  int cursor = len - 1;
  for (int i = 0; i < len; i++) {
    int val = ptr[cursor];
    std::string converter = "0123456789abcdef";
    retVal =
        retVal + converter[static_cast<int>(val / 16)] + converter[val % 16];
    cursor -= 1;
  }
  return "0x" + retVal;
}

std::string FbossEepromParser::parseBeHex(int len, unsigned char* ptr) {
  std::string retVal;
  for (int i = 0; i < len; i++) {
    int val = ptr[i];
    std::string converter = "0123456789abcdef";
    retVal =
        retVal + converter[static_cast<int>(val / 16)] + converter[val % 16];
  }
  return "0x" + retVal;
}

std::string FbossEepromParser::parseString(int len, unsigned char* ptr) {
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
std::string FbossEepromParser::parseMac(int len, unsigned char* ptr) {
  std::string retVal;
  // Pack two string with "," in between. This will be unpacked in the
  // dump functions.
  retVal =
      parseMacHelper(len - 2, ptr, true) + "," + parseBeUint(2, &ptr[len - 2]);
  return retVal;
}

std::string FbossEepromParser::parseDate(int len, unsigned char* ptr) {
  std::string retVal;
  if (len != 4) {
    throw std::runtime_error("Date field must be 4 Bytes Long!");
  }
  unsigned int year = (unsigned int)ptr[1] + (unsigned int)ptr[0];
  unsigned int month = (unsigned int)ptr[2];
  unsigned int day = (unsigned int)ptr[3];
  std::string yearString = std::to_string(year % 100);
  std::string monthString = std::to_string(month);
  std::string dayString = std::to_string(day);
  yearString = (yearString.length() == 1 ? "0" : "") + yearString;
  monthString = (monthString.length() == 1 ? "0" : "") + monthString;
  dayString = (dayString.length() == 1 ? "0" : "") + dayString;
  return monthString + "-" + dayString + "-" + yearString;
}

} // namespace facebook::fboss::platform
