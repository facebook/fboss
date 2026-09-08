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

#include <string_view>
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"

namespace facebook::fboss {

struct CmdShowL2Traits : public ReadCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = std::string;

  static std::string_view description();
};

class CmdShowL2 : public CmdHandler<CmdShowL2, CmdShowL2Traits> {
 public:
  using RetType = CmdShowL2Traits::RetType;

  RetType queryClient(const HostInfo& hostInfo);

  void printOutput(const RetType& message, std::ostream& out = std::cout);

  static RetType sampleModel();
};

} // namespace facebook::fboss
