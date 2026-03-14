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

  RetType queryClient(const HostInfo& hostInfo);

  RetType createModel(const ProductInfo& productInfo);

  void printOutput(const RetType& model, std::ostream& out = std::cout);
};

} // namespace facebook::fboss
