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
#include <cstdint>
#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/commands/show/route/gen-cpp2/model_types.h"

namespace facebook::fboss {

struct CmdShowRouteTraits : public BaseCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_IP_LIST;
  using ObjectArgType = std::vector<std::string>;
  using RetType = cli::ShowRouteModel;
};

class CmdShowRoute : public CmdHandler<CmdShowRoute, CmdShowRouteTraits> {
 public:
  RetType queryClient(
      const HostInfo& hostInfo,
      const ObjectArgType& queriedRoutes) {
    std::vector<facebook::fboss::RouteDetails> entries;
    auto client =
        utils::createClient<facebook::fboss::FbossCtrlAsyncClient>(hostInfo);

    client->sync_getRouteTableDetails(entries);
    return createModel(entries, queriedRoutes);
  }

  void printOutput(const RetType& model, std::ostream& out = std::cout) {
    for (const auto& entry : model.get_routeEntries()) {
      out << fmt::format(
          "\nNetwork Address: {}/{}\n",
          entry.get_ip(),
          entry.get_prefixLength());
    }
  }

  RetType createModel(
      std::vector<facebook::fboss::RouteDetails> routeEntries,
      const ObjectArgType& queriedRoutes) {
    RetType model;
    std::unordered_set<std::string> queriedSet(
        queriedRoutes.begin(), queriedRoutes.end());

    for (const auto& entry : routeEntries) {
      auto ipStr = getAddrStr(*entry.dest()->ip());
      auto ipPrefix =
          ipStr + "/" + std::to_string(*entry.dest()->prefixLength());
      if (queriedRoutes.size() == 0 || queriedSet.count(ipPrefix)) {
        cli::RouteEntry routeDetails;
        routeDetails.ip() = ipStr;
        routeDetails.prefixLength() = *entry.dest()->prefixLength();
        model.routeEntries()->push_back(routeDetails);
      }
    }
    return model;
  }

  std::string getAddrStr(network::thrift::BinaryAddress addr) {
    auto ip = *addr.addr();
    char ipBuff[INET6_ADDRSTRLEN];
    if (ip.size() == 16) {
      inet_ntop(
          AF_INET6, &((struct in_addr*)&ip)->s_addr, ipBuff, INET6_ADDRSTRLEN);
    } else if (ip.size() == 4) {
      inet_ntop(
          AF_INET, &((struct in_addr*)&ip)->s_addr, ipBuff, INET_ADDRSTRLEN);
    } else {
      return "invalid";
    }
    return std::string(ipBuff);
  }
};

} // namespace facebook::fboss
