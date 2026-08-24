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
#include "fboss/cli/fboss2/commands/config/interface/CmdConfigInterface.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"
#include "fboss/cli/fboss2/utils/InterfaceList.h"

namespace facebook::fboss {

// Parses the two positional tokens of
//   config interface <name> sflow <attr> <value>
// where <attr> is one of: sample-dest, ingress-rate, egress-rate.
class SflowAttrArgs : public utils::BaseObjectArgType<std::string> {
 public:
  /* implicit */ SflowAttrArgs( // NOLINT(google-explicit-constructor)
      std::vector<std::string> v);

  const std::string& attr() const {
    return attr_;
  }
  const std::string& value() const {
    return value_;
  }

 private:
  std::string attr_;
  std::string value_;
};

struct CmdConfigInterfaceSflowTraits : public WriteCommandTraits {
  using ParentCmd = CmdConfigInterface;
  static void addCliArg(CLI::App& cmd, std::vector<std::string>& args) {
    cmd.add_option(
        "sflow_attr",
        args,
        "<attr> <value> where <attr> is one of: sample-dest, ingress-rate, "
        "egress-rate");
  }
  using ObjectArgType = SflowAttrArgs;
  using RetType = std::string;
};

class CmdConfigInterfaceSflow : public CmdHandler<
                                    CmdConfigInterfaceSflow,
                                    CmdConfigInterfaceSflowTraits> {
 public:
  using ObjectArgType = CmdConfigInterfaceSflowTraits::ObjectArgType;
  using RetType = CmdConfigInterfaceSflowTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const utils::InterfaceList& interfaces,
      const ObjectArgType& sflowAttr);

  void printOutput(const RetType& logMsg);
};

} // namespace facebook::fboss
