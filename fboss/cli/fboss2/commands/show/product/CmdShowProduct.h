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
#include "fboss/cli/fboss2/commands/show/product/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/utils/CmdClientUtils.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"

namespace facebook::fboss {

struct CmdShowProductTraits : public BaseCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = cli::ShowProductModel;
};

class CmdShowProduct : public CmdHandler<CmdShowProduct, CmdShowProductTraits> {
 public:
  using ObjectArgType = CmdShowProductTraits::ObjectArgType;
  using RetType = CmdShowProductTraits::RetType;

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
    return model;
  }

  void printOutput(const RetType& model, std::ostream& out = std::cout) {
    out << "Product: " << model.get_product() << std::endl;
    out << "OEM: " << model.get_oem() << std::endl;
    out << "Serial: " << model.get_serial() << std::endl;
  }
};

} // namespace facebook::fboss
