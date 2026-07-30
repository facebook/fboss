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
#include "fboss/cli/fboss2/commands/delete/srv6/CmdDeleteSrv6.h"

namespace facebook::fboss {

// CLI: `delete srv6 my-sid <prefix>`
// Removes the entire mySidConfig block (locator and all entries).
struct CmdDeleteSrv6MySidTraits : public WriteCommandTraits {
  using ParentCmd = CmdDeleteSrv6;
  static void addCliArg(CLI::App& cmd, std::vector<std::string>& args) {
    cmd.add_option(
        "prefix", args, "SRv6 /32 locator prefix (e.g. fdad:ffff::/32)");
  }
  using ObjectArgType = LocatorPrefixArg;
  using RetType = std::string;
};

class CmdDeleteSrv6MySid
    : public CmdHandler<CmdDeleteSrv6MySid, CmdDeleteSrv6MySidTraits> {
 public:
  using ObjectArgType = CmdDeleteSrv6MySidTraits::ObjectArgType;
  using RetType = CmdDeleteSrv6MySidTraits::RetType;

  RetType queryClient(const HostInfo& hostInfo, const ObjectArgType& prefix);

  void printOutput(const RetType& output);
};

} // namespace facebook::fboss
