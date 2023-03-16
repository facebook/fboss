/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include <fboss/agent/if/gen-cpp2/ctrl_types.h>
#include <fboss/cli/fboss2/utils/CmdUtils.h>
#include <folly/String.h>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include "fboss/agent/if/gen-cpp2/common_types.h"
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/route/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/commands/show/route/utils.h"

namespace facebook::fboss {

struct CmdShowRouteTraits : public BaseCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = utils::NoneArgType;
  using RetType = cli::ShowRouteModel;
};

class CmdShowRoute : public CmdHandler<CmdShowRoute, CmdShowRouteTraits> {
 public:
  using NextHopThrift = facebook::fboss::NextHopThrift;
  using UnicastRoute = facebook::fboss::UnicastRoute;

  RetType queryClient(const HostInfo& hostInfo) {
    std::vector<UnicastRoute> entries;
    auto client =
        utils::createClient<facebook::fboss::FbossCtrlAsyncClient>(hostInfo);

    client->sync_getRouteTable(entries);
    return createModel(entries);
  }

  void printOutput(const RetType& model, std::ostream& out = std::cout) {
    for (const auto& entry : model.get_routeEntries()) {
      out << fmt::format("Network Address: {}\n", entry.get_networkAddress());

      for (const auto& nextHop : entry.get_nextHops()) {
        out << fmt::format(
            "\tvia {}\n", show::route::utils::getNextHopInfoStr(nextHop));
      }
    }
  }

  // Whether or not UCMP is considered active.
  // UCMP is considered inactive when all weight are the same
  // for a given set of next hops, and active when they differ.
  bool isUcmpActive(const std::vector<NextHopThrift>& nextHops) {
    // Let's avoid crashing the CLI when next_hops is blank ;)
    if (nextHops.empty()) {
      return false;
    }
    for (const auto& nh : nextHops) {
      if (nextHops[0].get_weight() != nh.get_weight()) {
        return true;
      }
    }
    return false;
  }

  RetType createModel(
      std::vector<facebook::fboss::UnicastRoute>& routeEntries) {
    RetType model;

    for (const auto& entry : routeEntries) {
      auto& nextHops = entry.get_nextHops();

      auto ipStr = utils::getAddrStr(*entry.dest()->ip());
      auto ipPrefix =
          ipStr + "/" + std::to_string(*entry.dest()->prefixLength());

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
        for (const auto& address : entry.get_nextHopAddrs()) {
          cli::NextHopInfo nextHopInfo;
          show::route::utils::getNextHopInfoAddr(address, nextHopInfo);
          routeEntry.nextHops()->emplace_back(nextHopInfo);
        }
      }
      model.routeEntries()->emplace_back(routeEntry);
    }
    return model;
  }
};

} // namespace facebook::fboss
