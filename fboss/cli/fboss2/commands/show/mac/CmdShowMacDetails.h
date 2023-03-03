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
#include "fboss/cli/fboss2/commands/show/mac/gen-cpp2/model_types.h"

namespace facebook::fboss {

struct CmdShowMacDetailsTraits : public BaseCommandTraits {
  using ObjectArgType = utils::NoneArgType;
  using RetType = cli::ShowMacDetailsModel;
  static constexpr bool ALLOW_FILTERING = true;
  static constexpr bool ALLOW_AGGREGATION = true;
};

class CmdShowMacDetails
    : public CmdHandler<CmdShowMacDetails, CmdShowMacDetailsTraits> {
 public:
  RetType queryClient(const HostInfo& hostInfo) {
    std::vector<facebook::fboss::L2EntryThrift> entries;
    std::map<int32_t, facebook::fboss::PortInfoThrift> portEntries;
    std::vector<facebook::fboss::AggregatePortThrift> aggPortentries;
    auto client =
        utils::createClient<facebook::fboss::FbossCtrlAsyncClient>(hostInfo);

    client->sync_getL2Table(entries);
    client->sync_getAllPortInfo(portEntries);
    client->sync_getAggregatePortTable(aggPortentries);
    return createModel(entries, portEntries, aggPortentries);
  }

  void printOutput(const RetType& model, std::ostream& out = std::cout) {
    constexpr auto fmtString = "{:<24}{:<19}{:<14}{:<19}{:<14}\n";
    out << fmt::format(
        fmtString, "MAC Address", "Port/Trunk", "VLAN", "TYPE", "CLASSID");

    for (const auto& entry : model.get_l2Entries()) {
      out << fmt::format(
          fmtString,
          entry.get_mac(),
          entry.get_ifName(),
          entry.get_vlanID(),
          entry.get_l2EntryType(),
          entry.get_classID());
    }
    out << std::endl;
  }

  RetType createModel(
      std::vector<facebook::fboss::L2EntryThrift> l2Entries,
      std::map<int32_t, facebook::fboss::PortInfoThrift> portEntries,
      std::vector<facebook::fboss::AggregatePortThrift> aggregatePortEntries) {
    RetType model;

    for (const auto& entry : l2Entries) {
      cli::L2Entry l2Details;

      l2Details.mac() = entry.get_mac();
      l2Details.port() = entry.get_port();
      l2Details.vlanID() = entry.get_vlanID();
      l2Details.l2EntryType() =
          utils::getl2EntryTypeStr(entry.get_l2EntryType());
      auto trunkPtr = entry.get_trunk();
      if (trunkPtr != nullptr) {
        l2Details.trunk() = *trunkPtr;
        std::vector<facebook::fboss::AggregatePortThrift> aggPortEntries;
        for (const auto& agg_port : aggregatePortEntries) {
          if (agg_port.get_key() == *trunkPtr) {
            aggPortEntries.push_back(agg_port);
          }
        }
        if (aggPortEntries.size() == 1) {
          l2Details.ifName() = aggPortEntries[0].get_name();
        } else {
          l2Details.ifName() = std::to_string(*trunkPtr) + " (Trunk)";
        }
      } else {
        l2Details.ifName() = portEntries[entry.get_port()].get_name();
      }
      auto classIdPtr = entry.get_classID();
      l2Details.classID() =
          (classIdPtr != nullptr) ? std::to_string(*classIdPtr) : "-";

      model.l2Entries()->push_back(l2Details);
    }
    return model;
  }
};

} // namespace facebook::fboss
