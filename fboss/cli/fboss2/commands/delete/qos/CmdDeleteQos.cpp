/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/delete/qos/CmdDeleteQos.h"

#include "fboss/cli/fboss2/CmdHandler.cpp"

#include <iostream>

namespace facebook::fboss {

CmdDeleteQosTraits::RetType CmdDeleteQos::queryClient(
    const HostInfo& /* hostInfo */) {
  return "Delete QoS configuration commands. Use subcommands: default-policy";
}

void CmdDeleteQos::printOutput(const RetType& output) {
  std::cout << output << std::endl;
}

template void CmdHandler<CmdDeleteQos, CmdDeleteQosTraits>::run();

} // namespace facebook::fboss
