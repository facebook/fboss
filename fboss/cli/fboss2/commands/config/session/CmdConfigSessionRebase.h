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

#include <string>
#include <variant>
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

namespace facebook::fboss {

struct CmdConfigSessionRebaseTraits : public WriteCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate; // no arg
  using RetType = std::string;
};

class CmdConfigSessionRebase
    : public CmdHandler<CmdConfigSessionRebase, CmdConfigSessionRebaseTraits> {
 public:
  using ObjectArgType = CmdConfigSessionRebaseTraits::ObjectArgType;
  using RetType = CmdConfigSessionRebaseTraits::RetType;

  RetType queryClient(const HostInfo& hostInfo);

  void printOutput(const RetType& logMsg);
};

} // namespace facebook::fboss
