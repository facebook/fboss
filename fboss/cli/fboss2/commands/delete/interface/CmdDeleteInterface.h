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

namespace facebook::fboss {

struct CmdDeleteInterfaceTraits : public WriteCommandTraits {
  static void addCliArg(CLI::App& cmd, std::vector<std::string>& args) {
    cmd.add_option("interface_config", args, "<port-list>");
  }
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
        "Incomplete command, please use one of the subcommands (e.g. ipv6 ndp)");
  }

  void printOutput(const RetType& /* model */) {}
};

} // namespace facebook::fboss
