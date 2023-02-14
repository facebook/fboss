/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/lib/platforms/PlatformProductInfo.h"
#include "fboss/agent/FbossError.h"

#include <boost/algorithm/string.hpp>
#include <folly/FileUtil.h>
#include <folly/MacAddress.h>
#include <folly/dynamic.h>
#include <folly/experimental/TestUtil.h>
#include <folly/json.h>
#include <folly/logging/xlog.h>

namespace {
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
} // namespace

DEFINE_string(
    mode,
    "",
    "The mode the FBOSS controller is running as, wedge, lc, or fc");
DEFINE_string(
    fruid_filepath,
    "/var/facebook/fboss/fruid.json",
    "File for storing the fruid data");

namespace facebook::fboss {

using folly::dynamic;
using folly::MacAddress;
using folly::parseJson;
using folly::StringPiece;

PlatformProductInfo::PlatformProductInfo(StringPiece path) : path_(path) {}

void PlatformProductInfo::initialize() {
  try {
    std::string data;
    folly::readFile(path_.str().c_str(), data);
    parse(data);
  } catch (const std::exception& err) {
    XLOG(ERR) << "Failed initializing ProductInfo from " << path_
              << ", fall back to use fbwhoami: " << err.what();
    // if fruid info fails fall back to fbwhoami
    initFromFbWhoAmI();
  }
  initMode();
}

std::string PlatformProductInfo::getFabricLocation() {
  return *productInfo_.fabricLocation();
}

std::string PlatformProductInfo::getProductName() {
  return *productInfo_.product();
}

void PlatformProductInfo::initMode() {
  if (FLAGS_mode.empty()) {
    auto modelName = getProductName();
    if (modelName.find("MINIPACK2") == 0) {
      mode_ = PlatformMode::FUJI;
    } else if (
        modelName.find("Wedge100") == 0 || modelName.find("WEDGE100") == 0) {
      // Wedge100 comes from fruid.json, WEDGE100 comes from fbwhoami
      mode_ = PlatformMode::WEDGE100;
    } else if (
        modelName.find("Wedge400c") == 0 || modelName.find("WEDGE400C") == 0) {
      mode_ = PlatformMode::WEDGE400C;
    } else if (
        modelName.find("Wedge400") == 0 || modelName.find("WEDGE400") == 0) {
      mode_ = PlatformMode::WEDGE400;
    } else if (
        modelName.find("Darwin") == 0 || modelName.find("DARWIN") == 0 ||
        modelName.find("DCS-7060") == 0 || modelName.find("Rackhawk") == 0) {
      mode_ = PlatformMode::DARWIN;
    } else if (modelName.find("Wedge") == 0 || modelName.find("WEDGE") == 0) {
      mode_ = PlatformMode::WEDGE;
    } else if (modelName.find("SCM-LC") == 0 || modelName.find("LC") == 0) {
      // TODO remove LC once fruid.json is fixed on Galaxy Linecards
      mode_ = PlatformMode::GALAXY_LC;
    } else if (
        modelName.find("SCM-FC") == 0 || modelName.find("SCM-FAB") == 0 ||
        modelName.find("FAB") == 0) {
      // TODO remove FAB once fruid.json is fixed on Galaxy fabric cards
      mode_ = PlatformMode::GALAXY_FC;
    } else if (
        modelName.find("MINIPACK") == 0 || modelName.find("MINIPHOTON") == 0) {
      mode_ = PlatformMode::MINIPACK;
    } else if (modelName.find("DCS-7368") == 0 || modelName.find("YAMP") == 0) {
      mode_ = PlatformMode::YAMP;
    } else if (
        modelName.find("DCS-7388") == 0 || modelName.find("ELBERT") == 0) {
      mode_ = PlatformMode::ELBERT;
    } else if (modelName.find("fake_wedge40") == 0) {
      mode_ = PlatformMode::FAKE_WEDGE40;
    } else if (modelName.find("fake_wedge") == 0) {
      mode_ = PlatformMode::FAKE_WEDGE;
    } else if (modelName.find("CLOUDRIPPER") == 0) {
      mode_ = PlatformMode::CLOUDRIPPER;
    } else if (modelName.find("Lassen") == 0 || modelName.find("LASSEN") == 0) {
      mode_ = PlatformMode::LASSEN;
    } else if (
        modelName.find("Sandia") == 0 || modelName.find("SANDIA") == 0 ||
        modelName.find("8508-F-SYS-HV") == 0) {
      mode_ = PlatformMode::SANDIA;
    } else if (
        modelName.find("Makalu") == 0 ||
        modelName.find("S9710-76D-BB12") == 0) {
      mode_ = PlatformMode::MAKALU;
    } else if (
        modelName.find("Kamet") == 0 || modelName.find("S9705-48D-4B4") == 0) {
      mode_ = PlatformMode::KAMET;
    } else if (
        modelName.find("Montblanc") == 0 || modelName.find("MONTBLANC") == 0) {
      mode_ = PlatformMode::MONTBLANC;
    } else {
      throw std::runtime_error("invalid model name " + modelName);
    }
  } else {
    if (FLAGS_mode == "wedge") {
      mode_ = PlatformMode::WEDGE;
    } else if (FLAGS_mode == "wedge100") {
      mode_ = PlatformMode::WEDGE100;
    } else if (FLAGS_mode == "galaxy_lc") {
      mode_ = PlatformMode::GALAXY_LC;
    } else if (FLAGS_mode == "galaxy_fc") {
      mode_ = PlatformMode::GALAXY_FC;
    } else if (FLAGS_mode == "minipack") {
      mode_ = PlatformMode::MINIPACK;
    } else if (FLAGS_mode == "yamp") {
      mode_ = PlatformMode::YAMP;
    } else if (FLAGS_mode == "fake_wedge40") {
      mode_ = PlatformMode::FAKE_WEDGE40;
    } else if (FLAGS_mode == "wedge400") {
      mode_ = PlatformMode::WEDGE400;
    } else if (FLAGS_mode == "fuji") {
      mode_ = PlatformMode::FUJI;
    } else if (FLAGS_mode == "elbert") {
      mode_ = PlatformMode::ELBERT;
    } else if (FLAGS_mode == "darwin") {
      mode_ = PlatformMode::DARWIN;
    } else if (FLAGS_mode == "lassen") {
      mode_ = PlatformMode::LASSEN;
    } else if (FLAGS_mode == "sandia") {
      mode_ = PlatformMode::SANDIA;
    } else if (FLAGS_mode == "makalu") {
      mode_ = PlatformMode::MAKALU;
    } else if (FLAGS_mode == "kamet") {
      mode_ = PlatformMode::KAMET;
    } else if (FLAGS_mode == "wedge400c") {
      mode_ = PlatformMode::WEDGE400C;
    } else if (FLAGS_mode == "wedge400c_voq") {
      mode_ = PlatformMode::WEDGE400C_VOQ;
    } else if (FLAGS_mode == "wedge400c_fabric") {
      mode_ = PlatformMode::WEDGE400C_FABRIC;
    } else if (FLAGS_mode == "cloudripper_voq") {
      mode_ = PlatformMode::CLOUDRIPPER_VOQ;
    } else if (FLAGS_mode == "cloudripper_fabric") {
      mode_ = PlatformMode::CLOUDRIPPER_FABRIC;
    } else if (FLAGS_mode == "montblanc") {
      mode_ = PlatformMode::MONTBLANC;
    } else {
      throw std::runtime_error("invalid mode " + FLAGS_mode);
    }
  }
}

void PlatformProductInfo::parse(std::string data) {
  dynamic info;
  try {
    info = parseJson(data).at(kInfo);
  } catch (const std::exception& err) {
    XLOG(ERR) << err.what();
    // Handle fruid data present outside of "Information" i.e.
    // {
    //   "Information" : fruid json
    // }
    // vs
    // {
    //  Fruid json
    // }
    info = parseJson(data);
  }
  productInfo_.oem() = folly::to<std::string>(info[kSysMfg].asString());
  productInfo_.product() = folly::to<std::string>(info[kProdName].asString());
  productInfo_.serial() = folly::to<std::string>(info[kSerialNum].asString());
  productInfo_.mfgDate() = folly::to<std::string>(info[kSysMfgDate].asString());
  productInfo_.systemPartNumber() =
      folly::to<std::string>(info[kSysAmbPartNum].asString());
  productInfo_.assembledAt() = folly::to<std::string>(info[kAmbAt].asString());
  productInfo_.pcbManufacturer() =
      folly::to<std::string>(info[kPcbMfg].asString());
  productInfo_.assetTag() =
      folly::to<std::string>(info[kProdAssetTag].asString());
  productInfo_.partNumber() =
      folly::to<std::string>(info[kProdPartNum].asString());
  productInfo_.odmPcbaPartNumber() =
      folly::to<std::string>(info[kOdmPcbaPartNum].asString());
  productInfo_.odmPcbaSerial() =
      folly::to<std::string>(info[kOdmPcbaSerialNum].asString());
  productInfo_.fbPcbaPartNumber() =
      folly::to<std::string>(info[kFbPcbaPartNum].asString());
  productInfo_.fbPcbPartNumber() =
      folly::to<std::string>(info[kFbPcbPartNum].asString());

  productInfo_.fabricLocation() =
      folly::to<std::string>(info[kFabricLocation].asString());
  // FB only - we apply custom logic to construct unique SN for
  // cases where we create multiple assets for a single physical
  // card in chassis.
  setFBSerial();
  productInfo_.version() = info[kVersion].asInt();
  productInfo_.subVersion() = info[kSubVersion].asInt();
  productInfo_.productionState() = info[kProductionState].asInt();
  productInfo_.productVersion() = info[kProdVersion].asInt();
  productInfo_.bmcMac() = folly::to<std::string>(info[kLocalMac].asString());
  productInfo_.mgmtMac() = folly::to<std::string>(info[kExtMacBase].asString());
  auto macBase = MacAddress(info[kExtMacBase].asString()).u64HBO() + 1;
  productInfo_.macRangeStart() = MacAddress::fromHBO(macBase).toString();
  productInfo_.macRangeSize() = info[kExtMacSize].asInt() - 1;
}

std::unique_ptr<PlatformProductInfo> fakeProductInfo() {
  // Dummy Fruid for fake platform
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

  folly::test::TemporaryDirectory tmpDir;
  auto fruidFilename = tmpDir.path().string() + "fruid.json";
  folly::writeFile(kFakeFruidJson, fruidFilename.c_str());
  auto productInfo =
      std::make_unique<facebook::fboss::PlatformProductInfo>(fruidFilename);
  productInfo->initialize();
  return productInfo;
}
} // namespace facebook::fboss
