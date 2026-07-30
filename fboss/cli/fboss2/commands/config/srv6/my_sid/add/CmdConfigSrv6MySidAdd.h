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
#include "fboss/cli/fboss2/commands/config/srv6/my_sid/CmdConfigSrv6MySid.h"
#include "fboss/cli/fboss2/commands/config/srv6/utils/Srv6MySidCliUtils.h"

namespace facebook::fboss {

// CLI: `config srv6 my-sid <prefix> add entry <fn> type adjacency|node|decap
// ...` Upserts one MySID function entry under the configured locator prefix.
struct CmdConfigSrv6MySidAddTraits : public WriteCommandTraits {
  using ParentCmd = CmdConfigSrv6MySid;
  static void addCliArg(CLI::App& cmd, std::vector<std::string>& args) {
    cmd.add_option(
        "entry_args",
        args,
        "entry <function> type <adjacency|node|decap> [type-specific args]");
  }
  using ObjectArgType = MySidAddArg;
  using RetType = std::string;
};

class CmdConfigSrv6MySidAdd
    : public CmdHandler<CmdConfigSrv6MySidAdd, CmdConfigSrv6MySidAddTraits> {
 public:
  using ObjectArgType = CmdConfigSrv6MySidAddTraits::ObjectArgType;
  using RetType = CmdConfigSrv6MySidAddTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const LocatorPrefixArg& prefix,
      const ObjectArgType& addArg);

  void printOutput(const RetType& output);
};

} // namespace facebook::fboss
