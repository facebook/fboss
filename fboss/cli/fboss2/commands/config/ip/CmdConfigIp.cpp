/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/config/ip/CmdConfigIp.h"

#include "fboss/cli/fboss2/CmdHandler.cpp"

namespace facebook::fboss {

CmdConfigIpTraits::RetType CmdConfigIp::queryClient(
    const HostInfo& /* hostInfo */) {
  return "IPv4 configuration commands. Use subcommands: route";
}

void CmdConfigIp::printOutput(const RetType& output) {
  std::cout << output << std::endl;
}

CmdConfigIpv6Traits::RetType CmdConfigIpv6::queryClient(
    const HostInfo& /* hostInfo */) {
  return "IPv6 configuration commands. Use subcommands: route";
}

void CmdConfigIpv6::printOutput(const RetType& output) {
  std::cout << output << std::endl;
}

// Explicit template instantiation
template void CmdHandler<CmdConfigIp, CmdConfigIpTraits>::run();
template void CmdHandler<CmdConfigIpv6, CmdConfigIpv6Traits>::run();

} // namespace facebook::fboss
