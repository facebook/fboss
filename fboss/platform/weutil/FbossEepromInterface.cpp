// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <iostream>
#include <sstream>
#include <stdexcept>

#include <folly/String.h>

#include "fboss/platform/weutil/FbossEepromInterface.h"
#include "fboss/platform/weutil/ParserUtils.h"

namespace facebook::fboss::platform {

namespace {

// Header size in EEPROM. First two bytes are 0xFBFB followed
// by a byte specifying the EEPROM version and one byte of 0xFF
constexpr int kHeaderSize = 4;
// Field Type and Length are 1 byte each.
constexpr int kEepromTypeLengthSize = 2;

using entryType = FbossEepromInterface::entryType;
using entryType::FIELD_BE_HEX;
using entryType::FIELD_BE_UINT;
using entryType::FIELD_MAC;
using entryType::FIELD_STRING;

const std::map<int, FbossEepromInterface::EepromFieldEntry> kV5Map = {
    {0, {"NA", FIELD_BE_UINT, -1}},
    {1, {"Product Name", FIELD_STRING}},
    {2, {"Product Part Number", FIELD_STRING}},
    {3, {"System Assembly Part Number", FIELD_STRING, 8}},
    {4, {"Meta PCBA Part Number", FIELD_STRING, 12}},
    {5, {"Meta PCB Part Number", FIELD_STRING, 12}},
    {6, {"ODM/JDM PCBA Part Number", FIELD_STRING}},
    {7, {"ODM/JDM PCBA Serial Number", FIELD_STRING}},
    {8, {"Product Production State", FIELD_BE_UINT, 1}},
    {9, {"Product Version", FIELD_BE_UINT, 1}},
    {10, {"Product Sub-Version", FIELD_BE_UINT, 1}},
    {11, {"Product Serial Number", FIELD_STRING}},
    {12, {"System Manufacturer", FIELD_STRING}},
    {13, {"System Manufacturing Date", FIELD_STRING, 8}},
    {14, {"PCB Manufacturer", FIELD_STRING}},
    {15, {"Assembled At", FIELD_STRING}},
    {16, {"EEPROM location on Fabric", FIELD_STRING}},
    {17, {"X86 CPU MAC", FIELD_MAC, 8}},
    {18, {"BMC MAC", FIELD_MAC, 8}},
    {19, {"Switch ASIC MAC", FIELD_MAC, 8}},
    {20, {"META Reserved MAC", FIELD_MAC, 8}},
    {250, {"CRC16", FIELD_BE_HEX, 2}}};

const std::map<int, FbossEepromInterface::EepromFieldEntry> kV6Map = {
    {0, {"NA", FIELD_BE_UINT, -1}}, // TypeCode 0 is reserved
    {1, {"Product Name", FIELD_STRING}},
    {2, {"Product Part Number", FIELD_STRING}},
    {3, {"System Assembly Part Number", FIELD_STRING, 8}},
    {4, {"Meta PCBA Part Number", FIELD_STRING, 12}},
    {5, {"Meta PCB Part Number", FIELD_STRING, 12}},
    {6, {"ODM/JDM PCBA Part Number", FIELD_STRING}},
    {7, {"ODM/JDM PCBA Serial Number", FIELD_STRING}},
    {8, {"Production State", FIELD_BE_UINT, 1}},
    {9, {"Production Sub-State", FIELD_BE_UINT, 1}},
    {10, {"Re-Spin/Variant Indicator", FIELD_BE_UINT, 1}},
    {11, {"Product Serial Number", FIELD_STRING}},
    {12, {"System Manufacturer", FIELD_STRING}},
    {13, {"System Manufacturing Date", FIELD_STRING, 8}},
    {14, {"PCB Manufacturer", FIELD_STRING}},
    {15, {"Assembled At", FIELD_STRING}},
    {16, {"EEPROM location on Fabric", FIELD_STRING}},
    {17, {"X86 CPU MAC", FIELD_MAC, 8}},
    {18, {"BMC MAC", FIELD_MAC, 8}},
    {19, {"Switch ASIC MAC", FIELD_MAC, 8}},
    {20, {"META Reserved MAC", FIELD_MAC, 8}},
    {21, {"RMA", FIELD_BE_UINT, 1}},
    {101, {"Vendor Defined Field 1", FIELD_BE_HEX}},
    {102, {"Vendor Defined Field 2", FIELD_BE_HEX}},
    {103, {"Vendor Defined Field 3", FIELD_BE_HEX}},
    {250, {"CRC16", FIELD_BE_HEX, 2}}};

} // namespace

FbossEepromInterface::FbossEepromInterface(
    const std::string& eepromPath,
    const uint16_t offset) {
  auto buffer = ParserUtils::loadEeprom(eepromPath, offset);
  if (buffer.size() < kHeaderSize) {
    throw std::runtime_error("Invalid EEPROM size");
  }

  version_ = buffer.at(2);
  switch (version_) {
    case 5:
      fieldMap_ = kV5Map;
      break;
    case 6:
      fieldMap_ = kV6Map;
      break;
    default:
      throw std::runtime_error(
          "Invalid EEPROM version : " + std::to_string(version_));
  }

  parseEepromBlobTLV(buffer);
}

void FbossEepromInterface::parseEepromBlobTLV(
    const std::vector<uint8_t>& buffer) {
  // A variable to count the number of items parsed so far
  int juice = 0;
  // According to the Meta EEPROM V5 spec and later,
  // the actual data starts from 4th byte of eeprom.
  int cursor = kHeaderSize;

  std::string value;

  while (cursor < buffer.size()) {
    // Increment the item counter (mainly for debugging purposes)
    // Very important to do this.
    juice = juice + 1;
    // First, get the itemCode of the TLV (T)
    int fieldCode = static_cast<int>(buffer[cursor]);

    // Vendors pad EEPROM with 0xff. Therefore, if item code is
    // 0xff, then we reached to the end of the actual content.
    if (fieldCode == 0xFF) {
      break;
    }

    FbossEepromInterface::entryType fieldType{FIELD_INVALID};
    std::string fieldName;
    try {
      fieldType = fieldMap_.at(fieldCode).fieldType;
      fieldName = fieldMap_.at(fieldCode).fieldName;
    }
    // If no entry found, throw an exception
    catch (const std::out_of_range&) {
      std::cout << " Unknown field code " << fieldCode << " at position "
                << cursor << " item number " << juice << std::endl;
      throw std::runtime_error(
          "Invalid field code in EEPROM at :" + std::to_string(cursor));
    }

    // Find Length and Variable (L and V)
    int itemLength = buffer[cursor + 1];
    unsigned char* itemDataPtr =
        (unsigned char*)&buffer[cursor + kEepromTypeLengthSize];
    // Parse the value according to the itemType
    switch (fieldType) {
      case FIELD_BE_UINT:
        value = ParserUtils::parseBeUint(itemLength, itemDataPtr);
        break;
      case FIELD_BE_HEX:
        value = ParserUtils::parseBeHex(itemLength, itemDataPtr);
        break;
      case FIELD_STRING:
        value = ParserUtils::parseString(itemLength, itemDataPtr);
        break;
      case FIELD_MAC:
        value = ParserUtils::parseMac(itemLength, itemDataPtr);
        break;
      default:
        std::cout << " Unknown field type " << fieldType << " at position "
                  << cursor << " item number " << juice << std::endl;
        throw std::runtime_error("Invalid field type in EEPROM.");
    }
    // Fill the corresponding field
    fieldMap_.at(fieldCode).value = folly::trimWhitespace(value).str();
    // Increment the cursor
    cursor += itemLength + kEepromTypeLengthSize;
    // the CRC16 is the last content, parsing must stop.
    if (fieldName == "CRC16") {
      uint16_t crcProgrammed = std::stoi(value, nullptr, 16);
      uint16_t crcCalculated =
          ParserUtils::calculateCrc16(buffer.data(), cursor);
      if (crcProgrammed == crcCalculated) {
        value.append(" (CRC Matched)");
      } else {
        std::stringstream ss;
        ss << std::hex << crcCalculated;
        value.append(" (CRC Mismatch. Expected 0x" + ss.str() + ")");
      }
      fieldMap_.at(fieldCode).value = value;
      break;
    }
  }
}

std::vector<std::pair<std::string, std::string>>
FbossEepromInterface::getContents() const {
  std::vector<std::pair<std::string, std::string>> contents;
  contents.emplace_back("Version", std::to_string(getVersion()));
  for (const auto& [_, entry] : fieldMap_) {
    if (entry.fieldName == "NA") {
      continue;
    }
    if (entry.fieldType == FIELD_MAC) {
      // Format
      // (value): (00:00:00:00:00:00,222)
      // (value1);(value2): (00:00:00:00:00:00);(222)
      std::string value1 = entry.value.substr(0, entry.value.find(','));
      std::string value2 = entry.value.substr(entry.value.find(',') + 1);
      contents.emplace_back(entry.fieldName + " Base", value1);
      contents.emplace_back(entry.fieldName + " Address Size", value2);
    } else {
      contents.emplace_back(entry.fieldName, entry.value);
    }
  }
  return contents;
}

int FbossEepromInterface::getVersion() const {
  return version_;
}

std::string FbossEepromInterface::getProductName() const {
  return fieldMap_.at(1).value;
}

std::string FbossEepromInterface::getProductPartNumber() const {
  return fieldMap_.at(2).value;
}

std::string FbossEepromInterface::getProductionState() const {
  return fieldMap_.at(8).value;
}

std::string FbossEepromInterface::getProductionSubState() const {
  return fieldMap_.at(9).value;
}

std::string FbossEepromInterface::getVariantVersion() const {
  return fieldMap_.at(10).value;
}

std::string FbossEepromInterface::getProductSerialNumber() const {
  return fieldMap_.at(11).value;
}

EepromContents FbossEepromInterface::getEepromContents() const {
  EepromContents result;
  result.version() = version_;
  try {
    result.productName() = fieldMap_.at(1).value;
    result.productPartNumber() = fieldMap_.at(2).value;
    result.systemAssemblyPartNumber() = fieldMap_.at(3).value;
    result.metaPCBAPartNumber() = fieldMap_.at(4).value;
    result.metaPCBPartNumber() = fieldMap_.at(5).value;
    result.odmJdmPCBAPartNumber() = fieldMap_.at(6).value;
    result.odmJdmPCBASerialNumber() = fieldMap_.at(7).value;
    result.productionState() = fieldMap_.at(8).value;
    result.productionSubState() = fieldMap_.at(9).value;
    result.variantIndicator() = fieldMap_.at(10).value;
    result.productSerialNumber() = fieldMap_.at(11).value;
    result.systemManufacturer() = fieldMap_.at(12).value;
    result.systemManufacturingDate() = fieldMap_.at(13).value;
    result.pcbManufacturer() = fieldMap_.at(14).value;
    result.assembledAt() = fieldMap_.at(15).value;
    result.eepromLocationOnFabric() = fieldMap_.at(16).value;
    result.x86CpuMac() = fieldMap_.at(17).value;
    result.bmcMac() = fieldMap_.at(18).value;
    result.switchAsicMac() = fieldMap_.at(19).value;
    result.metaReservedMac() = fieldMap_.at(20).value;
    result.crc16() = fieldMap_.at(250).value;

    // V6 unique fields
    if (version_ == 6) {
      result.rma() = fieldMap_.at(21).value;
      result.vendorDefinedField1() = fieldMap_.at(101).value;
      result.vendorDefinedField2() = fieldMap_.at(102).value;
      result.vendorDefinedField3() = fieldMap_.at(103).value;
    }
  } catch (const std::out_of_range&) {
    auto availableKeys = fieldMap_ | std::views::keys;
    std::string joinedKeys = folly::join(", ", availableKeys);
    throw std::runtime_error(
        fmt::format(
            "Invalid FbossEepromInterface structure. Version: {}, Available keys: [{}]",
            version_,
            joinedKeys));
  }
  return result;
}

} // namespace facebook::fboss::platform
