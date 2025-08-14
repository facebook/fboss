// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <cstring>
#include <iomanip>
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

// ONIE TlvInfo format constants
constexpr char kOnieTlvInfoIdString[] = "TlvInfo";
constexpr int kOnieTlvInfoVersion = 0x01;
constexpr int kOnieTlvInfoHdrLen = 11;
constexpr int kOnieTlvInfoMaxLen = 2048;
constexpr int kOnieCrcSize = 4;

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

// ONIE TlvInfo format field dictionary (version 1000)
const std::map<int, FbossEepromInterface::EepromFieldEntry> kOnieMap = {
    {0x21, {"Product Name", FIELD_STRING}},
    {0x22, {"Part Number", FIELD_STRING}},
    {0x23, {"Serial Number", FIELD_STRING}},
    {0x24, {"Base MAC Address", FIELD_MAC, 6}},
    {0x25, {"Manufacture Date", FIELD_STRING}},
    {0x26, {"Device Version", FIELD_BE_UINT, 1}},
    {0x27, {"Label Revision", FIELD_STRING}},
    {0x28, {"Platform Name", FIELD_STRING}},
    {0x29, {"ONIE Version", FIELD_STRING}},
    {0x2A, {"MAC Addresses", FIELD_BE_UINT, 2}},
    {0x2B, {"Manufacturer", FIELD_STRING}},
    {0x2C, {"Manufacture Country", FIELD_STRING}},
    {0x2D, {"Vendor Name", FIELD_STRING}},
    {0x2E, {"Diag Version", FIELD_STRING}},
    {0x2F, {"Service Tag", FIELD_STRING}},
    {0xFD, {"Vendor Extension", FIELD_BE_HEX}},
    {0xFE, {"CRC-32", FIELD_BE_HEX, 4}},
};

} // namespace

FbossEepromInterface::FbossEepromInterface(
    const std::string& eepromPath,
    const uint16_t offset) {
  auto buffer = ParserUtils::loadEeprom(eepromPath, offset);
  if (buffer.size() < kHeaderSize) {
    throw std::runtime_error("Invalid EEPROM size");
  }

  // Check if this is ONIE TlvInfo format
  if (isOnieTlvInfoFormat(buffer)) {
    version_ = kOnieEepromVersion;
    fieldMap_ = kOnieMap;
    parseEepromBlobTLVOnie(buffer);
    return;
  }

  // Check for FBOSS EEPROM format signature (0xFBFB)
  if (buffer[0] != 0xFB || buffer[1] != 0xFB) {
    std::stringstream ss;
    ss << "Invalid FBOSS EEPROM format: Expected signature 0xFBFB, got 0x"
       << std::hex << std::uppercase << std::setfill('0') << std::setw(2)
       << static_cast<int>(buffer[0]) << std::setw(2)
       << static_cast<int>(buffer[1]);
    throw std::runtime_error(ss.str());
  }

  // Parse Meta EEPROM format
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

FbossEepromInterface FbossEepromInterface::createEepromInterface(int version) {
  FbossEepromInterface result;
  result.version_ = version;
  switch (version) {
    case 5:
      result.fieldMap_ = kV5Map;
      break;
    case 6:
      result.fieldMap_ = kV6Map;
      break;
    case kOnieEepromVersion:
      result.fieldMap_ = kOnieMap;
      break;
    default:
      throw std::runtime_error(
          "Invalid EEPROM version : " + std::to_string(version));
  }
  return result;
}

std::vector<std::pair<std::string, std::string>>
FbossEepromInterface::getContents() const {
  std::vector<std::pair<std::string, std::string>> contents;

  // Handle version display differently for ONIE format
  if (version_ == kOnieEepromVersion) {
    contents.emplace_back("Format", "ONIE TlvInfo");
  } else {
    contents.emplace_back("Version", std::to_string(getVersion()));
  }

  for (const auto& [_, entry] : fieldMap_) {
    if (entry.fieldName == "NA") {
      continue;
    }
    if (entry.fieldType == FIELD_MAC) {
      if (version_ == kOnieEepromVersion) {
        // ONIE format: MAC is just the address, no size field
        contents.emplace_back(entry.fieldName, entry.value);
      } else {
        // Meta format: MAC field is composite with base and size
        // Format: (00:00:00:00:00:00,222)
        std::string value1 = entry.value.substr(0, entry.value.find(','));
        std::string value2 = entry.value.substr(entry.value.find(',') + 1);
        contents.emplace_back(entry.fieldName + " Base", value1);
        contents.emplace_back(entry.fieldName + " Address Size", value2);
      }
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
  return fieldMap_.at(version_ < kOnieEepromVersion ? 1 : 0x21).value;
}

std::string FbossEepromInterface::getProductPartNumber() const {
  return fieldMap_.at(version_ < kOnieEepromVersion ? 2 : 0x22).value;
}

std::string FbossEepromInterface::getProductionState() const {
  if (version_ >= kOnieEepromVersion) {
    return "1"; // Production State not available in ONIE format
  }
  return fieldMap_.at(8).value;
}

std::string FbossEepromInterface::getProductionSubState() const {
  if (version_ >= kOnieEepromVersion) {
    return "1"; // Production Sub-State not available in ONIE format
  }
  return fieldMap_.at(9).value;
}

std::string FbossEepromInterface::getVariantVersion() const {
  return fieldMap_.at(version_ < kOnieEepromVersion ? 10 : 0x26).value;
}

std::string FbossEepromInterface::getProductSerialNumber() const {
  return fieldMap_.at(version_ < kOnieEepromVersion ? 11 : 0x23).value;
}

EepromContents FbossEepromInterface::getEepromContents() const {
  EepromContents result;
  result.version() = version_;
  try {
    result.productName() = getProductName();
    result.productPartNumber() = getProductPartNumber();
    result.productionState() = getProductionState();
    result.productionSubState() = getProductionSubState();
    result.variantIndicator() = getVariantVersion();
    result.productSerialNumber() = getProductSerialNumber();
    if (version_ < kOnieEepromVersion) {
      result.systemAssemblyPartNumber() = fieldMap_.at(3).value;
      result.metaPCBAPartNumber() = fieldMap_.at(4).value;
      result.metaPCBPartNumber() = fieldMap_.at(5).value;
      result.odmJdmPCBAPartNumber() = fieldMap_.at(6).value;
      result.odmJdmPCBASerialNumber() = fieldMap_.at(7).value;
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
    } else { // ONIE EEPROM format
      result.systemAssemblyPartNumber() = "NA";
      result.metaPCBAPartNumber() = "NA";
      result.metaPCBPartNumber() = "NA";
      result.odmJdmPCBAPartNumber() = "NA";
      result.odmJdmPCBASerialNumber() = "NA";
      result.systemManufacturer() = fieldMap_.at(0x2D).value;
      result.systemManufacturingDate() = "NA";
      result.pcbManufacturer() = fieldMap_.at(0x2B).value;
      result.assembledAt() = fieldMap_.at(0x2C).value;
      result.eepromLocationOnFabric() = "NA";
      // TODO: figure out how to allocate the MAC addresses across those
      // fields. The number of MAC addresses we have is defined by field 0x2A.
      result.x86CpuMac() = fieldMap_.at(0x24).value;
      result.bmcMac() = fieldMap_.at(0x24).value;
      result.switchAsicMac() = fieldMap_.at(0x24).value;
      result.metaReservedMac() = "NA";

      result.rma() = fieldMap_.at(0x2F).value; // unsure
      result.vendorDefinedField1() = fieldMap_.at(0xFD).value;
      result.vendorDefinedField2() = fieldMap_.at(0x2E).value; // unsure
      result.crc16() = fieldMap_.at(0xFE).value; // even though it's a CRC32...
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

void FbossEepromInterface::parseEepromBlobTLVOnie(
    const std::vector<uint8_t>& buffer) {
  // Validate ONIE header
  if (!isOnieTlvInfoFormat(buffer)) {
    throw std::runtime_error("Invalid ONIE TlvInfo format");
  }

  // Get total length from header
  uint16_t totalLen = (buffer[9] << 8) | buffer[10];
  int tlvEnd = kOnieTlvInfoHdrLen + totalLen;

  // Start parsing TLVs after the header
  int cursor = kOnieTlvInfoHdrLen;

  while (cursor < buffer.size() && cursor < tlvEnd) {
    // Check if we have at least 2 bytes for TLV header
    if (cursor + 2 > buffer.size()) {
      break;
    }

    int itemCode = static_cast<int>(buffer[cursor]);
    int itemLength = static_cast<int>(buffer[cursor + 1]);

    // Check if we have enough bytes for the value
    if (cursor + 2 + itemLength > buffer.size()) {
      break;
    }

    unsigned char* itemDataPtr =
        const_cast<unsigned char*>(&buffer[cursor + 2]);
    std::string value;

    // Parse based on known ONIE TLV codes
    switch (itemCode) {
      case 0x21: // Product Name
      case 0x22: // Part Number
      case 0x23: // Serial Number
      case 0x25: // Manufacture Date
      case 0x27: // Label Revision
      case 0x28: // Platform Name
      case 0x29: // ONIE Version
      case 0x2B: // Manufacturer
      case 0x2C: // Manufacture Country
      case 0x2D: // Vendor Name
      case 0x2E: // Diag Version
      case 0x2F: // Service Tag
        value = ParserUtils::parseString(itemLength, itemDataPtr);
        break;
      case 0x24: // Base MAC Address
        value = ParserUtils::parseOnieMac(itemLength, itemDataPtr);
        break;
      case 0x26: // Device Version
      case 0x2A: // MAC Addresses
        value = ParserUtils::parseBeUint(itemLength, itemDataPtr);
        break;
      case 0xFD: // Vendor Extension
      case 0xFE: // CRC-32
        value = ParserUtils::parseBeHex(itemLength, itemDataPtr);
        break;
      default:
        std::cout << " Unknown field code " << itemCode << " at position "
                  << cursor << std::endl;
        throw std::runtime_error(
            "Invalid field code in ONIE EEPROM at :" + std::to_string(cursor));
        break;
    }

    fieldMap_.at(itemCode).value = value;

    // Handle CRC-32 validation
    if (itemCode == 0xFE) { // CRC-32 code
      uint32_t crcProgrammed = std::stoul(value, nullptr, 16);
      // Calculate CRC over data including CRC TLV header but excluding CRC
      // value cursor points to start of CRC TLV, so include TLV header (2
      // bytes) but exclude CRC value (4 bytes)
      uint32_t crcCalculated = calculateCrc32(buffer.data(), cursor + 2);
      if (crcProgrammed == crcCalculated) {
        value.append(" (CRC Matched)");
      } else {
        std::stringstream ss;
        ss << "0x" << std::hex << crcCalculated;
        value.append(" (CRC Mismatch. Expected " + ss.str() + ")");
      }
      fieldMap_.at(itemCode).value = value;
      break; // CRC is the last field
    }

    cursor += 2 + itemLength;
  }
}

bool FbossEepromInterface::isOnieTlvInfoFormat(
    const std::vector<uint8_t>& buffer) {
  // Check if we have enough bytes for the ONIE header
  if (buffer.size() < kOnieTlvInfoHdrLen) {
    return false;
  }

  // Check for "TlvInfo\x00" signature (8 bytes)
  if (std::memcmp(buffer.data(), kOnieTlvInfoIdString, 7) != 0 ||
      buffer[7] != 0x00) {
    return false;
  }

  // Check version byte (should be 0x01)
  if (buffer[8] != kOnieTlvInfoVersion) {
    return false;
  }

  // Check total length field (bytes 9-10)
  uint16_t totalLen = (buffer[9] << 8) | buffer[10];
  if (totalLen > (kOnieTlvInfoMaxLen - kOnieTlvInfoHdrLen)) {
    return false;
  }

  return true;
}

uint32_t FbossEepromInterface::calculateCrc32(
    const uint8_t* buffer,
    size_t len) {
  // Standard CRC-32 polynomial (IEEE 802.3)
  const uint32_t polynomial = 0xEDB88320;
  uint32_t crc = 0xFFFFFFFF;

  for (size_t i = 0; i < len; i++) {
    crc ^= buffer[i];
    for (int j = 0; j < 8; j++) {
      if (crc & 1) {
        crc = (crc >> 1) ^ polynomial;
      } else {
        crc >>= 1;
      }
    }
  }

  return ~crc;
}

} // namespace facebook::fboss::platform
