// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <unordered_map>
#include <utility>

#include <folly/Conv.h>
#include <folly/Format.h>
#include <folly/json.h>
#include <folly/logging/xlog.h>
#include <filesystem>
#include <string>

#include "fboss/platform/helpers/PlatformUtils.h"
#include "fboss/platform/weutil/WeutilImpl.h"
#include "fboss/platform/weutil/prefdl/Prefdl.h"

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

std::vector<std::pair<std::string, std::string>> WeutilImpl::getInfo(
    const std::string& eeprom) {
  std::vector<std::pair<std::string, std::string>> info;

  XLOG(INFO) << "getInfo: eeprom = " << eeprom;
  return info;
}

void WeutilImpl::printInfo() {
  XLOG(INFO) << "printInfo";
}

void WeutilImpl::printInfoJson() {
  XLOG(INFO) << "printInfoJson";
}

bool WeutilImpl::verifyOptions() {
  // If eeprom is in fact the FRU name, replace this name with the actual path
  // If it's not, treat it as the path to the eeprom file / sysfs
  std::string lEeprom = eepromPath;
  std::transform(lEeprom.begin(), lEeprom.end(), lEeprom.begin(), ::tolower);
  if (config_.configEntry.find(lEeprom) != config_.configEntry.end()) {
    eepromPath = get<0>(config_.configEntry[lEeprom]);
  } else {
    throw std::runtime_error("Invalid EEPROM Name.");
  }
  return true;
}
void WeutilImpl::printUsage() {
  XLOG(INFO) << "printUSage";
}

} // namespace facebook::fboss::platform
