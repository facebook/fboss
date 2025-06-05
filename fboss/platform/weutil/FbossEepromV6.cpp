// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/weutil/FbossEepromV6.h"

namespace facebook::fboss::platform {

std::vector<FbossEepromV6::EepromFieldEntry>
FbossEepromV6::getFieldDictionary() {
  std::vector<EepromFieldEntry> fieldsDictionary = {
      {0, "NA", FIELD_BE_UINT, nullptr, -1}, // TypeCode 0 is reserved
      {1, "Product Name", FIELD_STRING, &productName},
      {2, "Product Part Number", FIELD_STRING, &productPartNumber},
      {3,
       "System Assembly Part Number",
       FIELD_STRING,
       &systemAssemblyPartNumber,
       8},
      {4, "Meta PCBA Part Number", FIELD_STRING, &metaPCBAPartNumber, 12},
      {5, "Meta PCB Part Number", FIELD_STRING, &metaPCBPartNumber, 12},
      {6, "ODM/JDM PCBA Part Number", FIELD_STRING, &odmJdmPCBAPartNumber},
      {7, "ODM/JDM PCBA Serial Number", FIELD_STRING, &odmJdmPCBASerialNumber},
      {8, "Production State", FIELD_BE_UINT, &productionState, 1},
      {9, "Production Sub-State", FIELD_BE_UINT, &productionSubState, 1},
      {10, "Re-Spin/Variant Indicator", FIELD_BE_UINT, &variantIndicator, 1},
      {11, "Product Serial Number", FIELD_STRING, &productSerialNumber},
      {12, "System Manufacturer", FIELD_STRING, &systemManufacturer},
      {13,
       "System Manufacturing Date",
       FIELD_STRING,
       &systemManufacturingDate,
       8},
      {14, "PCB Manufacturer", FIELD_STRING, &pcbManufacturer},
      {15, "Assembled At", FIELD_STRING, &assembledAt},
      {16, "EEPROM location on Fabric", FIELD_STRING, &eepromLocationOnFabric},
      {17, "X86 CPU MAC", FIELD_MAC, &x86CpuMac, 8},
      {18, "BMC MAC", FIELD_MAC, &bmcMac, 8},
      {19, "Switch ASIC MAC", FIELD_MAC, &switchAsicMac, 8},
      {20, "META Reserved MAC", FIELD_MAC, &metaReservedMac, 8},
      {21, "RMA", FIELD_BE_UINT, &rma, 1},
      {101, "Vendor Defined Field 1", FIELD_BE_HEX, &vendorDefinedField1},
      {102, "Vendor Defined Field 2", FIELD_BE_HEX, &vendorDefinedField2},
      {103, "Vendor Defined Field 3", FIELD_BE_HEX, &vendorDefinedField3},
      {250, "CRC16", FIELD_BE_HEX, &crc16, 2}};
  return fieldsDictionary;
}

constexpr int FbossEepromV6::getVersion() const {
  return 6;
}

std::string FbossEepromV6::getProductionState() const {
  return productionState;
}

std::string FbossEepromV6::getProductionSubState() const {
  return productionSubState;
}

std::string FbossEepromV6::getVariantVersion() const {
  return variantIndicator;
}

} // namespace facebook::fboss::platform
