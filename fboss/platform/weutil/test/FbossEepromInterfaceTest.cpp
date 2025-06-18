// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>

#include "fboss/platform/weutil/FbossEepromInterface.h"
#include "fboss/platform/weutil/if/gen-cpp2/eeprom_contents_types.h"

namespace facebook::fboss::platform {

namespace {

constexpr auto kProductName = "MINIPACK3N";
constexpr auto kProductPartNumber = "30100077";
constexpr auto kSystemAssemblyPartNumber = "RANDOM";
constexpr auto kMetaPCBAPartNumber = "132100104";
constexpr auto kMetaPCBPartNumber = "131100044";
constexpr auto kOdmJdmPCBAPartNumber = "142000004138A";
constexpr auto kOdmJdmPCBASerialNumber = "AP06040470";
constexpr auto kProductionState = "1";
constexpr auto kProductionSubState = "1";
constexpr auto kVariantIndicator = "0";
constexpr auto kProductSerialNumber = "CP6XM9497401A";
constexpr auto kSystemManufacturer = "ACCTON";
constexpr auto kSystemManufacturingDate = "20250219";
constexpr auto kPCBManufacturer = "GCE";
constexpr auto kAssembledAt = "ACCZB";
constexpr auto kEepromLocationOnFabric = "FCB_T";
constexpr auto kX86CpuMac = "DATA";
constexpr auto kBmcMac = "ac:81:b5:02:9f:5f,1";
constexpr auto kSwitchAsicMac = "ac:81:b5:02:95:bc,274";
constexpr auto kMetaReservedMac = "HERE";
constexpr auto kRma = "TO";
constexpr auto kVendorDefinedField1 = "CHECK";
constexpr auto kVendorDefinedField2 = "IF";
constexpr auto kVendorDefinedField3 = "ASSIGNED";
constexpr auto kCrc16 = "0xd6d6 (CRC Matched)";

FbossEepromInterface createFbossEepromInterface(int version) {
  auto result = FbossEepromInterface::createEepromInterface(version);
  result.setField(1, kProductName);
  result.setField(2, kProductPartNumber);
  result.setField(3, kSystemAssemblyPartNumber);
  result.setField(4, kMetaPCBAPartNumber);
  result.setField(5, kMetaPCBPartNumber);
  result.setField(6, kOdmJdmPCBAPartNumber);
  result.setField(7, kOdmJdmPCBASerialNumber);
  result.setField(8, kProductionState);
  result.setField(9, kProductionSubState);
  result.setField(10, kVariantIndicator);
  result.setField(11, kProductSerialNumber);
  result.setField(12, kSystemManufacturer);
  result.setField(13, kSystemManufacturingDate);
  result.setField(14, kPCBManufacturer);
  result.setField(15, kAssembledAt);
  result.setField(16, kEepromLocationOnFabric);
  result.setField(17, kX86CpuMac);
  result.setField(18, kBmcMac);
  result.setField(19, kSwitchAsicMac);
  result.setField(20, kMetaReservedMac);
  result.setField(250, kCrc16);

  if (version == 6) {
    result.setField(21, kRma);
    result.setField(101, kVendorDefinedField1);
    result.setField(102, kVendorDefinedField2);
    result.setField(103, kVendorDefinedField3);
  }

  return result;
}

EepromContents createEepromContents(int version) {
  EepromContents result;
  result.version() = version;
  result.productName() = kProductName;
  result.productPartNumber() = kProductPartNumber;
  result.systemAssemblyPartNumber() = kSystemAssemblyPartNumber;
  result.metaPCBAPartNumber() = kMetaPCBAPartNumber;
  result.metaPCBPartNumber() = kMetaPCBPartNumber;
  result.odmJdmPCBAPartNumber() = kOdmJdmPCBAPartNumber;
  result.odmJdmPCBASerialNumber() = kOdmJdmPCBASerialNumber;
  result.productionState() = kProductionState;
  result.productionSubState() = kProductionSubState;
  result.variantIndicator() = kVariantIndicator;
  result.productSerialNumber() = kProductSerialNumber;
  result.systemManufacturer() = kSystemManufacturer;
  result.systemManufacturingDate() = kSystemManufacturingDate;
  result.pcbManufacturer() = kPCBManufacturer;
  result.assembledAt() = kAssembledAt;
  result.eepromLocationOnFabric() = kEepromLocationOnFabric;
  result.x86CpuMac() = kX86CpuMac;
  result.bmcMac() = kBmcMac;
  result.switchAsicMac() = kSwitchAsicMac;
  result.metaReservedMac() = kMetaReservedMac;
  result.crc16() = kCrc16;

  // V6 unique fields
  if (version == 6) {
    result.rma() = kRma;
    result.vendorDefinedField1() = kVendorDefinedField1;
    result.vendorDefinedField2() = kVendorDefinedField2;
    result.vendorDefinedField3() = kVendorDefinedField3;
  }

  return result;
}

} // namespace

TEST(FbossEepromInterfaceTest, V5) {
  auto eeprom = createFbossEepromInterface(5);

  EXPECT_EQ(eeprom.getProductName(), kProductName);
  EXPECT_EQ(eeprom.getProductPartNumber(), kProductPartNumber);
  EXPECT_EQ(eeprom.getProductionState(), kProductionState);
  EXPECT_EQ(eeprom.getProductionSubState(), kProductionSubState);
  EXPECT_EQ(eeprom.getVariantVersion(), kVariantIndicator);
  EXPECT_EQ(eeprom.getProductSerialNumber(), kProductSerialNumber);
}

TEST(FbossEepromInterfaceTest, V6) {
  auto eeprom = createFbossEepromInterface(6);
  EXPECT_EQ(eeprom.getProductName(), kProductName);
  EXPECT_EQ(eeprom.getProductPartNumber(), kProductPartNumber);
  EXPECT_EQ(eeprom.getProductionState(), kProductionState);
  EXPECT_EQ(eeprom.getProductionSubState(), kProductionSubState);
  EXPECT_EQ(eeprom.getVariantVersion(), kVariantIndicator);
  EXPECT_EQ(eeprom.getProductSerialNumber(), kProductSerialNumber);
}

TEST(FbossEepromInterfaceTest, V5Object) {
  auto eepromInterace = createFbossEepromInterface(5);
  auto actualObj = eepromInterace.getEepromContents();

  EepromContents expectedObj = createEepromContents(5);

  EXPECT_EQ(actualObj, expectedObj);
}

TEST(FbossEepromInterfaceTest, V6Object) {
  auto eepromInterace = createFbossEepromInterface(6);
  auto actualObj = eepromInterace.getEepromContents();
  EepromContents expectedObj = createEepromContents(6);

  EXPECT_EQ(actualObj, expectedObj);
}

} // namespace facebook::fboss::platform
