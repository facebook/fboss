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
#include "fboss/cli/fboss2/utils/CmdClientUtils.h"
#include "fboss/cli/fboss2/utils/CmdUtils.h"
#include "fboss/cli/fboss2/utils/Table.h"

namespace facebook::fboss {

using utils::Table;

struct CmdShowL2Traits : public BaseCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = std::string;
};

class CmdShowL2 : public CmdHandler<CmdShowL2, CmdShowL2Traits> {
 public:
  using RetType = CmdShowL2Traits::RetType;

  RetType queryClient(const HostInfo& hostInfo) {
    return "Please run \"show mac details\" for L2 entries.";
  }

  void printOutput(const RetType& message, std::ostream& out = std::cout) {
    out << message << std::endl;
  }
};

} // namespace facebook::fboss
