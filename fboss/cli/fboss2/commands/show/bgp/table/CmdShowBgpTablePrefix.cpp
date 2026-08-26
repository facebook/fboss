/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/show/bgp/table/CmdShowBgpTablePrefix.h"

#include "fboss/cli/fboss2/commands/show/bgp/CanonicalRibResolver.h"

namespace facebook::fboss {

CmdShowBgpTablePrefix::RetType CmdShowBgpTablePrefix::queryClient(
    const HostInfo& hostInfo,
    const std::vector<std::string>& prefixes) {
  auto client = utils::createClient<apache::thrift::Client<
      facebook::neteng::fboss::bgp::thrift::TBgpService>>(hostInfo);
  TRibEntryWithHost result;

  if (prefixes.empty()) {
    std::cout
        << "No prefixes entered. Usage: fboss2 show bgp table prefix <prefix>"
        << std::endl;
    return result;
  }

  auto allEntries = runMethodWithLegacyFallback(
      [&]() {
        std::vector<TRibEntry> entries;
        for (const auto& prefix : prefixes) {
          TCanonicalRibState canonical;
          client->sync_getRibPrefixCanonical(canonical, prefix);
          auto resolved = resolveCanonicalRibState(canonical);
          entries.insert(entries.end(), resolved.begin(), resolved.end());
        }
        return entries;
      },
      [&]() {
        std::vector<TRibEntry> entries;
        for (const auto& prefix : prefixes) {
          std::vector<TRibEntry> newEntry;
          client->sync_getRibPrefix(newEntry, prefix);
          entries.insert(entries.end(), newEntry.begin(), newEntry.end());
        }
        return entries;
      });

  result.tRibEntries() = std::move(allEntries);
  result.host() = hostInfo.getName();
  result.oobName() = hostInfo.getOobName();
  result.ip() = hostInfo.getIpStr();
  return result;
}

} // namespace facebook::fboss
