/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/show/route/CmdShowRouteStaticIpv6.h"
#include "fboss/cli/fboss2/CmdHandler.cpp"

namespace facebook::fboss {

CmdShowRouteStaticIpv6::RetType CmdShowRouteStaticIpv6::queryClient(
    const HostInfo& hostInfo) {
  auto entries = CmdShowRouteStatic::queryStaticRoutes(hostInfo);
  return CmdShowRouteStatic::createModel(
      entries, CmdShowRouteStatic::AddressFamily::V6);
}

void CmdShowRouteStaticIpv6::printOutput(
    const RetType& model,
    std::ostream& out) {
  CmdShowRouteStatic::printRouteModel(model, out);
}

// Explicit template instantiation
template void
CmdHandler<CmdShowRouteStaticIpv6, CmdShowRouteStaticIpv6Traits>::run();
template const ValidFilterMapType CmdHandler<
    CmdShowRouteStaticIpv6,
    CmdShowRouteStaticIpv6Traits>::getValidFilters();

} // namespace facebook::fboss
