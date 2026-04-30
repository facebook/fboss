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
#include "fboss/cli/fboss2/commands/show/bgp/config/CmdShowConfigTraits.h"

namespace facebook::fboss {

class CmdShowConfigRunningBgp
    : public CmdHandler<CmdShowConfigRunningBgp, CmdShowConfigDynamicTraits> {
 public:
  using RetType = CmdShowConfigDynamicTraits::RetType;

  RetType queryClient(const HostInfo& hostInfo);
  void printOutput(RetType& bgpConfig, std::ostream& out = std::cout);
};

} // namespace facebook::fboss
