/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/show/bgp/table/CmdShowBgpTable.h"

#include "fboss/cli/fboss2/commands/show/bgp/CanonicalRibResolver.h"
#include "fboss/cli/fboss2/commands/show/bgp/CmdShowUtils.h"
#include "fboss/cli/fboss2/utils/CmdClientUtilsCommon.h"
#include "neteng/fboss/bgp/if/gen-cpp2/TBgpService.h"

namespace facebook::fboss {

CmdShowBgpTable::RetType CmdShowBgpTable::queryClient(
    const HostInfo& hostInfo) {
  auto client = utils::createClient<apache::thrift::Client<
      facebook::neteng::fboss::bgp::thrift::TBgpService>>(hostInfo);

  auto entries = queryCanonicalRibWithFallback(
      *client,
      [](auto& c, TCanonicalRibState& state, TBgpAfi afi) {
        c.sync_getRibEntriesCanonical(state, afi);
      },
      [](auto& c, std::vector<TRibEntry>& out, TBgpAfi afi) {
        c.sync_getRibEntries(out, afi);
      });

  TRibEntryWithHost result;
  result.tRibEntries() = std::move(entries);
  result.host() = hostInfo.getName();
  result.oobName() = hostInfo.getOobName();
  result.ip() = hostInfo.getIpStr();
  return result;
}

void CmdShowBgpTable::printOutput(RetType& entries, std::ostream& out) {
  printRIBEntries(out, entries);
}

} // namespace facebook::fboss
