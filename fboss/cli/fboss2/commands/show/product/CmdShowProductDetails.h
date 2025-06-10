/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/product/CmdShowProduct.h"

namespace facebook::fboss {

struct CmdShowProductDetailsTraits : public ReadCommandTraits {
  using ParentCmd = CmdShowProduct;
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = cli::ShowProductDetailsModel;
};

class CmdShowProductDetails
    : public CmdHandler<CmdShowProductDetails, CmdShowProductDetailsTraits> {
 public:
  using ObjectArgType = CmdShowProductDetailsTraits::ObjectArgType;
  using RetType = CmdShowProductDetailsTraits::RetType;

  RetType queryClient(const HostInfo& hostInfo) {
    ProductInfo productInfo;
    auto client =
        utils::createClient<apache::thrift::Client<FbossCtrl>>(hostInfo);
    client->sync_getProductInfo(productInfo);
    return createModel(productInfo);
  }

  RetType createModel(const ProductInfo& productInfo) {
    RetType model;
    model.product() = productInfo.product().value();
    model.oem() = productInfo.oem().value();
    model.serial() = productInfo.serial().value();
    model.mgmtMac() = productInfo.mgmtMac().value();
    model.bmcMac() = productInfo.bmcMac().value();
    model.macRangeStart() = productInfo.macRangeStart().value();
    model.macRangeSize() =
        folly::to<std::string>(folly::copy(productInfo.macRangeSize().value()));
    model.assembledAt() = productInfo.assembledAt().value();
    model.assetTag() = productInfo.assetTag().value();
    model.partNumber() = productInfo.partNumber().value();
    model.productionState() = folly::to<std::string>(
        folly::copy(productInfo.productionState().value()));
    model.subVersion() =
        folly::to<std::string>(folly::copy(productInfo.subVersion().value()));
    model.productVersion() = folly::to<std::string>(
        folly::copy(productInfo.productVersion().value()));
    model.systemPartNumber() = productInfo.systemPartNumber().value();
    model.mfgDate() = productInfo.mfgDate().value();
    model.pcbManufacturer() = productInfo.pcbManufacturer().value();
    model.fbPcbaPartNumber() = productInfo.fbPcbaPartNumber().value();
    model.fbPcbPartNumber() = productInfo.fbPcbPartNumber().value();
    model.odmPcbaPartNumber() = productInfo.odmPcbaPartNumber().value();
    model.odmPcbaSerial() = productInfo.odmPcbaSerial().value();
    model.version() =
        folly::to<std::string>(folly::copy(productInfo.version().value()));
    return model;
  }

  void printOutput(const RetType& model, std::ostream& out = std::cout) {
    out << "Product: " << model.product().value() << std::endl;
    out << "OEM: " << model.oem().value() << std::endl;
    out << "Serial: " << model.serial().value() << std::endl;
    out << "Management MAC Address: " << model.mgmtMac().value() << std::endl;
    out << "BMC MAC Address: " << model.bmcMac().value() << std::endl;
    out << "Extended MAC Start: " << model.macRangeStart().value() << std::endl;
    out << "Extended MAC Size: " << model.macRangeSize().value() << std::endl;
    out << "Assembled At: " << model.assembledAt().value() << std::endl;
    out << "Product Asset Tag: " << model.assetTag().value() << std::endl;
    out << "Product Part Number: " << model.partNumber().value() << std::endl;
    out << "Product Production State: " << model.productionState().value()
        << std::endl;
    out << "Product Sub-Version: " << model.subVersion().value() << std::endl;
    out << "Product Version: " << model.productVersion().value() << std::endl;
    out << "System Assembly Part Number: " << model.systemPartNumber().value()
        << std::endl;
    out << "System Manufacturing Date: " << model.mfgDate().value()
        << std::endl;
    out << "PCB Manufacturer: " << model.pcbManufacturer().value() << std::endl;
    out << "Facebook PCBA Part Number: " << model.fbPcbaPartNumber().value()
        << std::endl;
    out << "Facebook PCB Part Number: " << model.fbPcbPartNumber().value()
        << std::endl;
    out << "ODM PCBA Part Number: " << model.odmPcbaPartNumber().value()
        << std::endl;
    out << "ODM PCBA Serial Number: " << model.odmPcbaSerial().value()
        << std::endl;
    out << "Version: " << model.version().value() << std::endl;
  }
};

} // namespace facebook::fboss
