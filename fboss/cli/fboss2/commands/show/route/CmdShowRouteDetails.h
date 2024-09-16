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
#include <cstdint>
#include "fboss/agent/if/gen-cpp2/common_types.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/route/CmdShowRoute.h"
#include "fboss/cli/fboss2/commands/show/route/gen-cpp2/model_types.h"
#include "fboss/cli/fboss2/commands/show/route/utils.h"

namespace facebook::fboss {

struct CmdShowRouteDetailsTraits : public BaseCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_IP_LIST;
  using ParentCmd = CmdShowRoute;
  using ObjectArgType = utils::IPList;
  using RetType = cli::ShowRouteDetailsModel;
};

class CmdShowRouteDetails
    : public CmdHandler<CmdShowRouteDetails, CmdShowRouteDetailsTraits> {
 public:
  RetType queryClient(
      const HostInfo& hostInfo,
      const ObjectArgType& queriedRoutes) {
    std::vector<facebook::fboss::RouteDetails> entries;
    auto client =
        utils::createClient<facebook::fboss::FbossCtrlAsyncClient>(hostInfo);

    client->sync_getRouteTableDetails(entries);

    // queriedRoutes can take 2 forms, ip address or network address
    // Treat the address as IP only if no mask is provided. Lookup the
    // network address for this IP and add it to a new list for output
    std::vector<std::string> finalRoutes;
    std::transform(
        queriedRoutes.begin(),
        queriedRoutes.end(),
        std::back_inserter(finalRoutes),
        [&client](std::string queryRoute) -> std::string {
          if (queryRoute.find("/") == std::string::npos) {
            facebook::fboss::RouteDetails route;
            auto addr =
                facebook::network::toAddress(folly::IPAddress(queryRoute));
            client->sync_getIpRouteDetails(route, addr, 0);
            if (route.get_nextHopMulti().size() > 0) {
              auto ipStr = utils::getAddrStr(*route.dest()->ip());
              auto ipPrefix =
                  ipStr + "/" + std::to_string(*route.dest()->prefixLength());
              return ipPrefix;
            }
          }
          return queryRoute;
        });

    ObjectArgType finalQueriedRoutes(finalRoutes);
    return createModel(entries, finalQueriedRoutes);
  }

  void printOutput(const RetType& model, std::ostream& out = std::cout) {
    for (const auto& entry : model.get_routeEntries()) {
      out << fmt::format(
          "\nNetwork Address: {}/{}{}\n",
          entry.get_ip(),
          entry.get_prefixLength(),
          entry.get_isConnected() ? " (connected)" : "");

      for (const auto& clAndNxthops : entry.get_nextHopMulti()) {
        auto clientId = static_cast<ClientID>(*clAndNxthops.clientId());
        auto clientName = apache::thrift::util::enumNameSafe(clientId);
        out << fmt::format("  Nexthops from client {}\n", clientName);
        for (const auto& nextHop : clAndNxthops.get_nextHops()) {
          out << fmt::format(
              "    {}\n", show::route::utils::getNextHopInfoStr(nextHop));
        }
      }

      out << fmt::format("  Action: {}\n", entry.get_action());

      auto& nextHops = entry.get_nextHops();
      if (nextHops.size() > 0) {
        out << fmt::format("  Forwarding via:\n");
        for (const auto& nextHop : nextHops) {
          out << fmt::format(
              "    {}\n", show::route::utils::getNextHopInfoStr(nextHop));
        }
      } else {
        out << "  No Forwarding Info\n";
      }

      out << fmt::format("  Admin Distance: {}\n", entry.get_adminDistance());
      out << fmt::format("  Counter Id: {}\n", entry.get_counterID());
      out << fmt::format("  Class Id: {}\n", entry.get_classID());
    }
  }

  RetType createModel(
      std::vector<facebook::fboss::RouteDetails>& routeEntries,
      const ObjectArgType& queriedRoutes) {
    RetType model;
    std::unordered_set<std::string> queriedSet(
        queriedRoutes.begin(), queriedRoutes.end());

    for (const auto& entry : routeEntries) {
      auto ipStr = utils::getAddrStr(*entry.dest()->ip());
      auto ipPrefix =
          ipStr + "/" + std::to_string(*entry.dest()->prefixLength());
      if (queriedRoutes.size() == 0 || queriedSet.count(ipPrefix)) {
        cli::RouteDetailEntry routeDetails;
        routeDetails.ip() = ipStr;
        routeDetails.prefixLength() = *entry.dest()->prefixLength();
        routeDetails.action() = *entry.action();
        routeDetails.isConnected() = *entry.isConnected();

        auto& nextHopMulti = entry.get_nextHopMulti();
        for (const auto& clAndNxthops : nextHopMulti) {
          cli::ClientAndNextHops clAndNxthopsCli;
          clAndNxthopsCli.clientId() = clAndNxthops.get_clientId();
          auto& nextHopAddrs = clAndNxthops.get_nextHopAddrs();
          auto& nextHops = clAndNxthops.get_nextHops();
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
          routeDetails.nextHopMulti()->emplace_back(clAndNxthopsCli);
        }

        auto& fwdInfo = *entry.fwdInfo();
        auto& nextHops = entry.get_nextHops();
        if (nextHops.size() > 0) {
          for (const auto& nextHop : nextHops) {
            cli::NextHopInfo nextHopInfo;
            show::route::utils::getNextHopInfoThrift(nextHop, nextHopInfo);
            routeDetails.nextHops()->emplace_back(nextHopInfo);
          }
        } else if (fwdInfo.size() > 0) {
          for (const auto& ifAndIp : fwdInfo) {
            cli::NextHopInfo nextHopInfo;
            nextHopInfo.interfaceID() = ifAndIp.get_interfaceID();
            show::route::utils::getNextHopInfoAddr(
                ifAndIp.get_ip(), nextHopInfo);
            routeDetails.nextHops()->emplace_back(nextHopInfo);
          }
        }

        auto adminDistancePtr = entry.get_adminDistance();
        routeDetails.adminDistance() = adminDistancePtr == nullptr
            ? "None"
            : utils::getAdminDistanceStr(*adminDistancePtr);

        auto counterIDPtr = entry.get_counterID();
        routeDetails.counterID() =
            counterIDPtr == nullptr ? "None" : *counterIDPtr;

        auto classIDPtr = entry.get_classID();
        routeDetails.classID() =
            classIDPtr == nullptr ? "None" : getClassID(*classIDPtr);
        model.routeEntries()->push_back(routeDetails);
      }
    }
    return model;
  }

  std::string getClassID(cfg::AclLookupClass classID) {
    int classId = static_cast<int>(classID);
    switch (classID) {
      case cfg::AclLookupClass::DST_CLASS_L3_LOCAL_1:
        return fmt::format("DST_CLASS_L3_LOCAL_1({})", classId);
      case cfg::AclLookupClass::DST_CLASS_L3_LOCAL_2:
        return fmt::format("DST_CLASS_L3_LOCAL_2({})", classId);
      case cfg::AclLookupClass::CLASS_DROP:
        return fmt::format("CLASS_DROP({})", classId);
      case cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_0:
        return fmt::format("CLASS_QUEUE_PER_HOST_QUEUE_0({})", classId);
      case cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_1:
        return fmt::format("CLASS_QUEUE_PER_HOST_QUEUE_1({})", classId);
      case cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_2:
        return fmt::format("CLASS_QUEUE_PER_HOST_QUEUE_2({})", classId);
      case cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_3:
        return fmt::format("CLASS_QUEUE_PER_HOST_QUEUE_3({})", classId);
      case cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_4:
        return fmt::format("CLASS_QUEUE_PER_HOST_QUEUE_4({})", classId);
      case cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_5:
        return fmt::format("CLASS_QUEUE_PER_HOST_QUEUE_5({})", classId);
      case cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_6:
        return fmt::format("CLASS_QUEUE_PER_HOST_QUEUE_6({})", classId);
      case cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_7:
        return fmt::format("CLASS_QUEUE_PER_HOST_QUEUE_7({})", classId);
      case cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_8:
        return fmt::format("CLASS_QUEUE_PER_HOST_QUEUE_8({})", classId);
      case cfg::AclLookupClass::CLASS_QUEUE_PER_HOST_QUEUE_9:
        return fmt::format("CLASS_QUEUE_PER_HOST_QUEUE_9({})", classId);
      case cfg::AclLookupClass::DST_CLASS_L3_DPR:
        return fmt::format("DST_CLASS_L3_DPR({})", classId);
      case cfg::AclLookupClass::DEPRECATED_CLASS_UNRESOLVED_ROUTE_TO_CPU:
        return fmt::format("CLASS_UNRESOLVED_ROUTE_TO_CPU({})", classId);
      case cfg::AclLookupClass::DEPRECATED_CLASS_CONNECTED_ROUTE_TO_INTF:
        return fmt::format("CLASS_CONNECTED_ROUTE_TO_INTF({})", classId);
    }
    throw std::runtime_error(
        "Unsupported ClassID: " + std::to_string(static_cast<int>(classID)));
  }
};

} // namespace facebook::fboss
