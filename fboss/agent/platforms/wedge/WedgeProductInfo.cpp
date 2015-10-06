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
#include <folly/FileUtil.h>
#include <folly/json.h>
#include <folly/dynamic.h>
#include <folly/MacAddress.h>

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
constexpr auto kOdmPcbPartNum = "ODM PCB Part Number";
constexpr auto kOdmPcbSerialNum = "ODM PCB Serial Number";
constexpr auto kFbPcbPartNum = "Facebook PCB Part Number";
constexpr auto kExtMacSize = "Extended MAC Address Size";
constexpr auto kExtMacBase = "Extended MAC Base";
constexpr auto kLocalMac = "Local MAC";
constexpr auto kVersion = "Version";
constexpr auto kFabricLocation = "Location on Fabric";
constexpr auto kFabricLocationLeft = "LEFT";
constexpr auto kFabricLocationRight = "RIGHT";
}

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
    LOG(ERROR) << err.what();
    // if fruid info fails fall back to hostname
    initFromHostname();
  }
}

void WedgeProductInfo::getInfo(ProductInfo& info) {
  info = productInfo_;
}

std::string WedgeProductInfo::getFabricLocation() {
  return productInfo_.fabricLocation;
}

std::string WedgeProductInfo::getProductName() {
  return productInfo_.product;
}

void WedgeProductInfo::parse(std::string data) {
  dynamic info = parseJson(data)[kInfo];
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
  productInfo_.odmPcbPartNumber = folly::to<std::string>
                                    (info[kOdmPcbPartNum].asString());
  productInfo_.odmPcbSerial = folly::to<std::string>
                                  (info[kOdmPcbSerialNum].asString());
  productInfo_.fbPcbPartNumber = folly::to<std::string>
                                    (info[kFbPcbPartNum].asString());
  productInfo_.fabricLocation = folly::to<std::string>
                                    (info[kFabricLocation].asString());
  // Append L/R to the serial number based on the fabricLocation
  if (productInfo_.fabricLocation == kFabricLocationLeft) {
    productInfo_.serial = productInfo_.serial + "L";
  } else if (productInfo_.fabricLocation == kFabricLocationRight) {
    productInfo_.serial = productInfo_.serial + "R";
  }
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
