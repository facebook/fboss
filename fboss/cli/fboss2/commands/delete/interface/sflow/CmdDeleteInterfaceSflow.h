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
#include <vector>
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/delete/interface/CmdDeleteInterface.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"
#include "fboss/cli/fboss2/utils/InterfaceList.h"

namespace facebook::fboss {

// Parses the single positional token of
//   delete interface <name> sflow <attr>
// where <attr> is currently one of: sample-dest.
class SflowDeleteAttrArg : public utils::BaseObjectArgType<std::string> {
 public:
  /* implicit */ SflowDeleteAttrArg( // NOLINT(google-explicit-constructor)
      std::vector<std::string> v);

  const std::string& attr() const {
    return attr_;
  }

 private:
  std::string attr_;
};

struct CmdDeleteInterfaceSflowTraits : public WriteCommandTraits {
  using ParentCmd = CmdDeleteInterface;
  static void addCliArg(CLI::App& cmd, std::vector<std::string>& args) {
    cmd.add_option(
        "sflow_attr",
        args,
        "<attr> - which sflow attribute to reset: sample-dest");
  }
  using ObjectArgType = SflowDeleteAttrArg;
  using RetType = std::string;
};

class CmdDeleteInterfaceSflow : public CmdHandler<
                                    CmdDeleteInterfaceSflow,
                                    CmdDeleteInterfaceSflowTraits> {
 public:
  using ObjectArgType = CmdDeleteInterfaceSflowTraits::ObjectArgType;
  using RetType = CmdDeleteInterfaceSflowTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const utils::InterfaceList& interfaces,
      const ObjectArgType& sflowAttr);

  void printOutput(const RetType& logMsg);
};

} // namespace facebook::fboss
