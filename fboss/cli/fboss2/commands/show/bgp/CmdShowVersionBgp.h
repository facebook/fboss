/*
 *  Copyright (c) 2004-present, Meta Platforms, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/bgp/CmdShowUtils.h"

namespace facebook::fboss {

class CmdShowVersionBgp
    : public CmdHandler<CmdShowVersionBgp, CmdShowVersionTraits> {
 public:
  using RetType = CmdShowVersionTraits::RetType;

  RetType queryClient(const HostInfo& hostInfo);

  void printOutput(RetType& bgpVer);
};

} // namespace facebook::fboss
