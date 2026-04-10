/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/clear/CmdClearArp.h"
#include "fboss/cli/fboss2/CmdHandler.cpp"

#include "fboss/cli/fboss2/commands/clear/CmdClearUtils.h"
#include "fboss/cli/fboss2/utils/CmdClientUtils.h"

namespace facebook::fboss {

CmdClearArp::RetType CmdClearArp::queryClient(
    const HostInfo& hostInfo,
    const ObjectArgType& queriedNetworks) {
  auto client =
      utils::createClient<facebook::fboss::FbossCtrlAsyncClient>(hostInfo);
  std::vector<facebook::fboss::ArpEntryThrift> arpEntries;
  client->sync_getArpTable(arpEntries);
  return utils::flushNeighborEntries(client, arpEntries, queriedNetworks);
}

void CmdClearArp::printOutput(const RetType& logMsg) {
  std::cout << logMsg << std::endl;
}

// Explicit template instantiation
template void CmdHandler<CmdClearArp, CmdClearArpTraits>::run();
template const ValidFilterMapType
CmdHandler<CmdClearArp, CmdClearArpTraits>::getValidFilters();

} // namespace facebook::fboss
