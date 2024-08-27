// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <set>
#include <string>

namespace facebook::fboss {

constexpr auto kInfo = "Information";
constexpr auto kSysMfgDate = "System Manufacturing Date";
constexpr auto kSysMfg = "System Manufacturer";
constexpr auto kSysAmbPartNum = "System Assembly Part Number";
constexpr auto kAmbAt = "Assembled At";
constexpr auto kPcbMfg = "PCB Manufacturer";
constexpr auto kProdAssetTag = "Product Asset Tag";
constexpr auto kProdName = "Product Name";
constexpr auto kProdVersion = "Product Version";
constexpr auto kProductionState = "Product Production State";
constexpr auto kProdPartNum = "Product Part Number";
constexpr auto kSerialNum = "Product Serial Number";
constexpr auto kSubVersion = "Product Sub-Version";
constexpr auto kOdmPcbaPartNum = "ODM PCBA Part Number";
constexpr auto kOdmPcbaSerialNum = "ODM PCBA Serial Number";
constexpr auto kFbPcbaPartNum = "Facebook PCBA Part Number";
constexpr auto kFbPcbPartNum = "Facebook PCB Part Number";
constexpr auto kExtMacSize = "Extended MAC Address Size";
constexpr auto kExtMacBase = "Extended MAC Base";
constexpr auto kLocalMac = "Local MAC";
constexpr auto kVersion = "Version";
constexpr auto kFabricLocation = "Location on Fabric";

// Dummy Fruid for fake platform
inline const std::string& getFakeFruIdJson() {
  static const std::string kFakeFruidJson = R"<json>({"Information": {
      "PCB Manufacturer" : "Facebook",
      "System Assembly Part Number" : "42",
      "ODM PCBA Serial Number" : "SN",
      "Product Name" : "fake_wedge",
      "Location on Fabric" : "",
      "ODM PCBA Part Number" : "PN",
      "CRC8" : "0xcc",
      "Version" : "1",
      "Product Asset Tag" : "42",
      "Product Part Number" : "42",
      "Assembled At" : "Facebook",
      "System Manufacturer" : "Facebook",
      "Product Production State" : "42",
      "Facebook PCB Part Number" : "42",
      "Product Serial Number" : "SN",
      "Local MAC" : "42:42:42:42:42:42",
      "Extended MAC Address Size" : "1",
      "Extended MAC Base" : "42:42:42:42:42:42",
      "System Manufacturing Date" : "01-01-01",
      "Product Version" : "42",
      "Product Sub-Version" : "22",
      "Facebook PCBA Part Number" : "42"
    }, "Actions": [], "Resources": []})<json>";
  return kFakeFruidJson;
}

} // namespace facebook::fboss
