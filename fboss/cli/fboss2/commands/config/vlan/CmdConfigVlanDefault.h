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
#include "fboss/cli/fboss2/utils/CmdUtils.h"

namespace facebook::fboss {

struct CmdConfigVlanDefaultTraits : public WriteCommandTraits {
  static void addCliArg(CLI::App& cmd, std::vector<std::string>& args) {
    cmd.add_option("vlan_id", args, "VLAN ID (1-4094)");
  }
  using ObjectArgType = utils::VlanIdValue;
  using RetType = std::string;
};

class CmdConfigVlanDefault
    : public CmdHandler<CmdConfigVlanDefault, CmdConfigVlanDefaultTraits> {
 public:
  using ObjectArgType = CmdConfigVlanDefaultTraits::ObjectArgType;
  using RetType = CmdConfigVlanDefaultTraits::RetType;

  RetType queryClient(const HostInfo& hostInfo, const ObjectArgType& vlanId);

  void printOutput(const RetType& logMsg);
};

} // namespace facebook::fboss
