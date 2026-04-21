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
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/config/interface/CmdConfigInterface.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"

namespace facebook::fboss {

struct CmdDeleteInterfaceTraits : public WriteCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_INTERFACES_CONFIG;
  using ObjectArgType = InterfacesConfig;
  using RetType = std::string;
};

class CmdDeleteInterface
    : public CmdHandler<CmdDeleteInterface, CmdDeleteInterfaceTraits> {
 public:
  using ObjectArgType = CmdDeleteInterfaceTraits::ObjectArgType;
  using RetType = CmdDeleteInterfaceTraits::RetType;

  RetType queryClient(
      const HostInfo& /* hostInfo */,
      const ObjectArgType& /* interfaceConfig */) {
    throw std::runtime_error(
        "Incomplete command, please use one of the subcommands (e.g. ipv6 nd)");
  }

  void printOutput(const RetType& /* model */) {}
};

} // namespace facebook::fboss
