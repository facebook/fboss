/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/show/route/CmdShowRouteStaticMpls.h"
#include "fboss/cli/fboss2/CmdHandler.cpp"
#include "fboss/cli/fboss2/commands/show/route/utils.h"

namespace facebook::fboss {

CmdShowRouteStaticMpls::RetType CmdShowRouteStaticMpls::queryClient(
    const HostInfo& hostInfo) {
  std::vector<MplsRoute> entries;
  auto client =
      utils::createClient<apache::thrift::Client<FbossCtrl>>(hostInfo);
  client->sync_getMplsRouteTableByClient(
      entries, static_cast<int16_t>(ClientID::STATIC_ROUTE));
  return createModel(entries);
}

void CmdShowRouteStaticMpls::printOutput(
    const RetType& model,
    std::ostream& out) {
  for (const auto& entry : model.mplsEntries().value()) {
    out << fmt::format(
        "MPLS Label: {}\n", folly::copy(entry.topLabel().value()));
    auto& nextHops = entry.nextHops().value();
    if (!nextHops.empty()) {
      for (const auto& nextHop : nextHops) {
        out << fmt::format(
            "\tvia {}\n", show::route::utils::getNextHopInfoStr(nextHop));
      }
    } else {
      out << "\tNo Forwarding Info\n";
    }
  }
}

CmdShowRouteStaticMpls::RetType CmdShowRouteStaticMpls::createModel(
    const std::vector<facebook::fboss::MplsRoute>& mplsRoutes) {
  RetType model;

  for (const auto& entry : mplsRoutes) {
    cli::MplsEntry mplsDetails;
    mplsDetails.topLabel() = folly::copy(entry.topLabel().value());
    if (entry.adminDistance().has_value()) {
      mplsDetails.adminDistance() = static_cast<int>(*entry.adminDistance());
    }
    for (const auto& nextHop : entry.nextHops().value()) {
      cli::NextHopInfo nextHopInfo;
      show::route::utils::getNextHopInfoThrift(nextHop, nextHopInfo);
      mplsDetails.nextHops()->emplace_back(nextHopInfo);
    }
    model.mplsEntries()->emplace_back(mplsDetails);
  }
  return model;
}

// Explicit template instantiation
template void
CmdHandler<CmdShowRouteStaticMpls, CmdShowRouteStaticMplsTraits>::run();
template const ValidFilterMapType CmdHandler<
    CmdShowRouteStaticMpls,
    CmdShowRouteStaticMplsTraits>::getValidFilters();

} // namespace facebook::fboss
