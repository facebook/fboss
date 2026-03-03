/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/show/product/CmdShowProduct.h"

namespace facebook::fboss {

CmdShowProduct::RetType CmdShowProduct::queryClient(const HostInfo& hostInfo) {
  ProductInfo productInfo;
  auto client =
      utils::createClient<apache::thrift::Client<FbossCtrl>>(hostInfo);
  client->sync_getProductInfo(productInfo);
  return createModel(productInfo);
}

CmdShowProduct::RetType CmdShowProduct::createModel(
    const ProductInfo& productInfo) {
  RetType model;
  model.product() = productInfo.product().value();
  model.oem() = productInfo.oem().value();
  model.serial() = productInfo.serial().value();
  return model;
}

void CmdShowProduct::printOutput(const RetType& model, std::ostream& out) {
  out << "Product: " << model.product().value() << std::endl;
  out << "OEM: " << model.oem().value() << std::endl;
  out << "Serial: " << model.serial().value() << std::endl;
}

} // namespace facebook::fboss
