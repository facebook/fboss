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
#include "fboss/cli/fboss2/commands/show/aggregateport/gen-cpp2/model_types.h"

namespace facebook::fboss {

struct CmdShowAggregatePortTraits : public BaseCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_PORT_LIST;
  using ObjectArgType = std::vector<std::string>;
  using RetType = cli::ShowAggregatePortModel;
};

class CmdShowAggregatePort
    : public CmdHandler<CmdShowAggregatePort, CmdShowAggregatePortTraits> {
 public:
  RetType queryClient(
      const HostInfo& hostInfo,
      const ObjectArgType& queriedPorts) {
    std::vector<facebook::fboss::AggregatePortThrift> entries;
    std::map<int32_t, facebook::fboss::PortInfoThrift> portInfo;
    auto client =
        utils::createClient<facebook::fboss::FbossCtrlAsyncClient>(hostInfo);

    client->sync_getAggregatePortTable(entries);
    client->sync_getAllPortInfo(portInfo);
    return createModel(entries, portInfo, queriedPorts);
  }

  void printOutput(const RetType& model, std::ostream& out = std::cout) {
    for (const auto& entry : model.get_aggregatePortEntries()) {
      out << fmt::format("\nPort name: {}\n", entry.get_name());
      out << fmt::format("Description: {}\n", entry.get_description());
      out << fmt::format(
          "Active members/Configured members/Min members: {}/{}/{}\n",
          entry.get_activeMembers(),
          entry.get_configuredMembers(),
          entry.get_minMembers());
      for (const auto& member : entry.get_members()) {
        out << fmt::format(
            "\t Member: {:>10}, id: {:>3}, Up: {:>5}, Rate: {}\n",
            member.get_name(),
            member.get_id(),
            member.get_isUp() ? "True" : "False",
            member.get_lacpRate());
      }
    }
  }

  RetType createModel(
      std::vector<facebook::fboss::AggregatePortThrift> aggregatePortEntries,
      std::map<int32_t, facebook::fboss::PortInfoThrift> portInfo,
      const ObjectArgType& queriedPorts) {
    RetType model;
    std::unordered_set<std::string> queriedSet(
        queriedPorts.begin(), queriedPorts.end());

    for (const auto& entry : aggregatePortEntries) {
      auto portName = *entry.name_ref();
      if (queriedPorts.size() == 0 || queriedSet.count(portName)) {
        cli::AggregatePortEntry aggPortDetails;
        aggPortDetails.name_ref() = portName;
        aggPortDetails.description_ref() = *entry.description_ref();
        aggPortDetails.minMembers_ref() = *entry.minimumLinkCount_ref();
        aggPortDetails.configuredMembers_ref() =
            entry.memberPorts_ref()->size();
        int32_t activeMemberCount = 0;
        for (const auto& subport : *entry.memberPorts_ref()) {
          cli::AggregateMemberPortEntry memberDetails;
          memberDetails.id_ref() = *subport.memberPortID_ref();
          memberDetails.name_ref() =
              *portInfo[*subport.memberPortID_ref()].name_ref();
          memberDetails.isUp_ref() = *subport.isForwarding_ref();
          memberDetails.lacpRate_ref() =
              *subport.rate_ref() == LacpPortRateThrift::FAST ? "Fast" : "Slow";
          if (*subport.isForwarding_ref()) {
            activeMemberCount++;
          }
          aggPortDetails.members()->push_back(memberDetails);
        }
        aggPortDetails.activeMembers_ref() = activeMemberCount;
        model.aggregatePortEntries()->push_back(aggPortDetails);
      }
    }
    return model;
  }
};

} // namespace facebook::fboss
