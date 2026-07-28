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
#include "fboss/cli/fboss2/commands/config/srv6/CmdConfigSrv6.h"
#include "fboss/cli/fboss2/commands/config/srv6/utils/Srv6MySidCliUtils.h"

namespace facebook::fboss {

// CLI: `config srv6 my-sid <prefix>`
// Creates the mySidConfig block (locator + empty entries map) when none exists.
// Also acts as the parent context for add/delete entry subcommands.
struct CmdConfigSrv6MySidTraits : public WriteCommandTraits {
  using ParentCmd = CmdConfigSrv6;
  static void addCliArg(CLI::App& cmd, std::vector<std::string>& args) {
    cmd.add_option(
        "prefix", args, "SRv6 /32 locator prefix (e.g. fdad:ffff::/32)");
  }
  using ObjectArgType = LocatorPrefixArg;
  using RetType = std::string;
};

class CmdConfigSrv6MySid
    : public CmdHandler<CmdConfigSrv6MySid, CmdConfigSrv6MySidTraits> {
 public:
  using ObjectArgType = CmdConfigSrv6MySidTraits::ObjectArgType;
  using RetType = CmdConfigSrv6MySidTraits::RetType;

  RetType queryClient(const HostInfo& hostInfo, const ObjectArgType& prefix);

  void printOutput(const RetType& output);
};

} // namespace facebook::fboss
