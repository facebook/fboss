/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/protocol/bgp/global/CmdConfigProtocolBgpGlobal.h"

namespace facebook::fboss {

CmdConfigProtocolBgpGlobalTraits::RetType
CmdConfigProtocolBgpGlobal::queryClient(const HostInfo& /* hostInfo */) {
  return "BGP global configuration. Use subcommands: router-id, local-asn, hold-time, graceful-restart, best-path, link-bw, nh-tracking, network-originate";
}

void CmdConfigProtocolBgpGlobal::printOutput(const RetType& output) {
  std::cout << output << std::endl;
}

} // namespace facebook::fboss
