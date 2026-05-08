/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/show/bgp/shadowrib/CmdShowBgpShadowRib.h"

#include "fboss/cli/fboss2/commands/show/bgp/CmdShowUtils.h"
#include "fboss/cli/fboss2/utils/CmdClientUtilsCommon.h"
#include "neteng/fboss/bgp/if/gen-cpp2/TBgpService.h"

namespace facebook::fboss {

CmdShowBgpShadowRib::RetType CmdShowBgpShadowRib::queryClient(
    const HostInfo& hostInfo) {
  std::vector<TRibEntry> entriesIPv4;
  std::vector<TRibEntry> entriesIPv6;
  auto client = utils::createClient<apache::thrift::Client<
      facebook::neteng::fboss::bgp::thrift::TBgpService>>(hostInfo);

  client->sync_getShadowRibEntries(entriesIPv4, TBgpAfi::AFI_IPV4);
  client->sync_getShadowRibEntries(entriesIPv6, TBgpAfi::AFI_IPV6);

  // Append both entries into a single vector
  entriesIPv4.insert(entriesIPv4.end(), entriesIPv6.begin(), entriesIPv6.end());
  TRibEntryWithHost result;
  result.tRibEntries() = std::move(entriesIPv4);
  result.host() = hostInfo.getName();
  result.oobName() = hostInfo.getOobName();
  result.ip() = hostInfo.getIpStr();
  return result;
}

void CmdShowBgpShadowRib::printOutput(RetType& entries, std::ostream& out) {
  printRIBEntries(out, entries);
}

} // namespace facebook::fboss
