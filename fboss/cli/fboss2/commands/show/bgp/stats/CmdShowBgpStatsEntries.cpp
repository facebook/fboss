// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/cli/fboss2/commands/show/bgp/stats/CmdShowBgpStatsEntries.h"

#include <iostream>

#include "fboss/bgp/if/gen-cpp2/TBgpService.h"
#include "fboss/cli/fboss2/utils/CmdClientUtilsCommon.h"

namespace facebook::fboss {

CmdShowBgpStatsEntries::RetType CmdShowBgpStatsEntries::queryClient(
    const HostInfo& hostInfo) {
  TEntryStats stats;
  auto client = utils::createClient<apache::thrift::Client<
      facebook::neteng::fboss::bgp::thrift::TBgpService>>(hostInfo);
  client->sync_getEntryStats(stats);
  return stats;
}

void CmdShowBgpStatsEntries::printOutput(
    const RetType& stats,
    std::ostream& out) {
  out << "BGP entry statistics:" << std::endl;
  out << " Total number of unicast routes: "
      << folly::copy(stats.total_ucast_routes().value()) << std::endl;
  out << " Total number of rib paths: "
      << folly::copy(stats.total_rib_paths().value()) << std::endl;
  out << " Total number of adjribs: "
      << folly::copy(stats.total_adj_ribs().value()) << std::endl;
  out << " Total number of originated routes: "
      << folly::copy(stats.total_originated_routes().value()) << std::endl;
  out << " Total number of shadow rib entries: "
      << folly::copy(stats.total_shadow_rib_entries().value()) << std::endl;
  out << " Total number of tracked netlink wrapper interfaces: "
      << folly::copy(stats.total_netlink_wrapper_interfaces().value())
      << std::endl;
}

} // namespace facebook::fboss
