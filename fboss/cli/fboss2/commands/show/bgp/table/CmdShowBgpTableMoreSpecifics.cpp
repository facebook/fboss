/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/show/bgp/table/CmdShowBgpTableMoreSpecifics.h"

#include "fboss/cli/fboss2/commands/show/bgp/CanonicalRibResolver.h"

namespace facebook::fboss {

CmdShowBgpTableMoreSpecifics::RetType CmdShowBgpTableMoreSpecifics::queryClient(
    const HostInfo& hostInfo,
    const std::vector<std::string>& prefixes) {
  auto client = utils::createClient<apache::thrift::Client<
      facebook::neteng::fboss::bgp::thrift::TBgpService>>(hostInfo);
  TRibEntryWithHost result;

  if (prefixes.empty()) {
    std::cout
        << "No prefixes entered. Usage: fboss2 show bgp table more-specifics <prefix>"
        << std::endl;
    return result;
  }

  auto entries = runMethodWithLegacyFallback(
      [&]() {
        std::vector<TRibEntry> allEntries;
        for (const auto& prefix : prefixes) {
          TCanonicalRibState canonical;
          client->sync_getRibSubprefixesCanonical(canonical, prefix);
          auto resolved = resolveCanonicalRibState(canonical);
          allEntries.insert(allEntries.end(), resolved.begin(), resolved.end());
        }
        return allEntries;
      },
      [&]() {
        std::vector<TRibEntry> allEntries;
        for (const auto& prefix : prefixes) {
          std::vector<TRibEntry> newEntry;
          client->sync_getRibSubprefixes(newEntry, prefix);
          allEntries.insert(allEntries.end(), newEntry.begin(), newEntry.end());
        }
        return allEntries;
      });

  result.tRibEntries() = std::move(entries);
  result.host() = hostInfo.getName();
  result.oobName() = hostInfo.getOobName();
  result.ip() = hostInfo.getIpStr();
  return result;
}

} // namespace facebook::fboss
