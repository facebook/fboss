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

namespace facebook::fboss {

// Parent stub for SRv6 delete commands (`delete srv6 ...`).
struct CmdDeleteSrv6Traits : public WriteCommandTraits {
  using ObjectArgType = utils::NoneArgType;
  using RetType = std::string;
};

class CmdDeleteSrv6 : public CmdHandler<CmdDeleteSrv6, CmdDeleteSrv6Traits> {
 public:
  using RetType = CmdDeleteSrv6Traits::RetType;

  RetType queryClient(const HostInfo& /* hostInfo */) {
    throw std::runtime_error(
        "Incomplete command, please use one of the subcommands");
  }

  void printOutput(const RetType& /* model */) {}
};

} // namespace facebook::fboss
