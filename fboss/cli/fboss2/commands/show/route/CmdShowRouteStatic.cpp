/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/commands/show/route/CmdShowRouteStatic.h"
#include <fboss/cli/fboss2/utils/CmdUtils.h>
#include "fboss/agent/AddressUtil.h"
#include "fboss/cli/fboss2/CmdHandler.cpp"
#include "fboss/cli/fboss2/commands/show/route/utils.h"

namespace facebook::fboss {

namespace {
// Forwarding action strings as produced by the agent's forwardActionStr()
// (fboss/agent/state/RouteTypes.cpp).
constexpr auto kToCpu = "ToCPU";

bool isV4(const facebook::fboss::RouteDetails& route) {
  return facebook::network::toIPAddress(*route.dest()->ip()).isV4();
}

// Returns the STATIC_ROUTE client's contribution to this route, or nullptr if
// the route has no static entry.
const facebook::fboss::ClientAndNextHops* staticClientEntry(
    const facebook::fboss::RouteDetails& route) {
  for (const auto& clAndNh : route.nextHopMulti().value()) {
    if (folly::copy(clAndNh.clientId().value()) ==
        static_cast<int32_t>(ClientID::STATIC_ROUTE)) {
      return &clAndNh;
    }
  }
  return nullptr;
}
} // namespace

std::vector<facebook::fboss::RouteDetails>
CmdShowRouteStatic::queryStaticRoutes(const HostInfo& hostInfo) {
  std::vector<facebook::fboss::RouteDetails> allEntries;
  auto client =
      utils::createClient<apache::thrift::Client<FbossCtrl>>(hostInfo);
  client->sync_getRouteTableDetails(allEntries);

  std::vector<facebook::fboss::RouteDetails> staticEntries;
  for (auto& entry : allEntries) {
    if (staticClientEntry(entry) != nullptr) {
      staticEntries.emplace_back(std::move(entry));
    }
  }
  return staticEntries;
}

CmdShowRouteStatic::RetType CmdShowRouteStatic::queryClient(
    const HostInfo& hostInfo) {
  auto entries = queryStaticRoutes(hostInfo);
  return createModel(entries, AddressFamily::ALL);
}

void CmdShowRouteStatic::printOutput(const RetType& model, std::ostream& out) {
  printRouteModel(model, out);
}

void CmdShowRouteStatic::printRouteModel(
    const RetType& model,
    std::ostream& out) {
  for (const auto& entry : model.routeEntries().value()) {
    out << fmt::format("Network Address: {}\n", entry.networkAddress().value());
    for (const auto& nextHop : entry.nextHops().value()) {
      out << fmt::format(
          "\tvia {}\n", show::route::utils::getNextHopInfoStr(nextHop));
    }
  }
}

CmdShowRouteStatic::RetType CmdShowRouteStatic::createModel(
    const std::vector<facebook::fboss::RouteDetails>& routeEntries,
    AddressFamily family) {
  RetType model;

  for (const auto& entry : routeEntries) {
    if (family == AddressFamily::V4 && !isV4(entry)) {
      continue;
    }
    if (family == AddressFamily::V6 && isV4(entry)) {
      continue;
    }

    const auto* clAndNh = staticClientEntry(entry);
    if (clAndNh == nullptr) {
      // Not a static route (only reachable if an unfiltered list is passed in).
      continue;
    }

    auto ipStr = utils::getAddrStr(*entry.dest()->ip());
    auto ipPrefix = ipStr + "/" + std::to_string(*entry.dest()->prefixLength());

    cli::RouteEntry routeEntry;
    routeEntry.networkAddress() = ipPrefix;

    const auto& action = entry.action().value();
    const auto& nextHops = clAndNh->nextHops().value();
    const auto& nextHopAddrs = clAndNh->nextHopAddrs().value();

    if (!nextHops.empty()) {
      for (const auto& nh : nextHops) {
        cli::NextHopInfo nextHopInfo;
        show::route::utils::getNextHopInfoThrift(nh, nextHopInfo);
        routeEntry.nextHops()->emplace_back(nextHopInfo);
      }
    } else if (!nextHopAddrs.empty()) {
      // Fall back to the deprecated nextHopAddrs field.
      for (const auto& addr : nextHopAddrs) {
        cli::NextHopInfo nextHopInfo;
        show::route::utils::getNextHopInfoAddr(addr, nextHopInfo);
        routeEntry.nextHops()->emplace_back(nextHopInfo);
      }
    } else {
      // No nexthops -> a cpu (ToCPU) or null0 (Drop) route, distinguished by
      // the route's forwarding action.
      cli::NextHopInfo nextHopInfo;
      nextHopInfo.addr() = (action == kToCpu) ? "cpu" : "null0";
      routeEntry.nextHops()->emplace_back(nextHopInfo);
    }
    model.routeEntries()->emplace_back(routeEntry);
  }
  return model;
}

// Explicit template instantiation
template void CmdHandler<CmdShowRouteStatic, CmdShowRouteStaticTraits>::run();
template const ValidFilterMapType
CmdHandler<CmdShowRouteStatic, CmdShowRouteStaticTraits>::getValidFilters();

} // namespace facebook::fboss
