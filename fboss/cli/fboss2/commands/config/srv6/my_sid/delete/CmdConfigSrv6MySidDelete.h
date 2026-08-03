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

// CLI: `config srv6 my-sid <prefix> delete entry <fn>`
// Removes one MySID function entry from the session config.
struct CmdConfigSrv6MySidDeleteTraits : public WriteCommandTraits {
  using ParentCmd = CmdConfigSrv6MySid;
  static void addCliArg(CLI::App& cmd, std::vector<std::string>& args) {
    cmd.add_option("entry_args", args, "entry <function>");
  }
  using ObjectArgType = MySidDeleteEntryArg;
  using RetType = std::string;
};

class CmdConfigSrv6MySidDelete : public CmdHandler<
                                     CmdConfigSrv6MySidDelete,
                                     CmdConfigSrv6MySidDeleteTraits> {
 public:
  using ObjectArgType = CmdConfigSrv6MySidDeleteTraits::ObjectArgType;
  using RetType = CmdConfigSrv6MySidDeleteTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const LocatorPrefixArg& prefix,
      const ObjectArgType& deleteArg);

  void printOutput(const RetType& output);
};

} // namespace facebook::fboss
