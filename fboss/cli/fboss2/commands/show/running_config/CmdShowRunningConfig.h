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

#include <iostream>
#include <ostream>
#include <string>
#include <variant>
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

namespace facebook::fboss {

struct CmdShowRunningConfigTraits : public ReadCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = std::string;
};

class CmdShowRunningConfig
    : public CmdHandler<CmdShowRunningConfig, CmdShowRunningConfigTraits> {
 public:
  using ObjectArgType = CmdShowRunningConfigTraits::ObjectArgType;
  using RetType = CmdShowRunningConfigTraits::RetType;

  RetType queryClient(const HostInfo& hostInfo);

  void printOutput(const RetType& config, std::ostream& out = std::cout);
};

} // namespace facebook::fboss
