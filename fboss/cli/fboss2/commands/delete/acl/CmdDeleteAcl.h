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

#include <stdexcept>
#include <string>
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

namespace facebook::fboss {

// Parent node for `delete acl <subcmd>`. Mirrors CmdConfigAcl's role for
// the `config acl ...` tree: a real handler so addCommandBranch()
// increments depth before descending into the leaves.
struct CmdDeleteAclTraits : public WriteCommandTraits {
  using ObjectArgType = utils::NoneArgType;
  using RetType = std::string;
};

class CmdDeleteAcl : public CmdHandler<CmdDeleteAcl, CmdDeleteAclTraits> {
 public:
  using ObjectArgType = CmdDeleteAclTraits::ObjectArgType;
  using RetType = CmdDeleteAclTraits::RetType;

  RetType queryClient(const HostInfo& /* hostInfo */) {
    throw std::runtime_error(
        "Incomplete command, please use a subcommand (e.g. 'rule')");
  }

  void printOutput(const RetType& /* model */) {}
};

} // namespace facebook::fboss
