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

struct CmdShowRouteDetailsTraits : public ReadCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_IP_LIST;
  using ParentCmd = CmdShowRoute;
  using ObjectArgType = utils::IPList;
  using RetType = cli::ShowRouteDetailsModel;
};

class CmdShowRouteDetails
    : public CmdHandler<CmdShowRouteDetails, CmdShowRouteDetailsTraits> {
 private:
  std::map<std::string, std::string> vlanAggregatePortMap;
  std::map<std::string, std::map<std::string, std::vector<std::string>>>
      vlanPortMap;

  std::string parseRootPort(const std::string& str) {
    /*
    This function parses out the root port from the port name. Example:
    Port name: eth1/16/1
    rootPort: eth1
     */
    std::string port = "";
    size_t pos = str.find('/');
    if (pos != std::string::npos) {
      port = str.substr(0, pos);
    }
    return port;
  }

  void populateAggregatePortMap(
      const std::unique_ptr<facebook::fboss::FbossCtrlAsyncClient>& client) {
    std::map<int32_t, PortInfoThrift> portInfoEntries;
    client->sync_getAllPortInfo(portInfoEntries);

    std::vector<::facebook::fboss::AggregatePortThrift> aggregatePortThrift;
    client->sync_getAggregatePortTable(aggregatePortThrift);

    for (auto aggregatePort : aggregatePortThrift) {
      std::string aggPortName = *aggregatePort.name();
      for (auto memberPort : *aggregatePort.memberPorts()) {
        auto memberPortID = memberPort.memberPortID().value();
        auto it = portInfoEntries.find(memberPortID);
        if (it != portInfoEntries.end()) {
          auto vlans = it->second.vlans();
          // If L3 routing with multiple vlans, we can skip this port
          if (vlans->size() > 1) {
            continue;
          }
          this->vlanAggregatePortMap[std::to_string(vlans[0])] = aggPortName;
        }
      }
    }
  }

  void populateVlanPortMap(
      const HostInfo& hostInfo,
      const std::unique_ptr<facebook::fboss::FbossCtrlAsyncClient>& client) {
    std::map<int32_t, PortInfoThrift> portInfoEntries;
    client->sync_getAllPortInfo(portInfoEntries);

    for (const auto& portInfo : portInfoEntries) {
      if ((portInfo.second.vlans()->size() == 0) ||
          (portInfo.second.vlans()->size() > 1)) {
        continue;
      }
      auto vlan = std::to_string(portInfo.second.vlans()[0]);
      auto portName = portInfo.second.name().value();
      auto rootPort = parseRootPort(portName);
      vlanPortMap[vlan][rootPort].push_back(portName);
    }
  }

 public:
  RetType queryClient(
      const HostInfo& hostInfo,
      const ObjectArgType& queriedRoutes) {
    std::vector<facebook::fboss::RouteDetails> entries;
    auto client =
        utils::createClient<facebook::fboss::FbossCtrlAsyncClient>(hostInfo);
    client->sync_getRouteTableDetails(entries);
    populateAggregatePortMap(client);
    populateVlanPortMap(hostInfo, client);
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
            if (route.nextHopMulti().value().size() > 0) {
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
    for (const auto& entry : model.routeEntries().value()) {
      out << fmt::format(
          "\nNetwork Address: {}/{}{}\n",
          entry.ip().value(),
          folly::copy(entry.prefixLength().value()),
          folly::copy(entry.isConnected().value()) ? " (connected)" : "");

      for (const auto& clAndNxthops : entry.nextHopMulti().value()) {
        auto clientId = static_cast<ClientID>(*clAndNxthops.clientId());
        auto clientName = apache::thrift::util::enumNameSafe(clientId);
        out << fmt::format("  Nexthops from client {}\n", clientName);
        for (const auto& nextHop : clAndNxthops.nextHops().value()) {
          out << fmt::format(
              "    {}\n", show::route::utils::getNextHopInfoStr(nextHop));
        }
      }

      out << fmt::format("  Action: {}\n", entry.action().value());

      auto printNextHops = [this, &out](
                               const std::string& header,
                               const auto& nextHops,
                               bool isOverride,
                               const auto& nhToTopoInfo) {
        out << fmt::format("  {}\n", header);
        std::string overrideStr = (isOverride ? "(override) :" : "");
        std::map<int, int> planeIdToPathCount;
        for (const auto& nextHop : nextHops) {
          out << fmt::format(
              "  {}  {}\n",
              overrideStr,
              show::route::utils::getNextHopInfoStr(
                  nextHop, vlanAggregatePortMap, vlanPortMap));

          auto it = nhToTopoInfo.find(nextHop.addr().value());
          if (it != nhToTopoInfo.end()) {
            const auto& topologyInfo = it->second;
            if (topologyInfo.plane_id().has_value()) {
              int planeId = topologyInfo.plane_id().value();
              planeIdToPathCount[planeId]++;
            }
          }
        }
        if (planeIdToPathCount.size() > 0) {
          out << fmt::format("  Paths per plane:\n");
          for (const auto& [planeId, pathCount] : planeIdToPathCount) {
            out << fmt::format("    Plane {}: {}\n", planeId, pathCount);
          }
        }
      };
      auto& nextHops = entry.nextHops().value();
      auto& nhToTopoInfo = entry.nhAddressToTopologyInfo().value();
      if (nextHops.size() > 0) {
        std::string header =
            (entry.overridenNextHops() ? "Original next hops:"
                                       : "Forwarding via:");
        printNextHops(header, nextHops, false /*isOverride*/, nhToTopoInfo);
      } else if (!entry.overridenNextHops().has_value()) {
        out << "  No Forwarding Info\n";
      }
      if (entry.overridenNextHops()) {
        if (entry.overridenNextHops()->size()) {
          printNextHops(
              "Forwarding via:",
              *entry.overridenNextHops(),
              true /*isOverride*/,
              nhToTopoInfo);
        } else if (!entry.overridenNextHops().has_value()) {
          out << "  No Forwarding Info\n";
        }
        out << fmt::format(
            " Num next hops lost: {}\n",
            nextHops.size() - entry.overridenNextHops()->size());
      }

      out << fmt::format(
          "  Admin Distance: {}\n", entry.adminDistance().value());
      out << fmt::format("  Counter Id: {}\n", entry.counterID().value());
      out << fmt::format("  Class Id: {}\n", entry.classID().value());
      out << fmt::format(
          "  Overridden ECMP mode: {}\n", entry.overridenEcmpMode().value());
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
        // Map to hold address to topologyInfo from client (NextHopsMulti
        // fields)
        std::map<std::string, NetworkTopologyInformation> nhToTopoInfo;

        auto& nextHopMulti = entry.nextHopMulti().value();
        for (const auto& clAndNxthops : nextHopMulti) {
          cli::ClientAndNextHops clAndNxthopsCli;
          clAndNxthopsCli.clientId() =
              folly::copy(clAndNxthops.clientId().value());
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
              auto topologyInfo =
                  apache::thrift::get_pointer(nextHop.topologyInfo());
              if (topologyInfo) {
                nhToTopoInfo[facebook::network::toIPAddress(*nextHop.address())
                                 .str()] = *topologyInfo;
              }
            }
          }
          routeDetails.nextHopMulti()->emplace_back(clAndNxthopsCli);
        }

        routeDetails.nhAddressToTopologyInfo() = nhToTopoInfo;

        auto& fwdInfo = *entry.fwdInfo();
        auto& nextHops = entry.nextHops().value();

        if (nextHops.size() > 0) {
          for (const auto& nextHop : nextHops) {
            cli::NextHopInfo nextHopInfo;
            show::route::utils::getNextHopInfoThrift(nextHop, nextHopInfo);
            routeDetails.nextHops()->emplace_back(nextHopInfo);
          }
        } else if (fwdInfo.size() > 0) {
          for (const auto& ifAndIp : fwdInfo) {
            cli::NextHopInfo nextHopInfo;
            nextHopInfo.interfaceID() =
                folly::copy(ifAndIp.interfaceID().value());
            show::route::utils::getNextHopInfoAddr(
                ifAndIp.ip().value(), nextHopInfo);
            routeDetails.nextHops()->emplace_back(nextHopInfo);
          }
        }

        if (entry.overridenNextHops().has_value()) {
          routeDetails.overridenNextHops() = std::vector<cli::NextHopInfo>();
          for (const auto& nextHop : *entry.overridenNextHops()) {
            cli::NextHopInfo nextHopInfo;
            show::route::utils::getNextHopInfoThrift(nextHop, nextHopInfo);
            routeDetails.overridenNextHops()->emplace_back(nextHopInfo);
          }
          routeDetails.nhopsLostDueToOverride() =
              nextHops.size() - entry.overridenNextHops()->size();
        }

        auto adminDistancePtr =
            apache::thrift::get_pointer(entry.adminDistance());
        routeDetails.adminDistance() = adminDistancePtr == nullptr
            ? "None"
            : utils::getAdminDistanceStr(*adminDistancePtr);

        auto counterIDPtr = apache::thrift::get_pointer(entry.counterID());
        routeDetails.counterID() =
            counterIDPtr == nullptr ? "None" : *counterIDPtr;

        auto classIDPtr = apache::thrift::get_pointer(entry.classID());
        routeDetails.classID() =
            classIDPtr == nullptr ? "None" : getClassID(*classIDPtr);
        auto overrideEcmpModePtr =
            apache::thrift::get_pointer(entry.overridenEcmpMode());
        routeDetails.overridenEcmpMode() = overrideEcmpModePtr == nullptr
            ? "None"
            : apache::thrift::util::enumNameSafe(*overrideEcmpModePtr);
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
      case cfg::AclLookupClass::ARS_ALTERNATE_MEMBERS_CLASS:
        return fmt::format("ARS_ALTERNATE_MEMBERS_CLASS({})", classId);
    }
    throw std::runtime_error(
        "Unsupported ClassID: " + std::to_string(static_cast<int>(classID)));
  }
};

} // namespace facebook::fboss
