// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.
#include <cctype>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <fboss/platform/weutil/FbossEepromParser.h>

namespace {

auto constexpr kMaxEepromSize = 2048;
const std::optional<int> VARIABLE = std::nullopt;

enum entryType {
  FIELD_INVALID,
  FIELD_UINT,
  FIELD_HEX,
  FIELD_STRING,
  FIELD_MAC,
  FIELD_LEGACY_MAC,
  FIELD_DATE
};

typedef struct {
  int typeCode;
  std::string fieldName;
  entryType fieldType;
  std::optional<int> length;
  std::optional<int> offset;
} EepromFieldEntry;

} // namespace

namespace facebook::fboss::platform {

std::vector<EepromFieldEntry> kFieldDictionaryV3 = {
    {0, "Version", FIELD_UINT, 1, 2}, // TypeCode 0 is reserved
    {1, "Product Name", FIELD_STRING, 20, 3},
    {2, "Product Part Number", FIELD_STRING, 8, 23},
    {3, "System Assembly Part Number", FIELD_STRING, 12, 31},
    {4, "Meta PCBA Part Number", FIELD_STRING, 12, 43},
    {5, "Meta PCB Part Number", FIELD_STRING, 12, 55},
    {6, "ODM/JDM PCBA Part Number", FIELD_STRING, 13, 67},
    {7, "ODM/JDM PCBA Serial Number", FIELD_STRING, 13, 80},
    {8, "Product Production State", FIELD_UINT, 1, 93},
    {9, "Product Version", FIELD_UINT, 1, 94},
    {10, "Product Sub-Version", FIELD_UINT, 1, 95},
    {11, "Product Serial Number", FIELD_STRING, 13, 96},
    {12, "Product Asset Tag", FIELD_STRING, 12, 109},
    {13, "System Manufacturer", FIELD_STRING, 8, 121},
    {14, "System Manufacturing Date", FIELD_DATE, 4, 129},
    {15, "PCB Manufacturer", FIELD_STRING, 8, 133},
    {16, "Assembled at", FIELD_STRING, 8, 141},
    {17, "Local MAC", FIELD_LEGACY_MAC, 12, 149},
    {17, "Extended MAC Base", FIELD_LEGACY_MAC, 12, 161},
    {18, "Extended MAC Address Size", FIELD_UINT, 2, 173},
    {19, "Location on Fabric", FIELD_STRING, 20, 175},
    {250, "CRC8", FIELD_HEX, 1, 195}};

std::vector<EepromFieldEntry> kFieldDictionaryV4 = {
    {0, "NA", FIELD_UINT, -1, -1}, // TypeCode 0 is reserved
    {1, "Product Name", FIELD_STRING, VARIABLE, VARIABLE},
    {2, "Product Part Number", FIELD_STRING, VARIABLE, VARIABLE},
    {3, "System Assembly Part Number", FIELD_STRING, 8, VARIABLE},
    {4, "Meta PCBA Part Number", FIELD_STRING, 12, VARIABLE},
    {5, "Meta PCB Part Number", FIELD_STRING, 12, VARIABLE},
    {6, "ODM/JDM PCBA Part Number", FIELD_STRING, VARIABLE, VARIABLE},
    {7, "ODM/JDM PCBA Serial Number", FIELD_STRING, VARIABLE, VARIABLE},
    {8, "Product Production State", FIELD_UINT, 1, VARIABLE},
    {9, "Product Version", FIELD_UINT, 1, VARIABLE},
    {10, "Product Sub-Version", FIELD_UINT, 1, VARIABLE},
    {11, "Product Serial Number", FIELD_STRING, VARIABLE, VARIABLE},
    {12, "System Manufacturer", FIELD_STRING, VARIABLE, VARIABLE},
    {13, "System Manufacturing Date", FIELD_STRING, 8, VARIABLE},
    {14, "PCB Manufacturer", FIELD_STRING, VARIABLE, VARIABLE},
    {15, "Assembled at", FIELD_STRING, VARIABLE, VARIABLE},
    {16, "Local MAC", FIELD_MAC, 6, VARIABLE},
    {17, "Extended MAC Base", FIELD_MAC, 6, VARIABLE},
    {18, "Extended MAC Address Size", FIELD_UINT, 2, VARIABLE},
    {19, "EEPROM location on Fabric", FIELD_STRING, VARIABLE, VARIABLE},
    {250, "CRC16", FIELD_HEX, 2, VARIABLE},
};

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
    readCount = (int)file.gcount();
    file.close();
  } catch (std::exception& ex) {
    std::cout << "Failed to read EEPROM contents " << ex.what() << std::endl;
    readCount = 0;
  }
  return readCount;
}

/*
 * Helper function of getInfo, for V3 eeprom
 * V3 eeprom has fixed length / offset fields, so is parsed
 * differently from V4, which is basically TLV structured.
 */
std::unordered_map<int, std::string> FbossEepromParser::parseEepromBlobV3(
    const unsigned char* buffer) {
  std::unordered_map<int, std::string> parsedValue;
  for (auto dictItem : kFieldDictionaryV3) {
    std::string key = dictItem.fieldName;
    std::string value;
    int itemCode = dictItem.typeCode;
    int itemOffset = dictItem.offset.value();
    int itemLength = dictItem.length.value();
    unsigned char* itemDataPtr = (unsigned char*)&buffer[itemOffset];
    entryType itemType = dictItem.fieldType;
    switch (itemType) {
      case FIELD_UINT:
        value = parseUint(itemLength, itemDataPtr);
        break;
      case FIELD_HEX:
        value = parseHex(itemLength, itemDataPtr);
        break;
      case FIELD_STRING:
        value = parseString(itemLength, itemDataPtr);
        break;
      case FIELD_LEGACY_MAC:
        value = parseLegacyMac(itemLength, itemDataPtr);
        break;
      case FIELD_DATE:
        value = parseDate(itemLength, itemDataPtr);
        break;
      default:
        // Create the issue (story) of this item (juice) as juicetory
        std::string juicetory = "Unknown field type " +
            std::to_string(itemType) + " at position " +
            std::to_string(itemOffset);
        std::cout << juicetory << std::endl;
        throw std::runtime_error(juicetory);
        break;
    }
    parsedValue[itemCode] = value;
  }
  return parsedValue;
}

// Helper function of getInfo, for V4 eeprom
std::unordered_map<int, std::string> FbossEepromParser::parseEepromBlobV4(
    const unsigned char* buffer,
    const int readCount) {
  int juice = 0; // A variable to count the number of items
                 // parsed so far
  int cursor = 4; // According to the Meta EEPROM v4 spec,
                  // the actual data starts from 4th byte of eeprom.
  std::unordered_map<int, std::string> parsedValue;
  std::string value;

  while (cursor < readCount) {
    // Increment the item counter (mainly for debugging purposes)
    // Very important to do this.
    juice = juice + 1;
    // First, get the itemCode of the TLV (T)
    int itemCode = (int)buffer[cursor];
    entryType itemType = FIELD_INVALID;
    int itemLen = (int)buffer[cursor + 1];
    std::string key;

    // Vendors pad EEPROM with 0xff. Therefore, if item code is
    // 0xff, then we reached to the end of the actual content.
    if (itemCode == 0xFF) {
      break;
    }
    // Look up our table to find the itemType and field name of this itemCode
    for (int i = 0; i < kFieldDictionaryV4.size(); i++) {
      if (kFieldDictionaryV4[i].typeCode == itemCode) {
        itemType = kFieldDictionaryV4[i].fieldType;
        key = kFieldDictionaryV4[i].fieldName;
      }
    }
    // If no entry found, throw an exception
    if (itemType == FIELD_INVALID) {
      std::cout << " Unknown field code " << itemCode << " at position "
                << cursor << " item number " << juice << std::endl;
      throw std::runtime_error(
          "Invalid field code in EEPROM at :" + std::to_string(cursor));
    }

    // Find Length and Variable (L and V)
    int itemLength = buffer[cursor + 1];
    unsigned char* itemDataPtr = (unsigned char*)&buffer[cursor + 2];
    // Parse the value according to the itemType
    switch (itemType) {
      case FIELD_UINT:
        value = parseUint(itemLength, itemDataPtr);
        break;
      case FIELD_HEX:
        value = parseHex(itemLength, itemDataPtr);
        break;
      case FIELD_STRING:
        value = parseString(itemLength, itemDataPtr);
        break;
      case FIELD_MAC:
        value = parseMac(itemLength, itemDataPtr);
        break;
      default:
        std::cout << " Unknown field type " << itemType << " at position "
                  << cursor << " item number " << juice << std::endl;
        throw std::runtime_error("Invalid field type in EEPROM.");
        break;
    }
    // Add the key-value pair to the result
    parsedValue[itemCode] = value;
    // Increment the cursor
    cursor += itemLen + 2;
  }
  return parsedValue;
}

// Another helper function of getInfo
// This method will translate <field_id, value> pair into
// <field_name, value> pair, so as to be used in other
// methods to print the human readable information
std::vector<std::pair<std::string, std::string>>
FbossEepromParser::prepareEepromFieldMap(
    std::unordered_map<int, std::string> parsedValue,
    int eepromVer) {
  std::vector<std::pair<std::string, std::string>> result;
  std::vector<EepromFieldEntry> fieldDictionary;
  switch (eepromVer) {
    case 3:
      fieldDictionary = kFieldDictionaryV3;
      break;
    case 4:
      fieldDictionary = kFieldDictionaryV4;
      break;
    default:
      throw std::runtime_error(
          "EEPROM version is not supported. Only ver 3+ is supported.");
      break;
  }
  for (auto dictItem : fieldDictionary) {
    std::string key = dictItem.fieldName;
    std::string value;
    auto match = parsedValue.find(dictItem.typeCode);
    // "NA" is reservered, and not for display
    if (key == "NA") {
      continue;
    }
    // if match exists for an itemCode, use the value, otherwise use empty
    // string.
    if (match != parsedValue.end()) {
      value = parsedValue.find(dictItem.typeCode)->second;
    } else {
      value = "";
    }
    result.push_back({key, value});
  }
  return result;
}

parsedEepromEntry FbossEepromParser::getAllInfo(
    const std::string& eeprom,
    const int offset) {
  unsigned char buffer[kMaxEepromSize + 1] = {};

  int readCount = loadEeprom(eeprom, buffer, offset, kMaxEepromSize);

  // Ensure that this is EEPROM v4 or later
  std::unordered_map<int, std::string> parsedValue;
  int eepromVer = buffer[2];

  switch (eepromVer) {
    case 3:
      parsedValue = parseEepromBlobV3(buffer);
      break;
    case 4:
      parsedValue =
          parseEepromBlobV4(buffer, std::min(readCount, kMaxEepromSize));
      break;
    default:
      throw std::runtime_error(
          "EEPROM version is not supported. Only ver 4+ is supported.");
      break;
  }

  return prepareEepromFieldMap(parsedValue, eepromVer);
}

// Parse Uint field
std::string FbossEepromParser::parseUint(int len, unsigned char* ptr) {
  // For now, we only support up to 4 Bytes of data
  if (len > 4) {
    throw std::runtime_error("Unsigned int can be up to 4 bytes only.");
  }
  unsigned int readVal = 0;
  // Values in the EEPROM is big endian
  // Thus cursor starts from the end and goes backwards
  int cursor = len - 1;
  for (int i = 0; i < len; i++) {
    readVal <<= 8;
    readVal |= (unsigned int)ptr[cursor];
    cursor -= 1;
  }
  return std::to_string(readVal);
}

std::string FbossEepromParser::parseHex(int len, unsigned char* ptr) {
  std::string retVal = "";
  // Values in the EEPROM is big endian
  // Thus cursor starts from the end and goes backwards
  int cursor = len - 1;
  for (int i = 0; i < len; i++) {
    int val = ptr[cursor];
    std::string converter = "0123456789abcdef";
    retVal = retVal + converter[(int)(val / 16)] + converter[val % 16];
    cursor -= 1;
  }
  return "0x" + retVal;
}

// Parse String field
std::string FbossEepromParser::parseString(int len, unsigned char* ptr) {
  std::string retVal = "";
  // We convert char array to string only upto len or null pointer
  int juice = 0;
  while ((juice < len) && (ptr[juice] != 0)) {
    retVal += (ptr[juice]);
    juice = juice + 1;
  }
  return retVal;
}

// For EEPROM V4, Parse MAC with the format XX:XX:XX:XX:XX:XX
std::string FbossEepromParser::parseMac(int len, unsigned char* ptr) {
  std::string retVal = "";
  // We convert char array to string only upto len or null pointer
  int juice = 0;
  while ((juice < len) && (ptr[juice] != 0)) {
    unsigned int val = ptr[juice];
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

// For EEPROM V3, MAC is represented as XXXXXXXXXXXX. Therefore,
// parse the MAC into the human readable format as XX:XX:XX:XX:XX:XX
std::string FbossEepromParser::parseLegacyMac(int len, unsigned char* ptr) {
  std::string retVal = "";
  if (len != 12) {
    throw std::runtime_error("Legacy(v3) MAC field must be 12 Bytes Long!");
  }
  for (int i = 0; i < 12; i++) {
    retVal += (ptr[i]);
    if (((i % 2) == 1) && (i != 11)) {
      retVal += ":";
    }
  }
  return retVal;
}

std::string FbossEepromParser::parseDate(int len, unsigned char* ptr) {
  std::string retVal = "";
  // We convert char array to string only upto len or null pointer
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

parsedEepromEntry FbossEepromParser::getEeprom(
    const std::string& eeprom,
    const int offset) {
  // If eeprom is empty, use the chassis eeprom entity from the config.
  std::string eepromEntity = eeprom;
  return getAllInfo(eepromEntity, offset);
}

} // namespace facebook::fboss::platform
