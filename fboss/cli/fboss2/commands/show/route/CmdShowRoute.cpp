/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/show/route/CmdShowRoute.h"

namespace facebook::fboss {

CmdShowRoute::RetType CmdShowRoute::queryClient(const HostInfo& hostInfo) {
  std::vector<UnicastRoute> entries;
  auto client =
      utils::createClient<apache::thrift::Client<FbossCtrl>>(hostInfo);

  client->sync_getRouteTable(entries);
  return createModel(entries);
}

void CmdShowRoute::printOutput(const RetType& model, std::ostream& out) {
  for (const auto& entry : model.routeEntries().value()) {
    out << fmt::format("Network Address: {}\n", entry.networkAddress().value());

    if (!entry.overridenEcmpMode()->empty()) {
      out << fmt::format(
          "\tOverride ECMP mode: {}\n", entry.overridenEcmpMode().value());
    }
    if (entry.overridenNextHops()) {
      for (const auto& nextHop : entry.overridenNextHops().value()) {
        out << fmt::format(
            "\tvia (override) {}\n",
            show::route::utils::getNextHopInfoStr(nextHop));
      }
      out << fmt::format(
          "\t # next hops lost: {}\n",
          entry.nextHops()->size() - entry.overridenNextHops()->size());
    } else {
      for (const auto& nextHop : entry.nextHops().value()) {
        out << fmt::format(
            "\tvia {}\n", show::route::utils::getNextHopInfoStr(nextHop));
      }
    }
  }
}

bool CmdShowRoute::isUcmpActive(const std::vector<NextHopThrift>& nextHops) {
  // Let's avoid crashing the CLI when next_hops is blank ;)
  if (nextHops.empty()) {
    return false;
  }
  for (const auto& nh : nextHops) {
    if (folly::copy(nextHops[0].weight().value()) !=
        folly::copy(nh.weight().value())) {
      return true;
    }
  }
  return false;
}

CmdShowRoute::RetType CmdShowRoute::createModel(
    std::vector<facebook::fboss::UnicastRoute>& routeEntries) {
  RetType model;

  for (const auto& entry : routeEntries) {
    auto& nextHops = entry.nextHops().value();

    auto ipStr = utils::getAddrStr(*entry.dest()->ip());
    auto ipPrefix = ipStr + "/" + std::to_string(*entry.dest()->prefixLength());

    std::string ucmpActive;
    if (isUcmpActive(nextHops)) {
      ucmpActive = " (UCMP Active)";
    }

    cli::RouteEntry routeEntry;
    routeEntry.networkAddress() = fmt::format("{}{}", ipPrefix, ucmpActive);

    if (!nextHops.empty()) {
      for (const auto& nh : nextHops) {
        cli::NextHopInfo nextHopInfo;
        show::route::utils::getNextHopInfoThrift(nh, nextHopInfo);
        routeEntry.nextHops()->emplace_back(nextHopInfo);
      }
    } else {
      for (const auto& address : entry.nextHopAddrs().value()) {
        cli::NextHopInfo nextHopInfo;
        show::route::utils::getNextHopInfoAddr(address, nextHopInfo);
        routeEntry.nextHops()->emplace_back(nextHopInfo);
      }
    }
    if (entry.overrideEcmpSwitchingMode()) {
      routeEntry.overridenEcmpMode() = apache::thrift::util::enumNameSafe(
          *entry.overrideEcmpSwitchingMode());
    }
    if (entry.overrideNextHops()) {
      routeEntry.overridenNextHops() = std::vector<cli::NextHopInfo>();
      for (const auto& nh : *entry.overrideNextHops()) {
        cli::NextHopInfo nextHopInfo;
        show::route::utils::getNextHopInfoThrift(nh, nextHopInfo);
        routeEntry.overridenNextHops()->emplace_back(nextHopInfo);
      }
    }
    model.routeEntries()->emplace_back(routeEntry);
  }
  return model;
}

} // namespace facebook::fboss
