// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/cli/fboss2/commands/show/bgp/changelist/CmdShowBgpChangelist.h"

#include "fboss/bgp/if/gen-cpp2/TBgpService.h"
#include "fboss/cli/fboss2/commands/show/bgp/CmdShowUtils.h"
#include "fboss/cli/fboss2/utils/CmdClientUtilsCommon.h"

namespace facebook::fboss {

CmdShowBgpChangelist::RetType CmdShowBgpChangelist::queryClient(
    const HostInfo& hostInfo) {
  std::vector<TRibEntry> entriesIPv4;
  std::vector<TRibEntry> entriesIPv6;
  auto client = utils::createClient<apache::thrift::Client<
      facebook::neteng::fboss::bgp::thrift::TBgpService>>(hostInfo);

  client->sync_getChangeListEntries(entriesIPv4, TBgpAfi::AFI_IPV4);
  client->sync_getChangeListEntries(entriesIPv6, TBgpAfi::AFI_IPV6);

  // Append both entries into a single vector
  entriesIPv4.insert(entriesIPv4.end(), entriesIPv6.begin(), entriesIPv6.end());
  TRibEntryWithHost result;
  result.tRibEntries() = std::move(entriesIPv4);
  result.host() = hostInfo.getName();
  result.oobName() = hostInfo.getOobName();
  result.ip() = hostInfo.getIpStr();
  return result;
}

void CmdShowBgpChangelist::printOutput(RetType& entries, std::ostream& out) {
  printRIBEntries(out, entries);
}

} // namespace facebook::fboss
