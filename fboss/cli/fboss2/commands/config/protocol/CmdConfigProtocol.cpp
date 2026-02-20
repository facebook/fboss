/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/protocol/CmdConfigProtocol.h"

namespace facebook::fboss {

CmdConfigProtocolTraits::RetType CmdConfigProtocol::queryClient(
    const HostInfo& /* hostInfo */) {
  return "Protocol configuration commands. Use subcommands: bgp";
}

void CmdConfigProtocol::printOutput(const RetType& output) {
  std::cout << output << std::endl;
}

} // namespace facebook::fboss
