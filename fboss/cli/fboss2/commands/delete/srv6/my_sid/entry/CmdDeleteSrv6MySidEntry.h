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
#include "fboss/cli/fboss2/commands/config/srv6/utils/Srv6MySidCliUtils.h"
#include "fboss/cli/fboss2/commands/delete/srv6/my_sid/CmdDeleteSrv6MySid.h"

namespace facebook::fboss {

// CLI: `delete srv6 my-sid <prefix> entry <fn>`
// Removes one MySID function entry from the session config.
struct CmdDeleteSrv6MySidEntryTraits : public WriteCommandTraits {
  using ParentCmd = CmdDeleteSrv6MySid;
  static void addCliArg(CLI::App& cmd, std::vector<std::string>& args) {
    cmd.add_option("function", args, "MySID function ID")
        ->required()
        ->expected(1);
  }
  using ObjectArgType = MySidDeleteEntryArg;
  using RetType = std::string;
};

class CmdDeleteSrv6MySidEntry : public CmdHandler<
                                    CmdDeleteSrv6MySidEntry,
                                    CmdDeleteSrv6MySidEntryTraits> {
 public:
  using ObjectArgType = CmdDeleteSrv6MySidEntryTraits::ObjectArgType;
  using RetType = CmdDeleteSrv6MySidEntryTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const LocatorPrefixArg& prefix,
      const ObjectArgType& deleteArg);

  void printOutput(const RetType& output);
};

} // namespace facebook::fboss
