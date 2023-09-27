// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.
#include <folly/Conv.h>
#include <folly/Format.h>
#include <folly/json.h>
#include <folly/logging/xlog.h>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "fboss/platform/helpers/PlatformUtils.h"
#include "fboss/platform/weutil/WeutilImpl.h"
#include "fboss/platform/weutil/prefdl/Prefdl.h"

namespace {
auto constexpr kMaxEepromSize = 2048;
} // namespace

namespace facebook::fboss::platform {

WeutilImpl::WeutilImpl(const std::string& eeprom, PlainWeutilConfig config)
    : config_(config) {
  XLOG(INFO) << "WeutilImpl: eeprom = " << eeprom;
  initializeFieldDictionaryV3();
  initializeFieldDictionaryV4();
  eepromPath = eeprom;
}

//
//  This method is called inside the constructor to initialize the
//  internal look-up table. The content of this table follows FBOSS
//  EEPROM spec v4.
//
void WeutilImpl::initializeFieldDictionaryV4() {
  //
  // VARIABLE in the offset and length means it's variable, and
  // will be determined by the TLV structure in the EEPROM
  // itself (v4 and above)
  // {Typecode, Name, FieldType, Length, Offset}
  // Note that this is the order that the EEPROM will be printed
  //
  fieldDictionaryV4_.push_back(
      {0, "NA", FIELD_UINT, -1, -1}); // TypeCode 0 is reserved
  fieldDictionaryV4_.push_back(
      {1, "Product Name", FIELD_STRING, VARIABLE, VARIABLE});
  fieldDictionaryV4_.push_back(
      {2, "Product Part Number", FIELD_STRING, VARIABLE, VARIABLE});
  fieldDictionaryV4_.push_back(
      {3, "System Assembly Part Number", FIELD_STRING, 8, VARIABLE});
  fieldDictionaryV4_.push_back(
      {4, "Meta PCBA Part Number", FIELD_STRING, 12, VARIABLE});
  fieldDictionaryV4_.push_back(
      {5, "Meta PCB Part Number", FIELD_STRING, 12, VARIABLE});
  fieldDictionaryV4_.push_back(
      {6, "ODM/JDM PCBA Part Number", FIELD_STRING, VARIABLE, VARIABLE});
  fieldDictionaryV4_.push_back(
      {7, "ODM/JDM PCBA Serial Number", FIELD_STRING, VARIABLE, VARIABLE});
  fieldDictionaryV4_.push_back(
      {8, "Product Production State", FIELD_UINT, 1, VARIABLE});
  fieldDictionaryV4_.push_back({9, "Product Version", FIELD_UINT, 1, VARIABLE});
  fieldDictionaryV4_.push_back(
      {10, "Product Sub-Version", FIELD_UINT, 1, VARIABLE});
  fieldDictionaryV4_.push_back(
      {11, "Product Serial Number", FIELD_STRING, VARIABLE, VARIABLE});
  fieldDictionaryV4_.push_back(
      {12, "System Manufacturer", FIELD_STRING, VARIABLE, VARIABLE});
  fieldDictionaryV4_.push_back(
      {13, "System Manufacturing Date", FIELD_STRING, 8, VARIABLE});
  fieldDictionaryV4_.push_back(
      {14, "PCB Manufacturer", FIELD_STRING, VARIABLE, VARIABLE});
  fieldDictionaryV4_.push_back(
      {15, "Assembled at", FIELD_STRING, VARIABLE, VARIABLE});
  fieldDictionaryV4_.push_back({16, "Local MAC", FIELD_MAC, 6, VARIABLE});
  fieldDictionaryV4_.push_back(
      {17, "Extended MAC Base", FIELD_MAC, 6, VARIABLE});
  fieldDictionaryV4_.push_back(
      {18, "Extended MAC Address Size", FIELD_UINT, 2, VARIABLE});
  fieldDictionaryV4_.push_back(
      {19, "EEPROM location on Fabric", FIELD_STRING, VARIABLE, VARIABLE});
  fieldDictionaryV4_.push_back({250, "CRC16", FIELD_HEX, 2, VARIABLE});
}

// Same as above, but for EEPROM V3.
void WeutilImpl::initializeFieldDictionaryV3() {
  //
  // VARIABLE in the offset and length means it's variable, and
  // will be determined by the TLV structure in the EEPROM
  // itself (v4 and above)
  // {Typecode, Name, FieldType, Length, Offset}
  // Note that this is the order that the EEPROM will be printed
  //
  fieldDictionaryV3_.push_back(
      {0, "Version", FIELD_UINT, 1, 2}); // TypeCode 0 is reserved
  fieldDictionaryV3_.push_back({1, "Product Name", FIELD_STRING, 20, 3});
  fieldDictionaryV3_.push_back({2, "Product Part Number", FIELD_STRING, 8, 23});
  fieldDictionaryV3_.push_back(
      {3, "System Assembly Part Number", FIELD_STRING, 12, 31});
  fieldDictionaryV3_.push_back(
      {4, "Meta PCBA Part Number", FIELD_STRING, 12, 43});
  fieldDictionaryV3_.push_back(
      {5, "Meta PCB Part Number", FIELD_STRING, 12, 55});
  fieldDictionaryV3_.push_back(
      {6, "ODM/JDM PCBA Part Number", FIELD_STRING, 13, 67});
  fieldDictionaryV3_.push_back(
      {7, "ODM/JDM PCBA Serial Number", FIELD_STRING, 13, 80});
  fieldDictionaryV3_.push_back(
      {8, "Product Production State", FIELD_UINT, 1, 93});
  fieldDictionaryV3_.push_back({9, "Product Version", FIELD_UINT, 1, 94});
  fieldDictionaryV3_.push_back({10, "Product Sub-Version", FIELD_UINT, 1, 95});
  fieldDictionaryV3_.push_back(
      {11, "Product Serial Number", FIELD_STRING, 13, 96});
  fieldDictionaryV3_.push_back(
      {12, "Product Asset Tag", FIELD_STRING, 12, 109});
  fieldDictionaryV3_.push_back(
      {12, "System Manufacturer", FIELD_STRING, 8, 121});
  fieldDictionaryV3_.push_back(
      {13, "System Manufacturing Date", FIELD_DATE, 4, 129});
  fieldDictionaryV3_.push_back({14, "PCB Manufacturer", FIELD_STRING, 8, 133});
  fieldDictionaryV3_.push_back({15, "Assembled at", FIELD_STRING, 8, 141});
  fieldDictionaryV3_.push_back({16, "Local MAC", FIELD_LEGACY_MAC, 12, 149});
  fieldDictionaryV3_.push_back(
      {17, "Extended MAC Base", FIELD_LEGACY_MAC, 12, 161});
  fieldDictionaryV3_.push_back(
      {18, "Extended MAC Address Size", FIELD_UINT, 2, 173});
  fieldDictionaryV3_.push_back(
      {19, "Location on Fabric", FIELD_STRING, 20, 175});
  fieldDictionaryV3_.push_back({250, "CRC8", FIELD_HEX, 1, 195});
}

/*
 * Helper function, given the eeprom path, read it and store the blob
 * to the char array output
 */
int WeutilImpl::loadEeprom(
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
std::unordered_map<int, std::string> WeutilImpl::parseEepromBlobV3(
    const unsigned char* buffer) {
  std::unordered_map<int, std::string> parsedValue;
  int juice = 0;
  for (auto dictItem : fieldDictionaryV3_) {
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
        XLOG(ERR) << juicetory << std::endl;
        throw std::runtime_error(juicetory);
        break;
    }
    parsedValue[itemCode] = value;
    juice = juice + 1;
    // As in V4 parsing routine, print the log for every
    // 10th juice (item) so as not to flooding the log file
    // (Celebrating 10th Juice)
    if (juice % 10 == 0) {
      XLOG(DBG) << "Parsing eeprom entry " << juice << std::endl;
    }
  }
  return parsedValue;
}

// Helper function of getInfo, for V4 eeprom
std::unordered_map<int, std::string> WeutilImpl::parseEepromBlobV4(
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
    for (int i = 0; i < fieldDictionaryV4_.size(); i++) {
      if (fieldDictionaryV4_[i].typeCode == itemCode) {
        itemType = fieldDictionaryV4_[i].fieldType;
        key = fieldDictionaryV4_[i].fieldName;
      }
    }
    // If no entry found, throw an exception
    if (itemType == FIELD_INVALID) {
      XLOG(ERR) << " Unknown field code " << itemCode << " at position "
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
        XLOG(ERR) << " Unknown field type " << itemType << " at position "
                  << cursor << " item number " << juice << std::endl;
        throw std::runtime_error("Invalid field type in EEPROM.");
        break;
    }
    // Print the log for every 10th juice (item) so as not
    // to flooding the log file (Celebrating 10th Juice)
    if (juice % 10 == 0) {
      XLOG(INFO) << "Parsing eeprom entry " << juice << std::endl;
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
WeutilImpl::prepareEepromFieldMap(
    std::unordered_map<int, std::string> parsedValue,
    int eepromVer) {
  std::vector<std::pair<std::string, std::string>> result;
  std::vector<EepromFieldEntry> fieldDictionary;
  switch (eepromVer) {
    case 3:
      fieldDictionary = fieldDictionaryV3_;
      break;
    case 4:
      fieldDictionary = fieldDictionaryV4_;
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

std::vector<std::pair<std::string, std::string>> WeutilImpl::getInfo(
    const std::string& eeprom) {
  unsigned char buffer[kMaxEepromSize + 1];
  int offset = 0;
  if (config_.configEntry.find(translatedFrom) != config_.configEntry.end()) {
    offset = std::get<1>(config_.configEntry[translatedFrom]);
  }

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
std::string WeutilImpl::parseUint(int len, unsigned char* ptr) {
  // For now, we only support up to 4 Bytes of data
  if (len > 4) {
    throw std::runtime_error("Unsigned int can be up to 4 bytes only.");
  }
  unsigned int readVal = 0;
  // Values in the EEPROM is little endian
  // Thus cursor starts from the end and goes backwards
  int cursor = len - 1;
  for (int i = 0; i < len; i++) {
    readVal <<= 8;
    readVal |= (unsigned int)ptr[cursor];
    cursor -= 1;
  }
  return std::to_string(readVal);
}

// Parse String field
std::string WeutilImpl::parseString(int len, unsigned char* ptr) {
  // std::string only takes char *, not unsigned char*
  // but the data we want to parse is alphanumeric string
  // whose ASCII value is less than 127. So it is safe to cast
  // unsigned char to char here.
  return std::string((const char*)ptr, len);
}

// For EEPROM V4, Parse MAC with the format XX:XX:XX:XX:XX:XX
std::string WeutilImpl::parseMac(int len, unsigned char* ptr) {
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
std::string WeutilImpl::parseLegacyMac(int len, unsigned char* ptr) {
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

std::string WeutilImpl::parseDate(int len, unsigned char* ptr) {
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

std::string WeutilImpl::parseHex(int len, unsigned char* ptr) {
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

void WeutilImpl::printInfo() {
  XLOG(INFO) << "printInfo";
  std::vector<std::pair<std::string, std::string>> info = getInfo(eepromPath);
  std::cout << "Wedge EEPROM : " << eepromPath << std::endl;
  for (const auto& item : info) {
    std::cout << item.first << ": " << item.second << std::endl;
  }
  return;
}

void WeutilImpl::printInfoJson() {
  // Use getInfo, then go thru our table to generate JSON in order
  XLOG(INFO) << "printInfoJson";
  std::vector<std::pair<std::string, std::string>> info = getInfo(eepromPath);
  int vectorSize = info.size();
  int cursor = 0;
  // Manually create JSON object without using folly, so that this code
  // will be ported to BMC later
  // Print the first part of the JSON - fixed entry to make a JSON
  std::cout << "{";
  std::cout << "\"Information\": {";
  // Print the second part of the JSON, the dynamic entry
  for (auto [key, value] : info) {
    // CRC16 is not needed in JSON output
    if (key == "CRC16") {
      continue;
    }
    std::cout << "\"" << key << "\": ";
    std::cout << "\"" << value << "\"";
    if (cursor++ != vectorSize - 1) {
      std::cout << ", ";
    }
  }

  // Finally, print the third part of the JSON - fixed entry
  std::cout << "}, \"Actions\": [], \"Resources\": []";
  std::cout << "}" << std::endl;
}

bool WeutilImpl::getEepromPath() {
  // If eeprom is in fact the FRU name, replace this name with the actual path
  // If it's not, treat it as the path to the eeprom file / sysfs
  std::string lEeprom;
  if (eepromPath == "") {
    eepromPath = config_.chassisEeprom;
  }
  lEeprom = eepromPath;
  std::transform(lEeprom.begin(), lEeprom.end(), lEeprom.begin(), ::tolower);
  if (config_.configEntry.find(lEeprom) != config_.configEntry.end()) {
    translatedFrom = eepromPath;
    eepromPath = std::get<0>(config_.configEntry[lEeprom]);
  } else {
    throw std::runtime_error("Invalid EEPROM Name.");
  }
  return true;
}
void WeutilImpl::printUsage() {
  std::cout
      << "weutil [--h] [--json] [--eeprom module_name] [--path absolute_path_to_eeprom]"
      << std::endl;
  std::cout << "usage examples:" << std::endl;
  std::cout << "    weutil" << std::endl;
  std::cout << "    weutil --eeprom pem" << std::endl;
  std::cout << "    weutil --path /sys/bus/i2c/drivers/at24/6-0051/eeprom"
            << std::endl;
  std::cout << std::endl;
  std::cout << "Module names are : ";
  for (auto item : config_.configEntry) {
    std::cout << item.first << " ";
  }
  std::cout << std::endl;
  std::cout
      << "Note that supplying --path option will override any module name or config."
      << std::endl;
  XLOG(INFO) << "printUSage";
}

} // namespace facebook::fboss::platform
