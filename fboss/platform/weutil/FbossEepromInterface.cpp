#include "fboss/platform/weutil/FbossEepromInterface.h"
#include <stdexcept>

namespace facebook::fboss::platform {

namespace {

using entryType = FbossEepromInterface::entryType;
using entryType::FIELD_BE_HEX;
using entryType::FIELD_BE_UINT;
using entryType::FIELD_MAC;
using entryType::FIELD_STRING;
std::map<int, FbossEepromInterface::EepromFieldEntry> getV5FieldData() {
  std::map<int, FbossEepromInterface::EepromFieldEntry> result = {
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
  return result;
}

std::map<int, FbossEepromInterface::EepromFieldEntry> getV6FieldData() {
  std::map<int, FbossEepromInterface::EepromFieldEntry> result = {
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
  return result;
}

} // namespace

FbossEepromInterface FbossEepromInterface::createEepromInterface(int version) {
  FbossEepromInterface result;
  result.version_ = version;
  switch (version) {
    case 5:
      result.fieldMap_ = getV5FieldData();
      break;
    case 6:
      result.fieldMap_ = getV6FieldData();
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

EepromContents FbossEepromInterface::getContents() const {
  EepromContents contents;
  contents.emplace_back("Version", std::to_string(getVersion()));
  for (const auto& [_, entry] : getFieldDictionary()) {
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

} // namespace facebook::fboss::platform
