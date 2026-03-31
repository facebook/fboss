/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/clear/CmdClearNdp.h"
#include "fboss/cli/fboss2/CmdHandler.cpp"

#include "fboss/cli/fboss2/commands/clear/CmdClearUtils.h"
#include "fboss/cli/fboss2/utils/CmdClientUtils.h"

namespace facebook::fboss {

CmdClearNdp::RetType CmdClearNdp::queryClient(
    const HostInfo& hostInfo,
    const ObjectArgType& queriedNetworks) {
  auto client =
      utils::createClient<facebook::fboss::FbossCtrlAsyncClient>(hostInfo);
  std::vector<facebook::fboss::NdpEntryThrift> ndpEntries;
  client->sync_getNdpTable(ndpEntries);
  return utils::flushNeighborEntries(client, ndpEntries, queriedNetworks);
}

void CmdClearNdp::printOutput(const RetType& logMsg) {
  std::cout << logMsg << std::endl;
}

// Explicit template instantiation
template void CmdHandler<CmdClearNdp, CmdClearNdpTraits>::run();
template const ValidFilterMapType
CmdHandler<CmdClearNdp, CmdClearNdpTraits>::getValidFilters();

} // namespace facebook::fboss
