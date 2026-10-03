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

// Parses one or more directions of
//   delete interface <name> mirror <direction> [<direction>]
// where <direction> is ingress or egress.
class MirrorDeleteAttrArgs : public utils::BaseObjectArgType<std::string> {
 public:
  /* implicit */ MirrorDeleteAttrArgs( // NOLINT(google-explicit-constructor)
      std::vector<std::string> v);

  const std::vector<std::string>& getAttributes() const {
    return attributes_;
  }

 private:
  std::vector<std::string> attributes_;
};

struct CmdDeleteInterfaceMirrorTraits : public WriteCommandTraits {
  using ParentCmd = CmdDeleteInterface;
  static void addCliArg(CLI::App& cmd, std::vector<std::string>& args) {
    cmd.add_option(
        "mirror_attrs",
        args,
        "<direction> [<direction>] where <direction> is one of: ingress, "
        "egress");
  }
  using ObjectArgType = MirrorDeleteAttrArgs;
  using RetType = std::string;
};

class CmdDeleteInterfaceMirror : public CmdHandler<
                                     CmdDeleteInterfaceMirror,
                                     CmdDeleteInterfaceMirrorTraits> {
 public:
  using ObjectArgType = CmdDeleteInterfaceMirrorTraits::ObjectArgType;
  using RetType = CmdDeleteInterfaceMirrorTraits::RetType;

  RetType queryClient(
      const HostInfo& hostInfo,
      const utils::InterfaceList& interfaces,
      const ObjectArgType& mirrorAttrs);

  void printOutput(const RetType& logMsg);
};

} // namespace facebook::fboss
