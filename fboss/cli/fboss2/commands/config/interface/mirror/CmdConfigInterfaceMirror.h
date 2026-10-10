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
#include <utility>
#include <vector>
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/config/interface/CmdConfigInterface.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"
#include "fboss/cli/fboss2/utils/InterfaceList.h"

namespace facebook::fboss {

// Parses one or more <direction> <mirror-name> pairs of
//   config interface <name> mirror <direction> <mirror-name> [...]
// where <direction> is ingress or egress. Both directions can be bound in
// one call (e.g. "ingress span0 egress span1").
class MirrorAttrArgs : public utils::BaseObjectArgType<std::string> {
 public:
  /* implicit */ MirrorAttrArgs( // NOLINT(google-explicit-constructor)
      std::vector<std::string> v);

  const std::vector<std::pair<std::string, std::string>>& getAttributes()
      const {
    return attributes_;
  }

 private:
  std::vector<std::pair<std::string, std::string>> attributes_;
};

struct CmdConfigInterfaceMirrorTraits : public WriteCommandTraits {
  using ParentCmd = CmdConfigInterface;
  static void addCliArg(CLI::App& cmd, std::vector<std::string>& args) {
    cmd.add_option(
        "mirror_attrs",
        args,
        "<direction> <mirror-name> [<direction> <mirror-name>] where "
        "<direction> is one of: ingress, egress");
  }
  using ObjectArgType = MirrorAttrArgs;
  using RetType = std::string;
};

class CmdConfigInterfaceMirror : public CmdHandler<
                                     CmdConfigInterfaceMirror,
                                     CmdConfigInterfaceMirrorTraits> {
 public:
  using ObjectArgType = CmdConfigInterfaceMirrorTraits::ObjectArgType;
  using RetType = CmdConfigInterfaceMirrorTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const utils::InterfaceList& interfaces,
      const ObjectArgType& mirrorAttrs);

  void printOutput(const RetType& logMsg);
};

} // namespace facebook::fboss
