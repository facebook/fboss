#include "fboss/platform/weutil/FbossEepromInterface.h"

#include <folly/String.h>

#include <stdexcept>

namespace facebook::fboss::platform {

namespace {

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
    default:
      throw std::runtime_error(
          "Invalid EEPROM version : " + std::to_string(version));
  }
  return result;
}

void FbossEepromInterface::setField(int typeCode, const std::string& value) {
  fieldMap_.at(typeCode).value = value;
}

const std::map<int, FbossEepromInterface::EepromFieldEntry>&
FbossEepromInterface::getFieldDictionary() const {
  return fieldMap_;
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
    throw std::runtime_error(fmt::format(
        "Invalid FbossEepromInterface structure. Version: {}, Available keys: [{}]",
        version_,
        joinedKeys));
  }
  return result;
}

} // namespace facebook::fboss::platform
