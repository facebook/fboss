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
#include "fboss/cli/fboss2/CmdHandler.cpp"

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

std::string_view CmdShowRouteTraits::description() {
  return "Displays the switch's routing table: each destination prefix and its ECMP nexthops (via address, egress interface, and weight). Use it to inspect installed routes and their paths.";
}

CmdShowRoute::RetType CmdShowRoute::sampleModel() {
  RetType model;

  // First route: 100::/64 with no nexthops
  cli::RouteEntry route1;
  route1.networkAddress() = "100::/64";
  route1.nextHops() = std::vector<cli::NextHopInfo>();
  route1.overridenEcmpMode() = "";
  model.routeEntries()->emplace_back(route1);

  // Second route: 2001:db8:2215:f000::/52 with two nexthops
  cli::RouteEntry route2;
  route2.networkAddress() = "2001:db8:2215:f000::/52";
  route2.overridenEcmpMode() = "";

  cli::NextHopInfo nh1;
  nh1.addr() = "2001:db8:e22f:219::2a";
  nh1.ifName() = "fboss2006";
  nh1.weight() = 1;
  route2.nextHops()->emplace_back(nh1);

  cli::NextHopInfo nh2;
  nh2.addr() = "2001:db8:e22f:119::3a";
  nh2.ifName() = "fboss2008";
  nh2.weight() = 1;
  route2.nextHops()->emplace_back(nh2);

  model.routeEntries()->emplace_back(route2);

  return model;
}

// Explicit template instantiation
template void CmdHandler<CmdShowRoute, CmdShowRouteTraits>::run();
template const ValidFilterMapType
CmdHandler<CmdShowRoute, CmdShowRouteTraits>::getValidFilters();

} // namespace facebook::fboss
