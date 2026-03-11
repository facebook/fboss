/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/show/mpls/CmdShowMplsRoute.h"

namespace facebook::fboss {

CmdShowMplsRoute::RetType CmdShowMplsRoute::queryClient(
    const HostInfo& hostInfo) {
  std::vector<MplsRoute> entries;
  auto client =
      utils::createClient<apache::thrift::Client<FbossCtrl>>(hostInfo);

  client->sync_getAllMplsRouteDetails(entries);
  return createModel(entries);
}

void CmdShowMplsRoute::printOutput(const RetType& model, std::ostream& out) {
  for (const auto& entry : model.mplsEntries().value()) {
    out << fmt::format(
        "MPLS Label: {}\n", folly::copy(entry.topLabel().value()));

    for (const auto& clAndNxthops : entry.nextHopMulti().value()) {
      out << fmt::format(
          "  Nexthops from client {}\n",
          folly::copy(clAndNxthops.clientId().value()));
      for (const auto& nextHop : clAndNxthops.nextHops().value()) {
        out << fmt::format(
            "    {}\n", show::route::utils::getNextHopInfoStr(nextHop));
      }
    }

    out << fmt::format("  Action: {}\n", entry.action().value());

    auto& nextHops = entry.nextHops().value();
    if (nextHops.size() > 0) {
      out << fmt::format("  Forwarding via:\n");
      for (const auto& nextHop : nextHops) {
        out << fmt::format(
            "    {}\n", show::route::utils::getNextHopInfoStr(nextHop));
      }
    } else {
      out << "  No Forwarding Info\n";
    }

    out << fmt::format(
        "  Admin Distance: {}\n", folly::copy(entry.adminDistance().value()));
  }
}

CmdShowMplsRoute::RetType CmdShowMplsRoute::createModel(
    std::vector<MplsRoute>& mplsEntries) {
  RetType model;

  for (const auto& entry : mplsEntries) {
    cli::MplsEntry mplsDetails;

    mplsDetails.topLabel() = folly::copy(entry.topLabel().value());
    mplsDetails.action() = entry.action().value();
    const auto adminDistance = folly::copy(entry.adminDistance().value());
    mplsDetails.adminDistance() = static_cast<int>(adminDistance);

    auto& nextHopMulti = entry.nextHopMulti().value();
    for (const auto& clAndNxthops : nextHopMulti) {
      cli::ClientAndNextHops clAndNxthopsCli;
      clAndNxthopsCli.clientId() = folly::copy(clAndNxthops.clientId().value());
      auto& nextHopAddrs = clAndNxthops.nextHopAddrs().value();
      auto& nextHops = clAndNxthops.nextHops().value();
      if (nextHopAddrs.size() > 0) {
        for (const auto& address : nextHopAddrs) {
          cli::NextHopInfo nextHopInfo;
          show::route::utils::getNextHopInfoAddr(address, nextHopInfo);
          clAndNxthopsCli.nextHops()->emplace_back(nextHopInfo);
        }
      } else if (nextHops.size() > 0) {
        for (const auto& nextHop : nextHops) {
          cli::NextHopInfo nextHopInfo;
          show::route::utils::getNextHopInfoThrift(nextHop, nextHopInfo);
          clAndNxthopsCli.nextHops()->emplace_back(nextHopInfo);
        }
      }
      mplsDetails.nextHopMulti()->emplace_back(clAndNxthopsCli);
    }

    auto& nextHops = entry.nextHops().value();
    if (nextHops.size() > 0) {
      for (const auto& nextHop : nextHops) {
        cli::NextHopInfo nextHopInfo;
        show::route::utils::getNextHopInfoThrift(nextHop, nextHopInfo);
        mplsDetails.nextHops()->emplace_back(nextHopInfo);
      }
    }

    model.mplsEntries()->push_back(mplsDetails);
  }

  return model;
}

} // namespace facebook::fboss
