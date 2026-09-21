/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/show/bgp/table/CmdShowBgpTableCommunity.h"

#include "fboss/cli/fboss2/commands/show/bgp/CanonicalRibResolver.h"

namespace facebook::fboss {

CmdShowBgpTableCommunity::RetType CmdShowBgpTableCommunity::queryClient(
    const HostInfo& hostInfo,
    const std::vector<std::string>& queriedCommunities) {
  TRibEntryWithHost result;
  auto client = utils::createClient<apache::thrift::Client<
      facebook::neteng::fboss::bgp::thrift::TBgpService>>(hostInfo);

  if (queriedCommunities.empty()) {
    std::cout
        << "No community entered. Usage: fboss2 show bgp table community <community>"
        << std::endl;
    return result;
  }

  std::vector<std::string> communityValues;
  try {
    communityValues = communityNameToValue(queriedCommunities[0], hostInfo);
  } catch (std::invalid_argument& e) {
    std::cout << e.what() << std::endl;
    return {};
  }

  auto entries = runMethodWithLegacyFallback(
      [&]() {
        std::vector<TRibEntry> canonicalEntries;
        for (const auto afi : {TBgpAfi::AFI_IPV4, TBgpAfi::AFI_IPV6}) {
          TCanonicalRibState canonical;
          client->sync_getRibEntriesForCommunityCanonical(
              canonical, afi, communityValues[0]);
          auto resolved = resolveCanonicalRibState(canonical);
          canonicalEntries.insert(
              canonicalEntries.end(), resolved.begin(), resolved.end());
        }
        return canonicalEntries;
      },
      [&]() {
        std::vector<TRibEntry> legacyEntries;
        client->sync_getRibEntriesForCommunity(
            legacyEntries, TBgpAfi::AFI_ALL, communityValues[0]);
        return legacyEntries;
      });

  auto data = (communityValues.size() > 1)
      ? filterEntriesByCommunities(entries, communityValues[1])
      : entries;

  result.tRibEntries() = std::move(data);
  result.host() = hostInfo.getName();
  result.oobName() = hostInfo.getOobName();
  result.ip() = hostInfo.getIpStr();
  return result;
}

} // namespace facebook::fboss
