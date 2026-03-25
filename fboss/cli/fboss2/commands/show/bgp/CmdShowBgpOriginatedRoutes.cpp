// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/cli/fboss2/commands/show/bgp/CmdShowBgpOriginatedRoutes.h"

#include <folly/IPAddress.h>

#include "fboss/bgp/if/gen-cpp2/TBgpService.h"
#include "fboss/cli/fboss2/commands/show/bgp/CmdShowUtils.h"
#include "fboss/cli/fboss2/utils/CmdClientUtilsCommon.h"
#include "fboss/cli/fboss2/utils/Table.h"

namespace facebook::fboss {
using facebook::fboss::utils::Table;
using facebook::neteng::fboss::bgp::thrift::TOriginatedRouteWithHost;

CmdShowBgpOriginatedRoutes::RetType CmdShowBgpOriginatedRoutes::queryClient(
    const HostInfo& hostInfo) {
  std::vector<TOriginatedRoute> routes;
  auto client = utils::createClient<apache::thrift::Client<
      facebook::neteng::fboss::bgp::thrift::TBgpService>>(hostInfo);

  client->sync_getOriginatedRoutes(routes);
  TOriginatedRouteWithHost result;
  result.tOriginatedRoutes() = std::move(routes);
  result.host() = hostInfo.getName();
  result.oobName() = hostInfo.getOobName();
  result.ip() = hostInfo.getIpStr();
  return result;
}

void CmdShowBgpOriginatedRoutes::printOutput(
    const RetType& originatedRouteWithHost,
    std::ostream& out) {
  // Table provides a cleaner display and reduces the spacing needed
  // for the output to the minimum.
  Table table;
  table.setHeader(
      {"Prefix",
       "Communities",
       "Supporting Route Cnt",
       "Minimum supporting route",
       "Require Nexthop Resolution"});

  for (const auto& route : *originatedRouteWithHost.tOriginatedRoutes()) {
    std::string ip_version;
    const auto& route_prefix = *route.prefix();
    const auto ip_address = folly::IPAddress::fromBinary(
        folly::ByteRange(folly::StringPiece(*route_prefix.prefix_bin())));

    const auto prefix =
        fmt::format("{}/{}", ip_address.str(), *route_prefix.num_bits());
    const HostInfo hostInfo(
        *originatedRouteWithHost.host(),
        *originatedRouteWithHost.oobName(),
        folly::IPAddress(*originatedRouteWithHost.ip()));
    const auto communities = printCommunities(
        apache::thrift::can_throw(*route.communities()),
        hostInfo,
        true /* print community with multi-lines */);

    table.addRow(
        {prefix,
         communities,
         folly::to<std::string>(
             folly::copy(route.supporting_route_count().value())),
         folly::to<std::string>(
             folly::copy(route.minimum_supporting_routes().value())),
         route.require_nexthop_resolution()
             ? folly::to<std::string>(*route.require_nexthop_resolution())
             : "N/A"});
  }
  out << table << std::endl;
}

} // namespace facebook::fboss
