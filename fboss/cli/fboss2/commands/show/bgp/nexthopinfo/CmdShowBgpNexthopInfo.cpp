/*
 *  Copyright (c) 2004-present, Meta Platforms, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/show/bgp/nexthopinfo/CmdShowBgpNexthopInfo.h"

#include "fboss/cli/fboss2/CmdHandler.cpp"

#include "fboss/cli/fboss2/utils/CmdClientUtilsCommon.h"

namespace facebook::fboss {

CmdShowBgpNexthopInfo::RetType CmdShowBgpNexthopInfo::queryClient(
    const HostInfo& hostInfo,
    const ObjectArgType& queriedIps) {
  RetType nexthopInfo;

  if (queriedIps.empty()) {
    std::cerr
        << "No IP entered. Usage: fboss2 show bgp nexthopinfo <nexthop IP address>"
        << std::endl;
    return nexthopInfo;
  }

  auto client = utils::createClient<apache::thrift::Client<
      facebook::neteng::fboss::bgp::thrift::TBgpService>>(hostInfo);

  client->sync_getNexthopInfoForNexthop(nexthopInfo, queriedIps[0]);
  return nexthopInfo;
}

void CmdShowBgpNexthopInfo::printOutput(
    const RetType& data,
    std::ostream& out) {
  const auto& prefixBin = *data.next_hop()->prefix_bin();
  if (prefixBin.empty()) {
    out << "IP not found in the nexthopCache" << std::endl;
    return;
  }

  auto nexthopIp =
      folly::IPAddress::fromBinary(
          folly::ByteRange(
              reinterpret_cast<const unsigned char*>(prefixBin.data()),
              prefixBin.size()))
          .str();

  utils::Table table;
  table.setHeader({"Nexthop", "Reachable", "IGP Cost"});

  const auto reachable = *data.is_reachable() ? "Yes" : "No";

  const auto igpCost = data.igp_cost().has_value()
      ? folly::to<std::string>(*data.igp_cost())
      : "N/A";

  table.addRow({nexthopIp, reachable, igpCost});
  out << table << std::endl;
}

template void
CmdHandler<CmdShowBgpNexthopInfo, CmdShowBgpNexthopInfoTraits>::run();

} // namespace facebook::fboss
