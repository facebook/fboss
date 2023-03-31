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

struct CmdShowProductDetailsTraits : public BaseCommandTraits {
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
    model.product() = productInfo.get_product();
    model.oem() = productInfo.get_oem();
    model.serial() = productInfo.get_serial();
    model.mgmtMac() = productInfo.get_mgmtMac();
    model.bmcMac() = productInfo.get_bmcMac();
    model.macRangeStart() = productInfo.get_macRangeStart();
    model.macRangeSize() =
        folly::to<std::string>(productInfo.get_macRangeSize());
    model.assembledAt() = productInfo.get_assembledAt();
    model.assetTag() = productInfo.get_assetTag();
    model.partNumber() = productInfo.get_partNumber();
    model.productionState() =
        folly::to<std::string>(productInfo.get_productionState());
    model.subVersion() = folly::to<std::string>(productInfo.get_subVersion());
    model.productVersion() =
        folly::to<std::string>(productInfo.get_productVersion());
    model.systemPartNumber() = productInfo.get_systemPartNumber();
    model.mfgDate() = productInfo.get_mfgDate();
    model.pcbManufacturer() = productInfo.get_pcbManufacturer();
    model.fbPcbaPartNumber() = productInfo.get_fbPcbaPartNumber();
    model.fbPcbPartNumber() = productInfo.get_fbPcbPartNumber();
    model.odmPcbaPartNumber() = productInfo.get_odmPcbaPartNumber();
    model.odmPcbaSerial() = productInfo.get_odmPcbaSerial();
    model.version() = folly::to<std::string>(productInfo.get_version());
    return model;
  }

  void printOutput(const RetType& model, std::ostream& out = std::cout) {
    out << "Product: " << model.get_product() << std::endl;
    out << "OEM: " << model.get_oem() << std::endl;
    out << "Serial: " << model.get_serial() << std::endl;
    out << "Management MAC Address: " << model.get_mgmtMac() << std::endl;
    out << "BMC MAC Address: " << model.get_bmcMac() << std::endl;
    out << "Extended MAC Start: " << model.get_macRangeStart() << std::endl;
    out << "Extended MAC Size: " << model.get_macRangeSize() << std::endl;
    out << "Assembled At: " << model.get_assembledAt() << std::endl;
    out << "Product Asset Tag: " << model.get_assetTag() << std::endl;
    out << "Product Part Number: " << model.get_partNumber() << std::endl;
    out << "Product Production State: " << model.get_productionState()
        << std::endl;
    out << "Product Sub-Version: " << model.get_subVersion() << std::endl;
    out << "Product Version: " << model.get_productVersion() << std::endl;
    out << "System Assembly Part Number: " << model.get_systemPartNumber()
        << std::endl;
    out << "System Manufacturing Date: " << model.get_mfgDate() << std::endl;
    out << "PCB Manufacturer: " << model.get_pcbManufacturer() << std::endl;
    out << "Facebook PCBA Part Number: " << model.get_fbPcbaPartNumber()
        << std::endl;
    out << "Facebook PCB Part Number: " << model.get_fbPcbPartNumber()
        << std::endl;
    out << "ODM PCBA Part Number: " << model.get_odmPcbaPartNumber()
        << std::endl;
    out << "ODM PCBA Serial Number: " << model.get_odmPcbaSerial() << std::endl;
    out << "Version: " << model.get_version() << std::endl;
  }
};

} // namespace facebook::fboss
