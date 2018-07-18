/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/platforms/wedge/WedgeProductInfo.h"
#include "fboss/agent/FbossError.h"

#include <boost/algorithm/string.hpp>
#include <folly/FileUtil.h>
#include <folly/MacAddress.h>
#include <folly/dynamic.h>
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
constexpr auto kFabricLocationLeft = "LEFT";
constexpr auto kFabricLocationRight = "RIGHT";
}

DEFINE_string(mode, "",
              "The mode the FBOSS controller is running as, wedge, lc, or fc");

namespace facebook { namespace fboss {

using folly::MacAddress;
using folly::StringPiece;
using folly::dynamic;
using folly::parseJson;

WedgeProductInfo::WedgeProductInfo(StringPiece path)
  : path_(path) {
}

void WedgeProductInfo::initialize() {
  try {
    std::string data;
    folly::readFile(path_.str().c_str(), data);
    parse(data);
  } catch (const std::exception& err) {
    XLOG(ERR) << err.what();
    // if fruid info fails fall back to fbwhoami
    initFromFbWhoAmI();
  }
  initMode();
}

void WedgeProductInfo::getInfo(ProductInfo& info) {
  info = productInfo_;
}

WedgePlatformMode WedgeProductInfo::getMode() const {
  return mode_;
}

std::string WedgeProductInfo::getFabricLocation() {
  return productInfo_.fabricLocation;
}

std::string WedgeProductInfo::getProductName() {
  return productInfo_.product;
}

void WedgeProductInfo::initMode() {
  if (FLAGS_mode.empty()) {
    auto modelName = getProductName();
    if (modelName.find("Wedge100") == 0 ||
               modelName.find("WEDGE100") == 0) {
      // Wedge100 comes from fruid.json, WEDGE100 comes from fbwhoami
      mode_ = WedgePlatformMode::WEDGE100;
    } else if (modelName.find("Wedge") == 0 ||
               modelName.find("WEDGE") == 0) {
      // Wedge100 comes from fruid.json, WEDGE comes from fbwhoami
      mode_ = WedgePlatformMode::WEDGE;
    } else if (modelName.find("SCM-LC") == 0 || modelName.find("LC") == 0) {
       // TODO remove LC once fruid.json is fixed on Galaxy Linecards
       mode_ = WedgePlatformMode::GALAXY_LC;
    } else if (
        modelName.find("SCM-FC") == 0 || modelName.find("SCM-FAB") == 0 ||
        modelName.find("FAB") == 0) {
      // TODO remove FAB once fruid.json is fixed on Galaxy fabric cards
      mode_ = WedgePlatformMode::GALAXY_FC;
    } else if (modelName.find("MINIPACK") == 0) {
      mode_ = WedgePlatformMode::MINIPACK;
    } else if (modelName.find("DCS-7368") == 0 || modelName.find("YAMP") == 0) {
      mode_ = WedgePlatformMode::YAMP;
    } else {
      throw std::runtime_error("invalid model name " + modelName);
    }
  } else {
    if (FLAGS_mode == "wedge") {
      mode_ = WedgePlatformMode::WEDGE;
    } else if (FLAGS_mode == "wedge100") {
      mode_ = WedgePlatformMode::WEDGE100;
    } else if (FLAGS_mode == "galaxy_lc") {
      mode_ = WedgePlatformMode::GALAXY_LC;
    } else if (FLAGS_mode == "galaxy_fc") {
      mode_ = WedgePlatformMode::GALAXY_FC;
    } else if (FLAGS_mode == "minipack") {
      mode_ = WedgePlatformMode::MINIPACK;
    } else if (FLAGS_mode == "yamp") {
      mode_ = WedgePlatformMode::YAMP;
    } else {
      throw std::runtime_error("invalid mode " + FLAGS_mode);
    }
  }
}

void WedgeProductInfo::parse(std::string data) {
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
  productInfo_.oem = folly::to<std::string>(info[kSysMfg].asString());
  productInfo_.product = folly::to<std::string>(info[kProdName].asString());
  productInfo_.serial = folly::to<std::string>(info[kSerialNum].asString());
  productInfo_.mfgDate = folly::to<std::string>(info[kSysMfgDate].asString());
  productInfo_.systemPartNumber =
                      folly::to<std::string>(info[kSysAmbPartNum].asString());
  productInfo_.assembledAt = folly::to<std::string>(info[kAmbAt].asString());
  productInfo_.pcbManufacturer =
                              folly::to<std::string>(info[kPcbMfg].asString());
  productInfo_.assetTag =
                        folly::to<std::string>(info[kProdAssetTag].asString());
  productInfo_.partNumber =
                          folly::to<std::string>(info[kProdPartNum].asString());
  productInfo_.odmPcbaPartNumber = folly::to<std::string>
                                    (info[kOdmPcbaPartNum].asString());
  productInfo_.odmPcbaSerial = folly::to<std::string>
                                  (info[kOdmPcbaSerialNum].asString());
  productInfo_.fbPcbaPartNumber = folly::to<std::string>
                                    (info[kFbPcbaPartNum].asString());
  productInfo_.fbPcbPartNumber = folly::to<std::string>
                                    (info[kFbPcbPartNum].asString());

  productInfo_.fabricLocation = folly::to<std::string>
                                    (info[kFabricLocation].asString());
  // FB only - we apply custom logic to construct unique SN for
  // cases where we create multiple assets for a single physical
  // card in chassis.
  setFBSerial();
  productInfo_.version = info[kVersion].asInt();
  productInfo_.subVersion = info[kSubVersion].asInt();
  productInfo_.productionState = info[kProductionState].asInt();
  productInfo_.productVersion = info[kProdVersion].asInt();
  productInfo_.bmcMac = folly::to<std::string>(info[kLocalMac].asString());
  productInfo_.mgmtMac = folly::to<std::string>(info[kExtMacBase].asString());
  auto macBase = MacAddress(info[kExtMacBase].asString()).u64HBO() + 1;
  productInfo_.macRangeStart = MacAddress::fromHBO(macBase).toString();
  productInfo_.macRangeSize = info[kExtMacSize].asInt() - 1;
}

}} // facebook::fboss
